"""Experiment 1 -- Kitty Hawk, 1900-1901: the gliders that disappointed.

The Wrights' first gliders were built faithfully from Lilienthal's
published lift tables. They produced far less lift than the tables
promised, and the 1901 season ended with Wilbur saying man would not fly
in a thousand years.

Reproduction: fly two naive craft through the wind tunnel's synthetic
airmass and log exactly how they fail.

    Glider A ("published tables"): pure trend-following. Full lift
        whenever the glide path slopes -- no angle-of-attack instrument
        at all. The pilot cannot feel the stall coming.
    Glider B ("more wing"): the same craft with more leverage. The
        pre-Wright instinct: if it doesn't fly, add area/power.

Expected finding, and the reason experiment 2 exists: both gliders make
lift in steady trends and give it back in blow-off cracks, and more wing
only deepens the crash. Lift is not the problem. Control is.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import numpy as np
import pandas as pd

from flyer.backtest import backtest, performance_metrics
from flyer.config import FlyerConfig
from flyer.aerodynamics import instruments
from flyer.wind_tunnel import WindTunnel

REPORT = Path(__file__).resolve().parent.parent / "report"


def naive_trend_weights(prices: pd.Series, cfg: FlyerConfig, leverage: float) -> pd.Series:
    """Glider with no angle-of-attack instrument: ride any slope, always."""
    panel = instruments(prices, cfg)
    w = panel["direction"] * panel["rho"] * panel["q"] * leverage
    return w.fillna(0.0).clip(-cfg.max_leverage * leverage, cfg.max_leverage * leverage)


def main() -> None:
    REPORT.mkdir(exist_ok=True)
    cfg = FlyerConfig()
    tunnel = WindTunnel(seed=42)
    prices, regimes = tunnel.generate(n_days=4000)

    rows = []
    crash_logs = {}
    for name, lev in [("glider_A_tables", 1.0), ("glider_B_more_wing", 2.0)]:
        w = naive_trend_weights(prices, cfg, leverage=lev)
        bt = backtest(prices, w)
        m = performance_metrics(bt["net_ret"])

        # Lift by regime: where is it made, where is it destroyed?
        by_regime = bt["net_ret"].groupby(regimes).sum()
        m.update({f"pnl_{k}": float(v) for k, v in by_regime.items()})
        m["craft"] = name
        rows.append(m)
        crash_logs[name] = by_regime

    df = pd.DataFrame(rows).set_index("craft")
    df.to_csv(REPORT / "01_glider_results.csv")

    print("=" * 72)
    print("EXPERIMENT 1: the gliders (synthetic airmass, 4000 days)")
    print("=" * 72)
    with pd.option_context("display.width", 200, "display.float_format", "{:.3f}".format):
        print(df[["ann_return", "ann_vol", "sharpe", "max_drawdown"]])
        print()
        print("Cumulative net P&L by regime (log-return sum):")
        print(pd.DataFrame(crash_logs))

    crack_a = crash_logs["glider_A_tables"].get("blowoff_crack", 0.0)
    crack_b = crash_logs["glider_B_more_wing"].get("blowoff_crack", 0.0)
    print()
    print(f"Blow-off CRACK P&L, glider A: {crack_a:+.3f}   glider B: {crack_b:+.3f}")
    print("Finding: the craft makes lift in trends and hands it back when the")
    print("airflow separates. More wing (B) deepens the crash instead of fixing it.")
    print("=> The problem is not lift. It is knowing the stall angle. Build a tunnel.")


if __name__ == "__main__":
    main()
