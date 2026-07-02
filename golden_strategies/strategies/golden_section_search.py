"""Strategy 7 - Golden-Section Momentum.

Golden-section search (Kiefer, 1953) is a human invention that uses the
golden mean itself to find the extremum of a one-dimensional function
with the fewest possible evaluations. Here the algorithm is put to work
*inside* the strategy: every 21 bars it re-optimises the lookback of a
simple momentum rule (position = sign of the trailing L-bar return) by
maximising the rule's Sharpe ratio over the last 233 bars of history.

The probe points shrink the search bracket [8, 89] by a factor of
1/phi per iteration, and the tuned lookback is then traded forward
until the next re-fit - a fully causal, walk-forward, self-adapting
momentum machine whose optimiser literally embodies the golden ratio.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import INV_PHI


def _momentum_sharpe(log_price: np.ndarray, lookback: int) -> float:
    """In-sample Sharpe of the sign-momentum rule, honest one-bar delay."""
    if lookback + 2 >= len(log_price):
        return -np.inf
    momentum = np.sign(log_price[lookback:] - log_price[:-lookback])
    bar_returns = np.diff(log_price[lookback:])
    strategy_returns = momentum[:-1] * bar_returns  # signal at t earns t -> t+1
    std = strategy_returns.std()
    if std <= 0:
        return -np.inf
    return float(strategy_returns.mean() / std)


def golden_section_search_int(
    objective, low: int, high: int, iterations: int = 12
) -> int:
    """Maximise ``objective`` over integers in [low, high] via golden sections."""
    a, b = float(low), float(high)
    c = b - (b - a) * INV_PHI
    d = a + (b - a) * INV_PHI
    fc, fd = objective(round(c)), objective(round(d))

    for _ in range(iterations):
        if b - a <= 1.0:
            break
        if fc >= fd:
            b, d, fd = d, c, fc
            c = b - (b - a) * INV_PHI
            fc = objective(round(c))
        else:
            a, c, fc = c, d, fd
            d = a + (b - a) * INV_PHI
            fd = objective(round(d))

    candidates = {round(a), round(b), round(c), round(d)}
    return max(candidates, key=objective)


class GoldenSectionMomentumStrategy(Strategy):
    __doc__ = __doc__

    name = "golden_section_momentum"

    def __init__(
        self,
        min_lookback: int = 8,
        max_lookback: int = 89,
        fit_window: int = 233,
        refit_every: int = 21,
    ):
        self.min_lookback = min_lookback
        self.max_lookback = max_lookback
        self.fit_window = fit_window
        self.refit_every = refit_every

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        log_price = np.log(df["close"].to_numpy(dtype=float))
        n = len(log_price)
        warmup = self.fit_window + self.max_lookback

        positions = np.zeros(n)
        lookback = None

        for t in range(warmup, n):
            if lookback is None or (t - warmup) % self.refit_every == 0:
                window = log_price[t - self.fit_window - self.max_lookback : t + 1]
                lookback = golden_section_search_int(
                    lambda lb: _momentum_sharpe(window, lb),
                    self.min_lookback,
                    self.max_lookback,
                )
            positions[t] = np.sign(log_price[t] - log_price[t - lookback])

        return pd.Series(positions, index=df.index, name=self.name)
