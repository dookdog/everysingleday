"""The aircraft: instruments + measured lift curve + controls.

Decision loop, once per day at the close of bar t:

    1. Read the instruments (all causal as of t).
    2. If due, recalibrate the stall angle in the wind tunnel, using only
       pairs (alpha_u, fwd_lift_u) with u <= t-2. fwd_lift_u contains the
       return of day u+1, so u <= t-2 guarantees nothing later than day
       t-1 -- already known at the close of t -- enters the calibration.
    3. Assemble the lift equation:
           target = direction * rho * q(airspeed) * C_L(alpha)
       (the trading analogue of L = 1/2 rho v^2 S C_L).
    4. Pass the target through the three-axis controls and stall latch.

The weight decided at close of t earns the return of day t+1; that shift
is enforced in backtest.py, not here.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from flyer.aerodynamics import instruments, lift_coefficient, measure_stall_angle
from flyer.config import FlyerConfig
from flyer.controls import ControlState, apply_controls


class WrightFlyer:
    def __init__(self, cfg: FlyerConfig | None = None):
        self.cfg = cfg or FlyerConfig()

    def raw_target(self, panel_row: pd.Series, alpha_stall: float) -> float:
        """The lift equation, before the control system touches it."""
        cfg = self.cfg
        direction = panel_row["direction"]
        rho = panel_row["rho"]
        q = panel_row["q"]
        alpha = panel_row["alpha"]

        if not np.isfinite(direction) or not np.isfinite(rho) or not np.isfinite(q):
            return 0.0

        cl = lift_coefficient(alpha, alpha_stall, cfg.taper_frac)
        target = direction * rho * q * cl
        return float(np.clip(target, -cfg.max_leverage, cfg.max_leverage))

    def fly(self, prices: pd.Series) -> pd.DataFrame:
        """Run the full decision loop over a price series.

        Returns the flight log: one row per day with instrument readings,
        the stall angle in force, the raw target, and the weight actually
        held after the control system.
        """
        cfg = self.cfg
        panel = instruments(prices, cfg)

        alpha_arr = panel["alpha"].to_numpy()
        fwd_arr = panel["fwd_lift"].to_numpy()

        n = len(panel)
        weights = np.zeros(n)
        targets = np.zeros(n)
        stalls = np.full(n, cfg.alpha_stall_default)
        stalled_flags = np.zeros(n, dtype=bool)

        state = ControlState()
        alpha_stall = cfg.alpha_stall_default
        warmup = max(cfg.glide_span, cfg.vol_span) + cfg.slope_lookback

        for t in range(n):
            if t < warmup:
                stalls[t] = alpha_stall
                continue

            # Wind-tunnel recalibration on schedule. The [:t-1] slice ends
            # at index t-2 inclusive: the newest pair used is
            # (alpha_{t-2}, return of day t-1), fully known at close of t.
            if (t - warmup) % cfg.calib_step == 0:
                lo = max(0, t - 1 - cfg.calib_window)
                alpha_stall, _ = measure_stall_angle(
                    alpha_arr[lo : t - 1], fwd_arr[lo : t - 1], cfg
                )

            row = panel.iloc[t]
            target = self.raw_target(row, alpha_stall)
            state = apply_controls(state, target, row["alpha"], alpha_stall, cfg)

            targets[t] = target
            weights[t] = state.weight
            stalls[t] = alpha_stall
            stalled_flags[t] = state.stalled

        log = panel.copy()
        log["alpha_stall"] = stalls
        log["target"] = targets
        log["weight"] = weights
        log["stalled"] = stalled_flags
        return log
