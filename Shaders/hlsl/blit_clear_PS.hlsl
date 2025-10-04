struct PSInput
{
  [[vk::location(0)]] float4 Position : SV_POSITION;
};

float4 main(PSInput Input)
  : SV_Target0
{
  return float4(0.0, 0.0, 0.0, 1.0);
}
