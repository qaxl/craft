// TODO:
// struct Ubo {
//     float4x4 view_projection;
// };

// cbuffer ubo : register(b0) {
//     Ubo ubo;
// }

struct PushConstants {
    float4x4 view_projection;
    uint offset;
    uint count;
    uint64_t address;
};

[[vk::push_constant]] PushConstants pc;

struct VertexOutput {
    float4 pos : SV_Position;
[[vk::location(0)]] float4 color : COLOR0;
};

static const float4 pos[3] = {
    float4( 0.0, -10.0, 9.0, 1.0),
    float4(-10.0,  10.0, 9.0, 1.0),
    float4( 10.0,  10.0, 9.0, 1.0),
};

static const float4 colors[4] = {
    float4(0.0, 1.0, 0.0, 1.0),
    float4(0.0, 0.0, 1.0, 1.0),
    float4(1.0, 0.0, 0.0, 1.0),
    float4(1.0, 1.0, 1.0, 1.0),
};

static const float3 faces[6][4] = {
   // Front (+Z)
   { float3(1, 0, 1), float3(0, 0, 1), float3(0, 1, 1), float3(1, 1, 1) },
   // Back (-Z)
   { float3(0, 0, 0), float3(1, 0, 0), float3(1, 1, 0), float3(0, 1, 0) },
   // Left (-X)
   { float3(0, 0, 1), float3(0, 0, 0), float3(0, 1, 0), float3(0, 1, 1) },
   // Right (+X)
   { float3(1, 0, 0), float3(1, 0, 1), float3(1, 1, 1), float3(1, 1, 0) },
   // Top (+Y)
   { float3(0, 1, 0), float3(1, 1, 0), float3(1, 1, 1), float3(0, 1, 1) },
   // Bottom (-Y)
   { float3(0, 0, 1), float3(1, 0, 1), float3(1, 0, 0), float3(0, 0, 0) }
};

static const float3 normals[6] = {
    float3( 0,  0,  1),  // Front (Red)
    float3( 0,  0, -1),  // Back (Green)
    float3(-1,  0,  0),  // Left (Blue)
    float3( 1,  0,  0),  // Right (Yellow)
    float3( 0,  1,  0),  // Top (Purple)
    float3( 0, -1,  0)   // Bottom (Cyan)
};

static const float2 uvs[4] = {
    float2(0, 1),
    float2(1, 1),
    float2(1, 0),
    float2(0, 0)
};

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void main(out indices uint3 triangles[128], out vertices VertexOutput vertices[256], uint3 DispatchThreadID : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex, uint3 GroupThreadID : SV_GroupThreadID) {
    SetMeshOutputCounts(256, 128);

    uint data = vk::RawBufferLoad(pc.address + pc.offset + GroupThreadID.x * 4);

    uint x = data & 0x1F;
    uint y = (data >> 5) & 0x1F;
    uint z = (data >> 10) & 0x1F;
    uint face = (data >> 15) & 0x7;

    for (uint t = 0; t < 16; ++t) {

    }
    [unroll(4)] for (uint i = 0; i < 4; ++i) {
        vertices[i].pos = mul(pc.view_projection, float4(faces[face][i] + float3(x, y, z), 1.0));
        vertices[i].color = float4(0.2, 0.8, 0.2, 1.0);
    }

    triangles[0] = uint3(0, 1, 2);
    triangles[1] = uint3(0, 2, 3);
}
