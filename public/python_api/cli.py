from __future__ import annotations

import argparse
from importlib.metadata import PackageNotFoundError, version
from pathlib import Path

from . import Simulator


def package_version() -> str:
    try:
        return version("pyssp4sim")
    except PackageNotFoundError:
        return "unknown"


DOCS_USAGE = "https://github.com/jkCXf9X4/ssp4sim/blob/main/docs/usage.md"
DOCS_CONFIGURATION = "https://github.com/jkCXf9X4/ssp4sim/blob/main/docs/configuration.md"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="pyssp4sim",
        description="Run an SSP4SIM simulation from a JSON configuration file.",
        epilog=(
            "examples:\n"
            "  pyssp4sim ./resources/embrace/embrace.json\n"
            "  pyssp4sim --version\n"
            "\n"
            "documentation:\n"
            f"  {DOCS_USAGE}\n"
            "      Usage, examples, and result artifacts.\n"
            f"  {DOCS_CONFIGURATION}\n"
            "      Full JSON configuration key reference."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-v",
        "--version",
        action="version",
        version=package_version(),
        help="Show the installed Python API version and exit.",
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
