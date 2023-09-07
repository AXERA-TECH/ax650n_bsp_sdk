#!/usr/bin/python
import os
import sys
from struct import *

def hex2bin(hex_name, bin_name):
	fhex = open(hex_name, "r")
	fbin = open(bin_name, "wb+")
	while 1:
		hexstr = fhex.readline()
		if len(hexstr) == 0 :
			break
		hexstr1 = hexstr.replace(' ', '')
		hexstr1 = hexstr.replace('\n', '')
		result = ''
		for h in range(0,len(hexstr1) / 2):
			b = int(hexstr1[(len(hexstr1) - h*2 - 2):(len(hexstr1) - h*2)],16)
			result += pack('B',b)
		fbin.write(result)

	fbin.close();
	fhex.close();

if len(sys.argv) != 3:
	print 'usage:'
	print '	convert hexadecimal format to binary format:'
	print '		hex2bin.py hexfile binfile'
	exit(0)
hex2bin(sys.argv[1],sys.argv[2])
