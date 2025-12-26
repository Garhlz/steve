#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

// Cubemap 采样器
uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, TexCoords);
}