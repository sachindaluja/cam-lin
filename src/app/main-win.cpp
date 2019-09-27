#include "main-win.h"
#include <iostream>
#include "v4l2.h"

extern V4L2 g_videocam;

MainWin::MainWin()
{
	set_title("CamToolbox");
	set_default_size(600, 400);

	add(m_area);
	m_area.show();
}

MainWin::~MainWin()
{
}

void MainWin::setImageData(const void *buffer, const size_t len)
{
	m_area.setImageData(buffer, len);
}

void MainWin::redrawImage()
{
	m_area.redraw();
}
