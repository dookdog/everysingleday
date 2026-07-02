"""Aircraft configuration.

Every number here is a design parameter of the airframe, not a fitted
constant. The one parameter that actually matters -- the stall angle -- is
never set here: it is *measured* from the air the craft is flying through
(see strategy.WrightFlyer), exactly as the Wrights re-measured lift
coefficients in their own wind tunnel after Lilienthal's published tables
nearly killed them.
"""

from dataclasses import dataclass


@dataclass
class FlyerConfig:
    # ------------------------------------------------------------------
    # Instruments (aerodynamics.py)
    # ------------------------------------------------------------------
    glide_span: int = 50        # EMA span of the glide path (the trend line)
    slope_lookback: int = 10    # days used to read the glide-path slope
    vol_span: int = 30          # span of the daily-volatility gauge

    # ------------------------------------------------------------------
    # Lift and air density
    # ------------------------------------------------------------------
    vol_target_ann: float = 0.11   # annualised vol the wing is rated for
    rho_max: float = 1.5           # densest air we will trust (leverage cap on rho)
    v_scale: float = 0.05          # airspeed (slope/vol per day) at which lift saturates
    max_leverage: float = 1.5      # absolute structural limit on |weight|

    # ------------------------------------------------------------------
    # Stall-angle measurement (the wind-tunnel calibration)
    # ------------------------------------------------------------------
    alpha_stall_default: float = 2.5  # used until enough air has been sampled
    alpha_stall_min: float = 1.2      # never believe a stall angle below this
    alpha_bin_max: float = 4.0        # largest angle of attack we bin
    n_alpha_bins: int = 16
    min_bin_count: int = 25           # a bin needs this many samples to count
    min_pairs: int = 300              # total samples needed before trusting a fit
    calib_window: int = 1008          # trailing days of air used to calibrate (~4y)
    calib_step: int = 21              # recalibrate monthly

    # ------------------------------------------------------------------
    # Three-axis control system (controls.py)
    # ------------------------------------------------------------------
    taper_frac: float = 0.6      # elevator: begin pitching down at this fraction of stall
    recover_frac: float = 0.5    # stall latch releases below this fraction of stall
    roll_rate: float = 0.15      # max |d weight| per day when banking normally
    emergency_rate: float = 0.5  # control authority during stall recovery
    yaw_band: float = 0.03       # rudder dead-band: ignore corrections smaller than this
