#!/usr/bin/env python3
"""Backtest all ten golden strategies across three fractal market regimes.

Usage:
    python run_backtests.py [--bars 1500] [--seed 42] [--cost-bps 5]

Markets are synthetic geometric fractional Brownian motion with drift
regimes and jump events (see golden_strategies.data):

* trending  - Hurst 0.65 with drift regime flips (persistent)
* choppy    - Hurst 0.35, no drift (anti-persistent)
* mixed     - Hurst 0.50 with drift and frequent jumps

Research/educational code only - not financial advice.
"""

from __future__ import annotations

import argparse

import pandas as pd

from golden_strategies.backtest import run_many
from golden_strategies.data import make_market_suite
from golden_strategies.strategies import build_all_strategies


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--bars", type=int, default=1500, help="bars per market")
    parser.add_argument("--seed", type=int, default=42, help="RNG seed")
    parser.add_argument("--cost-bps", type=float, default=5.0, help="cost per unit turnover")
    args = parser.parse_args()

    pd.set_option("display.width", 140)

    markets = make_market_suite(n=args.bars, seed=args.seed)
    strategies = build_all_strategies()

    print("=" * 100)
    print("GOLDEN STRATEGIES - novelty, Fibonacci, fractals, the golden mean, human invention")
    print(f"{args.bars} bars/market, {args.cost_bps:.0f} bps costs, seed {args.seed}")
    print("=" * 100)

    for market_name, df in markets.items():
        buy_hold = df["close"].iloc[-1] / df["close"].iloc[0] - 1.0
        print(f"\n--- market: {market_name}  (buy & hold: {buy_hold:+.1%}) ---")
        table = run_many(strategies, df, cost_bps=args.cost_bps)
        print(table.to_string())

    print("\nStrategy descriptions:")
    for strategy in strategies:
        print(f"\n* {strategy.name}: {strategy.describe()}")


if __name__ == "__main__":
    main()
