#ifndef _MAIN_WIN_H_
#define _MAIN_WIN_H_

#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include "capture-draw-area.h"

class MainWin : public Gtk::Window
{
public:
	MainWin();
	virtual ~MainWin();

public:
	void setImageData(const void *buffer, const size_t len);
	void redrawImage();

private:
	CaptureDrawArea m_area;
};

#endif // _MAIN_WIN_H_
