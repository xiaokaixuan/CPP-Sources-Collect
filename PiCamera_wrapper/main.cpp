#include "PiCamera.h"
#include <thread>

#include <iostream>
#include <iomanip>

int main() {

	PiCamera::global_init();

	PiCamera::print_cameras();

	PiCamera cm("/base/soc/i2c0mux/i2c@1/imx296@1a");

	int icount = 0;
	time_t itime = 0;
	cm.registerFrameCallback([&](const PICAM_OUT_FRAME& frame)
		{

			time_t now(time(nullptr));

			if (now == itime) ++icount;
			else
			{
				itime = now;
				icount = 1;
			}
			std::cout << "frame size:" << frame.width << " x " << frame.height
				<< ", buff_size:" << frame.buff_size << ", time:" << itime << ", frameNum:" << icount << std::endl;

		});

	cm.start_capture(60); // framerate = 60

	getchar();

	return 0;
}

