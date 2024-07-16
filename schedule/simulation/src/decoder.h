#include "systemc.h"
#include "video_output.h"
#include "reader.h"

#define PRINT_TASK(core, task, frame)                     \
    cout << "@" << sc_time_stamp() << " Core " << core << \
            " runs " << task <<                           \
            " on frame " << frame <<" \n" << endl;

SC_MODULE(decoder) {
private:

    const int period = 41666667; /* ns */
    const int buffer_size = 5; /* tokens */
    const int delay = 4; /* periods */
    const char *file_name = "input.txt";

    /* Cache flush times (ns)*/
    int L1_time_i = 30000;
    int L1_time_p = 20000;
    int L2_time_i = 690000;
    int L2_time_p = 240000;

    /* Precedence constraints */
    // sc_fifo<int> sdr_to_sdr;
    // sc_fifo<int> sdr_to_ld_y;
    sc_fifo<int> sdr_to_ld_cb;
    sc_fifo<int> sdr_to_ld_cr;
    // sc_fifo<int> ld_to_l1_y;
    // sc_fifo<int> ld_to_l1_cb;
    // sc_fifo<int> ld_to_l1_cr;
    sc_fifo<int> l1_to_l2_y;
    sc_fifo<int> l1_to_l2_cb;
    // sc_fifo<int> l1_to_l2_cr;
    sc_fifo<int> l2_to_dma;
    sc_fifo<int> dma_to_idct;
    sc_fifo<int> idct_to_dma;
    sc_fifo<int> dma_to_ld_y;  /* for P-frames */
    sc_fifo<int> dma_to_ld_cb; /* for P-frames */
    sc_fifo<int> dma_to_ld_cr; /* for P-frames */
    sc_fifo<int> dma_to_cc;
    sc_fifo<int> cc_to_buff_reg;
    // sc_fifo<int> buff_reg_to_buff_reg;

    reader r;

    void task(int core, const char *name, int framenum, int time, int value)
    {
        static sc_signal<int> *signals[3] = { &core0_task, &core1_task, &acc_task };
        sc_signal<int> *signal = signals[core];
        PRINT_TASK(core, name, framenum);
        *signal = value;
        if (time)
            wait(time, SC_NS);
        else
            wait(SC_ZERO_TIME);
        *signal = NONE;
    }

    /* II schedule */

    void core0_ii(int framenum, int skip_prev, int skip_curr)
    {
        /* P13_L1_FLUSH */
        if (!skip_prev)
            task(0, "P13_L1_FLUSH", framenum, L1_time_p, CACHEM);
        cc_to_buff_reg.write(0);

        /* I1_SD_READ */
        if (!skip_curr)
            task(0, "I1_SD_READ", framenum, r.SDR_time(framenum), SDR);
        sdr_to_ld_cb.write(0);
        sdr_to_ld_cr.write(0);

        /* I1_LD_Y */
        if (!skip_curr)
            task(0, "I1_LD_Y", framenum, r.LDY_time(framenum), LDY);

        /* I1_L1_FLUSH */
        if (!skip_curr)
            task(0, "I1_L1_FLUSH", framenum, L1_time_i, CACHEM);
        l1_to_l2_y.write(0);
    }

    void core1_ii(int framenum, int skip_prev, int skip_curr)
    {
        /* P13_CC_1524 */
        if (!skip_prev) {
            dma_to_cc.read(); /* since II and PI write */
            task(1, "P13_CC_1524", framenum-1, r.CC_time(framenum-1) * 1524/14400, CC);
        }

        /* I1_LD_CB */
        sdr_to_ld_cb.read();
        if (!skip_curr)
            task(1, "I1_LD_CB", framenum, r.LDCb_time(framenum), LDCB);

        /* I1_LD_CR */
        sdr_to_ld_cr.read();
        if (!skip_curr)
            task(1, "I1_LD_CR", framenum, r.LDCr_time(framenum), LDCR);

        /* I1_FLUSH */
        if (!skip_curr)
            task(1, "I1_FLUSH_L1", framenum, L1_time_i, CACHEM);
        l1_to_l2_y.read();
        if (!skip_curr)
            task(1, "I1_FLUSH_L2", framenum, L2_time_i, CACHEM);

        /* I1_DMA_START_Y */
        if (!skip_curr) {
            task(1, "I1_DMA_START_Y", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* P13_CC_1671 */
        if (!skip_prev)
            task(1, "P13_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* I1_DMA_POLL_Y */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "I1_DMA_POLL_Y", framenum, 0, DMAC);
        }

        /* I1_DMA_START_CB */
        if (!skip_curr) {
            task(1, "I1_DMA_START_CB", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* P13_CC_1671 */
        if (!skip_prev)
            task(1, "P13_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* I1_DMA_POLL_CB */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "I1_DMA_POLL_CB", framenum, 0, DMAC);
        }

        /* I1_DMA_START_CR */
        if (!skip_curr) {
            task(1, "I1_DMA_START_CR", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* P13_CC_799 */
        if (!skip_prev)
            task(1, "P13_CC_799", framenum-1, r.CC_time(framenum-1) * 799/14400, CC);

        /* P13_BUFF_REG */
        cc_to_buff_reg.read();
        if (!skip_prev) {
            task(1, "P13_BUFF_REG", framenum-1, L1_time_p + L2_time_p, CACHEM);
            pvideo->buff_reg();
        }

        /* I1_DMA_POLL_CR */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "I1_DMA_POLL_CR", framenum, 0, DMAC);
            pvideo->buff_next(framenum);
        }
        dma_to_cc.write(0);

        /* I1_CC_8735 */
        if (!skip_curr)
            task(1, "I1_CC_8735", framenum, r.CC_time(framenum) * 8735/14400, CC);
    }

    /* IP schedule */

    void core0_ip(int framenum, int skip_prev, int skip_curr)
    {
        /* I2_SD_READ */
        if (!skip_curr)
            task(0, "I2_SD_READ", framenum, r.SDR_time(framenum), SDR);
        sdr_to_ld_cr.write(0);

        /* I2_LD_Y */
        if (!skip_curr)
            task(0, "I2_LD_Y", framenum, r.LDY_time(framenum), LDY);

        /* I2_LD_CB */
        if (!skip_curr)
            task(0, "I2_LD_CB", framenum, r.LDCb_time(framenum), LDCB);

        /* I2_L1_FLUSH */
        if (!skip_curr)
            task(0, "I2_L1_FLUSH", framenum, L1_time_i, CACHEM);
        l1_to_l2_y.write(0);
        l1_to_l2_cb.write(0);

        /* I1_CC_1524 */
        if (!skip_prev) {
            dma_to_cc.read();
            task(0, "I1_CC_1524", framenum-1, r.CC_time(framenum-1) * 1524 / 14400, CC);
        }

        /* I1_L1_FLUSH */
        if (!skip_prev)
            task(0, "I1_L1_FLUSH", framenum-1, L1_time_i, CACHEM);
        cc_to_buff_reg.write(0);
    }

    void core1_ip(int framenum, int skip_prev, int skip_curr)
    {
        /* I2_LD_CR */
        sdr_to_ld_cr.read();
        if (!skip_curr)
            task(1, "I2_LD_CR", framenum, r.LDCr_time(framenum), LDCR);

        /* I2_FLUSH */
        if (!skip_curr)
            task(1, "I2_FLUSH_L1", framenum, L1_time_i, CACHEM);
        l1_to_l2_y.read();
        l1_to_l2_cb.read();
        if (!skip_curr)
            task(1, "I2_FLUSH_L2", framenum, L2_time_i, CACHEM);

        /* I2_DMA_START_Y */
        if (!skip_curr) {
            task(1, "I2_DMA_START_Y", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* I1_CC_1671 */
        if (!skip_prev)
            task(1, "I1_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* I2_DMA_POLL_Y */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "I2_DMA_POLL_Y", framenum, 0, DMAC);
        }
        dma_to_ld_y.write(0);

        /* I2_DMA_START_CB */
        if (!skip_curr) {
            task(1, "I2_DMA_START_CB", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* I1_CC_1671 */
        if (!skip_prev)
            task(1, "I1_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* I2_DMA_POLL_CB */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "I2_DMA_POLL_CB", framenum, 0, DMAC);
        }
        dma_to_ld_cb.write(0);

        /* I2_DMA_START_CR */
        if (!skip_curr) {
            task(1, "I2_DMA_START_CR", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* I1_CC_799 */
        if (!skip_prev)
            task(1, "I1_CC_799", framenum-1, r.CC_time(framenum-1) * 799/14400, CC);

        /* I1_BUFF_REG */
        cc_to_buff_reg.read();
        if (!skip_prev) {
            task(1, "I1_BUFF_REG", framenum, L1_time_i + L2_time_i, CACHEM);
            pvideo->buff_reg();
        }

        /* I2_DMA_POLL_CR */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "I2_DMA_POLL_CR", framenum, 0, DMAC);
            pvideo->buff_next(framenum);
        }
        dma_to_ld_cr.write(0);
        dma_to_cc.write(0);
    }

    /* PP schedule */

    void core0_pp(int framenum, int skip_prev, int skip_curr)
    {
        /* P3_SD_READ */
        if (!skip_curr)
            task(0, "P3_SD_READ", framenum, r.SDR_time(framenum), SDR);

        /* P3_LD_Y */
        dma_to_ld_y.read();
        if (!skip_curr)
            task(0, "P3_LD_Y", framenum, r.LDY_time(framenum), LDY);

        /* P3_LD_CB */
        dma_to_ld_cb.read();
        if (!skip_curr)
            task(0, "P3_LD_CB", framenum, r.LDCb_time(framenum), LDCB);

        /* P3_LD_CR */
        dma_to_ld_cr.read();
        if (!skip_curr)
            task(0, "P3_LD_CR", framenum, r.LDCr_time(framenum), LDCR);

        /* P3_FLUSH */
        if (!skip_curr) {
            task(0, "P3_FLUSH_L1", framenum, L1_time_p, CACHEM);
            task(0, "P3_FLUSH_L2", framenum, L2_time_p, CACHEM);
        }
        l2_to_dma.write(0);

        /* I2_CC_3943 */
        if (!skip_prev) {
            dma_to_cc.read();
            task(0, "I2_CC_3943", framenum-1, r.CC_time(framenum-1) * 3943/14400, CC);
        }

        /* I2_L1_FLUSH */
        if (!skip_prev)
            task(0, "I2_L1_FLUSH", framenum-1, L1_time_p, CACHEM);
        cc_to_buff_reg.write(0);
    }

    void core1_pp(int framenum, int skip_prev, int skip_curr)
    {
        /* I2_CC_6316 */
        if (!skip_prev)
            task(1, "I2_CC_6316", framenum-1, r.CC_time(framenum-1) * 6316/14400, CC);

        /* P3_DMA_START_Y */
        l2_to_dma.read();
        if (!skip_curr) {
            task(1, "P3_DMA_START_Y", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* I2_CC_1671 */
        if (!skip_prev)
            task(1, "I2_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* P3_DMA_POLL_Y */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "P3_DMA_POLL_Y", framenum, 0, DMAC);
        }
        dma_to_ld_y.write(0);

        /* P3_DMA_START_CB */
        if (!skip_curr) {
            task(1, "P3_DMA_START_CB", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* I2_CC_1671 */
        if (!skip_prev)
            task(1, "I2_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* P3_DMA_POLL_CB */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "P3_DMA_POLL_CB", framenum, 0, DMAC);
        }
        dma_to_ld_cb.write(0);

        /* P3_DMA_START_CR */
        if (!skip_curr) {
            task(1, "P3_DMA_START_CR", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* I2_CC_799 */
        if (!skip_prev)
            task(1, "I2_CC_799", framenum-1, r.CC_time(framenum-1) * 799/14400, CC);

        /* I2_BUFF_REG */
        cc_to_buff_reg.read();
        if (!skip_prev) {
            task(1, "I2_BUFF_REG", framenum-1, L1_time_p + L2_time_p, CACHEM);
            pvideo->buff_reg();
        }

        /* P3_DMA_POLL_CR */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "P3_DMA_POLL_CR", framenum, 0, DMAC);
            pvideo->buff_next(framenum);
        }
        dma_to_ld_cr.write(0);
        dma_to_cc.write(0);
    }

    /* PI schedule */

    void core0_pi(int framenum, int skip_prev, int skip_curr)
    {
        /* P13_SD_READ */
        if (!skip_curr)
            task(0, "P13_SD_READ", framenum, r.SDR_time(framenum), SDR);
        sdr_to_ld_cb.write(0);
        sdr_to_ld_cr.write(0);

        /* P13_LD_Y */
        dma_to_ld_y.read();
        if (!skip_curr)
            task(0, "P13_LD_Y", framenum, r.LDY_time(framenum), LDY);

        /* P13_L1_FLUSH */
        if (!skip_curr)
            task(0, "P13_L1_FLUSH", framenum, L1_time_p, CACHEM);
        l1_to_l2_y.write(0);

        /* P12_CC_6387 */
        if (!skip_prev)
            task(0, "P12_CC_6387", framenum-1, r.CC_time(framenum-1) * 6387/14400, CC);

        /* P12_L1_FLUSH */
        if (!skip_prev)
            task(0, "P12_L1_FLUSH", framenum-1, L1_time_p, CACHEM);
        cc_to_buff_reg.write(0);

        /* P13_CC_3000 */
        if (!skip_curr) {
            dma_to_cc.read();
            task(0, "P13_CC_3000", framenum, r.CC_time(framenum) * 3000 / 14400, CC);
        }
    }

    void core1_pi(int framenum, int skip_prev, int skip_curr)
    {
        /* P13_LD_CB */
        sdr_to_ld_cb.read();
        dma_to_ld_cb.read();
        if (!skip_curr)
            task(1, "P13_LD_CB", framenum, r.LDCb_time(framenum), LDCB);

        /* P13_LD_CR */
        sdr_to_ld_cr.read();
        dma_to_ld_cr.read();
        if (!skip_curr)
            task(1, "P13_LD_CR", framenum, r.LDCr_time(framenum), LDCR);

        /* P13_FLUSH */
        if (!skip_curr)
            task(1, "P13_FLUSH_L1", framenum, L1_time_p, CACHEM);
        l1_to_l2_y.read();
        if (!skip_curr)
            task(1, "P13_FLUSH", framenum, L2_time_p, CACHEM);

        /* P13_DMA_START_Y */
        if (!skip_curr) {
            task(1, "P13_DMA_START_Y", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* P12_CC_1671 */
        if (!skip_prev)
            task(1, "P12_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* P13_DMA_POLL_Y */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "P13_DMA_POLL_Y", framenum, 0, DMAC);
        }

        /* P13_DMA_START_CB */
        if (!skip_curr) {
            task(1, "P13_DMA_START_CB", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* P12_CC_1671 */
        if (!skip_prev)
            task(1, "P12_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* P13_DMA_POLL_CB */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "P13_DMA_POLL_CB", framenum, 0, DMAC);
        }

        /* P13_DMA_START_CR */
        if (!skip_curr) {
            task(1, "P13_DMA_START_CR", framenum, 0, DMAS);
            dma_to_idct.write(framenum);
        }

        /* P12_CC_1671 */
        if (!skip_prev)
            task(1, "P12_CC_1671", framenum-1, r.CC_time(framenum-1) * 1671/14400, CC);

        /* P13_DMA_POLL_CR */
        if (!skip_curr) {
            idct_to_dma.read();
            task(1, "P13_DMA_POLL_CR", framenum, 0, DMAC);
            pvideo->buff_next(framenum);
        }
        dma_to_cc.write(0);

        /* P12_CC_3000 */
        if (!skip_prev)
            task(1, "P12_CC_3000", framenum-1, r.CC_time(framenum-1) * 3000/14400, CC);

        /* P12_BUFF_REG */
        cc_to_buff_reg.read();
        if (!skip_prev) {
            task(1, "P12_BUFF_REG", framenum-1, L1_time_p + L2_time_p, CACHEM);
            pvideo->buff_reg();
        }

        /* P13_CC_5735 */
        if (!skip_curr)
            task(1, "P13_CC_5735", framenum, r.CC_time(framenum) * 5735/14400, CC);
    }

public:

    video_output* pvideo;

    /*
     * Tracing signals are written to
     * every time a new frame starts being
     * processed, and a task starts/stops
     * being executed
     */
    enum Task_Type { NONE, SDR, LDY, LDCR, LDCB, DMAS, IDCT, DMAC, CC, CACHEM };
    sc_signal<int> core0_task;
    sc_signal<int> core1_task;
    sc_signal<int> acc_task;
    sc_signal<int> core0_framenum;
    sc_signal<int> core1_framenum;
    sc_signal<int> acc_framenum;

    void core0()
    {
        int framenum;

        /* Execute tasks for all but last frame */
        for (framenum = 0;; framenum++) {
            core0_framenum = framenum;

            if (framenum+1 == r.num_frames())
                break;
            
            if (r.type_frame(framenum) == 'I')
                if (r.type_frame(framenum+1) == 'I')
                    core0_ii(framenum, !framenum, 0);
                else
                    core0_ip(framenum, !framenum, 0);
            else
                if (r.type_frame(framenum+1) == 'I')
                    core0_pi(framenum, !framenum, 0);
                else
                    core0_pp(framenum, !framenum, 0);
        }

        /* Execute tasks for last frame */
        if (r.type_frame(framenum) == 'I') {
            core0_ii(framenum, 0, 0);
            core0_ii(framenum+1, 0, 1);
        } else {
            core0_pp(framenum, 0, 0);
            core0_pp(framenum+1, 0, 1);
        }

        /* All frames done, terminating the thread */
    }

    void core1()
    {
        int framenum;

        /* Execute tasks for all but last frame */
        for (framenum = 0;; framenum++) {
            core1_framenum = framenum;

            if (framenum+1 == r.num_frames())
                break;
            
            if (r.type_frame(framenum) == 'I')
                if (r.type_frame(framenum+1) == 'I')
                    core1_ii(framenum, !framenum, 0);
                else
                    core1_ip(framenum, !framenum, 0);
            else
                if (r.type_frame(framenum+1) == 'I')
                    core1_pi(framenum, !framenum, 0);
                else
                    core1_pp(framenum, !framenum, 0);
        }

        /* Execute tasks for last frame */
        if (r.type_frame(framenum) == 'I') {
            core1_ii(framenum, 0, 0);
            core1_ii(framenum+1, 0, 1);
        } else {
            core1_pp(framenum, 0, 0);
            core1_pp(framenum+1, 0, 1);
        }
    }

    void acc() 
    {
        int framenum;
        while (1) {
            framenum = dma_to_idct.read();
            acc_framenum = framenum;
            task(2, "IDCT", framenum, r.IDCT_time(framenum), IDCT);
            idct_to_dma.write(0);
        }
    }

    SC_CTOR(decoder) :
        sdr_to_ld_cb(buffer_size),
        sdr_to_ld_cr(buffer_size),
        l1_to_l2_y(buffer_size),
        l1_to_l2_cb(buffer_size),
        l2_to_dma(buffer_size),
        dma_to_idct(buffer_size),
        idct_to_dma(buffer_size),
        dma_to_ld_y(buffer_size),
        dma_to_ld_cb(buffer_size),
        dma_to_ld_cr(buffer_size),
        dma_to_cc(buffer_size),
        cc_to_buff_reg(buffer_size),
        r(file_name)
    {
        //create new video_output and initialize it using the defined period, delay, buffer_size, 
        //and the values of offset and num_frames obtained from the input file
        //note that we pass buffer_size + 1 to the video_output because the framebuffer needs one extra buffer space
        //to store the current frame being displayed
        //calling video_output.output_frame(framenum) will block if the number of frames that have been sent to the video output
        //but not displayed yet is equal to buffer_size
        pvideo = new video_output("video", r.output_offset(), period, delay, buffer_size + 1, r.num_frames());

        //initialize all signals 
        core0_task = NONE;
        core1_task = NONE;
        acc_task = NONE;
        core0_framenum = -1;
        core1_framenum = -1;
        acc_framenum = -1;

        //start the threads
        SC_THREAD(core0);
        SC_THREAD(core1);
        SC_THREAD(acc);
    } 

    ~decoder() {
        delete pvideo;
    }
};
