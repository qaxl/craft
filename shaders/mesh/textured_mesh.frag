#version 450

layout (location = 0) in vec2 uv;

layout (set = 0, binding = 0) uniform sampler2D texture_;

layout (location = 0) out vec4 frag_color;

void main() {
    frag_color = texture(texture_, uv);
}

