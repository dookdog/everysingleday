"""The three-axis control system.

Everyone before the Wrights tried to build inherently stable aircraft that
needed no pilot. The Wrights accepted instability and mastered *control*:
wing-warping (roll), elevator (pitch), and -- after the 1901 gliders kept
skidding out of banked turns -- the coupled movable rudder (yaw) of 1902.
That third axis is what turned a glider that crashed in turns into an
aircraft.

The same three axes, on a portfolio:

    pitch  the angle-of-attack limiter. The lift curve C_L(alpha) tapers
           exposure as extension approaches the measured stall angle
           (computed in strategy.py, enforced here through the target).
    roll   the rate limiter on position changes. Banking into a new trend
           happens at a bounded roll rate, never as a step.
    yaw    the adverse-yaw dead-band. Small differences between target and
           actual position are *drag corrections*: chasing them costs
           turnover (drag) and produces no lift. The rudder ignores them.

Plus the piece that keeps the craft alive, the stall latch: when the angle
of attack exceeds the measured stall angle, the wing has separated. Cut
lift to zero at emergency control authority, and do not rebuild exposure
until alpha has come back inside recover_frac * stall -- pushing the nose
back up the moment the buffeting stops is how pilots die twice in one stall.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from flyer.config import FlyerConfig


@dataclass
class ControlState:
    weight: float = 0.0     # actual position currently held
    stalled: bool = False   # the stall latch


def apply_controls(
    state: ControlState,
    target: float,
    alpha: float,
    alpha_stall: float,
    cfg: FlyerConfig,
) -> ControlState:
    """One control cycle: instrument readings in, new stick position out."""
    a = abs(alpha) if np.isfinite(alpha) else 0.0

    # ---- stall latch -------------------------------------------------
    if state.stalled:
        if a < cfg.recover_frac * alpha_stall:
            stalled = False          # airflow reattached; resume normal flight
        else:
            stalled = True
    else:
        stalled = a >= alpha_stall

    if stalled:
        # Recovery: fly the nose to zero exposure at emergency authority.
        step = np.clip(-state.weight, -cfg.emergency_rate, cfg.emergency_rate)
        return ControlState(weight=state.weight + step, stalled=True)

    # ---- yaw dead-band ------------------------------------------------
    error = target - state.weight
    if abs(error) < cfg.yaw_band:
        return ControlState(weight=state.weight, stalled=False)

    # ---- roll-rate limit ----------------------------------------------
    step = np.clip(error, -cfg.roll_rate, cfg.roll_rate)
    return ControlState(weight=state.weight + step, stalled=False)
