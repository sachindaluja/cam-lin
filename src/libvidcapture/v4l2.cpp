/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * <Copyright statement goes here>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <libv4l2.h>
#include <v4l2.h>

#ifdef NDEBUG
	#define ASSERT_ ;
#else
	#define ASSERT_(expression) \
		if(!(expression)) { fprintf (stderr, "Assertion failed at %s, line %d.\n", __FILE__,__LINE__); }
#endif

V4L2::V4L2()
	: m_fd(-1)
	, m_capability()
	, m_srcFormat()
	, m_destFormat()
	, m_buffers(NULL)
	, m_numBuffers(0)
	, m_frmSize()
{}

V4L2::~V4L2()
{
	if(m_fd >= 0)
		close();
}

bool V4L2::open(const std::string& device)
{
	if(m_fd >= 0){
		ASSERT_(false);	//Repeat call to open()
		return true;
	}

	m_capability = v4l2_capability();

	struct stat st;

	if (-1 == stat(device.c_str(), &st)) {
		fprintf(stderr, "Cannot find '%s': %d, %s",
			device.c_str(), errno, strerror(errno));
		return false;
	}

	if (!S_ISCHR(st.st_mode)) {
		error("Specified file is not a device");
		return false;
	}

	/*m_fd = v4l2_open(device.c_str(), O_RDWR | O_NONBLOCK);
	if (m_fd < 0) {
		error("Cannot open device");
		return false;
	}*/

	m_fd = ::open(device.c_str(), O_RDWR | O_NONBLOCK);
	if (m_fd < 0) {
		error("Cannot open device");
		return false;
	}
	
	if (!queryCap(m_capability)) {
		v4l2_close(m_fd);
		m_fd = -1;
		error(/*device + */"Device is not a V4L2 device");
		return false;
	}

	if (v4l2_fd_open(m_fd, V4L2_ENABLE_ENUM_FMT_EMULATION) < 0) {
		error("Cannot use libv4l2 wrapper for device");
		return false;
	}

	if (!(m_capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		v4l2_close(m_fd);
		m_fd = -1;
		error("Device is not a video capture device");
		return false;
	}

	const IoMethod io = methodMmap;	//Will become a class member.
	switch (io) {
	case methodRead:
		if (!(m_capability.capabilities & V4L2_CAP_READWRITE)) {
			error("Device does not support read i/o");
			v4l2_close(m_fd);
			m_fd = -1;
			return false;
		}
		break;
	case methodMmap:
	case methodUser:
		if (!(m_capability.capabilities & V4L2_CAP_STREAMING)) {
			error("Device does not support streaming i/o");
			v4l2_close(m_fd);
			m_fd = -1;
			return false;
		}
		break;
	default:
		ASSERT_(false);
		v4l2_close(m_fd);
		m_fd = -1;
		return false;
	}

	//TODO: Test if supported formats include V4L2_PIX_FMT_RGB24
	
	//Set largest available framesize as default
	m_frmSize = FrameSize();

	v4l2_frmsizeenum frmEnum;
	bool enumInit = true;
	while (enumFramesizes(frmEnum, V4L2_PIX_FMT_RGB24, enumInit)) {
		enumInit = false;
		//Only considering discrete framesizes at this time
		if(V4L2_FRMSIZE_TYPE_DISCRETE == frmEnum.type) {
			if(frmEnum.discrete.width > m_frmSize.width) {
			       m_frmSize.width = frmEnum.discrete.width;
			       m_frmSize.height = frmEnum.discrete.height;
			}
		}
	}

	return true;
}

void V4L2::close()
{
	//close() must reset m_fd regardless of failure.
	
	if(m_fd < 0)
		return;

	if(m_buffers) {
		const IoMethod io = methodMmap;	//Will become a class member.
		switch (io) {
			case methodRead:
				free(m_buffers[0].start);
				break;

			case methodMmap:
				for (unsigned int i = 0; i < m_numBuffers; ++i){
					if (-1 == v4l2_munmap(m_buffers[i].start, m_buffers[i].length))
						error("munmap() failed\n");
				}
				break;

			case methodUser:
				for (unsigned int i = 0; i < m_numBuffers; ++i)
					free(m_buffers[i].start);
				break;
		}

		delete m_buffers;
		m_buffers = NULL;
		m_numBuffers = 0;
	}

	if (-1 == v4l2_close(m_fd))
		error("close() failed");

	m_fd = -1;
}

void V4L2::error(const char* text)
{
	if (text)
		fprintf(stderr, "%s\n", text);
}

int V4L2::ioctl(unsigned int cmd, void *arg)
{
	return v4l2_ioctl(m_fd, cmd, arg);
}

bool V4L2::queryCap(v4l2_capability &cap)
{
	memset(&cap, 0, sizeof(cap));
	return ioctl(VIDIOC_QUERYCAP, &cap) >= 0;
}

bool V4L2::enumInput(v4l2_input &in, bool init, int index /*= 0*/)
{
	if (init) {
		memset(&in, 0, sizeof(in));
		in.index = index;
	} else {
		in.index++;
	}
	
	return ioctl(VIDIOC_ENUMINPUT, &in) >= 0;
}

bool V4L2::enumFramesizes(v4l2_frmsizeenum &frm, const __u32 pixfmt, bool init, const int index /*= 0*/)
{
	if (init) {
		memset(&frm, 0, sizeof(frm));
		frm.pixel_format = pixfmt;
		frm.index = index;
	} else {
		frm.index++;
	}

	return ioctl(VIDIOC_ENUM_FRAMESIZES, &frm) >= 0;
}

bool V4L2::initMmap()
{
	v4l2_requestbuffers req = {0};

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == ioctl(VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			error("Device does not support memory mapping");
			return false;
		} else {
			return false;
		}
	}

	if (req.count < 2) {
		error("Insufficient buffer memory");
		return false;
	}

	m_buffers = new Buffer[req.count];

	if (!m_buffers) {
		error("Out of memory");
		return false;
	}

	for (m_numBuffers = 0; m_numBuffers < req.count; ++m_numBuffers) {
		v4l2_buffer buf = {0};

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = m_numBuffers;

		if (-1 == ioctl(VIDIOC_QUERYBUF, &buf))
			return false;

		m_buffers[m_numBuffers].length = buf.length;
		m_buffers[m_numBuffers].start =
			v4l2_mmap(NULL /* start anywhere */,
				buf.length,
				PROT_READ | PROT_WRITE /* required */,
				MAP_SHARED /* recommended */,
				m_fd, buf.m.offset);

		if (MAP_FAILED == m_buffers[m_numBuffers].start)
			return false;
	}

	return true;
}

bool V4L2::setupFormat()
{
	m_srcFormat = v4l2_format();
	m_destFormat = v4l2_format();

	m_srcFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(VIDIOC_G_FMT, &m_srcFormat) < 0)
		return false;

	//Only support V4L2_PIX_FMT_RGB24.
	if(m_srcFormat.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
		v4l2_fract interval;
		const bool intervalOk = getInterval(interval);

		m_srcFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
		if (m_frmSize.width > 0) { //If frame size valid
			m_srcFormat.fmt.pix.width = m_frmSize.width;
			m_srcFormat.fmt.pix.height = m_frmSize.height;
		}

		const int nRes = ioctl(VIDIOC_S_FMT, &m_srcFormat);
		if(nRes < 0)
			return false;

		if(ioctl(VIDIOC_G_FMT, &m_srcFormat) < 0)
			return false;

		if (intervalOk)
		        setInterval(interval);
	}

	m_destFormat = m_srcFormat;

	return true;
}

bool V4L2::getInterval(v4l2_fract &interval)
{
	v4l2_streamparm parm = {};
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (ioctl(VIDIOC_G_PARM, &parm) < 0)
		return false;

	if (!(parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME))
		return false;

	interval = parm.parm.capture.timeperframe;

	return true;
}

bool V4L2::setInterval(const v4l2_fract &interval)
{
	v4l2_streamparm parm = {};
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (ioctl(VIDIOC_G_PARM, &parm) < 0)
		return false;

	if (!(parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME))
		return false;

	parm.parm.capture.timeperframe = interval;

	return (ioctl(VIDIOC_S_PARM, &parm) >= 0);
}

bool V4L2::startStreaming()
{
	if(!setupFormat()) {
		error("Error setting up format");
		return false;
	}

	//Using memory mapped io.
	//
	if(!initMmap()) {
		error("Error initializing memory mapped buffers");
		return false;
	}

	unsigned int i;
	v4l2_buf_type type;

	for (i = 0; i < m_numBuffers; ++i) {
		v4l2_buffer buf = {0};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == ioctl(VIDIOC_QBUF, &buf)){
			error("ioctl VIDIOC_QBUF failed");
			return false;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(VIDIOC_STREAMON, &type)){
		error("ioctl VIDIOC_STREAMON failed");
		return false;
	}

	return true;
}

bool V4L2::stopStreaming()
{
	const IoMethod io = methodMmap;	//Will become a class member.
	switch (io) {
	case methodRead:
		/* Nothing to do. */
		break;

	case methodMmap:
	case methodUser:
		{	
			v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == ioctl(VIDIOC_STREAMOFF, &type)){
				error("ioctl VIDIOC_STREAMOFF failed");
				return false;
			}
			
			if (methodMmap == io) {
				for (unsigned int i = 0; i < m_numBuffers; ++i){
					if (-1 == v4l2_munmap(m_buffers[i].start, m_buffers[i].length))
					error("munmap() failed\n");
				}
			}
			else {
				for (unsigned int i = 0; i < m_numBuffers; ++i)
					free(m_buffers[i].start);
			}

			break;
		}
	default:
		ASSERT_(false);
		return false;
	}

	delete m_buffers;
	m_buffers = NULL;
	m_numBuffers = 0;

	return true;
}

bool V4L2::readStreamFrame(void **buffer, size_t *len)
{
	bool read = false;
	*buffer = NULL;
	*len = 0;

	while(true) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_fd, &fds);

		/* Timeout. */
		timeval tv = {0};
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		const int r = select(m_fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;
			error("select failed");
			break;
		}

		if (0 == r) {
			error("select timeout");
			break;
		}

		v4l2_buffer buf = {0};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		bool ioctlFailed = false;
		if (-1 == ioctl(VIDIOC_DQBUF, &buf)) {
			if(EAGAIN == errno)
				continue;
			else if(EIO == errno)/* Could ignore EIO. */
				ioctlFailed = true;
			else
				ioctlFailed = true;
		}
	
		if(ioctlFailed){
			error("ioctl VIDIOC_DQBUF failed");
			break;
		}

		ASSERT_(buf.index < m_numBuffers);

		if (-1 == ioctl(VIDIOC_QBUF, &buf)){
			error("ioctl VIDIOC_DQBUF failed");
			break;
		}

		read = true;
		*buffer = m_buffers[buf.index].start;
		*len = buf.bytesused;

		break;
	}

	return read;
}
