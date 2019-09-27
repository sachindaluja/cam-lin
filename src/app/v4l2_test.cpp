#include <stdio.h>
#include <stdlib.h>
#include "v4l2.h"

#ifdef NDEBUG
	#define ASSERT_ ;
#else
	#define ASSERT_(expression) \
		if(!(expression)) { fprintf (stderr, "Assertion failed at %s, line %d.\n", __FILE__,__LINE__); }
#endif

int main(int argc, char **argv)
{
	if(argc > 2){
		ASSERT_(false);
		return EXIT_FAILURE;
	}

	std::string device;
	if(argc > 1)
		device = argv[1];
	else
		device = "/dev/video0";

	V4L2 videocam;

	if(!videocam.open(device)) {
		ASSERT_(false);
		return EXIT_FAILURE;
	}

	if(!videocam.startStreaming()) {
		ASSERT_(false);
		return EXIT_FAILURE;
	}

	void *buffer = NULL;
	size_t len = 0;

	if(videocam.readStreamFrame(&buffer, &len))
		fwrite(buffer, len, 1, stdout);
	else
		ASSERT_(false);

	fflush(stdout);

	videocam.stopStreaming();

	videocam.close();

	return EXIT_SUCCESS;
}
