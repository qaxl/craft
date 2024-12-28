#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_uv;

struct Vertex {
    vec4 pos_and_normal_x;
    vec4 normal_yz_and_uv_xy;
    vec4 color;
};

layout (buffer_reference, std430) readonly buffer Buffer {
    Vertex vertices[];
};

layout (push_constant) uniform constants {
    mat4 proj;
    Buffer vertex_buffer;
} push_constants;

void main() {
    Vertex v = push_constants.vertex_buffer.vertices[gl_VertexIndex];

    gl_Position = push_constants.world * vec4(v.pos_and_normal_x.xyz, 1.0);

    out_color = v.color.xyz;
    out_uv = v.normal_yz_and_uv_xy.zw;
}
