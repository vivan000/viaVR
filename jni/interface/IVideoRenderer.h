#pragma once
#include <android/native_window.h>
#include "interface/IVideoDecoder.h"

class IVideoRenderer {
public:
	virtual ~IVideoRenderer (){}

	virtual bool addVideoDecoder (IVideoDecoder* video) = 0;
	virtual bool addWindow (ANativeWindow* window) = 0;
	virtual bool init () = 0;

	virtual void seek (int timestamp) = 0;
	virtual void play (int timecode) = 0;
	virtual void pause () = 0;

	virtual const char* getName () = 0;
	virtual int getMajorVersion () = 0;
	virtual int getMinorVersion () = 0;
};
