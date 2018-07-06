#version 330

in vec4 v_Color;
out vec4 Fragment;

void main(void) {

	Fragment = vec4(v_Color);
}