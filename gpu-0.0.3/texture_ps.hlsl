struct input
{
  float2 uv : TEXCOORD;
};

sampler x;

float4 main(input In): COLOR
{
  return tex2D(x, In.uv);
}
