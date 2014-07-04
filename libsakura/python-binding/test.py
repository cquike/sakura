#!/bin/env python

import sys
import time
import math
import gc
from threading import *

import libsakurapy

print dir(libsakurapy);

libsakurapy.initialize()

print libsakurapy.get_current_time()

def test_stats():
	try:
		libsakurapy.compute_statistics()
	except Exception as e:
		print e

	num = 4
	mask = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_BOOL, (True,) * num)
	data = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, range(num))
	result = libsakurapy.compute_statistics(4, data, mask)
	print result
	del mask
	del data

def test_grid():
	def total_elements(dim):
		return reduce(lambda x, y: x * y, dim, 1)

	num_spectra = 1024*64
	start_spectrum = 0
	end_spectrum = num_spectra
	spectrum_mask = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_BOOL, (num_spectra,))
	x = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_DOUBLE, (num_spectra,))
	y = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_DOUBLE, (num_spectra,))
	support = 10
	sampling = 2
	num_polarizations = 4
	num_polarizations_for_grid = 2
	num_channels = 2048
	num_channels_for_grid = 1024
	polarization_map = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_INT32,
		map(lambda x: x % num_polarizations_for_grid, range(num_polarizations)))
	channel_map = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_INT32,
		map(lambda x: x % num_channels_for_grid, range(num_channels)))

	dim = (num_spectra, num_polarizations, num_channels)
	print total_elements(dim)
	mask = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_INT8, dim)
	mask = libsakurapy.uint8_to_bool(total_elements(dim),
		mask, libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_BOOL, dim))
	value = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, dim)
	weight = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, (num_spectra, num_channels))
	num_convolution_table = int(math.ceil(math.sqrt(2.)*(support+1)*sampling))
	convolution_table = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, (num_convolution_table, ))

	width, height = (160, 100)
	weight_sum = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_DOUBLE, (num_polarizations_for_grid, num_channels_for_grid))
	dim = (height, width, num_polarizations_for_grid, num_channels_for_grid)
	weight_of_grid = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, dim)
	grid = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, dim)

	libsakurapy.grid_convolving(num_spectra, start_spectrum, end_spectrum,
		spectrum_mask, x, y,
		support, sampling,
		num_polarizations, polarization_map,
		num_channels, channel_map,
		mask, value, weight, False,
		num_convolution_table, convolution_table,
		num_polarizations_for_grid, num_channels_for_grid,
		width, height,
		weight_sum, weight_of_grid, grid)

def test_AB():
	buf = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, (10, 20, 30, 40, 50))
	print(libsakurapy.get_elements_of_aligned_buffer(buf))
	del buf

def test_logical():
	n = 1024*1024*16
	dim = (n,)
	
	buf = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, dim)
	bl = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_BOOL, dim)
	libsakurapy.set_false_float_if_nan_or_inf(n, buf, bl)
	del buf
	del bl

	ui8 = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_INT8, dim)
	bl = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_BOOL, dim)
	libsakurapy.uint8_to_bool(n, ui8, bl)
	ui32 = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_INT32, dim)
	libsakurapy.uint32_to_bool(n, ui32, bl)
	bl2 = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_BOOL, dim)
	libsakurapy.invert_bool(n, bl, bl2)
	libsakurapy.invert_bool(n, bl, bl) # in place
	del ui8
	del ui32
	del bl
	del bl2

	src = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_BOOL, (True, False, True, False))
	dst = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_BOOL, (True, True, False, False))
	out = libsakurapy.logical_and(4, src, dst)


def test_range():
	n = 1024*1024*16
	dim = (n,)
	data = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_INT32, dim)
	mask = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_BOOL, dim)
	lower = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_INT32, (1, 100, 200))
	upper = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_INT32, (10, 110, 210))
	result = libsakurapy.set_true_int_in_ranges_exclusive(n, data, 3, lower, upper, mask)
	del n, dim, data, mask, lower, upper, result
	# Test set_true_float_in_ranges_exclusive
	dataf = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, [0., 2., 1., 3.])
	ndata = libsakurapy.get_elements_of_aligned_buffer(dataf)[0]
	maskf = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_BOOL, [True, True, True, True])
	lowerf = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, [0.5])
	upperf = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, [2.5])
	result = libsakurapy.set_true_float_in_ranges_exclusive(ndata,dataf,1,lowerf,upperf,maskf)
	del dataf, ndata, maskf, lowerf, upperf, result

def test_bit():
	# Test operate_bits_uint8_or
	mask = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_BOOL, [True]*4+[False]*4)
	data8 = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_INT8, [0, 2, 1, 3]*2)
	ndata = libsakurapy.get_elements_of_aligned_buffer(data8)[0]
	result = libsakurapy.operate_bits_uint8_or(2,ndata,data8,mask)
	del mask, data8, result, ndata

def test_interpolate():
	# interpolate in Y-axis
	nchan = 4
	yin = [0., 1.]
	yout = [0.75]
	zin = [float(6.0)]*nchan + [float(5.0)]*nchan
	nbase = len(yin)
	npos = len(yout)
	order = 1
	zindata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, zin)
	yindata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_DOUBLE, yin)
	youtdata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_DOUBLE, yout)
	zoutdata = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, (npos, nchan))
	result = libsakurapy.interpolate_float_yaxis(libsakurapy.INTERPOLATION_METHOD_LINEAR, order, nbase, yindata, nchan, zindata, npos, youtdata, zoutdata)
	# the result should be [5.25]*nchan
	del yin, yout, zin, zindata, yindata, youtdata, zoutdata
	# interpolate in X-axis
	xin = [0.,1.]
	xout = [0.25]
	nbase = len(xin)
	npos = len(xout)
	nrow = 3
	zin = [float(6.0), float(5.0)]*nrow
	order = 1
	zindata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, zin)
	xindata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_DOUBLE, xin)
	xoutdata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_DOUBLE, xout)
	zoutdata = libsakurapy.new_uninitialized_aligned_buffer(libsakurapy.TYPE_FLOAT, (npos, nrow))
	result = libsakurapy.interpolate_float_xaxis(libsakurapy.INTERPOLATION_METHOD_LINEAR, order, nbase, xindata, nrow, zindata, npos, xoutdata, zoutdata)
	# the result should be [5.75]*nrow

def test_calibration():
	ndata = 7
	yon = [5.0 for i in range(ndata)]
	yoff = [(on - 1.0) for on in yon]
	factor = [float(i) for i in range(ndata)]
	ondata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, yon)
	offdata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, yoff)
	facdata = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, factor)
	result = libsakurapy.apply_position_switch_calibration(len(factor), facdata, ndata, ondata, offdata)
	# the result should be [0.0, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5]
	

def test_convolve1D():
	ndata = 10
	width = 3
	y = [0.]*ndata
	y[5] = 1.0
	ctx1D = libsakurapy.create_convolve1D_context(ndata, libsakurapy.CONVOLVE1D_KERNEL_TYPE_GAUSSIAN, width, True);
	data = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, y)
	result = libsakurapy.convolve1D(ctx1D, ndata, data)
	del ctx1D, data, result

def test_baseline():
	ndata = 8
	order = 1
	ctxbl = libsakurapy.create_baseline_context(libsakurapy.BASELINE_TYPE_POLYNOMIAL, order, ndata)
	y = [0.5*i for i in range(ndata)]
	m = [True]*ndata
	y[4] += 3.
	m[4] = False
	data = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, y)
	mask = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_BOOL, m)
	result = libsakurapy.subtract_baseline(ndata, data, mask, ctxbl, 5., 1, True)
	# The result should be [0.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0]
	del ctxbl

def testAll():
	test_AB()
	test_stats()
	test_grid()
	test_logical()
	test_range()
	test_convolve1D()

testAll()
gc.collect(2)

libsakurapy.clean_up()
