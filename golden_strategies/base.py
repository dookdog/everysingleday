"""Strategy interface shared by all ten systems."""

from __future__ import annotations

import abc

import pandas as pd


class Strategy(abc.ABC):
    """A causal signal generator.

    Subclasses implement :meth:`target_positions`, returning the desired
    exposure per bar in ``[-1, +1]`` (fractions allowed for scaled
    entries). The value at bar ``t`` may only use information from bars
    ``<= t``; the backtester applies the position from bar ``t+1``
    onwards, so signals earn the *next* bar's return.
    """

    #: Human-readable name, set by subclasses.
    name: str = "strategy"

    @abc.abstractmethod
    def target_positions(self, df: pd.DataFrame) -> pd.Series:
        """Return the target exposure series aligned with ``df.index``."""

    def describe(self) -> str:
        """Title plus first body paragraph of the class docstring."""
        doc = (self.__doc__ or "").strip()
        paragraphs = [p.replace("\n", " ").strip() for p in doc.split("\n\n")]
        return " ".join(paragraphs[:2]).strip()

    def __repr__(self) -> str:  # pragma: no cover - cosmetic
        return f"<{type(self).__name__} {self.name!r}>"


def clamp_positions(positions: pd.Series) -> pd.Series:
    """Utility for subclasses: fill warmup NaNs with flat, clip to [-1, 1]."""
    return positions.fillna(0.0).clip(-1.0, 1.0)
