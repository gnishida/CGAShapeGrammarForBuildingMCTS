from matplotlib import pylab as plt
import sys
import re

def main(data_file):
	f = open(data_file)
	lines = f.readlines()
	f.close()
	
	pattern = r"Iteration ([0-9]+), loss = ([0-9\.e-]+)$"
	pattern1 = r"accuracy = ([0-9\.]+)$"
	pattern2 = r"Iteration ([0-9]+),"
	
	x = []
	y = []
	for line in lines:
		data = line.split(",")
		x.append(data[0])
		y.append(data[1])

	plt.plot(x, y)
	plt.ylim(0,0.5)
	plt.xlabel('#iterations')
	plt.ylabel('score')
	plt.show()

if __name__ == '__main__':
	if len(sys.argv) < 2:
		print "Usage: python draw_graph.py <data file>"
	else:
		main(sys.argv[1])
