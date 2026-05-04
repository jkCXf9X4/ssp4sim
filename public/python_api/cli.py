from __future__ import annotations

import argparse
from pathlib import Path

from . import Simulator


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="pyssp4sim",
        description="Run an SSP4SIM simulation from a JSON configuration file.",
    )
    parser.add_argument(
        "config",
        type=Path,
        help="Path to the simulation JSON configuration file.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

    simulator = Simulator(str(args.config))
    simulator.init()
    simulator.simulate()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
