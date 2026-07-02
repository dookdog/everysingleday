"""The ten golden strategies."""

from __future__ import annotations

from golden_strategies.base import Strategy
from golden_strategies.strategies.fib_time_cycles import FibonacciTimeCycleStrategy
from golden_strategies.strategies.fibonacci_ladder import FibonacciLadderStrategy
from golden_strategies.strategies.fractal_cascade import FractalCascadeStrategy
from golden_strategies.strategies.golden_ensemble import GoldenEnsembleStrategy
from golden_strategies.strategies.golden_section_search import GoldenSectionMomentumStrategy
from golden_strategies.strategies.hurst_regime import HurstRegimeSwitcherStrategy
from golden_strategies.strategies.novelty_burst import NoveltyBurstStrategy
from golden_strategies.strategies.phi_bands import PhiVolatilityBandsStrategy
from golden_strategies.strategies.phi_ema_cascade import PhiEmaCascadeStrategy
from golden_strategies.strategies.phi_retracement import GoldenRetracementStrategy

ALL_STRATEGIES: tuple[type[Strategy], ...] = (
    GoldenRetracementStrategy,
    FibonacciTimeCycleStrategy,
    PhiEmaCascadeStrategy,
    HurstRegimeSwitcherStrategy,
    FractalCascadeStrategy,
    NoveltyBurstStrategy,
    GoldenSectionMomentumStrategy,
    FibonacciLadderStrategy,
    PhiVolatilityBandsStrategy,
    GoldenEnsembleStrategy,
)


def build_all_strategies() -> list[Strategy]:
    """Instantiate every strategy with its default parameters."""
    return [cls() for cls in ALL_STRATEGIES]


__all__ = [
    "ALL_STRATEGIES",
    "build_all_strategies",
    "GoldenRetracementStrategy",
    "FibonacciTimeCycleStrategy",
    "PhiEmaCascadeStrategy",
    "HurstRegimeSwitcherStrategy",
    "FractalCascadeStrategy",
    "NoveltyBurstStrategy",
    "GoldenSectionMomentumStrategy",
    "FibonacciLadderStrategy",
    "PhiVolatilityBandsStrategy",
    "GoldenEnsembleStrategy",
]
