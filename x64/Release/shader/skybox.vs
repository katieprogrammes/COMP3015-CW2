#version 460 core

layout(location = 0) in vec3 VertexPosition;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = VertexPosition;

    vec4 pos = projection * mat4(mat3(view)) * vec4(VertexPosition, 1.0);
    gl_Position = pos.xyww;
}