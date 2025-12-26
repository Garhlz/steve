#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

// Cubemap 采样器
uniform samplerCube skybox;
uniform float intensity;
void main()
{
    vec4 texColor = texture(skybox, TexCoords);
    // 颜色 * 亮度系数
    FragColor = texColor * intensity;
}