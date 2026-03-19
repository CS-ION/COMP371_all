#include <string>
#include <GL/glew.h>

class Shader {
public:
    GLuint ID;

    Shader(const char* vertexPath, const char* fragmentPath);

    void use();
};