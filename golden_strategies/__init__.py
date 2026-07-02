"""Golden Strategies.

Ten automated trading strategies built around novelty detection, the
Fibonacci sequence, fractal geometry, the golden mean, and celebrated
human inventions (golden-section search, Williams fractals, the Hurst
exponent, ensemble learning).

Everything here is research code for educational exploration - not
financial advice.
"""

from golden_strategies.backtest import BacktestResult, run_backtest
from golden_strategies.data import make_fbm_prices, make_synthetic_ohlcv
from golden_strategies.strategies import ALL_STRATEGIES, build_all_strategies

__all__ = [
    "ALL_STRATEGIES",
    "BacktestResult",
    "build_all_strategies",
    "make_fbm_prices",
    "make_synthetic_ohlcv",
    "run_backtest",
]
