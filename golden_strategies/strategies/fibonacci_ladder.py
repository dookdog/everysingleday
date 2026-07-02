"""Strategy 8 - Fibonacci Ladder.

A mean-reversion accumulator that turns the Fibonacci sequence into a
position-sizing schedule. Price is measured as an ATR-normalised
deviation from a slow anchor (EMA-55). As the deviation stretches
through rungs placed at Fibonacci multiples of ATR (1, 2, 3, 5), the
strategy scales in *against* the move with rung sizes proportional to
the Fibonacci numbers themselves (1, 1, 2, 3 -> deeper dislocations get
larger adds, exactly how the sequence grows).

The whole ladder is unwound when price snaps back to within 1/phi ATR
of the anchor (profit) or stretches beyond the next Fibonacci rung of 8
ATR (disaster stop - the dislocation is a trend, not noise). Maximum
exposure is capped at 1x by construction since the rung sizes are
normalised to sum to one.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import INV_PHI, atr, ema


class FibonacciLadderStrategy(Strategy):
    __doc__ = __doc__

    name = "fibonacci_ladder"

    def __init__(
        self,
        anchor_span: int = 55,
        atr_period: int = 21,
        rung_levels: tuple[float, ...] = (1.0, 2.0, 3.0, 5.0),
        rung_sizes: tuple[float, ...] = (1.0, 1.0, 2.0, 3.0),
        take_profit: float = INV_PHI,
        stop_level: float = 8.0,
    ):
        if len(rung_levels) != len(rung_sizes):
            raise ValueError("rung_levels and rung_sizes must have equal length")
        self.anchor_span = anchor_span
        self.atr_period = atr_period
        self.rung_levels = rung_levels
        sizes = np.asarray(rung_sizes, dtype=float)
        self.rung_fractions = sizes / sizes.sum()
        self.take_profit = take_profit
        self.stop_level = stop_level

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        close = df["close"]
        anchor = ema(close, self.anchor_span)
        volatility = atr(df, period=self.atr_period)
        deviation = ((close - anchor) / volatility).to_numpy(dtype=float)

        n = len(df)
        positions = np.zeros(n)
        exposure, side = 0.0, 0  # side: +1 laddering long into dips, -1 short into rips
        rungs_filled = 0

        for t in range(max(self.anchor_span, self.atr_period), n):
            d = deviation[t]
            if np.isnan(d):
                continue

            if side != 0:
                stretched = -side * d  # how far price sits on the ladder side, in ATRs
                if stretched <= self.take_profit or stretched >= self.stop_level:
                    exposure, side, rungs_filled = 0.0, 0, 0

            if side == 0 and abs(d) >= self.rung_levels[0]:
                side = -1 if d > 0 else 1
                rungs_filled = 0

            if side != 0:
                stretched = -side * deviation[t]
                while rungs_filled < len(self.rung_levels) and stretched >= self.rung_levels[rungs_filled]:
                    exposure += side * self.rung_fractions[rungs_filled]
                    rungs_filled += 1

            positions[t] = exposure

        return pd.Series(positions, index=df.index, name=self.name).clip(-1.0, 1.0)
