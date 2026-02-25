#define GLEW_STATIC 1

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Shader.h"

class A1solution {
public:
    A1solution();
    void run(std::string filename);

private:
    void initGL();
    void loadFile(const std::string& filename);
    void computeNormals();
    void setupBuffers();
    void renderLoop();
    void processInput();

    // Window
    GLFWwindow* window;

    // Buffers
    GLuint VAO, VBO, NBO, EBO;

    // Shaders
    Shader* shaders[4];
    int currentMode;
    bool wireframe;

    // Uniform locations (cached)
    GLint mvLoc;
    GLint projLoc;
    GLint lightLoc;

    // Geometry
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    // Matrices
    glm::mat4 modelView;
    glm::mat4 projection;

    int width;
    int height;
};