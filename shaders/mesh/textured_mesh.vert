#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 out_uv;

struct Vertex {
    vec3 pos;
    float _pad;
    vec2 uv;
    vec2 _pad2;
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

    gl_Position = push_constants.proj * vec4(v.pos.xyz, 1.0);

    out_uv = v.uv;
}
