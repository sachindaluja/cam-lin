/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * <Copyright statement goes here>
 * 
 */

#ifndef _V4L2_H_
#define _V4L2_H_

#include <string>
#include <linux/videodev2.h>

class V4L2
{
	//TODO: Rename to V4L2Capture
public:
	enum IoMethod 
	{
		methodRead,
		methodMmap,
		methodUser
	};

public:
	struct FrameSize
	{
		__u32 width;
		__u32 height;
	};


private:
	struct Buffer 
	{
		void *start;
		size_t length;
	};

public:
	V4L2();
	~V4L2();

public:
	bool open(const std::string& device);
	void close();
	bool startStreaming();
	bool stopStreaming();
	bool readStreamFrame(void **buffer, size_t *len);
	int getFileDescriptor() const {return m_fd;}
	FrameSize getFrameSize() const {return m_frmSize;}

private:
	void error(const char *text);
	int ioctl(unsigned int cmd, void *arg);
	bool queryCap(v4l2_capability &cap);
	bool enumInput(v4l2_input &in, bool init, int index = 0);
	bool enumFramesizes(v4l2_frmsizeenum &frm, const __u32 pixfmt, bool init, const int index = 0);
	bool initMmap();
	bool setupFormat();
	bool getInterval(v4l2_fract &interval);
	bool setInterval(const v4l2_fract &interval);
	
private:
	int m_fd;
	v4l2_capability m_capability;
	v4l2_format m_srcFormat;
	v4l2_format m_destFormat;
	FrameSize m_frmSize;
	Buffer *m_buffers;
	unsigned int m_numBuffers;
};

#endif // _V4L2_H_
