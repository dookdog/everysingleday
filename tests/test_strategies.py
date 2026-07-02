"""Behavioural tests for all ten strategies plus the backtest engine.

The most important test here is the causality (no look-ahead) check:
signals up to bar t must not change when the future beyond t changes.
"""

import numpy as np
import pandas as pd
import pytest

from golden_strategies.backtest import run_backtest, run_many
from golden_strategies.base import Strategy
from golden_strategies.data import make_synthetic_ohlcv
from golden_strategies.strategies import ALL_STRATEGIES, build_all_strategies


@pytest.fixture(scope="module")
def market() -> pd.DataFrame:
    return make_synthetic_ohlcv(900, hurst=0.55, drift_strength=0.2, jump_probability=0.006, seed=123)


@pytest.mark.parametrize("strategy_cls", ALL_STRATEGIES, ids=lambda c: c.name)
def test_positions_are_well_formed(strategy_cls, market):
    positions = strategy_cls().target_positions(market)
    assert isinstance(positions, pd.Series)
    assert positions.index.equals(market.index)
    filled = positions.fillna(0.0)
    assert (filled.abs() <= 1.0 + 1e-9).all()
    assert np.isfinite(filled.to_numpy()).all()


@pytest.mark.parametrize("strategy_cls", ALL_STRATEGIES, ids=lambda c: c.name)
def test_strategy_actually_trades(strategy_cls, market):
    """Every strategy should take at least a few positions on 900 mixed bars."""
    positions = strategy_cls().target_positions(market).fillna(0.0)
    assert (positions != 0).sum() >= 5, f"{strategy_cls.name} never trades"


@pytest.mark.parametrize("strategy_cls", ALL_STRATEGIES, ids=lambda c: c.name)
def test_no_lookahead(strategy_cls, market):
    """Rewriting the future must not rewrite the past's signals."""
    cut = 600
    strategy = strategy_cls()
    full = strategy.target_positions(market).iloc[:cut].fillna(0.0)

    tampered = market.copy()
    rng = np.random.default_rng(0)
    scale = np.exp(rng.normal(0.0, 0.05, size=len(tampered) - cut))
    for column in ("open", "high", "low", "close"):
        tampered.loc[tampered.index[cut:], column] *= scale
    tampered.loc[tampered.index[cut:], "volume"] *= 3.0

    perturbed = strategy_cls().target_positions(tampered).iloc[:cut].fillna(0.0)
    pd.testing.assert_series_equal(full, perturbed, check_names=False)


def test_backtester_applies_one_bar_delay(market):
    """Positions must earn the *next* bar's return, not the signal bar's."""

    class SpyStrategy(Strategy):
        name = "spy"

        def target_positions(self, df):
            pos = pd.Series(0.0, index=df.index)
            pos.iloc[10] = 1.0  # long only on bar 10
            return pos

    result = run_backtest(SpyStrategy(), market, cost_bps=0.0)
    bar11_asset = market["close"].pct_change().iloc[11]
    assert result.returns.iloc[11] == pytest.approx(bar11_asset)
    assert result.returns.iloc[10] == pytest.approx(0.0)


def test_backtester_charges_costs(market):
    strategy = build_all_strategies()[2]  # phi_ema_cascade, trades plenty
    free = run_backtest(strategy, market, cost_bps=0.0)
    costly = run_backtest(strategy, market, cost_bps=25.0)
    assert costly.equity_curve.iloc[-1] < free.equity_curve.iloc[-1]


def test_run_many_produces_full_table(market):
    table = run_many(build_all_strategies(), market)
    assert len(table) == len(ALL_STRATEGIES)
    assert {"total_return", "sharpe", "max_drawdown", "trades"} <= set(table.columns)
    assert table["trades"].sum() > 0


def test_all_strategies_have_docs_and_unique_names():
    strategies = build_all_strategies()
    names = [s.name for s in strategies]
    assert len(set(names)) == len(ALL_STRATEGIES)
    for strategy in strategies:
        assert len(strategy.describe()) > 40, f"{strategy.name} lacks a description"
