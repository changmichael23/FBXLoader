#version 330

layout(location=0) in vec4 a_position;
layout(location=1) in vec3 a_normal;
layout(location=2) in vec2 a_texCoords;

uniform mat4 u_worldMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

out vec3 v_Position;
out vec3 v_CameraPosition;
out vec3 v_Normal;
out vec2 v_TexCoords;

void main(void)
{	
	v_Normal = mat3(u_worldMatrix) * a_normal;
	v_TexCoords = a_texCoords;
	v_Position = vec3(u_worldMatrix * a_position);
	v_CameraPosition = -vec3(u_viewMatrix[3]);
	gl_Position = u_projectionMatrix * u_viewMatrix * vec4(v_Position, 1.0);
	// gl_Position = u_projectionMatrix * u_viewMatrix * u_worldMatrix * a_position;
	// gl_Position = a_position;
}