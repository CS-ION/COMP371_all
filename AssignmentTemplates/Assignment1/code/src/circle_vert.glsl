#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 v0;
layout(location = 3) in vec3 v1;
layout(location = 4) in vec3 v2;

uniform mat4 modelView;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

out vec3 tv0;
out vec3 tv1;
out vec3 tv2;

void main()
{
    vec4 viewPos = modelView * vec4(aPos, 1.0);
    FragPos = viewPos.xyz;

    Normal = mat3(transpose(inverse(modelView))) * aNormal;

    tv0 = (modelView * vec4(v0, 1.0)).xyz;
    tv1 = (modelView * vec4(v1, 1.0)).xyz;
    tv2 = (modelView * vec4(v2, 1.0)).xyz;

    gl_Position = projection * viewPos;
}