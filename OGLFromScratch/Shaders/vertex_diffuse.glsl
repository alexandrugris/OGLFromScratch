#version 330 core
layout (location = 0) in vec4 vPos;
layout (location = 1) in vec4 vNormal;
layout (location = 2) in vec3 vTex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;


varying vec2 vUV;

void main(){
	gl_Position = projection * view * model * vPos;
	vUV = vTex.xy;
}