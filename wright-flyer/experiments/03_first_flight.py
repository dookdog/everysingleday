"""Experiment 3 -- Kill Devil Hills, December 17, 1903: first flight.

The full craft -- instruments, tunnel-measured stall angle, three-axis
controls, stall latch -- flown through real air. Four flights, like the
four flights of that morning:

    SPY  (US large cap)     GLD  (gold)
    QQQ  (US tech)          EFA  (developed ex-US)

Each flight is compared against the 1901 glider (same lift equation, no
stall control) and against staying on the ground (buy & hold). Costs:
5 bps per unit of traded notional on both strategies.

The claim under test is the Wright claim: control beats raw lift. The
craft should not out-lift buy-and-hold in a straight-up decade -- it
should fly at a fraction of the volatility, refuse the stalls, and keep
its drawdowns shallow enough to keep flying.

Final measurement: the squadron. Four craft flown in formation (equal
weight, over the dates all four are airborne) -- if each craft's residual
turbulence is its own, formation flight is where the low volatility and
shallow drawdowns compound into a risk-adjusted edge.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

from flyer.backtest import backtest, performance_metrics
from flyer.config import FlyerConfig
from flyer.data import load_prices
from flyer.strategy import WrightFlyer

from importlib import import_module

glider = import_module("01_kites_and_gliders")

REPORT = Path(__file__).resolve().parent.parent / "report"
FLIGHTS = ["SPY", "QQQ", "GLD", "EFA"]
COST_BPS = 5.0


def fly_one(ticker: str, cfg: FlyerConfig):
    prices = load_prices(ticker, start="1998-01-01")

    craft = WrightFlyer(cfg)
    log = craft.fly(prices)
    bt_flyer = backtest(prices, log["weight"], cost_bps=COST_BPS)

    w_glider = glider.naive_trend_weights(prices, cfg, leverage=1.0)
    bt_glider = backtest(prices, w_glider, cost_bps=COST_BPS)

    hold = prices.pct_change().fillna(0.0)

    rows = {
        "wright_flyer": performance_metrics(bt_flyer["net_ret"]),
        "glider_1901": performance_metrics(bt_glider["net_ret"]),
        "buy_and_hold": performance_metrics(hold),
    }
    metrics = pd.DataFrame(rows).T
    metrics.index.name = f"{ticker}"

    curves = pd.DataFrame(
        {
            "wright_flyer": bt_flyer["equity"],
            "glider_1901": bt_glider["equity"],
            "buy_and_hold": (1 + hold).cumprod(),
        }
    )
    return metrics, curves, log, bt_flyer, hold


def main() -> None:
    REPORT.mkdir(exist_ok=True)
    cfg = FlyerConfig()

    all_metrics = []
    net_returns = {}
    hold_returns = {}
    fig, axes = plt.subplots(2, 2, figsize=(13, 8))

    for ax, ticker in zip(axes.ravel(), FLIGHTS):
        metrics, curves, log, bt_flyer, hold = fly_one(ticker, cfg)
        all_metrics.append(metrics.assign(ticker=ticker))
        net_returns[ticker] = bt_flyer["net_ret"]
        hold_returns[ticker] = hold

        curves.plot(ax=ax, logy=True, lw=1.1, color=["#c0392b", "#95a5a6", "#2a7fbd"])
        stalled = log.index[log["stalled"]]
        if len(stalled):
            ax.scatter(
                stalled,
                curves.loc[stalled, "wright_flyer"],
                s=4,
                color="black",
                zorder=5,
                label="stall latch engaged",
            )
        ax.set_title(f"{ticker} -- flight vs glider vs ground")
        ax.set_ylabel("equity (log)")
        ax.legend(fontsize=8)

        print("=" * 72)
        print(f"FLIGHT: {ticker}")
        print("=" * 72)
        with pd.option_context("display.width", 200, "display.float_format", "{:.3f}".format):
            print(metrics[["ann_return", "ann_vol", "sharpe", "sortino", "max_drawdown", "calmar"]])
        n_stall_days = int(log["stalled"].sum())
        print(f"days with stall latch engaged: {n_stall_days} "
              f"({100.0 * n_stall_days / len(log):.1f}% of flight time)")
        print()

    fig.tight_layout()
    out = REPORT / "03_first_flights.png"
    fig.savefig(out, dpi=130)
    print(f"Flight tracks saved to {out}")

    summary = pd.concat(all_metrics)
    summary.to_csv(REPORT / "03_flight_metrics.csv")

    flyer_rows = summary[summary.index == "wright_flyer"]
    hold_rows = summary[summary.index == "buy_and_hold"]
    print()
    print("Across the four flights:")
    print(f"  mean Sharpe, Wright Flyer : {flyer_rows['sharpe'].mean():.2f}")
    print(f"  mean Sharpe, buy & hold   : {hold_rows['sharpe'].mean():.2f}")
    print(f"  worst drawdown, Flyer     : {flyer_rows['max_drawdown'].min():.1%}")
    print(f"  worst drawdown, hold      : {hold_rows['max_drawdown'].min():.1%}")

    # ---- the squadron: all four craft in formation ---------------------
    flyer_panel = pd.DataFrame(net_returns).dropna()
    hold_panel = pd.DataFrame(hold_returns).dropna()
    squad = flyer_panel.mean(axis=1)
    hold_squad = hold_panel.mean(axis=1)

    squad_metrics = pd.DataFrame(
        {
            "squadron (4 flyers, equal weight)": performance_metrics(squad),
            "hold basket (4 assets, equal weight)": performance_metrics(hold_squad),
        }
    ).T
    squad_metrics.to_csv(REPORT / "03_squadron_metrics.csv")

    corr = flyer_panel.corr()
    print()
    print("=" * 72)
    print(f"THE SQUADRON ({flyer_panel.index[0].date()} to {flyer_panel.index[-1].date()})")
    print("=" * 72)
    print("pairwise correlation of flyer daily returns:")
    with pd.option_context("display.float_format", "{:.2f}".format):
        print(corr)
    print()
    with pd.option_context("display.width", 200, "display.float_format", "{:.3f}".format):
        print(squad_metrics[["ann_return", "ann_vol", "sharpe", "sortino", "max_drawdown", "calmar"]])

    fig2, ax2 = plt.subplots(figsize=(10, 5))
    (1 + squad).cumprod().plot(ax=ax2, color="#c0392b", lw=1.4,
                               label="squadron (4 Wright Flyers)")
    (1 + hold_squad).cumprod().plot(ax=ax2, color="#2a7fbd", lw=1.4,
                                    label="hold basket (same 4 assets)")
    ax2.set_yscale("log")
    ax2.set_ylabel("equity (log)")
    ax2.set_title("Formation flight vs. staying on the ground")
    ax2.legend()
    fig2.tight_layout()
    fig2.savefig(REPORT / "03_squadron.png", dpi=130)
    print(f"\nSquadron track saved to {REPORT / '03_squadron.png'}")


if __name__ == "__main__":
    main()
