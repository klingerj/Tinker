struct PSInput
{
  [[vk::location(0)]] float4 Position : SV_POSITION;
  [[vk::location(1)]] float4 Color    : COLOR;
};

float4 main(PSInput Input)
  : SV_Target0
{
  return float4(Input.Color.rgb, 1.0f);
}
