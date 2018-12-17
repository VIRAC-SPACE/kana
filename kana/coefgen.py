#! /usr/bin/env python

import json
import re
import os
import numpy
import datetime
import time
import sys


class Date:
    def __init__(self, str):
        groups = re.match(r'(\d+)-(\w+)-(\d+) (\d+):(\d+)', str)
        self.year = int(groups[1])
        self.month = groups[2]
        self.day = int(groups[3])
        self.hour = int(groups[4])
        self.min = int(groups[5])

    def get_month_num(self):
        months = ['Jan', 'Feb', 'Mar',
                  'Apr', 'May', 'Jun', 'Jul',
                  'Aug', 'Sep', 'Oct', 'Dec']
        for i, month in enumerate(months):
            if self.month == month:
                return i + 1
        return 0

    def get_unix(self):
        date = datetime.datetime(
            self.year,
            self.get_month_num(),
            self.day,
            self.hour,
            self.min)
        return time.mktime(date.timetuple())


def open_range_file(setup, station):
    time = setup["start_time"]
    time = Date(time)
    root = setup["path"]

    directory = "{}/{}{}/{}_{}".format(root, time.day,
                                       time.month, station, time.day)

    closest = "AAA_0000_0000_00000"
    for file in os.listdir(directory):
        f_hour = int(file[9:-8])
        f_min = int(file[11:-6])
        c_hour = int(closest[9:-8])
        c_min = int(closest[11:-6])
        if f_hour > c_hour and f_hour <= time.hour:
            if f_hour < time.hour:
                closest = file
            elif f_min <= time.min:
                closest = file
        elif f_hour == c_hour:
            if f_min > c_min and f_min <= time.min:
                closest = file
    filepath = "{}/{}".format(directory, closest)
    return open(filepath, "r")


def read_object(file):
    return int(file.readline().split(" ")[0])


def read_ranges(setup, file):
    start_time = setup["start_time"]
    end_time = setup["end_time"]
    reading = False
    ranges = []
    for idx, line in enumerate(file.read().split("\n")):
        line = line.split(" ")
        if idx == 0:
            continue
        if " ".join(line[:2]) == start_time:
            reading = True
        if reading:
            ranges.append(float(line[-2]))
        if " ".join(line[:2]) == end_time:
            reading = False
    return ranges


def gen_matrix_S(size):
    mat = [0] * 7
    for i in range(len(mat)):
        for n in range(size):
            mat[i] += (n*60)**i
    return numpy.matrix(
        [[mat[0], mat[1], mat[2], mat[3]],
         [mat[1], mat[2], mat[3], mat[4]],
         [mat[2], mat[3], mat[4], mat[5]],
         [mat[3], mat[4], mat[5], mat[6]]], dtype='float')


def gen_array_L(ranges):
    arr = [0, 0, 0, 0]
    for i in range(4):
        for n in range(len(ranges)):
            arr[i] += ((n*60)**i)*ranges[n]
    return numpy.array(arr, dtype='float')


def calculate_coefficients(ranges1, ranges2):
    C = 299792.458
    matrix_S = gen_matrix_S(len(ranges1))
    array_L = gen_array_L(ranges1)
    array_SIL = numpy.dot(matrix_S.getI(), array_L)

    lags1 = [0] * len(ranges2)
    for i in range(len(lags1)):
        lags1[i] = 1 / C * ranges2[i]

    lags2 = [0] * len(ranges2)
    for i in range(len(lags2)):
        t = i * 60 - lags1[i]
        curr_lag = 0
        prev_lag = 1 / C * lag_function(array_SIL, t)
        ok = True
        while ok:
            t = 60 * i - lags1[i] + prev_lag
            curr_lag = 1 / C * lag_function(array_SIL, t)
            if abs(curr_lag - prev_lag) <= 1 / (10**9):
                ok = False
            else:
                prev_lag = curr_lag
        lags2[i] = curr_lag

    lags = [0] * len(ranges2)
    for i in range(len(lags)):
        lags[i] = lags1[i] + lags2[i]

    array_L2 = gen_array_L(lags)
    return numpy.dot(matrix_S.getI(), array_L2).tolist()[0]


def lag_function(A, t):
    return A[0, 0] + A[0, 1]*t + A[0, 2]*(t**2) + A[0, 3]*(t**3)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: provide path to setup.json")
        exit(-1)
    root = os.path.split(sys.argv[1])[0]
    setupFile = os.path.split(sys.argv[1])[1]
    os.chdir(root)

    setup = open(setupFile, "r")
    setup = setup.read()
    setup = json.loads(setup)

    stations = setup["stations"]
    stat1 = stations[:3]
    stat2 = stations[-3:]

    file = open_range_file(setup, stat1)
    obj = read_object(file)
    ranges1 = read_ranges(setup, file)
    file.close()
    file = open_range_file(setup, stat2)
    ranges2 = read_ranges(setup, file)
    file.close()

    A = calculate_coefficients(ranges1, ranges2)
    A[0] *= 10
    F0 = float(setup["F0(MHz)"]) * (10**6)
    Fsf = float(setup["Fsf(MHz)"]) * (10**6)
    F = float(setup["F(kHz)"]) * (10**3)
    Fv = F0 - Fsf + F0*A[0]
    Fsn = F - (F0 - Fsf) - F0*A[0]

    json_out = {}
    json_out["Stations"] = stations
    json_out["Start_time"] = Date(setup["start_time"]).get_unix()
    json_out["End_time"] = Date(setup["end_time"]).get_unix()
    json_out["Object"] = obj
    json_out["F0(MHz)"] = F0
    json_out["Fsf(MHz)"] = Fsf
    json_out["F(kHz)"] = F
    json_out["Fsn(Hz)"] = Fsn
    json_out["Fv(Hz)"] = Fv
    json_out["A0"] = A[0]
    json_out["A1"] = A[1]
    json_out["A2"] = A[2]
    json_out["A3"] = A[3]

    with open("{}.json".format(stations), "w") as out:
        out.write(json.dumps(json_out, indent=4))
