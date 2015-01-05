#include <GLES2/gl2.h>

class shader {
public:
	shader (const char* vertexShaderSource, const char* fragmentShaderSource, bool precision);
	~shader ();

	void addAtrib (const char* attribName, const GLuint attribLoaction);

	GLuint loadProgram ();
private:
	const bool precision;
	GLuint loadShader (GLenum type, const char* shaderSource);

	GLuint vertexShader, fragmentShader;
	GLuint programObject;
};
