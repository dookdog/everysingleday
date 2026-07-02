"""Strategy 5 - Fractal Cascade.

Fractals are self-similar: the pattern that matters at one scale should
matter at every scale. Bill Williams' fractal pivot (a human invention
from *Trading Chaos*) is detected here at three Fibonacci wing sizes
(2, 3 and 5 bars), giving three self-similar maps of support and
resistance. Each scale runs a breakout state machine - long after the
close breaks the latest confirmed fractal high, short after it breaks
the fractal low, holding in between. The scales are blended with golden
weights that favour the *coarsest* structure (weight 1 for wing 5,
1/phi for wing 3, 1/phi^2 for wing 2), so a breakout only reaches full
size when the fractal hierarchy agrees across scales.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import PHI, williams_fractals


class FractalCascadeStrategy(Strategy):
    __doc__ = __doc__

    name = "fractal_cascade"

    def __init__(self, wings: tuple[int, ...] = (2, 3, 5)):
        self.wings = tuple(sorted(wings))

    @staticmethod
    def _scale_signal(df: pd.DataFrame, wing: int) -> np.ndarray:
        high_marks, low_marks = williams_fractals(df, wing=wing)
        fractal_high = high_marks.ffill().to_numpy(dtype=float)
        fractal_low = low_marks.ffill().to_numpy(dtype=float)
        close = df["close"].to_numpy(dtype=float)

        signal = np.zeros(len(df))
        state = 0.0
        for t in range(len(df)):
            if not np.isnan(fractal_high[t]) and close[t] > fractal_high[t]:
                state = 1.0
            elif not np.isnan(fractal_low[t]) and close[t] < fractal_low[t]:
                state = -1.0
            signal[t] = state
        return signal

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        raw_weights = np.array([PHI**-k for k in range(len(self.wings))])[::-1]
        weights = raw_weights / raw_weights.sum()

        combined = np.zeros(len(df))
        for weight, wing in zip(weights, self.wings):
            combined += weight * self._scale_signal(df, wing)

        return pd.Series(combined, index=df.index, name=self.name).clip(-1.0, 1.0)
