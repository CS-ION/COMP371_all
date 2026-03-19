#include "A2solution.h"
#include <fstream>
#include <iostream>
#include <cmath>

A2solution::A2solution()
{
    mouseX = mouseY = 0;
    mousePressed = false;
}

void A2solution::run(std::string filename)
{
    loadFile(filename);
    initGL();
    computeNormals();
    setupBuffers();
    renderLoop();
}

void A2solution::loadFile(const std::string& filename)
{
    std::ifstream file(filename);

    
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            file >> modelView[i][j]; 

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            file >> projection[i][j]; 


    file >> width >> height;

    int N;
    file >> N;
    vertices.resize(N);

    for(int i=0;i<N;i++)
        file >> vertices[i].x >> vertices[i].y >> vertices[i].z;

    int M;
    file >> M;
    indices.resize(3*M);

    for(int i=0;i<3*M;i++)
        file >> indices[i];
}

void A2solution::computeNormals()
{
    normals.resize(vertices.size(), glm::vec3(0));

    for(size_t i=0;i<indices.size(); i+=3)
    {
        int i0 = indices[i];
        int i1 = indices[i+1];
        int i2 = indices[i+2];

        glm::vec3 v0 = vertices[i0];
        glm::vec3 v1 = vertices[i1];
        glm::vec3 v2 = vertices[i2];

        glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        normals[i0] += n;
        normals[i1] += n;
        normals[i2] += n;
    }

    for(size_t i=0;i<normals.size();i++)
        normals[i] = glm::normalize(normals[i]);
}

void A2solution::initGL()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "A2", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW\n";
        exit(-1);
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0,0,width,height);

    shader = new Shader("../src/phong_vert.glsl", "../src/phong_frag.glsl");

    shader->use();
    mvLoc   = glGetUniformLocation(shader->ID, "modelView");
    projLoc = glGetUniformLocation(shader->ID, "projection");
    lightLoc= glGetUniformLocation(shader->ID, "lightPos");

    // Mouse callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
}

void A2solution::setupBuffers()
{
    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> nrm;

    for(size_t i=0;i<indices.size(); i+=3)
    {
        int i0 = indices[i];
        int i1 = indices[i+1];
        int i2 = indices[i+2];

        pos.push_back(vertices[i0]);
        pos.push_back(vertices[i1]);
        pos.push_back(vertices[i2]);

        nrm.push_back(normals[i0]);
        nrm.push_back(normals[i1]);
        nrm.push_back(normals[i2]);
    }

    vertexCount = pos.size();

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO_pos);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
    glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(glm::vec3), pos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &VBO_nrm);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_nrm);
    glBufferData(GL_ARRAY_BUFFER, nrm.size()*sizeof(glm::vec3), nrm.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(1);
}

void A2solution::renderLoop()
{
    while(!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();

        glUniformMatrix4fv(mvLoc,1,GL_FALSE,&modelView[0][0]);
        glUniformMatrix4fv(projLoc,1,GL_FALSE,&projection[0][0]);

        glm::vec3 lightPos(0,0,0);
        glUniform3fv(lightLoc,1,&lightPos[0]);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}

void A2solution::projectVertices()
{
    screenVertices.resize(vertices.size());

    for(size_t i=0;i<vertices.size();i++)
    {
        glm::vec4 clip = projection * modelView * glm::vec4(vertices[i],1.0f);
        glm::vec3 ndc = glm::vec3(clip) / clip.w;

        float x = (ndc.x + 1.0f)*0.5f*width;
        float y = (ndc.y + 1.0f)*0.5f*height;

        screenVertices[i] = glm::vec3(x,y,ndc.z);
    }
}

bool A2solution::computeBarycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c,
                                    float &u, float &v, float &w)
{
    glm::vec2 v0 = b - a;
    glm::vec2 v1 = c - a;
    glm::vec2 v2 = p - a;

    float d00 = glm::dot(v0,v0);
    float d01 = glm::dot(v0,v1);
    float d11 = glm::dot(v1,v1);
    float d20 = glm::dot(v2,v0);
    float d21 = glm::dot(v2,v1);

    float denom = d00*d11 - d01*d01;
    if(fabs(denom) < 1e-6) return false;

    v = (d11*d20 - d01*d21)/denom;
    w = (d00*d21 - d01*d20)/denom;
    u = 1.0f - v - w;

    return (u>=0 && v>=0 && w>=0);
}

void A2solution::performPicking()
{
    projectVertices();

    float bestZ = 1e9;
    int bestTri = -1;
    float bu,bv,bw;
    glm::vec3 bestPoint;

    glm::vec2 mouse(mouseX, height - mouseY);

    for(size_t i=0;i<indices.size();i+=3)
    {
        int i0 = indices[i];
        int i1 = indices[i+1];
        int i2 = indices[i+2];

        glm::vec3 s0 = screenVertices[i0];
        glm::vec3 s1 = screenVertices[i1];
        glm::vec3 s2 = screenVertices[i2];

        float u,v,w;
        if(!computeBarycentric(mouse,
                               glm::vec2(s0),
                               glm::vec2(s1),
                               glm::vec2(s2),
                               u,v,w))
            continue;

        float z = u*s0.z + v*s1.z + w*s2.z;

        if(z < bestZ)
        {
            bestZ = z;
            bestTri = i/3;
            bu=u; bv=v; bw=w;

            glm::vec3 v0 = vertices[i0];
            glm::vec3 v1 = vertices[i1];
            glm::vec3 v2 = vertices[i2];

            bestPoint = u*v0 + v*v1 + w*v2;
        }
    }

    if(bestTri != -1)
    {
        std::cout << "Triangle Index: " << bestTri << "; "
                  << "Barycentric Cordinates: " << "(" << bu << ", " << bv << ", " << bw << "); "
                  << "Euclidean Cordinates: " << "(" << bestPoint.x << ", " << bestPoint.y << ", " << bestPoint.z << "); "
                  << std::endl;
    }
}

void A2solution::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    A2solution* self = static_cast<A2solution*>(glfwGetWindowUserPointer(window));

    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        self->mousePressed = true;
        self->performPicking();
    }

    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        self->mousePressed = false;
    }
}

void A2solution::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    A2solution* self = static_cast<A2solution*>(glfwGetWindowUserPointer(window));

    self->mouseX = xpos;
    self->mouseY = ypos;

    if(self->mousePressed)
        self->performPicking();
}