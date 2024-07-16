#include "systemc.h"

SC_MODULE(video_output) {
private:
    int offset;
    int period;
    int delay;
    int num_frames;
    sc_fifo<int> frame_queue;

    int num_regs;

public:

    void buff_next(int framenum){
        cerr << "VIDEO_OUT_MESSAGE:@" << sc_time_stamp() << " Obtaining buffer for frame: " << framenum << " \n" << endl;
        frame_queue.write(framenum);
        cerr << "VIDEO_OUT_MESSAGE:@" << sc_time_stamp() << " Obtained buffer for frame: " << framenum << " \n" << endl;
    }

    void buff_reg() {
        if (num_regs == frame_queue.num_available()) {
                    //no new frame to register, error
                    cerr << "VIDEO_OUT_MESSAGE:@" << sc_time_stamp() << " Video output: attempting to register more frames than obtained buffers, ERROR\n" << endl;
                    sc_stop();
        }
        num_regs++;
        cerr << "VIDEO_OUT_MESSAGE:@" << sc_time_stamp() << " Registered a buffer\n" << endl;
    }

    int vdma_out() {
        if (num_regs == 0) 
            return 0;
        //display new frame
        int framenum = frame_queue.read();
        num_regs--;
        cerr << "VIDEO_OUT_MESSAGE:@" << sc_time_stamp() << " Video output: outputting frame " << framenum << ", available registered frames: " << num_regs << ", unregistered buffers: " << frame_queue.num_available() - num_regs << " \n" << endl;
        return 1;
    }

    void timer_handler() {
        cerr << "VIDEO_OUT_MESSAGE: Video will output " << num_frames << " frames\n" << endl;
        //initial offset delay at start of simulation
        wait(offset, SC_NS);
        while (1) {
            if (delay > 0) {
                //initial delay, do not display frame
                cerr << "VIDEO_OUT_MESSAGE:@" << sc_time_stamp() << " Video output timer: delay = " << delay << " \n" << endl;
                delay--;
            }
            else {
                int res = vdma_out();
                if (res == 0) {
                    //no new frame to display, error
                    cerr << "VIDEO_OUT_MESSAGE:@" << sc_time_stamp() << " Video output timer: no registered frames, ERROR\n" << endl;
                    sc_stop();
                }
                num_frames--;
                if (num_frames == 0) {
                    //all frames displayed, end simulation
                    cerr << "VIDEO_OUT_MESSAGE:@" << sc_time_stamp() << " Video output timer: all frames sent correctly, STOP\n" << endl;
                    sc_stop();
                }

            }
            //wait for period until next interrupt
            wait(period, SC_NS);
        }
    }



    //the size of the frame_queue is buf - 1 because an extra buffer space is taken by the current frame (not modelled in this simulation)
    video_output(sc_module_name name, int off, int p, int d, int buf, int num) : sc_module(name), offset(off), period(p), delay(d), num_frames(num), frame_queue(buf - 1), num_regs(0) {
        SC_THREAD(timer_handler);
    }
    SC_HAS_PROCESS(video_output);

};
