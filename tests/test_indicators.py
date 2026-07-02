"""Tests for the shared mathematical building blocks."""

import numpy as np
import pandas as pd
import pytest

from golden_strategies.data import make_fbm_prices, make_synthetic_ohlcv
from golden_strategies.indicators import (
    INV_PHI,
    PHI,
    fibonacci_numbers,
    rolling_hurst,
    rolling_katz_fd,
    williams_fractals,
)


def test_golden_mean_identities():
    assert PHI == pytest.approx(1.6180339887, abs=1e-9)
    assert PHI - 1 == pytest.approx(INV_PHI)  # phi's defining self-similarity
    assert PHI * INV_PHI == pytest.approx(1.0)


def test_fibonacci_numbers():
    assert fibonacci_numbers(8) == [1, 1, 2, 3, 5, 8, 13, 21]
    # Ratio of consecutive terms converges to phi.
    seq = fibonacci_numbers(20)
    assert seq[-1] / seq[-2] == pytest.approx(PHI, abs=1e-6)


def test_williams_fractals_confirmation_delay():
    """Fractal marks must land on/after the pivot, never before it."""
    df = make_synthetic_ohlcv(300, seed=7)
    wing = 2
    high_marks, low_marks = williams_fractals(df, wing=wing)

    for marks, column, comparator in [(high_marks, "high", np.greater_equal), (low_marks, "low", np.less_equal)]:
        confirmed = marks.dropna()
        assert len(confirmed) > 0
        for confirm_bar, price in confirmed.items():
            pivot_bar = confirm_bar - wing
            assert df.loc[pivot_bar, column] == pytest.approx(price)
            window = df[column].iloc[pivot_bar - wing : pivot_bar + wing + 1]
            assert comparator(price, window).all()


def test_hurst_separates_regimes():
    """Rolling Hurst should read high on persistent fBm, low on anti-persistent."""
    trending = make_fbm_prices(1200, hurst=0.8, seed=1)
    choppy = make_fbm_prices(1200, hurst=0.2, seed=1)
    h_trend = rolling_hurst(trending).dropna().median()
    h_chop = rolling_hurst(choppy).dropna().median()
    assert h_trend > 0.6
    assert h_chop < 0.4
    assert h_trend > h_chop + 0.2


def test_katz_fd_orders_smooth_vs_jagged():
    smooth = pd.Series(np.linspace(100, 120, 400)) + 0.01 * np.sin(np.arange(400))
    jagged = make_fbm_prices(400, hurst=0.2, seed=3)
    fd_smooth = rolling_katz_fd(smooth).dropna().median()
    fd_jagged = rolling_katz_fd(jagged).dropna().median()
    assert fd_smooth < fd_jagged


def test_synthetic_ohlcv_shape_and_sanity():
    df = make_synthetic_ohlcv(500, seed=11)
    assert list(df.columns) == ["open", "high", "low", "close", "volume"]
    assert len(df) == 500
    assert (df["high"] >= df[["open", "close"]].max(axis=1) - 1e-9).all()
    assert (df["low"] <= df[["open", "close"]].min(axis=1) + 1e-9).all()
    assert (df["volume"] > 0).all()
    assert df.notna().all().all()
