#!/usr/bin/env python3
"""Inspect SSP4SIM DuckDB recorder metadata and compare recorded model runs."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Iterable, Optional

try:
    import duckdb
except ImportError as exc:  # pragma: no cover - depends on local environment
    raise SystemExit(
        "Missing Python dependency: duckdb. Install it with `python -m pip install duckdb`."
    ) from exc


METADATA_TABLE = "ssp4sim_metadata"


def quote_identifier(name: str) -> str:
    return '"' + name.replace('"', '""') + '"'


def connect_database(path: Path) -> duckdb.DuckDBPyConnection:
    if not path.exists():
        raise FileNotFoundError(f"DuckDB file does not exist: {path}")
    return duckdb.connect(str(path), read_only=True)


def metadata_frame(connection: duckdb.DuckDBPyConnection):
    return connection.sql(
        f"""
        SELECT table_name, model, storage_name, source_storage_name, created_at_s
        FROM {quote_identifier(METADATA_TABLE)}
        ORDER BY model, storage_name, created_at_s, table_name
        """
    ).df()


def print_metadata(connection: duckdb.DuckDBPyConnection) -> None:
    metadata = metadata_frame(connection)
    if metadata.empty:
        print(f"{METADATA_TABLE} is empty")
        return

    print(metadata.to_string(index=False))


def table_for_run(
    connection: duckdb.DuckDBPyConnection,
    *,
    model: str,
    storage_name: Optional[str],
    run_index: int,
) -> str:
    metadata = metadata_frame(connection)
    matches = metadata[metadata["model"] == model]
    if storage_name is not None:
        matches = matches[matches["storage_name"] == storage_name]

    if matches.empty:
        storage_detail = f" and storage_name={storage_name!r}" if storage_name else ""
        raise ValueError(f"No metadata row found for model={model!r}{storage_detail}")

    matches = matches.sort_values(["created_at_s", "table_name"]).reset_index(drop=True)
    if run_index < 0 or run_index >= len(matches):
        raise IndexError(
            f"Run index {run_index} is out of range for {len(matches)} matching metadata rows"
        )

    return str(matches.loc[run_index, "table_name"])


def compare_runs(
    connection: duckdb.DuckDBPyConnection,
    *,
    model: str,
    storage_name: Optional[str],
    left_index: int,
    right_index: int,
    time_column: str,
    tolerance: float,
) -> int:
    left_table = table_for_run(
        connection,
        model=model,
        storage_name=storage_name,
        run_index=left_index,
    )
    right_table = table_for_run(
        connection,
        model=model,
        storage_name=storage_name,
        run_index=right_index,
    )

    left = connection.sql(
        f"SELECT * FROM {quote_identifier(left_table)} ORDER BY {quote_identifier(time_column)}"
    ).df()
    right = connection.sql(
        f"SELECT * FROM {quote_identifier(right_table)} ORDER BY {quote_identifier(time_column)}"
    ).df()

    common_columns = [column for column in left.columns if column in right.columns]
    if time_column in common_columns:
        common_columns.remove(time_column)

    numeric_columns = [
        column
        for column in common_columns
        if left[column].dtype.kind in "biufc" and right[column].dtype.kind in "biufc"
    ]

    print(f"left_table:  {left_table}")
    print(f"right_table: {right_table}")
    print(f"left_rows:   {len(left)}")
    print(f"right_rows:  {len(right)}")

    if len(left) != len(right):
        print("row_count_equal: false")
    else:
        print("row_count_equal: true")

    if not numeric_columns:
        print("No common numeric variable columns to compare.")
        return 1

    row_count = min(len(left), len(right))
    if row_count == 0:
        print("No rows to compare.")
        return 1

    print()
    print("column max_abs_diff mean_abs_diff within_tolerance")
    failures = 0
    for column in numeric_columns:
        diff = (left[column].iloc[:row_count] - right[column].iloc[:row_count]).abs()
        max_abs_diff = float(diff.max())
        mean_abs_diff = float(diff.mean())
        within_tolerance = max_abs_diff <= tolerance
        if not within_tolerance:
            failures += 1
        print(f"{column} {max_abs_diff:.12g} {mean_abs_diff:.12g} {str(within_tolerance).lower()}")

    return 0 if failures == 0 and len(left) == len(right) else 1


def parse_args(argv: Optional[Iterable[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Inspect SSP4SIM DuckDB metadata and compare two recorded model runs."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    metadata = subparsers.add_parser("metadata", help=f"Print the {METADATA_TABLE} table.")
    metadata.add_argument("database", type=Path, help="Path to an SSP4SIM DuckDB result file.")

    compare = subparsers.add_parser("compare", help="Compare two runs for one model.")
    compare.add_argument("database", type=Path, help="Path to an SSP4SIM DuckDB result file.")
    compare.add_argument("--model", required=True, help="Model name from the metadata table.")
    compare.add_argument(
        "--storage-name",
        help="Optional storage_name filter. Use this when one model has multiple recorded storages.",
    )
    compare.add_argument(
        "--left-index",
        type=int,
        default=0,
        help="Zero-based metadata row index for the first run after filtering (default: 0).",
    )
    compare.add_argument(
        "--right-index",
        type=int,
        default=1,
        help="Zero-based metadata row index for the second run after filtering (default: 1).",
    )
    compare.add_argument(
        "--time-column",
        default="timestamp_ns",
        help="Column used to order rows before comparison (default: timestamp_ns).",
    )
    compare.add_argument(
        "--tolerance",
        type=float,
        default=0.0,
        help="Maximum allowed absolute difference per numeric column (default: 0.0).",
    )

    return parser.parse_args(argv)


def main(argv: Optional[Iterable[str]] = None) -> int:
    args = parse_args(argv)

    if args.command == "metadata":
        with connect_database(args.database) as connection:
            print_metadata(connection)
            return 0

    if args.command == "compare":
        with connect_database(args.database) as connection:
            return compare_runs(
                connection,
                model=args.model,
                storage_name=args.storage_name,
                left_index=args.left_index,
                right_index=args.right_index,
                time_column=args.time_column,
                tolerance=args.tolerance,
            )

    raise ValueError(f"Unknown command: {args.command}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc
