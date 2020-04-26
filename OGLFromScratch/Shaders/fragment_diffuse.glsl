#version 330 core
out vec4 FragColor;

uniform sampler2D diffuse;
varying vec2 vUV;

void main(){
	FragColor = texture2D(diffuse, vUV);
}

