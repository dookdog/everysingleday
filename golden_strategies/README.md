# Golden Strategies

Ten automated trading strategies grown from five seeds: **novelty**, the
**Fibonacci sequence**, **fractals**, the **golden mean** (φ ≈ 1.618),
and **human invention** — celebrated algorithms and methods (golden-section
search, Williams fractals, the Hurst exponent, Mahalanobis distance,
ensemble learning) repurposed as trading machinery.

> Research and educational code only. Nothing here is financial advice,
> and synthetic-market backtests say nothing about live profitability.

## The ten strategies

| # | Strategy | Core idea | Themes |
|---|----------|-----------|--------|
| 1 | `golden_retracement` | Buy pullbacks into the 38.2–61.8% "golden pocket" of fractal-defined swing legs; stop beyond 78.6% | golden mean, fractals |
| 2 | `fib_time_cycles` | Trade only when a Fibonacci **bar count** (8, 13, 21, 34, 55) since the last swing coincides with price at a Fibonacci **retracement level** — time and price confluence | Fibonacci |
| 3 | `phi_ema_cascade` | EMA spans at consecutive powers of φ (4, 7, 11, 18, 29, 47); adjacent pairs vote on trend, votes blended with 1/φᵏ golden-decay weights | golden mean |
| 4 | `hurst_regime` | Rolling Hurst exponent (window 144) classifies the market's fractal character: follow momentum when H > 0.55, fade moves when H < 0.45, flat in between | fractals, human invention |
| 5 | `fractal_cascade` | Williams fractal breakouts at three Fibonacci wing sizes (2, 3, 5); scales blended with golden weights favouring coarse structure — full size only when the fractal hierarchy agrees | fractals, Fibonacci |
| 6 | `novelty_burst` | Online Mahalanobis-distance novelty detector over (return, range, volume); joins the direction of genuinely *new* shocks, sizing down along a Fibonacci decay schedule (5 → 8 → 13 bars) | novelty, human invention |
| 7 | `golden_section_momentum` | Walk-forward momentum whose lookback is re-tuned every 21 bars by **golden-section search** (Kiefer, 1953) maximising trailing 233-bar Sharpe | golden mean, human invention |
| 8 | `fibonacci_ladder` | Mean-reversion grid: scale in against ATR-normalised dislocations at Fibonacci rungs (1, 2, 3, 5 ATR) with Fibonacci sizes (1, 1, 2, 3); unwind at 1/φ ATR, disaster-stop at 8 ATR | Fibonacci |
| 9 | `phi_bands` | Fade closes beyond φ²-sigma bands back to an EMA-34 midline — but only when Katz fractal dimension > 1.382 says the path is choppy, not trending | golden mean, fractals |
| 10 | `golden_ensemble` | Meta-strategy blending five of the above; every 21 bars members are ranked on trailing 89-bar performance and weighted by golden decay 1/φ^rank | novelty, human invention |

Notice the deliberate diversity: 1, 8 and 9 are mean-reverters, 3, 5 and 7
are trend-followers, 2 and 6 are event traders, and 4 and 10 are regime
switchers that decide *which* behaviour to run.

## Layout

```
golden_strategies/
├── base.py          # Strategy interface (causal target positions in [-1, 1])
├── indicators.py    # φ, Fibonacci numbers, Williams fractals, Hurst, Katz FD, ATR
├── data.py          # fractional-Brownian-motion synthetic OHLCV with jump "novelty" events
├── backtest.py      # vectorised close-to-close backtester with turnover costs
└── strategies/      # one module per strategy
tests/               # 40 tests, incl. per-strategy no-lookahead causality checks
run_backtests.py     # compare all ten across trending / choppy / mixed fractal markets
```

## Quick start

```bash
pip install -r requirements.txt
python run_backtests.py                 # full comparison across three regimes
python -m pytest tests/ -q              # test suite
```

Using a single strategy programmatically:

```python
from golden_strategies import make_synthetic_ohlcv, run_backtest
from golden_strategies.strategies import HurstRegimeSwitcherStrategy

df = make_synthetic_ohlcv(n=1500, hurst=0.35, seed=7)   # choppy fractal market
result = run_backtest(HurstRegimeSwitcherStrategy(), df, cost_bps=5)
print(result.stats)
```

## Design rules the code follows

- **Causality is enforced, not assumed.** Williams fractal pivots are only
  marked once the confirming wing bars have printed; every rolling window
  ends at the current bar; the backtester shifts positions one bar so a
  signal earns the *next* bar's return. The test suite rewrites the future
  of the data and asserts every strategy's past signals are unchanged.
- **Fractal test beds.** Synthetic markets are exact fractional Brownian
  motion (Davies–Harte circulant embedding) whose Hurst exponent dials in
  trending vs. mean-reverting character, plus jump events with volume
  spikes so the novelty detector has real surprises to find.
- **Costs matter.** Every backtest charges basis points on turnover;
  the comparison table reports Sharpe, Sortino, max drawdown, exposure,
  trade count and win rate.
- **Parameters come from the theme.** Windows and thresholds are Fibonacci
  numbers (8, 13, 21, 34, 55, 89, 144, 233) or golden-ratio quantities
  (0.382, 0.618, φ, φ²) rather than arbitrary round numbers — a bias
  discipline as much as an aesthetic.
