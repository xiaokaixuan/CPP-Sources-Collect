#if !defined(__MAPPED_FRAMEBUFFER_H__) && !defined(_WIN32)
#define __MAPPED_FRAMEBUFFER_H__
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Google Inc.
 *
 * mapped_framebuffer.h - Frame buffer memory mapping support
 */

#include <stdint.h>
#include <vector>

#include <libcamera/base/class.h>
#include <libcamera/base/flags.h>
#include <libcamera/base/span.h>
#include <libcamera/framebuffer.h>


namespace libcamera {

class MappedBuffer
{
public:
	using Plane = Span<uint8_t>;

	~MappedBuffer();

	MappedBuffer(MappedBuffer &&other);
	MappedBuffer &operator=(MappedBuffer &&other);

	bool isValid() const { return error_ == 0; }
	int error() const { return error_; }
	const std::vector<Plane> &planes() const { return planes_; }

protected:
	MappedBuffer();

	int error_;
	std::vector<Plane> planes_;
	std::vector<Plane> maps_;

private:
	LIBCAMERA_DISABLE_COPY(MappedBuffer)
};

class MappedFrameBuffer : public MappedBuffer
{
public:
	enum class MapFlag {
		Read = 1 << 0,
		Write = 1 << 1,
		ReadWrite = Read | Write,
	};

	using MapFlags = Flags<MapFlag>;

	MappedFrameBuffer(const FrameBuffer *buffer, MapFlags flags);
};

LIBCAMERA_FLAGS_ENABLE_OPERATORS(MappedFrameBuffer::MapFlag)

} /* namespace libcamera */

#endif // __MAPPED_FRAMEBUFFER_H__

