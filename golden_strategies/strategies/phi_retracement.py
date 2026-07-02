"""Strategy 1 - Golden Retracement.

The oldest trick built on the golden mean: after a swing leg completes,
price tends to retrace a phi-derived fraction of the leg before the
trend resumes. Swings are defined objectively by confirmed Williams
fractal pivots (no hindsight swing picking).

Trade logic for an up leg (most recent fractal pivot is a high above the
prior fractal low):

* the *golden pocket* is the band between the 38.2% and 61.8%
  retracements of the leg;
* go long when the close dips inside the pocket;
* take profit when price recovers to the swing high;
* stop out if the retracement deepens beyond 78.6% (leg invalidated).

Down legs are traded as the mirror image.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import last_swing_levels


class GoldenRetracementStrategy(Strategy):
    __doc__ = __doc__

    name = "golden_retracement"

    def __init__(self, wing: int = 2, pocket: tuple[float, float] = (0.382, 0.618), stop: float = 0.786):
        self.wing = wing
        self.pocket = pocket
        self.stop = stop

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        swings = last_swing_levels(df, wing=self.wing)
        close = df["close"].to_numpy(dtype=float)
        swing_high = swings["swing_high"].to_numpy(dtype=float)
        swing_low = swings["swing_low"].to_numpy(dtype=float)
        high_bar = swings["swing_high_bar"].to_numpy(dtype=float)
        low_bar = swings["swing_low_bar"].to_numpy(dtype=float)

        near, far = self.pocket
        positions = np.zeros(len(df))
        state = 0

        for t in range(len(df)):
            if np.isnan(swing_high[t]) or np.isnan(swing_low[t]):
                positions[t] = 0.0
                continue

            leg = swing_high[t] - swing_low[t]
            if leg <= 0:
                positions[t] = state = 0
                continue

            up_leg = high_bar[t] > low_bar[t]
            if up_leg:
                pocket_top = swing_high[t] - near * leg
                pocket_bottom = swing_high[t] - far * leg
                stop_level = swing_high[t] - self.stop * leg
                if state == 1:
                    if close[t] >= swing_high[t] or close[t] <= stop_level:
                        state = 0  # target reached or leg invalidated
                elif state == -1:
                    state = 0  # leg flipped upward, abandon shorts
                elif pocket_bottom <= close[t] <= pocket_top:
                    state = 1
            else:
                pocket_bottom = swing_low[t] + near * leg
                pocket_top = swing_low[t] + far * leg
                stop_level = swing_low[t] + self.stop * leg
                if state == -1:
                    if close[t] <= swing_low[t] or close[t] >= stop_level:
                        state = 0
                elif state == 1:
                    state = 0
                elif pocket_bottom <= close[t] <= pocket_top:
                    state = -1

            positions[t] = state

        return pd.Series(positions, index=df.index, name=self.name)
