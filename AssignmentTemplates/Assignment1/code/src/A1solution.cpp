#include "A1solution.h"
#include <fstream>
#include <iostream>

A1solution::A1solution() { }

void A1solution::run(std::string filename)
{
    loadFile(filename);
    initGL();
    computeNormals();
    setupBuffers();
    renderLoop();
}

void A1solution::loadFile(const std::string& filename)
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

void A1solution::computeNormals()
{
    normals.resize(vertices.size(), glm::vec3(0));

    for(size_t i=0;i<indices.size(); i+=3)
    {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i+1];
        unsigned int i2 = indices[i+2];

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

void A1solution::initGL()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "A1", NULL, NULL);
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

    shaders[0] = new Shader("../src/phong_vert.glsl", "../src/phong_frag.glsl");
    shaders[1] = new Shader("../src/flat_vert.glsl", "../src/flat_frag.glsl");
    shaders[2] = new Shader("../src/circle_vert.glsl", "../src/circle_frag.glsl");
    shaders[3] = new Shader("../src/voroni_vert.glsl", "../src/voroni_frag.glsl");

    currentMode = 0;
    wireframe = false;

    // Cache uniform locations once
    shaders[currentMode]->use();
    mvLoc = glGetUniformLocation(shaders[currentMode]->ID, "modelView");
    projLoc = glGetUniformLocation(shaders[currentMode]->ID, "projection");
    lightLoc = glGetUniformLocation(shaders[currentMode]->ID, "lightPos");
}

void A1solution::setupBuffers()
{
    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);

    // Position buffer
    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size()*sizeof(glm::vec3),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glEnableVertexAttribArray(0);

    // Normal buffer
    glGenBuffers(1,&NBO);
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(GL_ARRAY_BUFFER,
                 normals.size()*sizeof(glm::vec3),
                 normals.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glEnableVertexAttribArray(1);

    // --- Triangle-local vertex buffers ---
    std::vector<glm::vec3> triV0(vertices.size());
    std::vector<glm::vec3> triV1(vertices.size());
    std::vector<glm::vec3> triV2(vertices.size());

    for(size_t i = 0; i < indices.size(); i += 3)
    {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i+1];
        unsigned int i2 = indices[i+2];

        glm::vec3 v0 = vertices[i0];
        glm::vec3 v1 = vertices[i1];
        glm::vec3 v2 = vertices[i2];

        triV0[i0] = v0; triV1[i0] = v1; triV2[i0] = v2;
        triV0[i1] = v0; triV1[i1] = v1; triV2[i1] = v2;
        triV0[i2] = v0; triV1[i2] = v1; triV2[i2] = v2;
    }

    GLuint PBO[3];
    glGenBuffers(3, PBO);

    // Attribute 2
    glBindBuffer(GL_ARRAY_BUFFER, PBO[0]);
    glBufferData(GL_ARRAY_BUFFER,
                 triV0.size()*sizeof(glm::vec3),
                 triV0.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glEnableVertexAttribArray(2);

    // Attribute 3
    glBindBuffer(GL_ARRAY_BUFFER, PBO[1]);
    glBufferData(GL_ARRAY_BUFFER,
                 triV1.size()*sizeof(glm::vec3),
                 triV1.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glEnableVertexAttribArray(3);

    // Attribute 4
    glBindBuffer(GL_ARRAY_BUFFER, PBO[2]);
    glBufferData(GL_ARRAY_BUFFER,
                 triV2.size()*sizeof(glm::vec3),
                 triV2.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(4,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glEnableVertexAttribArray(4);

    // Index buffer
    glGenBuffers(1,&EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size()*sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);
}

void A1solution::renderLoop()
{
    while(!glfwWindowShouldClose(window))
    {
        processInput();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaders[currentMode]->use();

        glUniformMatrix4fv(mvLoc,1,GL_FALSE,&modelView[0][0]);
        glUniformMatrix4fv(projLoc,1,GL_FALSE,&projection[0][0]);

        //glm::vec3 lightPos(3.0f, 3.0f, 3.0f);
        glm::vec3 lightPos(0,0,0);
        glUniform3fv(lightLoc,1,&lightPos[0]);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(indices.size()),
                       GL_UNSIGNED_INT,
                       0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}

void A1solution::processInput()
{
    static bool sPressed = false;
    static bool wPressed = false;

    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        if(!sPressed)
        {
            currentMode = (currentMode + 1) % 4;
            sPressed = true;
        }
    }
    else sPressed = false;

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        if(!wPressed)
        {
            wireframe = !wireframe;
            wPressed = true;

            if(wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
    else wPressed = false;

    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}