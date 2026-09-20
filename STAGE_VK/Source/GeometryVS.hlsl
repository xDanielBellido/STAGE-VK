struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION0;
};

float4 main(VSInput input) : SV_POSITION
{
    return float4(input.position, 1.0f);
}