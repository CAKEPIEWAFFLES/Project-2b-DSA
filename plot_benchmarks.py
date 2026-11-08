"""Plots the CSV written by the C++ benchmark (option 6 in the CLI, or ./gatorboxd --benchmark).

Usage: python3 plot_benchmarks.py [csv_path] [output_dir]
Defaults: results/benchmark_results.csv -> results/
"""
import csv
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")  # save to files, no window needed
import matplotlib.pyplot as plt

csv_path = sys.argv[1] if len(sys.argv) > 1 else "results/benchmark_results.csv"
out_dir = sys.argv[2] if len(sys.argv) > 2 else "results"

# approach -> list of rows, sorted by number of ratings
rows_by_approach = defaultdict(list)
with open(csv_path, newline="") as f:
    for row in csv.DictReader(f):
        rows_by_approach[row["approach"]].append(row)
for rows in rows_by_approach.values():
    rows.sort(key=lambda r: int(r["num_ratings"]))


def line_plot(column, ylabel, title, filename, log_scale=False):
    """One line per approach, x axis is how many ratings the structure was built from."""
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for approach, rows in rows_by_approach.items():
        xs = [int(r["num_ratings"]) for r in rows]
        ys = [float(r[column]) for r in rows]
        ax.plot(xs, ys, marker="o", label=approach)
    ax.set_xlabel("Number of ratings")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    if log_scale:
        ax.set_yscale("log")
    ax.grid(alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(f"{out_dir}/{filename}", dpi=150)
    plt.close(fig)
    print(f"Wrote {out_dir}/{filename}")


line_plot("build_ms", "Build time (ms)", "Build time vs data size", "build_time.png")
line_plot("avg_query_ms", "Average query time (ms, log scale)", "Query time vs data size",
          "query_time.png", log_scale=True)

# Match rate against brute force at the full data size (Popularity shows the "ignore the movie" baseline)
fig, ax = plt.subplots(figsize=(7, 4.5))
names = []
matches = []
for approach, rows in rows_by_approach.items():
    names.append(approach)
    matches.append(float(rows[-1]["match_bruteforce_pct"]))
ax.bar(names, matches)
ax.set_ylabel("Top 10 identical to brute force (%)")
ax.set_ylim(0, 105)
ax.set_title("Agreement with brute force (largest data size)")
fig.tight_layout()
fig.savefig(f"{out_dir}/accuracy.png", dpi=150)
plt.close(fig)
print(f"Wrote {out_dir}/accuracy.png")
