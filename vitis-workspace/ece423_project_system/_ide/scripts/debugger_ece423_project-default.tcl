# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: C:\Users\mjfsawye\ece423-labs\vitis-workspace\ece423_project_system\_ide\scripts\debugger_ece423_project-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source C:\Users\mjfsawye\ece423-labs\vitis-workspace\ece423_project_system\_ide\scripts\debugger_ece423_project-default.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Xilinx PYNQ-Z1 003017AC82DBA" && level==0 && jtag_device_ctx=="jsn-Xilinx PYNQ-Z1-003017AC82DBA-23727093-0"}
fpga -file C:/Users/mjfsawye/ece423-labs/vitis-workspace/ece423_project/_ide/bitstream/lab_prefab_wrapper_dataflow.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw C:/Users/mjfsawye/ece423-labs/vitis-workspace/lab_prefab_wrapper/export/lab_prefab_wrapper/hw/lab_prefab_wrapper_dataflow.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source C:/Users/mjfsawye/ece423-labs/vitis-workspace/ece423_project/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow C:/Users/mjfsawye/ece423-labs/vitis-workspace/ece423_project/Release/ece423_project.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*A9*#0"}
con
