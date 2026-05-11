#version 460 core

layout(location = 0) in vec2 VertexPosition;
layout(location = 1) in vec2 VertexTexCoord;

out vec2 TexCoord;

void main()
{
    TexCoord = VertexTexCoord;
    gl_Position = vec4(VertexPosition, 0.0, 1.0);
}