############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
############################################################
open_project vitis_hls_workspace
set_top idct
add_files hls_src/2D_idct.cc -csimflags "-DTESTBENCH"
add_files hls_src/dct_math.h
add_files hls_src/mjpeg423_types.h
add_files hls_src/tables.c
add_files hls_src/util.c
add_files hls_src/util.h
add_files -tb hls_src/2D_idct_tb.cc
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg400-1}
create_clock -period 10 -name default
set_clock_uncertainty 1.25
#source "./vitis_hls_workspace/solution1/directives.tcl"
csim_design
csynth_design
cosim_design
export_design -format ip_catalog
