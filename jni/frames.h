#include "videoInfo.h"
#include <algorithm>

// frame for upload - CPU
class frameCPU {
public:
	char* plane;
	int timecode;

	frameCPU (videoInfo* f);
	~frameCPU ();
	void swap (frameCPU& that);
};

// frame for upload, 1-3 planes, 1-4 channels, byte
class frameGPUu {
public:
	GLuint* plane;
	int timecode;

	frameGPUu (videoInfo* f);
	~frameGPUu ();
	void swap (frameGPUu& that);

private:
	int numberOfPlanes;
};

// internal frame, RGB (half-)float
class frameGPUi {
public:
	GLuint plane;
	int timecode;

	frameGPUi (int w, int h, pFormat type);
	~frameGPUi ();
};

// output frame, RGB byte
class frameGPUo {
public:
	GLuint plane;
	int timecode;

	frameGPUo (videoInfo* f);
	~frameGPUo ();
	void swap (frameGPUo& that);
};
