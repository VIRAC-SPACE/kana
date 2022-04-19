#! /usr/bin/python3

import itertools
import json
import os
import sys


def run_m1(stations, ctrlfile):
    print("\033[33m<<<< KANA JOB: M1 >>>>\033[0m")
    for station in stations:
        os.system("mpirun m1 %s %s" % (ctrlfile, station))


def run_m2(stations, ctrlfile):
    print("\033[33m<<<< KANA JOB: M2 >>>>\033[0m")
    for station in stations:
        os.system("mpirun m1 %s %s" % (ctrlfile, station))
        os.system("timeCorrect %s %s" % (ctrlfile, station))
        os.system("mpirun m2 %s %s" % (ctrlfile, station))


def run_m3(stations, ctrlfile):
    print("\033[33m<<<< KANA JOB: M3 >>>>\033[0m")
    pool = []
    for station in stations:
        pool.append(station)
    pairs = itertools.combinations(pool, 2)
    for pair in pairs:
        os.system("mpirun m1 %s %s" % (ctrlfile, pair[0]))
        os.system("mpirun timeCorrect %s %s" % (ctrlfile, pair[0]))
        os.system("mpirun freqCorrect -t %s %s" % (ctrlfile, pair[0]))

        os.system("mpirun m1 %s %s" % (ctrlfile, pair[1]))
        os.system("mpirun freqCorrect %s %s" % (ctrlfile, pair[1]))

        os.system("mpirun m3 %s %s %s" % (ctrlfile, pair[0], pair[1]))


def main():
    os.chdir(os.path.split(sys.argv[0])[0])
    ctrl_file = sys.argv[1]
    with open(ctrl_file, "r") as file:
        ctrl_info = file.read()
    ctrl_info = json.loads(ctrl_info)

    jobs = ctrl_info["jobs"]
    stations = ctrl_info["stations"]

    for job in jobs: {
        "M1": lambda: run_m1(stations, ctrl_file),
        "M2": lambda: run_m2(stations, ctrl_file),
        "M3": lambda: run_m3(stations, ctrl_file)
    }[job]()


if __name__ == "__main__":
    main()
    sys.exit(0)
