#include <stdlib.h>

class videoDecoder {
public:
	videoDecoder ();
	~videoDecoder ();

	int getNextVideoframe (char* buf, int size);

	int getWidth ();
	int getHeight ();

	int getSarWidth ();
	int getSarHeight ();

	int getFpsNumerator ();
	int getFpsDenominator ();

	int getRange ();
	int getMatrix ();
	int getFourCC ();

	int getCurrentTimecode ();

private:
	void* infoptr;

	int width, height;
	int sarWidth, sarHeight;
	int fpsNumerator, fpsDenominator;
	int range, matrix, fourCC;
	int bpp;

	int decoderCount;

	unsigned char* dataR;
	unsigned char* dataG;
	unsigned char* dataB;
};
