#!/usr/bin/env python3
"""
Clean result CSV files by interpolating missing data and smoothing noise.

The script walks over the provided glob of CSV files (defaults to
``results/*.csv``), fills gaps via linear interpolation, applies a rolling
median to dampen noise, and writes the cleaned data to a sibling folder
(``results/cleaned`` by default).
"""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Iterable, List, Optional

import numpy as np
import pandas as pd


def _coerce_numeric(df: pd.DataFrame) -> pd.DataFrame:
    """Convert object columns to numeric when possible."""
    result = df.copy()
    object_cols = result.select_dtypes(include=["object"]).columns
    for col in object_cols:
        result[col] = pd.to_numeric(result[col], errors="ignore")
    return result


def clean_dataframe(
    df: pd.DataFrame,
    *,
    time_column: Optional[str],
    window_size: int,
) -> pd.DataFrame:
    """
    Interpolate missing values and smooth numeric columns.

    Parameters
    ----------
    df:
        Raw dataframe read from a CSV file.
    time_column:
        Name of the time axis column that should pass through untouched.
    window_size:
        Odd-sized rolling window used for the median filter.
    """
    if window_size < 1:
        raise ValueError("window_size must be >= 1")

    cleaned = df.replace(r"^\s*$", np.nan, regex=True)
    cleaned = _coerce_numeric(cleaned)

    numeric_cols: List[str] = cleaned.select_dtypes(include=[np.number]).columns.tolist()
    if time_column and time_column in numeric_cols:
        numeric_cols.remove(time_column)

    if not numeric_cols:
        return cleaned

    if window_size > 1:
        # cleaned[numeric_cols] = (cleaned[numeric_cols].rolling(window=window_size, min_periods=1, center=True).median())
        cleaned[numeric_cols] = (cleaned[numeric_cols].rolling(window=window_size, min_periods=1, center=True).quantile(0.50))

    # Fill gaps through linear interpolation and nearest-value propagation.
    cleaned[numeric_cols] = cleaned[numeric_cols].interpolate(
        method="linear", limit_direction="both"
    )
    cleaned[numeric_cols] = cleaned[numeric_cols].ffill().bfill()


    return cleaned


def _validate_window_size(value: str) -> int:
    size = int(value)
    if size < 1 or size % 2 == 0:
        raise argparse.ArgumentTypeError("window size must be a positive odd integer")
    return size


def _resolve_files(input_dir: Path, pattern: str) -> List[Path]:
    files = sorted(
        [
            path
            for path in input_dir.glob(pattern)
            if path.is_file() and path.suffix.lower() == ".csv"
        ]
    )
    if not files:
        raise FileNotFoundError(f"No CSV files matched pattern '{pattern}' in '{input_dir}'")
    return files


def parse_args(argv: Optional[Iterable[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Filter noise and missing values from result CSVs.")
    parser.add_argument(
        "--input-dir",
        type=Path,
        default=Path("results"),
        help="Directory that stores raw CSV result files (default: results).",
    )
    parser.add_argument(
        "--pattern",
        default="*.csv",
        help="Glob pattern relative to --input-dir identifying CSV files (default: *.csv).",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("results/cleaned"),
        help="Destination for cleaned CSV files (default: results/cleaned).",
    )
    parser.add_argument(
        "--time-column",
        default="time",
        help="Name of the time column to leave untouched (default: time).",
    )
    parser.add_argument(
        "--window-size",
        type=_validate_window_size,
        default=5,
        help="Odd rolling window size for the median noise filter (default: 5).",
    )

    return parser.parse_args(argv)


def main(argv: Optional[Iterable[str]] = None) -> int:
    args = parse_args(argv)

    input_dir: Path = args.input_dir
    output_dir: Path = args.output_dir

    if not input_dir.exists():
        raise FileNotFoundError(f"Input directory '{input_dir}' does not exist")

    files = _resolve_files(input_dir, args.pattern)

    output_dir.mkdir(parents=True, exist_ok=True)

    for source in files:
        df = pd.read_csv(source, skipinitialspace=True)
        cleaned = clean_dataframe(
            df,
            time_column=args.time_column,
            window_size=args.window_size,
        )
        destination = output_dir / f"{source.stem}.csv"
        cleaned.to_csv(destination, index=False)
        print(f"Wrote cleaned file: {destination}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

