float4x4 m: register(c0);

struct _IN
{
  float4 pos   : POSITION;
  float2 uv    : TEXCOORD;
};

struct _OUT
{
  float4 pos  : POSITION;
  float2 uv : TEXCOORD;
};

_OUT main(_IN In)
{
  _OUT Out;
  Out.pos = mul(m, In.pos);
  Out.uv  = In.uv;
  return Out;
}
