"""Real weather: daily prices from yfinance, cached locally as CSV."""

from __future__ import annotations

from pathlib import Path

import pandas as pd

CACHE_DIR = Path(__file__).resolve().parent.parent / "data_cache"


def load_prices(ticker: str, start: str = "1995-01-01", end: str | None = None) -> pd.Series:
    """Adjusted daily closes for ``ticker``, cached under data_cache/.

    The cache always holds the full downloaded history; the requested
    [start, end] window is sliced on the way out, so a cached series never
    silently changes the date range an experiment sees.
    """
    CACHE_DIR.mkdir(exist_ok=True)
    cache = CACHE_DIR / f"{ticker.replace('^', '_').replace('=', '_')}.csv"

    if cache.exists():
        df = pd.read_csv(cache, index_col=0, parse_dates=True)
        close = df["close"].astype(float)
    else:
        import yfinance as yf

        raw = yf.download(ticker, start="1990-01-01", progress=False, auto_adjust=True)
        if raw is None or len(raw) == 0:
            raise RuntimeError(f"no data returned for {ticker}")

        close = raw["Close"]
        if isinstance(close, pd.DataFrame):  # yfinance MultiIndex columns
            close = close.iloc[:, 0]
        close = close.dropna().astype(float)
        pd.DataFrame({"close": close}).to_csv(cache)

    return close.loc[start:end].rename(ticker)
