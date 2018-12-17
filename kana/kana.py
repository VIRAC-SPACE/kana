#! /usr/bin/env python3
import json
import sys
import os
import itertools

KANADIR = os.path.split(os.path.realpath(__file__))[0]


def runM1(stations, ctrlfile):
    print("\033[33m<<<< KANA JOB: M1 >>>>\033[0m")
    for station in stations:
        os.system("%s/m1 %s %s" % (KANADIR, ctrlfile, station))


def runM2(stations, ctrlfile):
    print("\033[33m<<<< KANA JOB: M2 >>>>\033[0m")
    for station in stations:
        os.system("%s/m1 %s %s" % (KANADIR, ctrlfile, station))
        os.system("%s/fc %s %s" % (KANADIR, ctrlfile, station))
        os.system("%s/m2 %s %s" % (KANADIR, ctrlfile, station))


def runM3(stations, ctrlfile):
    print("\033[33m<<<< KANA JOB: M3 >>>>\033[0m")
    pool = []
    for station in stations:
        pool.append(station)
    pairs = itertools.combinations(pool, 2)
    for pair in pairs:
        os.system("%s/m1 %s %s" % (KANADIR, ctrlfile, pair[0]))
        os.system("%s/tc %s %s" % (KANADIR, ctrlfile, pair[0]))
        os.system("%s/fc %s %s" % (KANADIR, ctrlfile, pair[0]))

        os.system("%s/m1 %s %s" % (KANADIR, ctrlfile, pair[1]))
        os.system("%s/fc %s %s" % (KANADIR, ctrlfile, pair[1]))

        os.system("%s/m3 %s %s %s" % (KANADIR, ctrlfile, pair[0], pair[1]))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: kana.py [path_to_ctrl_file]\n")
        exit(0)
    ctrlfile = sys.argv[1]
    with open(ctrlfile, "r") as file:
        ctrlinfo = file.read()
    ctrlinfo = json.loads(ctrlinfo)

    try:
        jobs = ctrlinfo["jobs"]
    except json.JSONDecodeError:
        print("Error: missing 'jobs' in control file\n")
        exit(-1)

    try:
        stations = ctrlinfo["stations"]
    except json.JSONDecodeError:
        print("Error: missing 'stations' in control file\n")
        exit(-1)

    for job in jobs:
        {"M1": lambda: runM1(stations, ctrlfile),
         "M2": lambda: runM2(stations, ctrlfile),
         "M3": lambda: runM3(stations, ctrlfile)}[job]()
