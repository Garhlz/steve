#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    // 技巧：让天空盒的深度永远是 1.0 (最大深度)
    gl_Position = pos.xyww;
}