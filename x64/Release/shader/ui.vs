#version 460

layout(location = 0) in vec2 VertexPosition;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(VertexPosition, 0.0, 1.0);
}