import os
from mltdict import MultiDict as multiDict


def file_size(fname):
    statinfo = os.stat(fname)
    return statinfo.st_size


def vex(vex_file_name):
    vex_file = open(vex_file_name, "r")
    vex_lines = vex_file.readlines()

    scans = list()
    stations_for_scan = list()
    stations_for_scans = list()
    start_time = list()
    exper_name = ""
    duration_for_scan = list()
    durations_for_scans = list()
    modes_for_scans = list()
    freqs = list()
    chan_def_for_freq = list()
    chan_defs = list()
    sites = list()
    site_names = list()

    for vexLine in vex_lines:
        if "scan " in vexLine:
            scans.append(vexLine.split()[1][:-1])

        if " station =" in vexLine:
            stations_for_scan.append(vexLine.split()[2])
            duration_for_scan.append(vexLine.split(":")[2].strip())

        if "endscan;" in vexLine:
            stations_for_scans.append(stations_for_scan)
            stations_for_scan = []

            durations_for_scans.append(duration_for_scan)
            duration_for_scan = []

        if "start = " in vexLine:
            start_time.append(vexLine.split(";")[0].split("=")[1])

        if "exper_name" in vexLine:
            exper_name = vexLine.split()[2][:-1]

        if "mode = " in vexLine:
            modes_for_scans.append(vexLine.split(";")[0].split("=")[1].strip())

        if "ref $FREQ =" in vexLine:
            freqs.append(vexLine.split()[3].split(":")[0])

        if "chan_def =" in vexLine:
            chan_def_for_freq.append(vexLine.split(":")[1])

        if "$SITE =" in vexLine:
            sites.append(vexLine.split()[3][:-1])

        if "site_name =" in vexLine:
            site_names.append(vexLine.split()[2][:-1])

    tmp_list = list()
    freqs = list(set(freqs))
    for i in range(0, len(chan_def_for_freq)):
        tmp_list.append(chan_def_for_freq[i].strip())

        if len(tmp_list) == len(chan_def_for_freq)/len(freqs):
            chan_defs.append(tmp_list)
            tmp_list = []

    total_stations = max([len(x) for x in stations_for_scans])
    all_stations = list()

    for s in stations_for_scans:
        if len(s) == total_stations:
            all_stations = s

    vex_dict = multiDict()
    vex = multiDict()

    for n in range(0, len(scans)):
        vex.append(scans[n], {"start": start_time[n], "station": ["aaaa", "bbbb",  max(durations_for_scans[n])],
                              "mode": modes_for_scans[n]})

    vex_dict.append('SCHED',  vex)
    vex_dict.append('GLOBAL', {"EXPER": exper_name})
    vex = multiDict()

    for n in range(0, len(modes_for_scans)):
        vex.append(modes_for_scans[n], {'FREQ': freqs})

    vex_dict.append('MODE', vex)
    vex = multiDict()

    for n in range(0, len(scans)):
        for m in range(0, len(freqs)):
            for b in range(0, len(chan_defs[m])):
                vex.append(vex_dict['MODE'][vex_dict['SCHED'][scans[n]]['mode']]['FREQ'][m],
                           {'chan_def': chan_defs[m][b]})

    vex_dict.append('FREQ', vex)
    vex = multiDict()

    all_stations = list(set(all_stations))
    for n in range(0, len(all_stations)):
        vex.append(all_stations[n], {'SITE': sites[n]})

    vex_dict.append('STATION', vex)
    vex = multiDict()

    for n in range(0, len(site_names)):
        vex.append(sites[n], {'site_name': site_names[n]})

    vex_dict.append('SITE', vex)
    return vex_dict
