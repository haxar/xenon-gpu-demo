float4x4 m: register(c0);

struct _IN
{   
  float4 pos: POSITION;
  int4 col: COLOR;
};
       
struct _OUT
{
  float4 pos: POSITION;
  float4 col: COLOR;
};

_OUT main(_IN In )
{
  _OUT Out;
  Out.pos = mul(m, In.pos);
  Out.col = In.col;
  return Out;
}

