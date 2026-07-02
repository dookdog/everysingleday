"""The Wright Flyer: a trading strategy invented the way the Wrights invented flight.

Price action is treated as an airmass. The strategy is an aircraft:

* aerodynamics.py -- the instruments: airspeed, angle of attack, air density,
  and the lift curve C_L(alpha) with its stall point.
* controls.py     -- the three-axis control system and the stall/recovery
  state machine that turns instrument readings into a position.
* wind_tunnel.py  -- a synthetic, regime-switching market used to *measure*
  the lift curve instead of trusting published tables.
* backtest.py     -- the flight recorder: causal backtests with costs.
* data.py         -- real-weather data (yfinance) with local caching.
"""

from flyer.config import FlyerConfig
from flyer.strategy import WrightFlyer
from flyer.backtest import backtest, performance_metrics
from flyer.wind_tunnel import WindTunnel

__all__ = ["FlyerConfig", "WrightFlyer", "backtest", "performance_metrics", "WindTunnel"]
