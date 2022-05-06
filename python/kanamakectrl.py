#! /usr/bin/env python3

import sys
import os
import json
from collections import OrderedDict
import re
import codecs

from vex_2 import vex as Vex


def get_mode(vex):
    sched = vex["SCHED"]
    for scan in sched:
        break
    return sched[scan]["mode"]


def get_discreditation(vex):
    mode = get_mode(vex)
    freq = vex["MODE"][mode]["FREQ"][0]
    matches = re.search("x(\\d+)MHz", freq)
    if not matches:
        print("Error: Cannot find frequency")
    return int(matches.group(1) + ("0" * 6))


def get_length(vex):
    sched = vex["SCHED"]
    for scan in sched:
        break
    length_str = sched[scan]["station"][2]
    return [int(s) for s in length_str.split() if s.isdigit()][0]


def doc2dic(filename):
    if not os.path.isfile(filename + ".doc"):
        print(filename + ".doc not found")
        exit(-1)
    os.system("antiword " + filename + ".doc > " + filename + ".txt")
    with open(filename + '.txt', 'r') as myfile:
        data = myfile.read().replace('\n', '').replace(' ', '')
    data = re.sub(".*\\|A3\\|\\|", "", data)
    data = re.sub("\\|\\|", "\n", data)
    
    json_out = list()
    for line in data.splitlines():
        line_list = line.split("|")
        json_out.append(OrderedDict())
        json_out[-1]["object"] = line_list[0]
        json_out[-1]["HH"] = line_list[1]
        json_out[-1]["MM"] = line_list[2]
        json_out[-1]["SS"] = line_list[3]
        json_out[-1]["FsF"] = line_list[4]
        json_out[-1]["F interf"] = line_list[5]
        json_out[-1]["A0"] = line_list[6]
        json_out[-1]["A1"] = line_list[7]
        json_out[-1]["A2"] = line_list[8]
        json_out[-1]["A3"] = line_list[9]
    os.system("rm " + filename + ".txt")
    return json_out


def vex2dic(filename, highest_job):
    if not os.path.isfile(filename):
        print(filename + " not found")
        exit(-1)
    vex = Vex(filename)

    json_out = OrderedDict()
    json_out["jobs"] = [highest_job]
    json_out["observation"] = vex["GLOBAL"]["EXPER"]
    json_out["observation_length"] = get_length(vex)
    json_out["discretization(Hz)"] = get_discreditation(vex)
    json_out["m1_buffer_begin"] = sys.argv[3]
    json_out["m1_buffer_end"] = sys.argv[4]
    json_out["m1_fft_compress"] = 0.00005
    json_out["m1_spectra_window(Hz)"] = 100_000
    json_out["data_dir"] = sys.argv[7]
    if sys.argv[8] == "True" or  sys.argv[8] == "true":
        json_out["realtime"] = True
        json_out["rt_in_address"] = sys.argv[9]
        json_out["rt_out_address"] = sys.argv[10]
        json_out["rt_batch_length"] = sys.argv[11]
    else:
        json_out["realtime"] = False

    if highest_job != 'M1':
        json_out["m2_Tu"] = 0.0002

    if highest_job == 'M3':
        json_out["m3_delay_begin"] = sys.argv[5]
        json_out["m3_delay_end"] = sys.argv[6]

    json_out["stations"] = OrderedDict()
    for station in vex["STATION"]:
        json_out["stations"][station] = {"format": "VDIF"}

    return json_out


def get_BBCs(filename):
    BBCs = {}
    if not os.path.isfile(filename + ".log"):
        print(filename + ".log not found. continuing without..")
    else:
        for line in codecs.open(filename + ".log", "r", "utf-8", "ignore"):
            match = re.search(r'/(bbc(\d)+)[/=] *([\d\.]+)', line)
            if match != None:
                BBCs[match.group(1).upper()] = float(match.group(3))
    return BBCs


def get_RFs(vex, station):
    mode = get_mode(vex)
    freq = "0"
    for f in vex["MODE"][mode].get("FREQ"):
        if f[1] == station:
            freq = f[0]
            break
    rfs = []
    for chan_def in vex["FREQ"][freq].get("chan_def"):
        rf = float(re.search(r'([\d\.]+) *MHz', chan_def[1]).group(1)) * 1_000_000
        rfs.append(rf)
    return rfs


def get_LO(vex, station):
    mode = get_mode(vex)
    freq = "LO@0MHz"
    print(vex["MODE"][mode])
    for i in vex["MODE"][mode].get("IF"):
        if i[1] == station:
            freq = i[0]
            break
    lo = float(re.search(r'LO@([\d\.]+) *MHz', freq).group(1)) * 1_000_000
    return lo


def get_IFs(vex, station, filename):
    mode = get_mode(vex)
    bbcs = get_BBCs(station + "_" + filename)
    freq = "0"
    for f in vex["MODE"][mode].get("FREQ"):
        if f[1] == station:
            freq = f[0]
            break
    ifs = []
    for chan_def in vex["FREQ"][freq].get("chan_def"):
        If = 0
        if chan_def[5] in list(bbcs.keys()):
            If = float(bbcs[chan_def[5]]) * 1_000_000
        ifs.append(If)
    return ifs


def get_spike_freqs(station, filename):
    if not os.path.isfile(filename):
        print(filename + " not found")
        exit(-1)
    vex = Vex(filename)
    rf = 6668519200
    lo = get_LO(vex, station)
    ifs = get_IFs(vex, station, filename)
    spike_freqs = []
    for i in range(len(ifs)):
        if ifs[i] == 0:
            spike_freqs.append(get_discreditation(vex) / 2)
        else:
            spike_freqs.append(int(rf - (lo + ifs[i])))
    return spike_freqs


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Error: arguments not provided")
        exit(-1)
        
    highest_job = sys.argv[1].upper()
    path = os.path.split(sys.argv[2])
    vex_file = sys.argv[2] + ".vix"
    os.chdir(path[0])

    if highest_job == "M3":
        paramPath = path[1]+"_"+sys.argv[1]+"_"+sys.argv[3]+"_"+sys.argv[4]+"_"+sys.argv[5]+"_"+sys.argv[6]

    else:
        paramPath = path[1]+"_"+sys.argv[1]+"_"+sys.argv[3]+"_"+sys.argv[4]

    if not os.path.exists(paramPath):
        os.mkdir(paramPath)
        os.chdir(paramPath)
        os.mkdir("plots")

    filename = path[1]
    json_out = vex2dic(vex_file, highest_job)

    controlFile = "/".join(sys.argv[2].split("/")[0:-1]) + "/" + paramPath + "/" + sys.argv[2].split("/")[-1] + ".ctrl"
        
    for station in json_out["stations"]:
        # irib22ib.log
        log_file_name = sys.argv[2][0:len(sys.argv[2]) - len(sys.argv[2].split("/")[-1])] + \
                        sys.argv[2].split("/")[-1]+station.lower()+".log"
        if os.path.isfile(log_file_name):
            # this do not work for multi mode observation
            json_out["stations"][station]["target(Hz)"] = get_spike_freqs(station, vex_file)

        if os.path.isfile(station + "_" + filename + ".doc"):
            json_out["stations"][station]["coeff"] = doc2dic(station + "_" + vex_file)

    control_file = open(controlFile, "w")
    control_file.write(json.dumps(json_out, indent=4))
    sys.exit(0)
