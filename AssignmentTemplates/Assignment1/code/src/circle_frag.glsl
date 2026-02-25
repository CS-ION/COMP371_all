#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 tv0;
in vec3 tv1;
in vec3 tv2;

uniform vec3 lightPos;

out vec4 FragColor;

float pointLineDistance(vec3 p, vec3 a, vec3 b)
{
    vec3 ab = b - a;
    vec3 ap = p - a;
    return length(ap - dot(ap, ab) / dot(ab, ab) * ab);
}

void main()
{
    // --- Compute incenter ---
    float a = length(tv1 - tv2);
    float b = length(tv0 - tv2);
    float c = length(tv0 - tv1);

    vec3 incenter = (a * tv0 + b * tv1 + c * tv2) / (a + b + c);

    // --- Compute inradius ---
    float r = pointLineDistance(incenter, tv0, tv1);

    float d = distance(FragPos, incenter);

    vec3 diffuseColor;
    vec3 ambientColor;
    vec3 specularColor = vec3(0.3,0.3,0.3);

    if(d <= r)
    {
        diffuseColor = vec3(0.5,0.5,1);
        ambientColor = vec3(0.05,0.05,0.1);
        specularColor = vec3(0);
    }
    else
    {
        diffuseColor = vec3(1,0.5,0.5);
        ambientColor = vec3(0.1,0.05,0.05);
    }

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir),0.0);
    vec3 diffuse = diff * diffuseColor;

    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir),0.0),5.0);
    vec3 specular = spec * specularColor;

    FragColor = vec4(ambientColor + diffuse + specular,1.0);
}