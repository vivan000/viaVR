#pragma once

class IVideoDecoder {
public:
	virtual ~IVideoDecoder (){}

	virtual int getNextVideoframe (char* buf, int size) = 0;
	virtual void seek (int timestamp) = 0;

	virtual int getWidth () = 0;
	virtual int getHeight () = 0;

	virtual int getSarWidth () = 0;
	virtual int getSarHeight () = 0;

	virtual int getFpsNumerator () = 0;
	virtual int getFpsDenominator () = 0;

	// see videoInfo.h for actual values
	virtual int getRange () = 0;
	virtual int getMatrix () = 0;
	virtual int getFourCC () = 0;
};
