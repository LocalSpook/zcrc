#!/usr/bin/env python3

import argparse
import matplotlib.pyplot as plt
import xml.etree.ElementTree as ET
from math import ceil

def main():
    parser = argparse.ArgumentParser(
        description='Construct the graph demonstrating the benefits of accepting unsized contiguous ranges.')
    parser.add_argument('-i', type=argparse.FileType('rt', encoding='utf-8'), required=True,
        help="Path to benchmark results in XML format (- for stdin)")
    parser.add_argument('-o', type=argparse.FileType('wt', encoding='utf-8'), required=True,
        help="File to write resulting SVG graph to (- for stdout)")
    args = parser.parse_args()

    test_case = ET.parse(args.i).getroot().find("TestCase[@name='cstr']")
    runs = test_case.findall("BenchmarkResults")
    unsized_runs = [r for r in runs if r.get("name").endswith("unsized")]
    sized_runs = [r for r in runs if r.get("name").endswith("strlen + sized")]

    x_coords = [float(r.get("name").split(":")[0]) for r in unsized_runs]
    y_coords_unsized = [(x / (2 ** 30)) / (float(r.find("mean").get("value")) / 1e9) for r, x in zip(unsized_runs, x_coords)]
    y_coords_sized = [(x / (2 ** 30)) / (float(r.find("mean").get("value")) / 1e9) for r, x in zip(sized_runs, x_coords)]

    plt.xscale("log", base=2)
    plt.plot(x_coords, y_coords_unsized, "r", label="Passed as unsized range")
    plt.plot(x_coords, y_coords_sized, "b", label="strlen + passed as sized range")
    plt.ylim(bottom=0)
    plt.title("Performance benefit of accepting unsized ranges")
    plt.xlabel("Message length (B)")
    plt.ylabel("Throughput (GiB/s)")
    plt.grid(True)
    plt.legend()
    plt.savefig(args.o, format="svg")

if __name__ == "__main__":
    main()
