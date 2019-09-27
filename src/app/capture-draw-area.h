#ifndef CAPTURE_DRAW_AREA_H
#define CAPTURE_DRAW_AREA_H

#include <gtkmm/drawingarea.h>
#include <gdkmm/pixbuf.h>

class CaptureDrawArea : public Gtk::DrawingArea
{
public:
	CaptureDrawArea();
	virtual ~CaptureDrawArea();

public:
	void setImageData(const void *buffer, const size_t len);
	void redraw();

protected:
	//Override default signal handler:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

private:	
	Glib::RefPtr<Gdk::Pixbuf> m_image;
};

#endif // CAPTURE_DRAW_AREA_H
