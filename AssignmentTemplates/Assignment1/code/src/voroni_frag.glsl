#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 tv0;
in vec3 tv1;
in vec3 tv2;

uniform vec3 lightPos;

out vec4 FragColor;

void main()
{
    float d0 = distance(FragPos, tv0);
    float d1 = distance(FragPos, tv1);
    float d2 = distance(FragPos, tv2);

    vec3 diffuseColor;

    if(d0 <= d1 && d0 <= d2)
        diffuseColor = vec3(1,0.5,0.5);
    else if(d1 <= d2)
        diffuseColor = vec3(0.5,1,0.5);
    else
        diffuseColor = vec3(0.5,0.5,1);

    vec3 ambient = vec3(0.1,0.05,0.05);

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * diffuseColor;

    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir),0.0),5.0);
    vec3 specular = spec * vec3(0.3,0.3,0.3);

    FragColor = vec4(ambient + diffuse + specular,1.0);
}