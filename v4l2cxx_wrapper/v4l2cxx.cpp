#include "v4l2cxx.hpp"

#if !defined(_WIN32)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>


bool v4l2cxx::V4LCapture::xioctl(int fd, int request, void* arg)
{
	for (int i(0); i < 10; ++i) // EAGAIN RETRY COUNT
	{
		int res = ioctl(fd, request, arg);
		if (-1 != res) return true;
		if (EAGAIN != errno) return false;
		fd_set fds{};
		struct timeval tv
		{
			.tv_sec = 5,
				.tv_usec = 0
		};
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		res = select(fd + 1, &fds, NULL, NULL, &tv);
		if (0 == res) // Timeout
		{
			perror("ERROR: Xioctl select timeout.\n");
			return false;
		}
		if (-1 == res) // Error
		{
			if (EAGAIN == errno) continue;
			perror("ERROR: Xioctl select failure.\n");
			return false;
		}
	}
	return false;
}

bool v4l2cxx::V4LCapture::open(const std::string& devpath)
{
	m_devpath = devpath;
	m_first_frame = true;
	m_buffer_index = -1;
	m_fd = ::open(m_devpath.c_str(), O_RDWR | O_NONBLOCK);
	if (m_fd == -1)
	{
		perror("ERROR: Open video device failure.\n");
		return false;
	}
	if (!_set_format(m_width, m_height, m_pix_fmt))
	{
		close();
		return false;
	}
	if (!_init_mmap())
	{
		close();
		return false;
	}
	if (!_set_capture_steamon())
	{
		close();
		return false;
	}
	return true;
}

void v4l2cxx::V4LCapture::close()
{
	m_first_frame = false;
	m_buffer_index = -1;
	for (auto i = 0; i < NUM_OF_MAP_BUFFER; ++i)
	{
		if (!m_buffers[i].start) continue;
		munmap(m_buffers[i].start, m_buffers[i].length);
		m_buffers[i].start = nullptr;
	}
	if (m_fd != -1)
	{
		struct v4l2_requestbuffers req;
		ZERO_MEM(req);
		req.count = 0;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;
		xioctl(m_fd, VIDIOC_REQBUFS, &req);
		::close(m_fd);
		m_fd = -1;
	}
}

bool v4l2cxx::V4LCapture::setFormat(int width, int height, PixFormat pix_fmt)
{
	if (m_fd == -1)
	{
		m_width = width;
		m_height = height;
		m_pix_fmt = pix_fmt;
		return true;
	}
	close();
	m_width = width;
	m_height = height;
	m_pix_fmt = pix_fmt;
	return open();
}

bool v4l2cxx::V4LCapture::_set_format(int width, int height, PixFormat pix_fmt)
{
	struct v4l2_format fmt;
	ZERO_MEM(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = static_cast<unsigned int>(pix_fmt);
	fmt.fmt.pix.field = V4L2_FIELD_ANY;
	if (!xioctl(m_fd, VIDIOC_S_FMT, &fmt))
	{
		perror("WARNING: Set format failure.\n");
		return false;
	}
	m_width = fmt.fmt.pix.width;
	m_height = fmt.fmt.pix.height;
	m_pix_fmt = static_cast<PixFormat>(fmt.fmt.pix.pixelformat);
	return true;
}

bool v4l2cxx::V4LCapture::_init_mmap()
{
	if (m_fd == -1)
	{
		perror("ERROR: Video device not opened, Init mmap failure.\n");
		return false;
	}
	struct v4l2_requestbuffers req;
	ZERO_MEM(req);
	req.count = NUM_OF_MAP_BUFFER;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (!xioctl(m_fd, VIDIOC_REQBUFS, &req))
	{
		if (EINVAL == errno)
		{
			perror("ERROR: Does not support memory mapping.\n");
		}
		else
		{
			perror("ERROR: Init mmap VIDIOC_REQBUFS failure.\n");
		}
		return false;
	}
	if (req.count < 2)
	{
		perror("ERROR: Insufficient buffer memory.\n");
		return false;
	}
	for (unsigned int i = 0; i < req.count; ++i)
	{
		struct v4l2_buffer buf;
		ZERO_MEM(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (!xioctl(m_fd, VIDIOC_QUERYBUF, &buf))
		{
			perror("ERROR: Init mmap VIDIOC_QUERYBUF failure.\n");
			return false;
		}
		m_buffers[i].length = buf.length;
		m_buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, buf.m.offset);
		if (MAP_FAILED == m_buffers[i].start)
		{
			m_buffers[i].start = nullptr;
			perror("ERROR: Init mmap failure.\n");
			return false;
		}
	}
	return true;
}

bool v4l2cxx::V4LCapture::_set_capture_steamon()
{
	if (m_fd == -1)
	{
		perror("ERROR: Video device not opened, Set capture steamon failure.\n");
		return false;
	}
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (!xioctl(m_fd, VIDIOC_STREAMON, &type))
	{
		perror("ERROR: Set capture steamon failure.\n");
		return false;
	}
	return true;
}

bool v4l2cxx::V4LCapture::setCtrl(unsigned int v4l_id, int value)
{
	if (m_fd == -1)
	{
		perror("WARNING: Video device not opened, Set ctrl failure.\n");
		return false;
	}
	struct v4l2_control control;
	ZERO_MEM(control);
	control.id = v4l_id;
	control.value = value;
	if (!xioctl(m_fd, VIDIOC_S_CTRL, &control))
	{
		perror("ERROR: Set ctrl failure.\n");
		return false;
	}
	return true;
}

bool v4l2cxx::V4LCapture::_queue_frames()
{
	if (m_fd == -1)
	{
		perror("ERROR:  Video device not opened, Queue frames failure.\n");
		return false;
	}
	// Queue into all MMAP buffers from capture device.
	for (unsigned int i = 0; i < NUM_OF_MAP_BUFFER; ++i) {
		struct v4l2_buffer buf;
		ZERO_MEM(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (!xioctl(m_fd, VIDIOC_QBUF, &buf))
		{
			perror("ERROR: Queue frames failure.\n");
			close();
			return false;
		}
	}
	return true;
}

bool v4l2cxx::V4LCapture::grabFrame(unsigned char** pp_data, unsigned int* p_size)
{
	if (m_first_frame)
	{
		if (!_queue_frames()) return false;
		m_first_frame = false;
	}
	else if (m_buffer_index >= 0 && m_buffer_index < NUM_OF_MAP_BUFFER)
	{
		struct v4l2_buffer buf;
		ZERO_MEM(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = m_buffer_index;
		if (!xioctl(m_fd, VIDIOC_QBUF, &buf))
		{
			perror("ERROR: Grab frame VIDIOC_QBUF failure.\n");
			close();
			return false;
		}
		m_buffer_index = -1;
	}
	struct v4l2_buffer buf;
	ZERO_MEM(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (!xioctl(m_fd, VIDIOC_DQBUF, &buf))
	{
		perror("ERROR: Grab frame VIDIOC_DQBUF failure.\n");
		close();
		return false;
	}
	if (buf.index >= NUM_OF_MAP_BUFFER) return false;
	m_buffer_index = buf.index;

	*pp_data = (unsigned char*)m_buffers[buf.index].start;
	*p_size = buf.bytesused;
	return true;
}

#endif // !defined(_WIN32)

