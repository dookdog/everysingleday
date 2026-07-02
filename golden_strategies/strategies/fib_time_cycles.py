"""Strategy 2 - Fibonacci Time Cycles.

Fibonacci analysis is usually applied to *price*; this strategy applies
the sequence to *time* as well and demands confluence of both. Counting
bars from the most recent confirmed fractal swing, turning points are
hypothesised at Fibonacci counts (8, 13, 21, 34, 55). A trade fires only
when a Fibonacci bar-count arrives while price is simultaneously sitting
near a Fibonacci retracement level of the current leg - the time spiral
and the price grid must agree. Entries follow the direction of the
prevailing leg (retracement is presumed complete) and are held for 13
bars.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import FIB_RETRACEMENTS, atr, last_swing_levels


class FibonacciTimeCycleStrategy(Strategy):
    __doc__ = __doc__

    name = "fib_time_cycles"

    def __init__(
        self,
        wing: int = 2,
        counts: tuple[int, ...] = (8, 13, 21, 34, 55),
        hold_bars: int = 13,
        tolerance_atr: float = 0.618,
    ):
        self.wing = wing
        self.counts = set(counts)
        self.hold_bars = hold_bars
        self.tolerance_atr = tolerance_atr

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        swings = last_swing_levels(df, wing=self.wing)
        close = df["close"].to_numpy(dtype=float)
        swing_high = swings["swing_high"].to_numpy(dtype=float)
        swing_low = swings["swing_low"].to_numpy(dtype=float)
        high_bar = swings["swing_high_bar"].to_numpy(dtype=float)
        low_bar = swings["swing_low_bar"].to_numpy(dtype=float)
        tolerance = self.tolerance_atr * atr(df).to_numpy(dtype=float)

        positions = np.zeros(len(df))
        state, bars_in_trade = 0, 0

        for t in range(len(df)):
            if state != 0:
                bars_in_trade += 1
                if bars_in_trade >= self.hold_bars:
                    state = 0

            if np.isnan(swing_high[t]) or np.isnan(swing_low[t]) or np.isnan(tolerance[t]):
                positions[t] = 0.0
                state = 0
                continue

            leg = swing_high[t] - swing_low[t]
            latest_swing_bar = max(high_bar[t], low_bar[t])
            count = int(t - latest_swing_bar)

            if state == 0 and leg > 0 and count in self.counts:
                up_leg = high_bar[t] > low_bar[t]
                anchor, sign = (swing_high[t], 1) if up_leg else (swing_low[t], -1)
                for ratio in FIB_RETRACEMENTS:
                    level = anchor - sign * ratio * leg
                    if abs(close[t] - level) <= tolerance[t]:
                        state = sign
                        bars_in_trade = 0
                        break

            positions[t] = state

        return pd.Series(positions, index=df.index, name=self.name)
