"""Strategy 6 - Novelty Burst.

Markets drift through the familiar most of the time; edges live where
something genuinely *new* happens. This strategy runs an online novelty
detector: each bar is summarised by a feature vector (log return,
true-range expansion, log volume) and scored by its Mahalanobis
distance from the rolling 89-bar history - the classic multivariate
"how surprising is this observation?" measure invented by
P. C. Mahalanobis in 1936.

When the distance crosses the novelty threshold the bar is treated as a
news-like event, and the strategy joins the direction of the shock
(post-announcement drift). The position then decays along a Fibonacci
schedule - full size for 5 bars, 1/phi until bar 8, 1/phi^2 until bar
13, then flat - so conviction fades as the novelty becomes the new
normal.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import INV_PHI, INV_PHI_SQ, atr


class NoveltyBurstStrategy(Strategy):
    __doc__ = __doc__

    name = "novelty_burst"

    def __init__(self, window: int = 89, threshold: float = 3.4, decay_bars: tuple[int, int, int] = (5, 8, 13)):
        self.window = window
        self.threshold = threshold
        self.decay_bars = decay_bars

    def _novelty_distance(self, features: np.ndarray) -> np.ndarray:
        """Causal rolling Mahalanobis distance of each row vs its history."""
        n = len(features)
        distances = np.full(n, np.nan)
        for t in range(self.window, n):
            history = features[t - self.window : t]
            if np.isnan(history).any() or np.isnan(features[t]).any():
                continue
            mean = history.mean(axis=0)
            cov = np.cov(history, rowvar=False)
            cov += 1e-9 * np.eye(cov.shape[0])
            delta = features[t] - mean
            distances[t] = float(np.sqrt(delta @ np.linalg.solve(cov, delta)))
        return distances

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        log_return = np.log(df["close"]).diff()
        range_expansion = (df["high"] - df["low"]) / atr(df)
        log_volume = np.log(df["volume"].clip(lower=1.0))

        features = np.column_stack(
            [
                log_return.to_numpy(dtype=float),
                range_expansion.to_numpy(dtype=float),
                log_volume.to_numpy(dtype=float),
            ]
        )
        distance = self._novelty_distance(features)
        shock_direction = np.sign(log_return.to_numpy(dtype=float))

        b1, b2, b3 = self.decay_bars
        positions = np.zeros(len(df))
        direction, age = 0.0, 0

        for t in range(len(df)):
            fresh_novelty = (
                not np.isnan(distance[t])
                and distance[t] >= self.threshold
                and shock_direction[t] != 0
            )
            if fresh_novelty:
                direction, age = shock_direction[t], 0
            elif direction != 0:
                age += 1
                if age >= b3:
                    direction = 0.0

            if direction != 0:
                size = 1.0 if age < b1 else INV_PHI if age < b2 else INV_PHI_SQ
                positions[t] = direction * size

        return pd.Series(positions, index=df.index, name=self.name)
