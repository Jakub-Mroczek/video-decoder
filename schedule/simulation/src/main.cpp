#include "systemc.h"
#include "decoder.h"

int sc_main (int argc, char* argv[]) {

  sc_set_time_resolution(1, SC_NS);

  //Create main module
  decoder application("APPLICATION");

  // Open VCD file
  sc_trace_file *wf = sc_create_vcd_trace_file("trace");
  // Dump the desired signals
  sc_trace(wf, application.core0_task, "Core0-Task");
  sc_trace(wf, application.core1_task, "Core1-Task");
  sc_trace(wf, application.acc_task, "Accelerator-Task");
  sc_trace(wf, application.core0_framenum, "Core0-FrameNum");
  sc_trace(wf, application.core1_framenum, "Core1-FrameNum");
  sc_trace(wf, application.acc_framenum, "Accelerator-FrameNum");

  //Start simulation. Simulation will be stopped by the video output once the last frame is displayed
  sc_start();

  //End of simulation. Close VCD file
  cout << " End of simulation \n" << endl;
  sc_close_vcd_trace_file(wf);
  return 0;
 }
