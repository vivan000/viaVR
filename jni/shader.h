#include <GLES3/gl3.h>

class shader {
public:
	shader (const char* vertexShaderSource, const char* fragmentShaderSource, ...);
	~shader ();

	GLuint loadProgram ();
private:
	GLuint loadShader (GLenum type, const char* shaderSource);

	GLuint vertexShader, fragmentShader;
	GLuint programObject;
};
