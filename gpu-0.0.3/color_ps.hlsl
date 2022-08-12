struct _IN
{
  float4 Color : COLOR;
};

float4 main(_IN In): COLOR
{
  return In.Color;
}
