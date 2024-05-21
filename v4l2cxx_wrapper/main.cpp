//
// Created by dan on 4/29/18.
//
#include <iostream>
#include "v4l2cxx.hpp"


int main()
{
    v4l2cxx::V4LCapture cap;
    cap.setFormat(1280, 720);
    if (!cap.open("/dev/video0"))
    {
        std::cerr << "Error opening camera." << std::endl;
        return -1;
    }
    
    static int frame_num = 0;
    static time_t old_time = 0;

    for (;;)
    {
        unsigned char* p_data(nullptr);
        unsigned int size(0);
        if (!cap.grabFrame(&p_data, &size)) break; // p_data is pointer to MJPG raw-data, (need cv::imdecode(cv::Mat(1, size, CV_8U, p_data),...))
        time_t new_time = time(nullptr);
        if (new_time == old_time) ++frame_num;
        else frame_num = 1;
        old_time = new_time;
        printf("time = %ld, frame = %d, image size: %u, %d x %d\n", old_time, frame_num, size, cap.getWidth(), cap.getHeight());
    }
}

