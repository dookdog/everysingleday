"""Strategy 9 - Phi Volatility Bands.

Bollinger bands re-imagined through the golden mean, with a fractal
gatekeeper. Around an EMA-34 midline, an inner band is drawn at
phi standard deviations and an outer band at phi^2 - so the outer band
is exactly one golden ratio further out than the inner, mirroring how
volatility clusters at self-similar scales.

Excursions beyond the *outer* band are treated as exhaustion and faded
(short strength, buy weakness); the trade is unwound when price crosses
back through the midline, or abandoned at a phi^3 extreme band (the
next golden scale out) if the excursion keeps running. Because fading
a genuine breakout is how band
traders die, the strategy only trades when the rolling Katz fractal
dimension of price (window 55) exceeds 1.382 - i.e. the path is jagged
and space-filling, the signature of mean-reverting chop rather than a
smooth trending line.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import PHI, ema, rolling_katz_fd


class PhiVolatilityBandsStrategy(Strategy):
    __doc__ = __doc__

    name = "phi_bands"

    def __init__(
        self,
        span: int = 34,
        fd_window: int = 55,
        fd_threshold: float = 1.382,
    ):
        self.span = span
        self.fd_window = fd_window
        self.fd_threshold = fd_threshold

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        close = df["close"]
        mid = ema(close, self.span)
        sigma = close.rolling(self.span).std()

        upper_outer = (mid + PHI**2 * sigma).to_numpy(dtype=float)
        lower_outer = (mid - PHI**2 * sigma).to_numpy(dtype=float)
        upper_extreme = (mid + PHI**3 * sigma).to_numpy(dtype=float)
        lower_extreme = (mid - PHI**3 * sigma).to_numpy(dtype=float)
        midline = mid.to_numpy(dtype=float)
        fd = rolling_katz_fd(close, window=self.fd_window).to_numpy(dtype=float)
        prices = close.to_numpy(dtype=float)

        positions = np.zeros(len(df))
        state = 0
        blocked_side = 0  # after a stop-out, that side re-arms only once
        # price closes back inside the outer bands.

        for t in range(len(df)):
            if np.isnan(fd[t]) or np.isnan(upper_outer[t]):
                positions[t] = 0.0
                continue

            if state == 1:  # long a washout, target the midline
                if prices[t] >= midline[t]:
                    state = 0
                elif prices[t] < lower_extreme[t]:
                    state, blocked_side = 0, 1
            elif state == -1:
                if prices[t] <= midline[t]:
                    state = 0
                elif prices[t] > upper_extreme[t]:
                    state, blocked_side = 0, -1

            if blocked_side != 0 and lower_outer[t] <= prices[t] <= upper_outer[t]:
                blocked_side = 0

            if state == 0 and fd[t] >= self.fd_threshold:
                if prices[t] > upper_outer[t] and blocked_side != -1:
                    state = -1
                elif prices[t] < lower_outer[t] and blocked_side != 1:
                    state = 1

            positions[t] = state

        return pd.Series(positions, index=df.index, name=self.name)
