#!/usr/bin/env python3
"""Export every DuckDB table in a result database to its own CSV file."""

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


def quote_identifier(name: str) -> str:
    return '"' + name.replace('"', '""') + '"'


def quote_literal(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def connect_database(path: Path) -> duckdb.DuckDBPyConnection:
    if not path.exists():
        raise FileNotFoundError(f"DuckDB file does not exist: {path}")
    return duckdb.connect(str(path), read_only=True)


def table_names(connection: duckdb.DuckDBPyConnection) -> list[str]:
    rows = connection.sql(
        """
        SELECT table_name
        FROM information_schema.tables
        WHERE table_schema = 'main' AND table_type = 'BASE TABLE'
        ORDER BY table_name
        """
    ).fetchall()
    return [str(row[0]) for row in rows]


def export_tables(
    connection: duckdb.DuckDBPyConnection,
    *,
    output_dir: Path,
) -> list[Path]:
    tables = table_names(connection)
    if not tables:
        raise ValueError("No base tables found in the DuckDB file")

    output_dir.mkdir(parents=True, exist_ok=True)

    exported_files: list[Path] = []
    for table_name in tables:
        destination = output_dir / f"{table_name}.csv"
        connection.execute(
            "COPY (SELECT * FROM "
            f"{quote_identifier(table_name)}"
            f") TO {quote_literal(str(destination))} (HEADER, DELIMITER ',')"
        )
        exported_files.append(destination)
        print(f"Wrote {destination}")

    return exported_files


def parse_args(argv: Optional[Iterable[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export all DuckDB tables to CSV files.")
    parser.add_argument(
        "database",
        type=Path,
        help="Path to a DuckDB result file, for example wd/signal_sine_gain_add/baseline/result.duckdb.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help=(
            "Directory that will receive one CSV per table. "
            "Defaults to <database parent>/<database stem>_csv."
        ),
    )
    return parser.parse_args(argv)


def main(argv: Optional[Iterable[str]] = None) -> int:
    args = parse_args(argv)
    database: Path = args.database
    output_dir: Path = args.output_dir or database.parent / f"{database.stem}_csv"

    with connect_database(database) as connection:
        export_tables(connection, output_dir=output_dir)

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
