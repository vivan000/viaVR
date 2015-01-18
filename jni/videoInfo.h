#include <GLES3/gl3.h>

#define FOURCC(a, b, c, d) ((a) | (b << 8) | (c << 16) | (d << 24))

enum class pFormat {
	NONE,

	// internal
	INT10,		// normalized 10-bit integer rgb (RGB10_A2)
	HALF_FLOAT,	// half float rgb (RGBA16F)
	FLOAT,		// full float rgb (RGBA32F)

	// 8-bit planar
	P008 = FOURCC ('I', '4', '2', '0'), //  8 bit 4:2:0
	P208 = FOURCC ('Y', '4', '2', 'B'), //  8 bit 4:2:2
	P408 = FOURCC ('4', '4', '4', 'P'), //  8 bit 4:4:4

	// 10-bit planar
	P010 = FOURCC ('Y', '3',  11,  10), // 10 bit 4:2:0
	P210 = FOURCC ('Y', '3',  10,  10), // 10 bit 4:2:2
	P410 = FOURCC ('Y', '3',   0,  10), // 10 bit 4:4:4

	// 16-bit planar
	P016 = FOURCC ('Y', '3',  11,  16), // 16 bit 4:2:0
	P216 = FOURCC ('Y', '3',  10,  16), // 16 bit 4:2:2
	P416 = FOURCC ('Y', '3',   0,  16), // 16 bit 4:4:4

	// other
	NV12 = FOURCC ('N', 'V', '1', '2'), // 8 bit 4:2:0, semiplanar (U+V)
	NV21 = FOURCC ('N', 'V', '2', '1'), // 8 bit 4:2:0, semiplanar (V+U)
	RGBA = FOURCC ('R', 'G', 'B', 'A'), // 8 bit 4:4:4, packed A(8) R(8) G(8) B(8)
};

enum class pRange {
	UNKNOWN = 0,
	TV = 1,
	PC = 2,
};

enum class pMatrix {
	UNKNOWN = 0,
	BT601 = 1,
	BT709 = 2,
};

class videoInfo {
public:
	videoInfo (int videoWidth, int videoHeight, int videoFourCC, int videoRange, int videoMatrix);
	~videoInfo ();

	bool init;

	pFormat fourCC;
	pRange range;
	pMatrix matrix;
	int width, height;

	int Bpp;
	bool halfWidth, halfHeight;

	int planes;
	GLenum internalLumaFormat, internalChromaFormat;
	GLenum lumaFormat, chromaFormat;
	GLenum lumaType, chromaType;

	int chromaWidth, chromaHeight;
	int offset1, offset2;

	int targetX, targetY;
	int targetWidth, targetHeight;

	GLfloat colorConversion[9];
	GLfloat colorOffset[3];

	bool hwChroma, hwScale;
	bool hwChromaLinear, hwScaleLinear;

private:
	double rangeY, rangeC, luma0, Kb, Kr;
};
