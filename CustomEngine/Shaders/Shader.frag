#version 330 core

in  vec3 Normal;
in  vec3 FragPos;
out vec4 FragColor;

uniform vec3 objectColor;
uniform bool bSelected;

void main()
{
    vec3 lightDir = normalize(vec3(1.0, 2.0, 1.0));
    float diff    = max(dot(normalize(Normal), lightDir), 0.0);

    vec3 ambient  = 0.25 * objectColor;
    vec3 diffuse  = diff * objectColor;
    vec3 col      = ambient + diffuse;

    if (bSelected)
        col = mix(col, vec3(1.0, 0.8, 0.2), 0.35);

    FragColor = vec4(col, 1.0);
}
