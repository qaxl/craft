#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 out_uv;

layout (buffer_reference, std430) readonly buffer Buffer {
    uint vertices[];
};

layout (push_constant) uniform constants {
    mat4 view_proj_model;
    Buffer vertex_buffer;
} push_constants;

 const vec3 face_corner_offsets[6][4] = {
     // Front (+Z)
     { vec3(1, 0, 1), vec3(0, 0, 1), vec3(0, 1, 1), vec3(1, 1, 1) },
     // Back (-Z)
     { vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 1, 0) },
     // Left (-X)
     { vec3(0, 0, 1), vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 1) },
     // Right (+X)
     { vec3(1, 0, 0), vec3(1, 0, 1), vec3(1, 1, 1), vec3(1, 1, 0) },
     // Top (+Y)
     { vec3(0, 1, 0), vec3(1, 1, 0), vec3(1, 1, 1), vec3(0, 1, 1) },
     // Bottom (-Y)
     { vec3(0, 0, 1), vec3(1, 0, 1), vec3(1, 0, 0), vec3(0, 0, 0) }
 };

const vec3 normals[6] = {
    vec3( 0,  0,  1),  // Front (Red)
    vec3( 0,  0, -1),  // Back (Green)
    vec3(-1,  0,  0),  // Left (Blue)
    vec3( 1,  0,  0),  // Right (Yellow)
    vec3( 0,  1,  0),  // Top (Purple)
    vec3( 0, -1,  0)   // Bottom (Cyan)
};

const vec2 corner_uvs[4] = {
    vec2(0, 1),
    vec2(1, 1),
    vec2(1, 0),
    vec2(0, 0)
};

vec2 get_texture_coords(uint tex_index, vec2 corner_uv) {
    const ivec2 atlas_tiles = ivec2(16, 16);
    const vec2 tile_size = 1.0 / vec2(atlas_tiles);
    
    ivec2 tile_pos = ivec2(
        int(tex_index) % atlas_tiles.x,
        int(tex_index) / atlas_tiles.x
    );
    
    return vec2(tile_pos) * tile_size + corner_uv * tile_size;
}

void main() {
    const uint v = push_constants.vertex_buffer.vertices[gl_VertexIndex];

    const uint x      = v         & 0x1F;  // 5 bits
    const uint y      = (v >> 5)  & 0x3F;  // 6 bits
    const uint z      = (v >> 11) & 0x1F;  // 5 bits
    const uint face   = (v >> 16) & 0x07;  // 3 bits
    const uint corner = (v >> 19) & 0x03;  // 2 bits
    const uint tex_id = (v >> 21) & 0x1FF; // 9 bits
    const uint ao     = (v >> 30) & 0x03;  // 2 bits
    
    out_uv = get_texture_coords(tex_id, corner_uvs[corner]);

    const vec3 offset = face_corner_offsets[face][corner];
    gl_Position = push_constants.view_proj_model * vec4(vec3(x, y, z) + offset, 1.0);
}
