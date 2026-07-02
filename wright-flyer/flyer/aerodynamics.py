"""Instruments and the wing.

The Wrights' central insight was that flight was not a power problem but a
measurement-and-control problem. This module turns a price series into the
quantities a pilot needs, and holds the wind-tunnel balance used to measure
the wing's lift curve -- including the one number that keeps the craft
alive: the stall angle.

Mapping (market -> airmass):

    glide path       EMA of log price; the chord line the craft flies along.
    angle of attack  alpha: the stretch of price away from the glide path,
                     standardized by its own stationary scale and signed
                     along the trend direction. Large positive alpha means
                     price is over-extended in the direction of its trend.
    airspeed         |glide-path slope| / volatility. Speed through the
                     airmass, not over ground: a trend signal-to-noise ratio.
    air density      rho ~ 1 / realized volatility. Thin air (high vol)
                     gives less lift per unit of exposure, so the craft
                     carries less of it.
    lift curve       C_L(alpha) = E[next-day lift | alpha]. Rises and holds
                     while the airflow is attached, then separates: past the
                     stall angle, extension along the trend stops paying and
                     starts reverting.

The lift equation assembled in strategy.py is deliberately the aerodynamic
one:  weight = direction * rho * q(v) * C_L(alpha),  the trading analogue of
L = 1/2 rho v^2 S C_L.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from flyer.config import FlyerConfig

TRADING_DAYS = 252.0


def _ema_deviation_scale(span: int) -> float:
    """Stationary std of (x_t - EMA_t) for a unit-variance random walk.

    With EMA_t = lam*x_t + (1-lam)*EMA_{t-1} and D_t = x_t - EMA_t, the
    recursion D_t = (1-lam)(D_{t-1} + r_t) gives
    Var(D) = (1-lam)^2 / (lam * (2 - lam)).
    """
    lam = 2.0 / (span + 1.0)
    return np.sqrt((1.0 - lam) ** 2 / (lam * (2.0 - lam)))


def instruments(prices: pd.Series, cfg: FlyerConfig) -> pd.DataFrame:
    """Read the instrument panel from a price series.

    Everything here is causal: the row at time t uses prices up to and
    including t, nothing later. The single exception is ``fwd_lift``, which
    by construction contains the NEXT day's return -- it exists only as the
    wind-tunnel balance's measurement target and must never be read at or
    before the decision time it is aligned to (strategy.py only ever reads
    ``fwd_lift[u]`` for u <= t-1 when deciding at t).
    """
    logp = np.log(prices.astype(float))
    r = logp.diff()

    glide = logp.ewm(span=cfg.glide_span, adjust=False).mean()
    slope = (glide - glide.shift(cfg.slope_lookback)) / cfg.slope_lookback
    vol = r.ewm(span=cfg.vol_span, adjust=False).std()

    stretch_scale = vol * _ema_deviation_scale(cfg.glide_span)
    alpha_raw = (logp - glide) / stretch_scale

    direction = np.sign(slope)
    alpha = direction * alpha_raw

    airspeed = slope.abs() / vol
    q = np.tanh((airspeed / cfg.v_scale) ** 2)  # dynamic pressure factor in [0, 1)

    rho = (cfg.vol_target_ann / (vol * np.sqrt(TRADING_DAYS))).clip(upper=cfg.rho_max)

    # Realized lift over the NEXT day, in the direction the wing points,
    # standardized by today's turbulence. Calibration target only.
    fwd_lift = direction * r.shift(-1) / vol

    return pd.DataFrame(
        {
            "log_ret": r,
            "glide": glide,
            "slope": slope,
            "vol": vol,
            "alpha_raw": alpha_raw,
            "alpha": alpha,
            "direction": direction,
            "airspeed": airspeed,
            "q": q,
            "rho": rho,
            "fwd_lift": fwd_lift,
        },
        index=prices.index,
    )


def lift_coefficient(alpha, alpha_stall: float, taper_frac: float):
    """The wing's lift coefficient as flown.

    Full lift while the airflow is attached (|alpha| below the taper point),
    a linear pitch-down as the angle of attack approaches stall (the
    elevator doing its job early), and zero beyond the stall angle.
    Symmetric: too far above trend is a blow-off, too far below is a dive,
    and the wing trusts neither.
    """
    a = np.abs(alpha)
    taper = taper_frac * alpha_stall
    denom = max(alpha_stall - taper, 1e-12)
    return np.clip((alpha_stall - a) / denom, 0.0, 1.0)


def measure_stall_angle(alpha, fwd_lift, cfg: FlyerConfig):
    """The wind-tunnel balance: find where the airflow separates.

    Lilienthal died trusting published lift tables, so the Wrights built a
    tunnel and re-measured everything. In that spirit: never assume the
    stall angle, measure it from the air actually flown through.

    Method: over pairs (alpha_u, fwd_lift_u) with alpha_u >= 0 (extension
    along the trend), bin by angle of attack and choose the bin edge that
    maximizes the *total lift accumulated below it*,
    sum over bins b < edge of (count_b * mean_fwd_lift_b). While the flow
    is attached each additional bin adds lift and the running sum rises;
    past separation each bin subtracts, so the sum peaks exactly at the
    angle where mean lift turns negative. Aggregating count-weighted sums
    keeps the measurement stable where individual bin means are noise, and
    unlike a tail-mean scan it is not dragged downward by a heavy negative
    region far above the true separation point.

    Returns (alpha_stall, table) where table is a per-bin DataFrame for
    plotting the measured lift curve, or (default, None) when there is not
    yet enough air on the balance.
    """
    alpha = np.asarray(alpha, dtype=float)
    fwd_lift = np.asarray(fwd_lift, dtype=float)

    ok = np.isfinite(alpha) & np.isfinite(fwd_lift)
    mask = ok & (alpha >= 0.0) & (alpha <= cfg.alpha_bin_max)
    a, f = alpha[mask], fwd_lift[mask]

    if a.size < cfg.min_pairs:
        return cfg.alpha_stall_default, None

    edges = np.linspace(0.0, cfg.alpha_bin_max, cfg.n_alpha_bins + 1)

    # Per-bin lift curve (for the plots and the flight log).
    idx = np.clip(np.digitize(a, edges) - 1, 0, cfg.n_alpha_bins - 1)
    counts = np.bincount(idx, minlength=cfg.n_alpha_bins)
    sums = np.bincount(idx, weights=f, minlength=cfg.n_alpha_bins)
    with np.errstate(invalid="ignore", divide="ignore"):
        means = np.where(counts > 0, sums / np.maximum(counts, 1), np.nan)
    table = pd.DataFrame(
        {
            "alpha_lo": edges[:-1],
            "alpha_hi": edges[1:],
            "alpha_mid": 0.5 * (edges[:-1] + edges[1:]),
            "count": counts,
            "mean_fwd_lift": means,
        }
    )

    # Cumulative lift accumulated below each edge: G_k = sum of bin sums
    # for bins 0..k-1. G_0 = 0 corresponds to "everything is stalled".
    cumulative = np.concatenate([[0.0], np.cumsum(sums)])
    stall = float(edges[int(np.argmax(cumulative))])

    stall = max(stall, cfg.alpha_stall_min)
    return stall, table
