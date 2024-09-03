#if !defined(__PICAMERA_H__) && !defined(_WIN32)
#define __PICAMERA_H__

#include <libcamera/libcamera.h>
#include <libcamera/camera.h>
#include <libcamera/framebuffer.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>


// PiCamera Out Frame
typedef struct tagPICAM_OUT_FRAME
{
	size_t width;
	size_t height;
	unsigned char* buffer;
	size_t buff_size;
} PICAM_OUT_FRAME, *LPPICAM_OUT_FRAME;


class PiCamera
{
public:
	PiCamera(const char* devname = nullptr);
	~PiCamera();

public:
	static int global_init();
	static void print_cameras();

public:
	bool open(const char* devname);
	void close();

	inline bool is_opened() const { return !camera_ptr; }

	bool start_capture(int framerate = 30);
	void stop_capture();

	void registerFrameCallback(const std::function<void(const PICAM_OUT_FRAME& frame)>& func) { frame_callback = func; }

private:
	void requestCompleted(libcamera::Request* request);

private:
	std::string m_devname;
	std::shared_ptr<libcamera::Camera> camera_ptr;
	std::unique_ptr<libcamera::CameraConfiguration> config_ptr;
	libcamera::FrameBufferAllocator* allocator_ = nullptr;
	std::vector<std::unique_ptr<libcamera::Request>> requests_;

	libcamera::ControlList controls_;
	std::function<void(const PICAM_OUT_FRAME& frame)> frame_callback;
	
private:
	static struct Initor
	{
	public:
		Initor() {}
		~Initor() { if (sm_pcm) delete sm_pcm; }
		int initialize();
		libcamera::CameraManager *sm_pcm = nullptr;
	} sm_initor;
};

#endif // __PICAMERA_H__

