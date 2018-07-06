#version 330

layout(location=0) in vec4 a_position;
layout(location=6) in vec4 a_color;

uniform mat4 u_worldMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

out vec4 v_Color;

void main(void)
{	
	v_Color = a_color;
	gl_Position = u_projectionMatrix * u_viewMatrix * u_worldMatrix * a_position;
}