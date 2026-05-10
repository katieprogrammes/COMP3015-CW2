#version 460

layout (location = 0) in vec3 VertexPosition;

uniform mat4 MVP;

out vec2 TexCoord;



void main()
{
	TexCoord = VertexPosition.xy * 0.5 + 0.5;
	gl_Position = MVP*vec4(VertexPosition,1.0);
}
