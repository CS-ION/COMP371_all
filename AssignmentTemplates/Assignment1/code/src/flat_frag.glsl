#version 330 core

flat in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos;

void main()
{
    vec3 ambient = vec3(0.1,0.05,0.05);

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1,0.5,0.5);

    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir),0.0),5.0);
    vec3 specular = spec * vec3(0.3);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result,1);
}