"""Experiment 2 -- Dayton, autumn 1901: the wind tunnel.

The Wrights put model wings on a bicycle-spoke balance inside a wooden
tunnel and measured lift directly, discovering Lilienthal's tables were
wrong for their wings. Here the "model wing" is the angle-of-attack
instrument, the "balance" reads mean next-day lift per alpha bin, and the
question is the same one they asked: where exactly does the airflow
separate?

Three measurements:
    1. The lift curve C_L(alpha) measured on synthetic air whose stall
       angle is PLANTED at a known value -- blow-offs in the tunnel crack
       when, and only when, extension crosses planted_stall = 2.5. Does
       the balance recover that number?
    2. The estimator's stability across 20 independent airmasses
       (different tunnel seeds), scored against the planted truth.
    3. The same lift curve measured on real air (SPY): does the real
       atmosphere have the same separated-flow shape, and at what angle?
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from flyer.aerodynamics import instruments, measure_stall_angle
from flyer.config import FlyerConfig
from flyer.wind_tunnel import WindTunnel

REPORT = Path(__file__).resolve().parent.parent / "report"


def lift_curve_of(prices: pd.Series, cfg: FlyerConfig):
    panel = instruments(prices, cfg)
    return measure_stall_angle(panel["alpha"], panel["fwd_lift"], cfg)


PLANTED_STALL = 2.5


def main() -> None:
    REPORT.mkdir(exist_ok=True)
    cfg = FlyerConfig()

    # -- 1. lift curve on air with a planted stall angle -----------------
    tunnel = WindTunnel(seed=42, planted_stall=PLANTED_STALL)
    prices, _ = tunnel.generate(n_days=6000)
    stall_syn, table_syn = lift_curve_of(prices, cfg)

    # -- 2. stability across independent airmasses ----------------------
    stalls = []
    for seed in range(20):
        p, _ = WindTunnel(seed=seed, planted_stall=PLANTED_STALL).generate(n_days=6000)
        s, _ = lift_curve_of(p, cfg)
        stalls.append(s)
    stalls = np.array(stalls)

    # -- 3. the real atmosphere ------------------------------------------
    table_real, stall_real = None, None
    try:
        from flyer.data import load_prices

        spy = load_prices("SPY", start="1995-01-01")
        stall_real, table_real = lift_curve_of(spy, cfg)
    except Exception as e:  # offline environments still get 1 and 2
        print(f"[real air unavailable: {e}]")

    # -- report ----------------------------------------------------------
    print("=" * 72)
    print("EXPERIMENT 2: the wind tunnel")
    print("=" * 72)
    print(f"Planted stall angle (ground truth)  : {PLANTED_STALL:.2f}")
    print(f"Measured stall angle, seed 42       : {stall_syn:.2f}")
    print(
        f"Across 20 independent airmasses     : "
        f"mean {stalls.mean():.2f}, std {stalls.std():.2f}, "
        f"range [{stalls.min():.2f}, {stalls.max():.2f}]"
    )
    if stall_real is not None:
        print(f"Measured stall angle, real air (SPY): {stall_real:.2f}")

    fig, axes = plt.subplots(1, 2 if table_real is not None else 1, figsize=(12, 4.5), squeeze=False)

    def draw(ax, table, stall, title):
        ok = table["count"] >= cfg.min_bin_count
        ax.bar(
            table.loc[ok, "alpha_mid"],
            table.loc[ok, "mean_fwd_lift"],
            width=(cfg.alpha_bin_max / cfg.n_alpha_bins) * 0.9,
            color=np.where(table.loc[ok, "mean_fwd_lift"] >= 0, "#2a7fbd", "#c0392b"),
        )
        ax.axvline(stall, color="black", ls="--", lw=1.2, label=f"stall angle = {stall:.2f}")
        ax.axhline(0, color="gray", lw=0.8)
        ax.set_xlabel(r"angle of attack $\alpha$ (trend-signed stretch, in $\sigma$)")
        ax.set_ylabel("mean next-day lift (vol units)")
        ax.set_title(title)
        ax.legend()

    draw(axes[0][0], table_syn, stall_syn, "Wind tunnel (synthetic air, 6000 days)")
    axes[0][0].axvline(
        PLANTED_STALL, color="#27ae60", ls=":", lw=1.6,
        label=f"planted truth = {PLANTED_STALL:.2f}",
    )
    axes[0][0].legend()
    if table_real is not None:
        draw(axes[0][1], table_real, stall_real, "Real air (SPY, 1995-present)")

    fig.tight_layout()
    out = REPORT / "02_lift_curves.png"
    fig.savefig(out, dpi=130)
    print(f"\nLift curves saved to {out}")

    if table_syn is not None:
        table_syn.to_csv(REPORT / "02_lift_table_synthetic.csv", index=False)
    if table_real is not None:
        table_real.to_csv(REPORT / "02_lift_table_spy.csv", index=False)

    print("Finding: lift holds while the airflow is attached, then separates --")
    print("mean next-day lift turns negative past a measurable angle of attack.")
    print("The wing must taper before that angle and cut lift beyond it.")


if __name__ == "__main__":
    main()
