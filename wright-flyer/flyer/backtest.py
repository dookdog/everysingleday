"""The flight recorder.

A weight decided at the close of day t earns the asset's return on day
t+1. Costs are charged on turnover, in basis points of traded notional.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

TRADING_DAYS = 252.0


def backtest(
    prices: pd.Series,
    weights: pd.Series,
    cost_bps: float = 5.0,
) -> pd.DataFrame:
    """Simple-return backtest with lag-1 execution and linear costs."""
    px = prices.astype(float)
    asset_ret = px.pct_change().fillna(0.0)

    w = weights.reindex(px.index).fillna(0.0)
    held = w.shift(1).fillna(0.0)          # position during day t was decided at t-1
    turnover = w.diff().abs().fillna(w.abs())

    gross = held * asset_ret
    costs = turnover * cost_bps * 1e-4
    net = gross - costs

    return pd.DataFrame(
        {
            "asset_ret": asset_ret,
            "weight": w,
            "held": held,
            "turnover": turnover,
            "gross_ret": gross,
            "cost": costs,
            "net_ret": net,
            "equity": (1.0 + net).cumprod(),
        },
        index=px.index,
    )


def performance_metrics(net_ret: pd.Series) -> dict:
    r = net_ret.dropna()
    n = len(r)
    if n == 0:
        return {}

    mean_d, std_d = r.mean(), r.std()
    ann_ret = (1.0 + r).prod() ** (TRADING_DAYS / n) - 1.0
    ann_vol = std_d * np.sqrt(TRADING_DAYS)
    sharpe = (mean_d / std_d) * np.sqrt(TRADING_DAYS) if std_d > 0 else 0.0

    equity = (1.0 + r).cumprod()
    drawdown = equity / equity.cummax() - 1.0
    max_dd = drawdown.min()

    downside = r[r < 0].std()
    sortino = (mean_d / downside) * np.sqrt(TRADING_DAYS) if downside and downside > 0 else 0.0

    return {
        "ann_return": float(ann_ret),
        "ann_vol": float(ann_vol),
        "sharpe": float(sharpe),
        "sortino": float(sortino),
        "max_drawdown": float(max_dd),
        "calmar": float(ann_ret / abs(max_dd)) if max_dd < 0 else float("inf"),
        "n_days": int(n),
    }
