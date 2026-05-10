#version 460

layout(location = 0) out vec4 FragColor;

uniform vec4 textColor;

void main()
{
    FragColor = textColor;
}