struct Payload {
    uint2 meshletData[32];
};

groupshared Payload payload;

[numthreads(32, 1, 1)]
void main(uint2 dtid : SV_DispatchThreadID)
{
    const uint meshletId = dtid.x;
    
    bool visible = true;
    
    if (visible)
    {
        const uint index = WavePrefixCountBits(visible);
        payload.meshletData[index] = uint2(meshletId, 0);
    }
    
    const uint meshletCount = WaveActiveCountBits(visible);
    DispatchMesh(meshletCount, 1, 1, payload);
}
