#version 440

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec2 inScreenCoords ;

layout (location = 0) out vec4 outColor;

uniform sampler2D xImage;

void main() {
	vec4 color = texture(xImage, inUV);	
	outColor = vec4(color.rgb * vec3(1.0, 0.8, 0.8), color.a);
}