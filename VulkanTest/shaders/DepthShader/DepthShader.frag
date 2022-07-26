#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1)uniform usampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	uvec4 vec_tex = texture(texSampler, fragTexCoord);
    outColor = vec4(float(vec_tex.r)/255, float(vec_tex.g)/255, float(vec_tex.b)/255, float(vec_tex.a)/255);
}