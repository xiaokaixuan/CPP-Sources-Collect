#if !defined(__V4L2CXX_HPP__) && !defined(_WIN32)
#define __V4L2CXX_HPP__

#include <string>
#include <vector>
#include <cstring>
#include <linux/videodev2.h>

namespace v4l2cxx
{
	constexpr static int NUM_OF_MAP_BUFFER = 4;
	enum class PixFormat
	{
		PIX_FMT_RGB332 = V4L2_PIX_FMT_RGB332,
		PIX_FMT_YUYV = V4L2_PIX_FMT_YUYV,
		PIX_FMT_YVYU = V4L2_PIX_FMT_YVYU,
		PIX_FMT_MJPEG = V4L2_PIX_FMT_MJPEG,
		PIX_FMT_NV12 = V4L2_PIX_FMT_NV12
	};
	struct Buffer
	{
		void* start{};
		size_t length{};
	};

	class V4LCapture
	{
	public:
		V4LCapture() {}
		V4LCapture(const std::string& devpath, unsigned int width = 640, unsigned int height = 480, PixFormat pix_fmt = PixFormat::PIX_FMT_MJPEG)
			: m_devpath(devpath), m_width(width), m_height(height), m_pix_fmt(pix_fmt) {}
		~V4LCapture() { close(); }
	private:
		V4LCapture(const V4LCapture&) = delete;
		V4LCapture(const V4LCapture&&) = delete;
		const V4LCapture& operator =(const V4LCapture&) = delete;
		const V4LCapture& operator =(const V4LCapture&&) = delete;

	public:
		inline bool isOpened() const { return m_fd != -1; }
		inline bool open() { return open(m_devpath); }
		bool open(const std::string& devpath);
		void close();

		inline bool setExposureMode(int value) { return setCtrl(V4L2_CID_EXPOSURE_AUTO, value); }
		inline bool setExposureTime(int value) { return setCtrl(V4L2_CID_EXPOSURE_ABSOLUTE, value); }

		inline int getWidth() const { return m_width; }
		inline int getHeight() const { return m_height; }
		bool grabFrame(unsigned char** pp_data, unsigned int* p_size);

	public:
		bool setFormat(int width, int height, PixFormat pix_fmt = PixFormat::PIX_FMT_MJPEG);
		bool setCtrl(unsigned int v4l_id, int value);

		static bool xioctl(int fd, int request, void* arg);
		static v4l2_buf_type get_video_buffer_type(int fd);

	private:
		bool _set_format(int width, int height, PixFormat pix_fmt);
		bool _init_mmap();
		bool _set_capture_steamon();
		bool _set_capture_steamoff();
		bool _queue_frames();

	private:
		std::string m_devpath{ "/dev/video0" };
		unsigned int m_width{ 640 }, m_height{ 480 };
		PixFormat m_pix_fmt{ PixFormat::PIX_FMT_MJPEG };

		int m_fd{ -1 };
		v4l2_buf_type m_v4l2_buf_type{ V4L2_BUF_TYPE_VIDEO_CAPTURE };
		int m_buffer_index{ -1 };
		int m_numOfmapBuff{ NUM_OF_MAP_BUFFER };
		struct Buffer m_buffers[NUM_OF_MAP_BUFFER]{};
	};
}

#endif // __V4L2CXX_HPP__

