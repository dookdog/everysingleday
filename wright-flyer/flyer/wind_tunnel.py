"""The wind tunnel.

After the disappointing 1901 gliding season the Wrights stopped trusting
Lilienthal's lift tables, built a six-foot wooden tunnel, and measured
lift on model wings under controlled, repeatable airflow. Only then did
the 1902 glider fly as designed.

This module is that tunnel for a trading strategy: a synthetic market
with a *planted, mechanical stall angle*. Blow-off segments melt up with
accelerating drift until the price's standardized extension above its own
glide path crosses ``planted_stall`` -- and only then crack. Separation
is caused by extension, not by the calendar, so the airmass has a true
angle of attack beyond which forward lift is negative, and the strategy's
stall-angle estimator can be scored against ground truth.

Regimes:
    trend_up / trend_down : drift with moderate vol (attached flow)
    churn                 : zero drift, mean-reverting chop (dead air)
    blowoff_up            : accelerating melt-up (flow still attached)
    blowoff_crack         : the break after extension crosses the planted
                            stall angle (separated flow)
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from flyer.aerodynamics import _ema_deviation_scale

TRADING_DAYS = 252.0


class WindTunnel:
    """Regime-switching synthetic price generator with a planted stall angle.

    The tunnel tracks the same glide path/vol instruments the strategy
    uses (EMA of log price, EWMA vol) so that "extension" means the same
    thing inside the generator as it does on the instrument panel.
    """

    def __init__(
        self,
        seed: int = 0,
        planted_stall: float = 2.5,
        crack_recover: float = 0.8,
        glide_span: int = 50,
        vol_span: int = 30,
    ):
        self.rng = np.random.default_rng(seed)
        self.planted_stall = planted_stall
        self.crack_recover = crack_recover
        self.glide_span = glide_span
        self.vol_span = vol_span

    def generate(
        self,
        n_days: int = 3000,
        regime_weights: dict | None = None,
        vol_scale: float = 1.0,
        p0: float = 100.0,
    ) -> tuple[pd.Series, pd.Series]:
        """Return (prices, regime_labels), both indexed by business days."""
        rng = self.rng
        base_vol = 0.012 * vol_scale
        weights = regime_weights or {
            "trend_up": 0.3,
            "trend_down": 0.15,
            "churn": 0.3,
            "blowoff": 0.25,
        }
        kinds = list(weights)
        probs = np.array([weights[k] for k in kinds], dtype=float)
        probs /= probs.sum()

        lam_g = 2.0 / (self.glide_span + 1.0)
        lam_v = 2.0 / (self.vol_span + 1.0)
        dev_scale = _ema_deviation_scale(self.glide_span)

        logp = np.log(p0)
        ema = logp
        # EW moments of returns for the tunnel's own vol gauge.
        m1, m2 = 0.0, base_vol**2

        rets = np.zeros(n_days)
        labels: list[str] = []

        seg_kind, seg_left = None, 0
        drift, accel = 0.0, 0.0
        prev_ret = 0.0
        cracking = False

        for t in range(n_days):
            if seg_left <= 0:
                seg_kind = rng.choice(kinds, p=probs)
                seg_left = int(rng.integers(60, 200))
                cracking = False
                if seg_kind == "trend_up":
                    drift, accel = rng.uniform(0.0006, 0.0016), 0.0
                elif seg_kind == "trend_down":
                    drift, accel = -rng.uniform(0.0006, 0.0016), 0.0
                elif seg_kind == "churn":
                    drift, accel = 0.0, 0.0
                else:  # blowoff: drift accelerates until the planted stall
                    drift, accel = 0.0010, rng.uniform(0.00003, 0.00007)

            vol_now = max(np.sqrt(max(m2 - m1 * m1, 1e-10)), 1e-6)
            alpha_now = (logp - ema) / (vol_now * dev_scale)

            if seg_kind == "blowoff":
                if not cracking and alpha_now >= self.planted_stall:
                    cracking = True  # flow separates: extension caused this
                elif cracking and alpha_now <= self.crack_recover:
                    # Flow reattached; the blow-off is spent. End the
                    # segment and let quiet air take over from tomorrow.
                    seg_kind, seg_left, cracking = "churn", 1, False

            if seg_kind in ("trend_up", "trend_down"):
                r = drift + base_vol * rng.standard_normal()
                label = seg_kind
            elif seg_kind == "churn":
                r = -0.25 * prev_ret + base_vol * rng.standard_normal()
                label = "churn"
            elif not cracking:
                drift += accel
                r = drift + base_vol * 0.9 * rng.standard_normal()
                label = "blowoff_up"
            else:
                r = -0.020 + base_vol * 2.5 * rng.standard_normal()
                label = "blowoff_crack"

            rets[t] = r
            labels.append(label)
            logp += r
            ema += lam_g * (logp - ema)
            m1 += lam_v * (r - m1)
            m2 += lam_v * (r * r - m2)
            prev_ret = r
            seg_left -= 1

        prices = p0 * np.exp(np.cumsum(rets))
        index = pd.bdate_range("2000-01-03", periods=n_days)
        return (
            pd.Series(prices, index=index, name="price"),
            pd.Series(labels, index=index, name="regime"),
        )
