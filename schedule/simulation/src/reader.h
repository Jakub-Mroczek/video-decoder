/* C++ program for reader class 
   Assignment 4 ECE 423 Winter 2021
   Author: Reza Mirosanlou
   objects in C++. */

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

using namespace std;

class reader
{
public:
   reader(const char *filename) : myfile(filename) {} //  class constructor with "filename" input initialization
   int num_frames();                                  //  returns the number of frames in the input file. Frames are indexed from 0 to n-1
   int output_offset();                               //  returns the time in nanoseconds at which the timer for the periodic output starts. This is guaranteed to be less than 1/24 of a second.
   char type_frame(int);                             //  return frame type, index of the frame should be passed
   int SDR_time(int);                                 //  returns the time in nanoseconds required to execute the SD Read for the i-th frame (see Table 4.1 for maximum values)
   int LDY_time(int);                                 //  returns the time in nanoseconds required to execute the Lossless Decode on Y for the i-th frame.
   int LDCr_time(int);                                //  returns the time in nanoseconds required to execute the Lossless Decode on Cr for the i-th frame.
   int LDCb_time(int);                                //  returns the time in nanoseconds required to execute the Lossless Decode on Cb for the i-th frame.
   int IDCT_time(int);                                //  returns the time in nanoseconds required to execute the idct for the i-th frame (all blocks in one frame)
   int CC_time(int);                                  //  returns the time in nanoseconds required to execute the color conversion for the i-th frame
private:
   ifstream myfile;
};