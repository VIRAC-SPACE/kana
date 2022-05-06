#! /usr/local/bin/python3

import sys
import os
from os.path import expanduser
import glob
import subprocess

import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl


def main():
	mpl.rcParams['agg.path.chunksize'] = 10000

	mode = sys.argv[3]

	if mode == "m1":
		home = expanduser("~")
		os.chdir(
			home + '/correlations/kana/' + sys.argv[1] + '/' + sys.argv[1] + "_" + sys.argv[2] + '/results/m1-spectra/')
		signal_list = glob.glob('*.spectra')

		if not os.path.isdir('../png'):
			os.system('mkdir ../png')

		for file in signal_list:
			fig1, ax1 = plt.subplots(nrows=1, ncols=1, figsize=(16, 16), dpi=150)
			data = np.fromfile(file)
			ax1.plot(data)
			ax1.set_xlabel('Hz offset 2000000')
			ax1.set_ylabel('Power')
			ax1.set_title(file[:-4])
			fig1.savefig('../png/' + file[:-8] + '.png')
			fig1.clear()

	elif mode == "m3":
		home = expanduser("~")
		os.chdir(home + '/experiments/KANA/' + sys.argv[1] + '/' + sys.argv[2] + '/results/m3-spectra/')
		print(os.getcwd())
		signal_list = glob.glob('*.spectra')
		if not os.path.isdir('../png'):
			os.system('mkdir ../png')

		plot = subprocess.Popen(['gnuplot'], stdin=subprocess.PIPE, encoding='utf8')
		plot.stdin.write("set terminal png size\n")
		for file in signal_list:
			plot.stdin.write("set title '" + file[:-4] + "'\n")
			plot.stdin.write("set output '../png/" + file[:-8] + "1.png'\n")
			plot.stdin.write("set xlabel 'Hz'\n")
			plot.stdin.write("set ylabel 'Delay'\n")
			plot.stdin.write("set zlabel 'Power'\n")
			plot.stdin.write("set contour base\n set hidden3d offset 1\n")
			plot.stdin.write("set grid layerdefault linetype -1 linecolor rgb 'gray' "
							 "linewidth 0.200, linetype -1 linecolor rgb 'gray' linewidth 0.200\n")
			plot.stdin.write("set xrange [-10:10]\n")
			plot.stdin.write("set palette rgbformulae 22,13,-31\n")
			plot.stdin.write("splot '" + file + "' u 2:1:3 with pm3d\n")

			plot.stdin.write("set pm3d map\n")
			plot.stdin.write("set output '../png/" + file[:-8] + "2.png'\n")
			plot.stdin.write("splot '" + file + "' u 2:1:3 with pm3d\n")
		plot.stdin.write("exit")


if __name__ == "__main__":
	main()
	sys.exit()
