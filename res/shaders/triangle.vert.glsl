#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	uint window_width;
	uint window_height;
} push_constants;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 0) out vec3 fragColor;

void main()
{    
    gl_Position = push_constants.mvp * ubo.model * ubo.view * ubo.proj * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
