"""Vectorised close-to-close backtester with transaction costs.

Execution model: a target position computed on bar ``t`` is entered at
the close of bar ``t`` and earns the return from ``t`` to ``t+1``. This
is enforced with a single ``shift(1)`` on the position series, which is
the standard guard against look-ahead bias in vectorised backtests.
"""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy

TRADING_DAYS_PER_YEAR = 252


@dataclass
class BacktestResult:
    strategy_name: str
    equity_curve: pd.Series
    returns: pd.Series
    positions: pd.Series
    cost_bps: float
    stats: dict[str, float] = field(default_factory=dict)

    def summary_row(self) -> dict[str, float | str]:
        return {"strategy": self.strategy_name, **self.stats}


def _compute_stats(returns: pd.Series, positions: pd.Series) -> dict[str, float]:
    equity = (1.0 + returns).cumprod()
    total_return = float(equity.iloc[-1] - 1.0)

    ann_factor = np.sqrt(TRADING_DAYS_PER_YEAR)
    vol = float(returns.std())
    sharpe = float(returns.mean() / vol * ann_factor) if vol > 0 else 0.0

    downside = returns[returns < 0]
    downside_vol = float(downside.std()) if len(downside) > 1 else 0.0
    sortino = float(returns.mean() / downside_vol * ann_factor) if downside_vol > 0 else 0.0

    running_max = equity.cummax()
    drawdown = equity / running_max - 1.0
    max_drawdown = float(drawdown.min())

    active = positions != 0
    exposure = float(active.mean())
    trades = int((positions.diff().abs() > 1e-12).sum())

    win_rate = float((returns[active & (returns != 0)] > 0).mean()) if active.any() else 0.0

    return {
        "total_return": round(total_return, 4),
        "sharpe": round(sharpe, 3),
        "sortino": round(sortino, 3),
        "max_drawdown": round(max_drawdown, 4),
        "exposure": round(exposure, 3),
        "trades": trades,
        "win_rate": round(win_rate, 3),
    }


def run_backtest(
    strategy: Strategy,
    df: pd.DataFrame,
    cost_bps: float = 5.0,
) -> BacktestResult:
    """Backtest ``strategy`` on OHLCV frame ``df``.

    ``cost_bps`` is charged on each unit of turnover (position change),
    covering commission plus slippage.
    """
    target = strategy.target_positions(df)
    if not target.index.equals(df.index):
        raise ValueError(f"{strategy.name}: positions index misaligned with data")

    target = target.astype(float).fillna(0.0).clip(-1.0, 1.0)

    # Enter at the close of the signal bar -> earn the next bar's return.
    held = target.shift(1).fillna(0.0)

    asset_returns = df["close"].pct_change().fillna(0.0)
    turnover = held.diff().abs().fillna(held.abs())
    costs = turnover * (cost_bps / 1e4)

    strategy_returns = held * asset_returns - costs
    equity = (1.0 + strategy_returns).cumprod()

    return BacktestResult(
        strategy_name=strategy.name,
        equity_curve=equity,
        returns=strategy_returns,
        positions=held,
        cost_bps=cost_bps,
        stats=_compute_stats(strategy_returns, held),
    )


def run_many(
    strategies: list[Strategy], df: pd.DataFrame, cost_bps: float = 5.0
) -> pd.DataFrame:
    """Backtest every strategy on ``df``; return a tidy comparison table."""
    rows = [run_backtest(s, df, cost_bps=cost_bps).summary_row() for s in strategies]
    return pd.DataFrame(rows).set_index("strategy")
