/*==============================================================================

   2D描画用ピクセルシェーダー [shader_pixel_2d.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOOD0;
};

Texture2D tex; //テクスチャ
SamplerState samp;

float4 main(PS_IN pi) : SV_TARGET
{
	//線の色
    //RGBの掛け算
    //a.r * b.r a.g*b+g a.b*b.b a.a*b.a ...各要素ごとの掛け算
    return tex.Sample(samp, pi.uv) * pi.color;
}
