
#!/usr/bin/env python3
"""
Compare the delay system reference output against recorded simulation runs.

The script:
    1. Detects the common columns between the reference data set and all
       matching result files (time must be included).
    2. Saves a plot of the reference signals across the common columns.
    3. For each result file, interpolates the data so it aligns with the
       reference time scale, reports basic error statistics, and plots the
       mean absolute difference over time.

Outputs are written to the repository's ``results`` directory.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np
import pandas as pd

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402


def load_csv(path: Path) -> pd.DataFrame:
    """Load a CSV file, normalise column names, and sort by time."""
    df = pd.read_csv(path, skipinitialspace=True)
    df.columns = [col.strip().lower() for col in df.columns]

    for column in df.columns:
        df[column] = pd.to_numeric(df[column], errors="ignore")

    if "time" not in df.columns:
        raise ValueError(f"'time' column missing in {path}")

    df = df.dropna(subset=["time"])
    df["time"] = pd.to_numeric(df["time"], errors="coerce")
    df = df.dropna(subset=["time"])
    df = df[~df["time"].duplicated(keep="first")]
    df = df.sort_values("time").reset_index(drop=True)
    return df


def interpolate_to_times(
    df: pd.DataFrame,
    data_columns: list[str],
    target_times: np.ndarray,
) -> pd.DataFrame:
    """Reindex the dataframe onto the requested time vector with interpolation."""
    indexed = df.set_index("time")[data_columns]
    indexed = indexed[~indexed.index.duplicated(keep="first")].sort_index()
    min_time = indexed.index.min()
    max_time = indexed.index.max()
    in_bounds = (target_times >= min_time) & (target_times <= max_time)
    if not np.any(in_bounds):
        empty_index = pd.Index([], name="time", dtype=float)
        return pd.DataFrame(columns=data_columns, index=empty_index)

    valid_times = target_times[in_bounds]
    reindexed = indexed.reindex(valid_times)
    interpolated = reindexed.interpolate(method="index", limit_area="inside")
    interpolated.index.name = "time"
    return interpolated


def plot_reference(
    reference: pd.DataFrame,
    columns: list[str],
    output_path: Path,
) -> None:
    """Plot the reference signals for the intersection of columns."""
    data_columns = [col for col in columns if col != "time"]
    if not data_columns:
        print("No data columns available for plotting the reference.")
        return

    # data_columns.reverse()
    data_columns.remove("sources.freq_output")
    data_columns = ["sources.freq_output"] + data_columns
    data_columns.reverse()


    plt.figure(figsize=(12, 7))
    for column in data_columns:
        plt.plot(reference["time"], reference[column], label=column)

    plt.xlabel("Time")
    plt.ylabel("Value")
    plt.title("Reference Signals")
    plt.legend(loc="upper right", ncol=2, fontsize="small")
    plt.grid(True, which="both", linestyle="--", linewidth=0.5)
    plt.xlim(0, 0.12) # Set x-axis
    plt.ylim(-1.1, 1.1) # Set y-axis  
    plt.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, dpi=200)
    plt.close()
    print(f"Saved reference plot to {output_path}")


def difference_plot(
    reference: pd.DataFrame,
    results: dict[Path, pd.DataFrame],
    columns: list[str],
    output_path: Path,
) -> None:
    """Compute interpolated differences and save a summary plot."""
    data_columns = [col for col in columns if col != "time"]
    plt.figure(figsize=(12, 7))
    has_lines = False

    for result_path, result_df in results.items():
        target_times = np.union1d(reference["time"].values, result_df["time"].values)
        print(target_times)

        ref_interp = interpolate_to_times(reference, data_columns, target_times)
        result_interp = interpolate_to_times(result_df, data_columns, target_times)

        common_index = ref_interp.index.intersection(result_interp.index)
        if common_index.empty:
            print(
                f"Skipping {result_path.name}: no overlapping time range after trimming.",
            )
            continue

        ref_interp = ref_interp.loc[common_index]
        result_interp = result_interp.loc[common_index]

        print("Ref interpolated")
        print(ref_interp.head())
        print("Res interpolated")
        print(result_interp.head())

        diff_df = result_interp - ref_interp
        print("Diff interpolated")
        diff_df["time"] = diff_df.index
        print(diff_df.head())


        plot_reference(diff_df, columns, output_path / f"diff_{result_path.stem}.png")


def main() -> None:
    script_dir = Path(__file__).resolve().parent
    reference_path = script_dir / "reference_system_native.csv"
    results_dir = script_dir.parents[2] / "results"

    result_files = sorted(results_dir.glob("*.csv"))
    if not result_files:
        print(f"No result files found in {results_dir}")
        return
    
    output_dir = results_dir / "plots"
    output_dir.mkdir(mode=0o777, parents=True, exist_ok=True)

    reference_df = load_csv(reference_path)
    result_dfs: dict[Path, pd.DataFrame] = {}
    common_columns = set(reference_df.columns)
    common_columns.remove("sources.ramp_output")
    common_columns.remove("sources.ramp_freq_output")
    common_columns = set([x for x in list(common_columns) if "input" not in x])

    for result_file in result_files:
        result_df = load_csv(result_file)
        result_dfs[result_file] = result_df
        common_columns &= set(result_df.columns)

    if "time" not in common_columns:
        raise RuntimeError("Common columns do not include 'time'; cannot compare datasets.")

    common_columns_list = ["time"] + sorted(col for col in common_columns if col != "time")
    print(f"Common columns: {', '.join(common_columns_list)}")
    print("Ref head():")
    print(reference_df[common_columns_list].head())

    plot_reference(reference_df, common_columns_list, output_dir / "linear_delay_sys_reference.png")

    # Plot results
    for p, result in result_dfs.items():
        print(f"Result {p} head():")
        print(result[common_columns_list].head())
        plot_reference(result, common_columns_list, output_dir / f"res_{p.stem}.png")

    difference_plot(reference_df, result_dfs, common_columns_list, output_dir)


if __name__ == "__main__":
    main()
