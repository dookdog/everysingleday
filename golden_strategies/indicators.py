"""Shared mathematical building blocks.

Fibonacci numbers, the golden mean, Williams fractal pivots, rolling
Hurst exponents and Katz fractal dimension. All rolling computations are
strictly causal: the value at bar ``t`` depends only on bars ``<= t``.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

PHI = (1.0 + np.sqrt(5.0)) / 2.0  # the golden mean, ~1.6180339887
INV_PHI = 1.0 / PHI  # ~0.618
INV_PHI_SQ = 1.0 / PHI**2  # ~0.382

#: Classic retracement grid derived from powers of 1/phi (0.5 is the
#: traditional half-way interloper that traders keep on the grid).
FIB_RETRACEMENTS = (0.236, 0.382, 0.5, 0.618, 0.786)


def fibonacci_numbers(count: int, start: int = 1) -> list[int]:
    """First ``count`` Fibonacci numbers beginning at ``start`` (1 -> 1,1,2...)."""
    a, b = 1, 1
    seq = []
    while len(seq) < count + start - 1:
        seq.append(a)
        a, b = b, a + b
    return seq[start - 1 :]


def ema(series: pd.Series, span: float) -> pd.Series:
    return series.ewm(span=span, adjust=False).mean()


def atr(df: pd.DataFrame, period: int = 21) -> pd.Series:
    """Average True Range (Wilder) - period defaults to Fibonacci 21."""
    prev_close = df["close"].shift(1)
    true_range = pd.concat(
        [
            df["high"] - df["low"],
            (df["high"] - prev_close).abs(),
            (df["low"] - prev_close).abs(),
        ],
        axis=1,
    ).max(axis=1)
    return true_range.ewm(alpha=1.0 / period, adjust=False).mean()


def williams_fractals(df: pd.DataFrame, wing: int = 2) -> tuple[pd.Series, pd.Series]:
    """Bill Williams fractal pivots with ``wing`` bars on each side.

    A fractal high at bar ``t`` requires ``high[t]`` to exceed the highs of
    the ``wing`` bars on both sides. Because that needs future bars, the
    signal is only *confirmed* ``wing`` bars later; the returned series
    are marked at the confirmation bar so that they are safe to trade on.

    Returns ``(fractal_high_price, fractal_low_price)``: NaN except on
    confirmation bars, where they carry the pivot's price.
    """
    high, low = df["high"], df["low"]
    window = 2 * wing + 1

    is_high = high.rolling(window, center=True).max() == high
    is_low = low.rolling(window, center=True).min() == low

    # Shift by `wing` so the mark lands on the bar where the pattern is
    # fully visible (no look-ahead).
    confirmed_high = (is_high & high.notna()).shift(wing, fill_value=False)
    confirmed_low = (is_low & low.notna()).shift(wing, fill_value=False)

    high_price = high.shift(wing).where(confirmed_high)
    low_price = low.shift(wing).where(confirmed_low)
    return high_price, low_price


def rolling_hurst(series: pd.Series, window: int = 144, min_lag: int = 2, max_lag: int = 34) -> pd.Series:
    """Rolling Hurst exponent via the variance-of-differences method.

    For self-affine series, ``Var[x(t+lag) - x(t)] ~ lag^(2H)``; H is the
    slope of the log-log regression. Window/lags default to Fibonacci
    numbers 144 and 34.
    """
    log_price = np.log(series.to_numpy(dtype=float))
    n = len(log_price)
    lags = np.unique(np.geomspace(min_lag, max_lag, num=8).astype(int))
    log_lags = np.log(lags.astype(float))

    out = np.full(n, np.nan)
    for t in range(window - 1, n):
        segment = log_price[t - window + 1 : t + 1]
        stds = np.array([np.std(segment[lag:] - segment[:-lag]) for lag in lags])
        if np.any(stds <= 0):
            continue
        slope = np.polyfit(log_lags, np.log(stds), 1)[0]
        out[t] = min(max(slope, 0.0), 1.0)
    return pd.Series(out, index=series.index, name="hurst")


def rolling_katz_fd(series: pd.Series, window: int = 55) -> pd.Series:
    """Rolling Katz fractal dimension of the price curve.

    FD ~ 1 for smooth trends, -> 1.5+ for jagged, space-filling chop.
    """
    values = series.to_numpy(dtype=float)
    n = len(values)
    out = np.full(n, np.nan)
    for t in range(window - 1, n):
        seg = values[t - window + 1 : t + 1]
        steps = np.abs(np.diff(seg))
        total_length = steps.sum()
        if total_length <= 0:
            continue
        max_dist = np.abs(seg - seg[0]).max()
        if max_dist <= 0:
            continue
        # Katz: FD = log10(m) / (log10(m) + log10(d/L))
        m = len(seg) - 1
        log_m = np.log10(m)
        out[t] = log_m / (log_m + np.log10(max_dist / total_length))
    return pd.Series(out, index=series.index, name="katz_fd")


def rolling_zscore(series: pd.Series, window: int) -> pd.Series:
    mean = series.rolling(window).mean()
    std = series.rolling(window).std()
    return (series - mean) / std.replace(0.0, np.nan)


def last_swing_levels(df: pd.DataFrame, wing: int = 2) -> pd.DataFrame:
    """Most recent confirmed fractal swing high/low, forward-filled.

    Also reports which of the two swings was confirmed more recently
    (+1 if the low is fresher -> latest leg is up, -1 otherwise).
    """
    high_marks, low_marks = williams_fractals(df, wing=wing)
    swing_high = high_marks.ffill()
    swing_low = low_marks.ffill()

    bar_numbers = pd.Series(np.arange(len(df)), index=df.index, dtype=float)
    high_time = bar_numbers.where(high_marks.notna()).ffill()
    low_time = bar_numbers.where(low_marks.notna()).ffill()
    leg_up = (low_time > high_time).astype(float) - (low_time < high_time).astype(float)

    return pd.DataFrame(
        {
            "swing_high": swing_high,
            "swing_low": swing_low,
            "swing_high_bar": high_time,
            "swing_low_bar": low_time,
            "leg_direction": leg_up,
        }
    )
