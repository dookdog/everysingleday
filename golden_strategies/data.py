"""Synthetic market data built on fractal mathematics.

Real markets exhibit fractal self-affinity (Mandelbrot), so the test
data here is generated from fractional Brownian motion (fBm) whose
roughness is controlled by the Hurst exponent ``H``:

* ``H > 0.5``  -> persistent, trending paths
* ``H = 0.5``  -> ordinary Brownian motion
* ``H < 0.5``  -> anti-persistent, mean-reverting chop

On top of the fBm backbone we add drift regimes and rare jump events so
that novelty-detection strategies have genuine surprises to find.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

GOLDEN_RATIO = (1.0 + np.sqrt(5.0)) / 2.0


def _fgn_davies_harte(n: int, hurst: float, rng: np.random.Generator) -> np.ndarray:
    """Exact fractional Gaussian noise via circulant embedding (Davies-Harte)."""
    k = np.arange(n + 1, dtype=float)
    two_h = 2.0 * hurst
    autocov = 0.5 * (np.abs(k - 1) ** two_h - 2.0 * k**two_h + (k + 1) ** two_h)

    # First row of the 2n x 2n circulant matrix that embeds the covariance.
    row = np.concatenate([autocov, autocov[n - 1 : 0 : -1]])
    eigenvalues = np.fft.fft(row).real
    if np.any(eigenvalues < -1e-8):
        raise np.linalg.LinAlgError("circulant embedding not non-negative definite")
    eigenvalues = np.clip(eigenvalues, 0.0, None)

    m = 2 * n
    g1 = rng.standard_normal(m)
    g2 = rng.standard_normal(m)
    w = np.empty(m, dtype=complex)
    w[0] = np.sqrt(eigenvalues[0] / m) * g1[0]
    w[n] = np.sqrt(eigenvalues[n] / m) * g2[0]
    idx = np.arange(1, n)
    half = np.sqrt(eigenvalues[idx] / (2.0 * m))
    w[idx] = half * (g1[idx] + 1j * g2[idx])
    w[m - idx] = np.conj(w[idx])
    return np.fft.fft(w).real[:n]


def _fgn_cholesky(n: int, hurst: float, rng: np.random.Generator) -> np.ndarray:
    """Fallback exact sampler; O(n^2) memory, used only if embedding fails."""
    k = np.abs(np.subtract.outer(np.arange(n), np.arange(n))).astype(float)
    two_h = 2.0 * hurst
    cov = 0.5 * (np.abs(k - 1) ** two_h - 2.0 * k**two_h + (k + 1) ** two_h)
    cov[np.diag_indices(n)] = 1.0
    chol = np.linalg.cholesky(cov + 1e-10 * np.eye(n))
    return chol @ rng.standard_normal(n)


def fractional_gaussian_noise(
    n: int, hurst: float = 0.5, seed: int | np.random.Generator | None = None
) -> np.ndarray:
    """Return ``n`` samples of unit-variance fractional Gaussian noise."""
    if not 0.0 < hurst < 1.0:
        raise ValueError(f"hurst must be in (0, 1), got {hurst}")
    rng = seed if isinstance(seed, np.random.Generator) else np.random.default_rng(seed)
    try:
        return _fgn_davies_harte(n, hurst, rng)
    except np.linalg.LinAlgError:
        return _fgn_cholesky(n, hurst, rng)


def make_fbm_prices(
    n: int = 1500,
    hurst: float = 0.5,
    volatility: float = 0.015,
    start_price: float = 100.0,
    seed: int | np.random.Generator | None = None,
) -> pd.Series:
    """Geometric fractional Brownian motion close prices."""
    noise = fractional_gaussian_noise(n, hurst=hurst, seed=seed)
    log_price = np.log(start_price) + np.cumsum(volatility * noise)
    index = pd.RangeIndex(n, name="bar")
    return pd.Series(np.exp(log_price), index=index, name="close")


def make_synthetic_ohlcv(
    n: int = 1500,
    hurst: float = 0.5,
    volatility: float = 0.015,
    drift_strength: float = 0.0,
    jump_probability: float = 0.004,
    jump_scale: float = 5.0,
    start_price: float = 100.0,
    seed: int | np.random.Generator | None = None,
) -> pd.DataFrame:
    """Full OHLCV frame: fBm backbone + drift regimes + rare jump events.

    Jumps model "novelty": sudden news-like shocks accompanied by volume
    spikes, which is exactly what the novelty-detection strategies hunt.
    """
    rng = seed if isinstance(seed, np.random.Generator) else np.random.default_rng(seed)

    log_returns = volatility * fractional_gaussian_noise(n, hurst=hurst, seed=rng)

    # Drift regimes: sign flips at Fibonacci-flavoured random intervals.
    if drift_strength != 0.0:
        drift = np.zeros(n)
        i, direction = 0, 1.0
        while i < n:
            length = int(rng.choice([89, 144, 233, 377]))
            drift[i : i + length] = direction * drift_strength * volatility
            direction *= -1.0
            i += length
        log_returns = log_returns + drift

    # Rare jumps = novelty events.
    jump_mask = rng.random(n) < jump_probability
    jump_sizes = rng.standard_normal(n) * jump_scale * volatility
    log_returns = log_returns + np.where(jump_mask, jump_sizes, 0.0)

    log_close = np.log(start_price) + np.cumsum(log_returns)
    close = np.exp(log_close)
    open_ = np.concatenate([[start_price], close[:-1]])

    # Intrabar range scales with realised move plus a noise floor.
    span = np.abs(log_returns) + volatility * (0.3 + 0.7 * rng.random(n))
    high = np.maximum(open_, close) * np.exp(0.5 * span * rng.random(n))
    low = np.minimum(open_, close) * np.exp(-0.5 * span * rng.random(n))

    base_volume = 1e6 * np.exp(0.4 * rng.standard_normal(n))
    surprise = np.abs(log_returns) / volatility
    volume = base_volume * (1.0 + 2.0 * surprise + np.where(jump_mask, 8.0, 0.0))

    index = pd.RangeIndex(n, name="bar")
    return pd.DataFrame(
        {
            "open": open_,
            "high": high,
            "low": low,
            "close": close,
            "volume": volume,
        },
        index=index,
    )


def make_market_suite(n: int = 1500, seed: int = 42) -> dict[str, pd.DataFrame]:
    """Three archetypal markets used by the runner and the tests."""
    rng = np.random.default_rng(seed)
    return {
        "trending": make_synthetic_ohlcv(
            n, hurst=0.65, drift_strength=0.25, jump_probability=0.002, seed=rng
        ),
        "choppy": make_synthetic_ohlcv(
            n, hurst=0.35, drift_strength=0.0, jump_probability=0.002, seed=rng
        ),
        "mixed": make_synthetic_ohlcv(
            n, hurst=0.5, drift_strength=0.15, jump_probability=0.006, seed=rng
        ),
    }
