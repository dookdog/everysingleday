"""Pre-flight inspection.

The checks that must pass before the craft leaves the ground, in order of
how quickly their failure would kill it:

    1. No lookahead: decisions at t must not depend on anything after t.
    2. Stall latch: engages past the stall angle, holds through hysteresis,
       and drives exposure to zero.
    3. Control limits: roll rate, leverage cap, yaw dead-band.
    4. Flight recorder: lag-1 execution and cost accounting are exact.
    5. Wind-tunnel balance: recovers a known stall angle from synthetic
       air with a planted separation point.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import numpy as np
import pandas as pd
import pytest

from flyer.aerodynamics import instruments, lift_coefficient, measure_stall_angle
from flyer.backtest import backtest
from flyer.config import FlyerConfig
from flyer.controls import ControlState, apply_controls
from flyer.strategy import WrightFlyer
from flyer.wind_tunnel import WindTunnel


@pytest.fixture(scope="module")
def cfg():
    return FlyerConfig()


@pytest.fixture(scope="module")
def synthetic_prices():
    prices, _ = WindTunnel(seed=7).generate(n_days=2500)
    return prices


# ---------------------------------------------------------------------------
# 1. No lookahead
# ---------------------------------------------------------------------------
class TestNoLookahead:
    def test_future_prices_do_not_change_past_weights(self, cfg, synthetic_prices):
        """Rewrite the last 250 days of price history; every weight decided
        before the rewrite point must be bit-for-bit identical."""
        craft = WrightFlyer(cfg)
        full = craft.fly(synthetic_prices)

        cut = len(synthetic_prices) - 250
        tampered = synthetic_prices.copy()
        rng = np.random.default_rng(123)
        tampered.iloc[cut:] = tampered.iloc[cut - 1] * np.exp(
            np.cumsum(0.05 * rng.standard_normal(250))
        )
        partial = craft.fly(tampered)

        pd.testing.assert_series_equal(
            full["weight"].iloc[:cut], partial["weight"].iloc[:cut]
        )

    def test_truncation_matches_full_run(self, cfg, synthetic_prices):
        """Running on a prefix must reproduce the full run's weights on
        that prefix (streaming consistency)."""
        craft = WrightFlyer(cfg)
        full = craft.fly(synthetic_prices)
        cut = 1500
        prefix = craft.fly(synthetic_prices.iloc[:cut])
        pd.testing.assert_series_equal(
            full["weight"].iloc[:cut], prefix["weight"]
        )

    def test_instruments_are_causal_except_fwd_lift(self, cfg, synthetic_prices):
        panel_full = instruments(synthetic_prices, cfg)
        cut = 1200
        panel_prefix = instruments(synthetic_prices.iloc[:cut], cfg)
        causal_cols = [c for c in panel_full.columns if c != "fwd_lift"]
        pd.testing.assert_frame_equal(
            panel_full[causal_cols].iloc[:cut], panel_prefix[causal_cols]
        )

    def test_fwd_lift_is_the_next_days_return(self, cfg, synthetic_prices):
        panel = instruments(synthetic_prices, cfg)
        t = 500
        r_next = np.log(synthetic_prices.iloc[t + 1] / synthetic_prices.iloc[t])
        expected = panel["direction"].iloc[t] * r_next / panel["vol"].iloc[t]
        assert panel["fwd_lift"].iloc[t] == pytest.approx(expected)


# ---------------------------------------------------------------------------
# 2. Stall latch
# ---------------------------------------------------------------------------
class TestStallLatch:
    def test_latch_engages_and_dumps_exposure(self, cfg):
        state = ControlState(weight=1.0, stalled=False)
        state = apply_controls(state, target=1.0, alpha=cfg.alpha_stall_default + 1,
                               alpha_stall=cfg.alpha_stall_default, cfg=cfg)
        assert state.stalled
        assert state.weight == pytest.approx(1.0 - cfg.emergency_rate)

    def test_latch_holds_inside_hysteresis_band(self, cfg):
        stall = 2.0
        inside_band = 0.5 * (cfg.recover_frac * stall + stall)  # between recover and stall
        state = ControlState(weight=0.5, stalled=True)
        state = apply_controls(state, target=1.0, alpha=inside_band,
                               alpha_stall=stall, cfg=cfg)
        assert state.stalled, "latch must not release until alpha < recover_frac * stall"

    def test_latch_releases_after_recovery(self, cfg):
        stall = 2.0
        state = ControlState(weight=0.0, stalled=True)
        state = apply_controls(state, target=0.5, alpha=0.9 * cfg.recover_frac * stall,
                               alpha_stall=stall, cfg=cfg)
        assert not state.stalled

    def test_stalled_craft_reaches_zero(self, cfg):
        state = ControlState(weight=1.4, stalled=False)
        for _ in range(10):
            state = apply_controls(state, target=1.4, alpha=5.0,
                                   alpha_stall=2.0, cfg=cfg)
        assert state.weight == pytest.approx(0.0)


# ---------------------------------------------------------------------------
# 3. Control limits
# ---------------------------------------------------------------------------
class TestControlLimits:
    def test_roll_rate_limit(self, cfg):
        state = ControlState(weight=0.0)
        state = apply_controls(state, target=1.0, alpha=0.0, alpha_stall=2.5, cfg=cfg)
        assert state.weight == pytest.approx(cfg.roll_rate)

    def test_yaw_dead_band_ignores_small_errors(self, cfg):
        state = ControlState(weight=0.5)
        target = 0.5 + 0.5 * cfg.yaw_band
        new = apply_controls(state, target=target, alpha=0.0, alpha_stall=2.5, cfg=cfg)
        assert new.weight == state.weight

    def test_weight_never_exceeds_structural_limit(self, cfg, synthetic_prices):
        log = WrightFlyer(cfg).fly(synthetic_prices)
        assert (log["weight"].abs() <= cfg.max_leverage + 1e-9).all()

    def test_daily_weight_change_bounded(self, cfg, synthetic_prices):
        log = WrightFlyer(cfg).fly(synthetic_prices)
        max_step = max(cfg.roll_rate, cfg.emergency_rate)
        assert (log["weight"].diff().abs().dropna() <= max_step + 1e-9).all()


# ---------------------------------------------------------------------------
# 4. Flight recorder
# ---------------------------------------------------------------------------
class TestBacktest:
    def test_lag_one_execution(self):
        idx = pd.bdate_range("2020-01-01", periods=5)
        prices = pd.Series([100.0, 110.0, 99.0, 108.9, 108.9], index=idx)
        weights = pd.Series([1.0, 0.0, 1.0, 1.0, 1.0], index=idx)
        bt = backtest(prices, weights, cost_bps=0.0)
        # Day 1: held = weight decided day 0 = 1.0, asset +10%.
        assert bt["gross_ret"].iloc[1] == pytest.approx(0.10)
        # Day 2: held = 0 (decided day 1), asset -10% -> flat.
        assert bt["gross_ret"].iloc[2] == pytest.approx(0.0)
        # Day 3: held = 1 (decided day 2), asset +10%.
        assert bt["gross_ret"].iloc[3] == pytest.approx(0.10)

    def test_costs_charged_on_turnover(self):
        idx = pd.bdate_range("2020-01-01", periods=4)
        prices = pd.Series(100.0, index=idx)  # flat market: only costs remain
        weights = pd.Series([1.0, -1.0, -1.0, 0.0], index=idx)
        bt = backtest(prices, weights, cost_bps=10.0)
        expected = -(1.0 + 2.0 + 0.0 + 1.0) * 10.0 * 1e-4
        assert bt["net_ret"].sum() == pytest.approx(expected)

    def test_constant_full_weight_tracks_asset(self, synthetic_prices):
        w = pd.Series(1.0, index=synthetic_prices.index)
        bt = backtest(synthetic_prices, w, cost_bps=0.0)
        hold = synthetic_prices.iloc[-1] / synthetic_prices.iloc[0]
        assert bt["equity"].iloc[-1] == pytest.approx(hold, rel=1e-9)


# ---------------------------------------------------------------------------
# 5. Wind-tunnel balance
# ---------------------------------------------------------------------------
class TestStallMeasurement:
    def _planted_air(self, true_stall: float, n: int = 20000, seed: int = 0):
        """Air with a known separation point: positive mean lift below the
        planted stall angle, negative above it."""
        rng = np.random.default_rng(seed)
        alpha = rng.uniform(0.0, 4.0, size=n)
        mean = np.where(alpha < true_stall, 0.05, -0.08)
        lift = mean + 0.5 * rng.standard_normal(n)
        return alpha, lift

    def test_recovers_planted_stall_angle(self, cfg):
        true_stall = 2.5
        alpha, lift = self._planted_air(true_stall)
        est, table = measure_stall_angle(alpha, lift, cfg)
        bin_width = cfg.alpha_bin_max / cfg.n_alpha_bins
        assert abs(est - true_stall) <= 2 * bin_width
        assert table is not None and table["count"].sum() > 0

    def test_healthy_air_gives_high_stall(self, cfg):
        """If lift never separates, the measured stall angle should sit at
        the top of the measured range, not somewhere in the middle."""
        rng = np.random.default_rng(1)
        alpha = rng.uniform(0.0, 4.0, size=20000)
        lift = 0.05 + 0.5 * rng.standard_normal(20000)
        est, _ = measure_stall_angle(alpha, lift, cfg)
        assert est == pytest.approx(cfg.alpha_bin_max)

    def test_insufficient_air_returns_default(self, cfg):
        alpha = np.array([0.5, 1.0, 1.5])
        lift = np.array([0.1, 0.2, -0.1])
        est, table = measure_stall_angle(alpha, lift, cfg)
        assert est == cfg.alpha_stall_default
        assert table is None

    def test_stall_never_below_floor(self, cfg):
        """Uniformly terrible air must not produce a stall angle below the
        structural floor (which would ground the craft forever)."""
        rng = np.random.default_rng(2)
        alpha = rng.uniform(0.0, 4.0, size=20000)
        lift = -0.10 + 0.5 * rng.standard_normal(20000)
        est, _ = measure_stall_angle(alpha, lift, cfg)
        assert est >= cfg.alpha_stall_min

    def test_recovers_tunnel_planted_stall_on_average(self, cfg):
        """End-to-end: air generated by the tunnel with a planted stall of
        2.5 must yield estimates centred near the truth across airmasses.
        Individual airmasses are noisy; the ensemble must not be biased."""
        from flyer.aerodynamics import instruments

        estimates = []
        for seed in range(10):
            p, _ = WindTunnel(seed=seed, planted_stall=2.5).generate(n_days=6000)
            panel = instruments(p, cfg)
            est, _ = measure_stall_angle(panel["alpha"], panel["fwd_lift"], cfg)
            estimates.append(est)
        assert abs(np.mean(estimates) - 2.5) <= 0.5


# ---------------------------------------------------------------------------
# Wing shape
# ---------------------------------------------------------------------------
class TestLiftCoefficient:
    def test_full_lift_below_taper(self, cfg):
        stall = 2.5
        assert lift_coefficient(0.0, stall, cfg.taper_frac) == 1.0
        assert lift_coefficient(cfg.taper_frac * stall - 0.01, stall, cfg.taper_frac) == 1.0

    def test_zero_lift_past_stall(self, cfg):
        stall = 2.5
        assert lift_coefficient(stall, stall, cfg.taper_frac) == 0.0
        assert lift_coefficient(stall + 1.0, stall, cfg.taper_frac) == 0.0

    def test_monotone_taper(self, cfg):
        stall = 2.5
        a = np.linspace(cfg.taper_frac * stall, stall, 50)
        cl = lift_coefficient(a, stall, cfg.taper_frac)
        assert (np.diff(cl) <= 1e-12).all()

    def test_symmetric_in_alpha(self, cfg):
        stall = 2.5
        for a in [0.5, 1.5, 2.0, 3.0]:
            assert lift_coefficient(a, stall, cfg.taper_frac) == pytest.approx(
                lift_coefficient(-a, stall, cfg.taper_frac)
            )
