#include "PiCamera.h"
#include "mapped_framebuffer.h"

#if !defined(_WIN32)

PiCamera::Initor PiCamera::sm_initor;

int PiCamera::Initor::initialize()
{
	sm_pcm = new libcamera::CameraManager();
	return sm_pcm->start();
}

void PiCamera::print_cameras()
{
	if (!sm_initor.sm_pcm)
	{
		perror("ERROR: libcamera::CameraManager not initialize !!!\n");
		return;
	}
	const std::vector<std::shared_ptr<libcamera::Camera>> cameras = sm_initor.sm_pcm->cameras();
	for (const auto& ptr : cameras)
	{
		printf("DEBUG: Found camera name: %s\n", ptr->id().c_str());
	}
}

int PiCamera::global_init()
{
	return sm_initor.initialize();
}

////////////////////////////////////////////////////////////////////////////

PiCamera::PiCamera(const char* devname)
{
	if (devname) open(devname);
}

PiCamera::~PiCamera()
{
	stop_capture();
	close();
}

bool PiCamera::open(const char* devname)
{
	m_devname = devname;
	if (!sm_initor.sm_pcm)
	{
		perror("ERROR: libcamera::CameraManager not initialize !!!\n");
		return false;
	}
	camera_ptr = sm_initor.sm_pcm->get(devname);
	if (!camera_ptr)
	{
		perror("ERROR: camera not found !!!\n");
		return false;
	}
	// open camera
	int ret = camera_ptr->acquire();
	if (0 != ret)
	{
		fprintf(stderr, "ERROR: camera_ptr->acquire error: %#x\n", ret);
		return false;
	}
	// config camera
	config_ptr = camera_ptr->generateConfiguration({ libcamera::StreamRole::VideoRecording });
	for (libcamera::StreamConfiguration& streamConfig : *config_ptr)
	{
		streamConfig.bufferCount = 6;
		config_ptr->validate();
		printf("DEBUG: Validated StreamConfiguration is: %s\n", streamConfig.toString().c_str());
	}
	ret = camera_ptr->configure(config_ptr.get());
	if (0 != ret)
	{
		fprintf(stderr, "ERROR: camera_ptr->configure error: %#x\n", ret);
		return false;
	}
	allocator_ = new libcamera::FrameBufferAllocator(camera_ptr);
	return true;
}

void PiCamera::close()
{
	if (allocator_) delete allocator_;
	if (camera_ptr)
	{
		camera_ptr->stop();
	}
}

void PiCamera::stop_capture()
{
	if (camera_ptr)
	{
		camera_ptr->stop();
		camera_ptr->requestCompleted.disconnect(this, &PiCamera::requestCompleted);
	}
	requests_.clear();
}

bool PiCamera::start_capture(int framerate)
{
	if (!camera_ptr)
	{
		perror("ERROR: camera not opened !!!\n");
		return false;
	}
	std::shared_ptr<libcamera::Request> request = camera_ptr->createRequest();
	if (!request)
	{
		perror("ERROR: Failed to create request !!!\n");
		return false;
	}
	for (const libcamera::StreamConfiguration& streamConfig : *config_ptr)
	{
		libcamera::Stream* stream = streamConfig.stream();
		int ret = allocator_->allocate(stream);
		if (ret < 0)
		{
			fprintf(stderr, "ERROR: allocator_->allocate: %#x\n", ret);
			return false;
		}
		for (const std::unique_ptr<libcamera::FrameBuffer>& buffer : allocator_->buffers(stream)) {
			std::unique_ptr<libcamera::Request> request = camera_ptr->createRequest();
			if (!request)
			{
				perror("ERROR: Failed to create request !!!\n");
				return false;
			}
			if (request->addBuffer(stream, buffer.get())) {
				perror("ERROR: Failed to associate buffer with request !!!\n");
				return false;
			}
			requests_.push_back(std::move(request));
		}
	}
	camera_ptr->requestCompleted.connect(this, &PiCamera::requestCompleted);
	libcamera::ControlList controls(camera_ptr->controls());
	if (!controls.contains(libcamera::controls::FrameDurationLimits.id()))
	{
		int64_t frame_time = 1000000 / framerate; // in us
		controls.set(libcamera::controls::FrameDurationLimits, { frame_time, frame_time });
	}
	int ret = camera_ptr->start(&controls);
	if (0 != ret)
	{
		fprintf(stderr, "ERROR: camera_ptr->start error: %#x\n", ret);
		return false;
	}

	for (std::unique_ptr<libcamera::Request>& request : requests_) {
		if (camera_ptr->queueRequest(request.get()))
		{
			perror("ERROR: Failed to queue request !!!\n");
			return false;
		}
	}
	return true;
}

void PiCamera::requestCompleted(libcamera::Request* request)
{
	if (request->status() == libcamera::Request::RequestCancelled) return;

	const libcamera::Request::BufferMap& buffers = request->buffers();
	for (const auto& bufferPair : buffers)
	{
		const libcamera::Stream* stream = bufferPair.first;
		libcamera::FrameBuffer* buffer = bufferPair.second;
		
		const libcamera::StreamConfiguration& streamConfig = stream->configuration();
		unsigned int width = streamConfig.size.width;
		unsigned int height = streamConfig.size.height;

		libcamera::MappedFrameBuffer mappedBuffer(buffer, libcamera::MappedFrameBuffer::MapFlag::Read);
		const std::vector<libcamera::Span<uint8_t>> mem = mappedBuffer.planes(); // mem[0],mem[1],mem[2] <=> YUV420

		PICAM_OUT_FRAME outFrame{ width, height, static_cast<unsigned char*>(mem[0].data()), mem[0].size() };
		if (frame_callback) frame_callback(outFrame);
	}

	/* Re-queue the Request to the camera. */
	request->reuse(libcamera::Request::ReuseBuffers);
	camera_ptr->queueRequest(request);
}

#endif // !defined(_WIN32)

