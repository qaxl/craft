struct VertexOutput {
    [[vk::location(0)]] float4 color : COLOR0;
};

float4 main(VertexOutput input) : SV_Target {
    return input.color;
}
