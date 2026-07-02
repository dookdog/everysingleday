"""Strategy 10 - Golden Ensemble.

The most powerful human invention in this collection is not an
indicator but a *method*: combining diverse imperfect judges into one
better judge (Galton's wisdom of crowds, modern ensemble learning).
This meta-strategy runs five of the other systems side by side - the
phi EMA cascade, the Hurst regime switcher, the fractal cascade, the
novelty burst and the phi bands - and blends their target positions.

Every 21 bars each sub-strategy is ranked by its trailing 89-bar
simulated performance (both Fibonacci windows), and capital is
allocated by golden decay over the ranks: the best recent judge gets
weight ~1, the next ~1/phi, then 1/phi^2, and so on, normalised to sum
to one. The ensemble therefore keeps listening to every voice but lets
recent skill amplify influence at exactly the golden rate - novelty in
*which strategy is working* is itself the signal being traded.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from golden_strategies.base import Strategy
from golden_strategies.indicators import PHI
from golden_strategies.strategies.fractal_cascade import FractalCascadeStrategy
from golden_strategies.strategies.hurst_regime import HurstRegimeSwitcherStrategy
from golden_strategies.strategies.novelty_burst import NoveltyBurstStrategy
from golden_strategies.strategies.phi_bands import PhiVolatilityBandsStrategy
from golden_strategies.strategies.phi_ema_cascade import PhiEmaCascadeStrategy


class GoldenEnsembleStrategy(Strategy):
    __doc__ = __doc__

    name = "golden_ensemble"

    def __init__(
        self,
        members: tuple[Strategy, ...] | None = None,
        eval_window: int = 89,
        refit_every: int = 21,
    ):
        self.members = members or (
            PhiEmaCascadeStrategy(),
            HurstRegimeSwitcherStrategy(),
            FractalCascadeStrategy(),
            NoveltyBurstStrategy(),
            PhiVolatilityBandsStrategy(),
        )
        self.eval_window = eval_window
        self.refit_every = refit_every

    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        member_positions = np.column_stack(
            [m.target_positions(df).to_numpy(dtype=float) for m in self.members]
        )
        member_positions = np.nan_to_num(member_positions)

        # Simulated per-bar member returns with the same one-bar delay the
        # real backtester applies, so ranking is fully causal.
        asset_returns = df["close"].pct_change().fillna(0.0).to_numpy(dtype=float)
        held = np.vstack([np.zeros(member_positions.shape[1]), member_positions[:-1]])
        member_returns = held * asset_returns[:, None]

        n, k = member_positions.shape
        rank_weights = np.array([PHI**-r for r in range(k)])
        rank_weights /= rank_weights.sum()

        weights = np.full(k, 1.0 / k)
        combined = np.zeros(n)
        warmup = self.eval_window

        for t in range(n):
            if t >= warmup and (t - warmup) % self.refit_every == 0:
                trailing = member_returns[t - self.eval_window : t + 1].sum(axis=0)
                order = np.argsort(-trailing)  # best first
                weights = np.empty(k)
                weights[order] = rank_weights
            combined[t] = float(weights @ member_positions[t])

        return pd.Series(combined, index=df.index, name=self.name).clip(-1.0, 1.0)
