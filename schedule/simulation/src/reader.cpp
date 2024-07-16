/* C++ program for reader class 
   Assignment 4 ECE 423 Winter 2021
   Author: Reza Mirosanlou
   objects in C++. 
   Input Format:

   Number of frame
   Interrupt timer   
   Frame type
   SDR time
   LDY time
   LDCR time
   LDCB time
   IDCT time
   CC time
   
   */

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include "reader.h"

using namespace std;

int reader::num_frames()
{
    string line_read;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);
            int i_auto = std::stoi(line_read, nullptr, 0);
            myfile.seekg(0);
            return i_auto;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}
int reader::output_offset()
{
    string line_read;
    string line_read1;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);
            getline(myfile, line_read1);
            if (line_read.empty())
            {
                cout << "The input file is corrupted or does not include this information" << endl;
                abort();
            }
            int i_auto = std::stoi(line_read1, nullptr, 0);
            myfile.seekg(0);
            return i_auto;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}

char reader::type_frame(int index)
{
    long lineNumber = 1;
    string line_read;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);

            if (lineNumber == index * 7 + 3)
            {
                if (line_read.empty())
                {
                    cout << "The input file is corrupted or does not include this information" << endl;
                    abort();
                }              
                myfile.seekg(0);
                return line_read[0];
            }
            lineNumber++;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}

int reader::SDR_time(int index)
{
    long lineNumber = 1;
    string line_read;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);
            if (lineNumber == index * 7 + 4)
            {
                if (line_read.empty())
                {
                    cout << "The input file is corrupted or does not include this information" << endl;
                    abort();
                }
                int i_auto = std::stoi(line_read, nullptr, 0);
                myfile.seekg(0);
                return i_auto;
            }
            lineNumber++;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}

int reader::LDY_time(int index)
{
    long lineNumber = 1;
    string line_read;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);
            if (lineNumber == index * 7 + 5)
            {
                if (line_read.empty())
                {
                    cout << "The input file is corrupted or does not include this information" << endl;
                    abort();
                }
                int i_auto = std::stoi(line_read, nullptr, 0);
                myfile.seekg(0);
                return i_auto;
            }
            lineNumber++;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}
int reader::LDCr_time(int index)
{
    long lineNumber = 1;
    string line_read;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);
            if (lineNumber == index * 7 + 6)
            {
                if (line_read.empty())
                {
                    cout << "The input file is corrupted or does not include this information" << endl;
                    abort();
                }
                int i_auto = std::stoi(line_read, nullptr, 0);
                myfile.seekg(0);
                return i_auto;
            }
            lineNumber++;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}
int reader::LDCb_time(int index)
{
    long lineNumber = 1;
    string line_read;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);
            if (lineNumber == index * 7 + 7)
            {
                if (line_read.empty())
                {
                    cout << "The input file is corrupted or does not include this information" << endl;
                    abort();
                }
                int i_auto = std::stoi(line_read, nullptr, 0);
                myfile.seekg(0);
                return i_auto;
            }
            lineNumber++;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}
int reader::IDCT_time(int index)
{
    long lineNumber = 1;
    string line_read;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);
            if (lineNumber == index * 7 + 8)
            {
                if (line_read.empty())
                {
                    cout << "The input file is corrupted or does not include this information" << endl;
                    abort();
                }
                int i_auto = std::stoi(line_read, nullptr, 0);
                myfile.seekg(0);
                return i_auto;
            }
            lineNumber++;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}
int reader::CC_time(int index)
{
    long lineNumber = 1;
    string line_read;
    if (myfile.is_open())
    {
        while (!myfile.eof())
        {
            getline(myfile, line_read);
            if (lineNumber == index * 7 + 9)
            {
                if (line_read.empty())
                {
                    cout << "The input file is corrupted or does not include this information" << endl;
                    abort();
                }
                int i_auto = std::stoi(line_read, nullptr, 0);
                myfile.seekg(0);
                return i_auto;
            }
            lineNumber++;
        }
        cout << "The input file is empty or the requested information is not found - check your input file" << endl;
        abort();
    }
    cout << "The input file is not open" << endl;
    return 0;
}

