#include "capture-draw-area.h"
#include <cairomm/context.h>
#include <gdkmm/general.h>
#include <glibmm/fileutils.h>
#include <iostream>

CaptureDrawArea::CaptureDrawArea()
{
	if (m_image)
		set_size_request(m_image->get_width(), m_image->get_height());
}

CaptureDrawArea::~CaptureDrawArea()
{
}

void CaptureDrawArea::setImageData(const void *buffer, const size_t len)
{
	m_image = Gdk::Pixbuf::create_from_data	((const guint8*)buffer,
			Gdk::COLORSPACE_RGB,
			false,
			8,
			640,
			480,
			1920);
}

void CaptureDrawArea::redraw()
{
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win)
	{
		Gdk::Rectangle r(0, 0, get_allocation().get_width(),
				get_allocation().get_height());
		win->invalidate_rect(r, false);
	}
}

bool CaptureDrawArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	if (!m_image)
		return false;

	Gtk::Allocation allocation = get_allocation();
	const int width = allocation.get_width();
	const int height = allocation.get_height();

	// Draw the image in the middle of the drawing area, or (if the image is
	// larger than the drawing area) draw the middle part of the image.
	Gdk::Cairo::set_source_pixbuf(cr, m_image,
			(width - m_image->get_width())/2, (height - m_image->get_height())/2);

	cr->paint();

	return true;
}
