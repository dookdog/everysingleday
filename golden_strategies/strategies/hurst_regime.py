"""Strategy 4 - Hurst Regime Switcher.

Mandelbrot taught that markets are fractal, and the Hurst exponent H is
the single number that summarises the fractal character of a price
path: H > 0.5 means moves tend to persist (trend), H < 0.5 means they
tend to reverse (chop). Instead of betting on one behaviour, this
strategy measures a rolling Hurst exponent over a Fibonacci window of
144 bars and *switches personality*:

* persistent regime (H > 0.55): follow 21-bar momentum;
* anti-persistent regime (H < 0.45): fade the last 8-bar move;
* indeterminate zone in between: stay flat - no edge, no trade.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import rolling_hurst


class HurstRegimeSwitcherStrategy(Strategy):
    __doc__ = __doc__

    name = "hurst_regime"

    def __init__(
        self,
        window: int = 144,
        trend_threshold: float = 0.55,
        revert_threshold: float = 0.45,
        momentum_lookback: int = 21,
        reversion_lookback: int = 8,
    ):
        self.window = window
        self.trend_threshold = trend_threshold
        self.revert_threshold = revert_threshold
        self.momentum_lookback = momentum_lookback
        self.reversion_lookback = reversion_lookback

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        close = df["close"]
        hurst = rolling_hurst(close, window=self.window)

        momentum = np.sign(close - close.shift(self.momentum_lookback))
        reversion = -np.sign(close - close.shift(self.reversion_lookback))

        position = pd.Series(0.0, index=df.index)
        position = position.mask(hurst > self.trend_threshold, momentum)
        position = position.mask(hurst < self.revert_threshold, reversion)
        return position.fillna(0.0).rename(self.name)
