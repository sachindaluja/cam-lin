#include "main-win.h"
#include <gtkmm/main.h>
#include <glibmm/main.h>
#include "v4l2.h"

#ifdef NDEBUG
	#define ASSERT_ ;
#else
	#define ASSERT_(expression) \
		if(!(expression)) { fprintf (stderr, "Assertion failed at %s, line %d.\n", __FILE__,__LINE__); }
#endif

MainWin *g_mainWin = NULL;
V4L2 g_videocam;

bool OnV4L2StreamDataAvailable(Glib::IOCondition io_condition)
{
	if(!g_mainWin) {
		ASSERT_(false);
		return false;
	}

	if ((io_condition & Glib::IO_IN) == 0) {
		ASSERT_(false);
	}
	else {
		void *buffer = NULL;
		size_t len = 0;

		if(g_videocam.readStreamFrame(&buffer, &len)) {
			g_mainWin->setImageData(buffer, len);
			g_mainWin->redrawImage();
		}
		else
			ASSERT_(false);
	}

	return true;
}

int main (int argc, char *argv[])
{
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "org.sachin.camtoolbox");

	std::string device;
	if(argc > 1)
		device = argv[1];
	else
		device = "/dev/video0";
	
	if(!g_videocam.open(device)) {
		ASSERT_(false);
		return EXIT_FAILURE;
	}

	if(!g_videocam.startStreaming()) {
		ASSERT_(false);
		return EXIT_FAILURE;
	}

	void *buffer = NULL;
	size_t len = 0;

	if(!g_videocam.readStreamFrame(&buffer, &len))
		ASSERT_(false);

	g_mainWin = new MainWin();

	const V4L2::FrameSize frmSize = g_videocam.getFrameSize();
	ASSERT_(frmSize.width > 0);
	if(frmSize.width > 0)
		g_mainWin->set_default_size(frmSize.width, frmSize.height);

	g_mainWin->setImageData(buffer, len);

	Glib::signal_io().connect(sigc::ptr_fun(OnV4L2StreamDataAvailable), g_videocam.getFileDescriptor(), Glib::IO_IN);

	const int nRet = app->run(*g_mainWin);

	delete g_mainWin;
	g_mainWin = NULL;

	g_videocam.stopStreaming();

	g_videocam.close();

	return nRet;
}
