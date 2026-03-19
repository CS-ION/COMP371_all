#define GLEW_STATIC 1

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Shader.h"

class A2solution {
public:
    A2solution();
    void run(std::string filename);

private:
    void initGL();
    void loadFile(const std::string& filename);
    void computeNormals();
    void setupBuffers();
    void renderLoop();

    // Picking
    void performPicking();
    void projectVertices();
    bool computeBarycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c,
                            float &u, float &v, float &w);

    // Input
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

    // Window
    GLFWwindow* window;

    GLuint VAO;
    GLuint VBO_pos, VBO_nrm;

    // Shader
    Shader* shader;

    // Uniform locations
    GLint mvLoc, projLoc, lightLoc;

    // Geometry
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    std::vector<glm::vec3> screenVertices;

    // Matrices
    glm::mat4 modelView;
    glm::mat4 projection;

    int width, height;

    size_t vertexCount;

    // Mouse
    double mouseX, mouseY;
    bool mousePressed;
};