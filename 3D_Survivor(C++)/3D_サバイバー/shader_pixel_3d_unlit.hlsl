/*==============================================================================

  光なし3D描画用ピクセルシェーダー [shader_pixel_unlit.hlsl]
														 Author : hibiki sakuma
														 Date   : 2025/11/21
--------------------------------------------------------------------------------

==============================================================================*/
cbuffer PS_CONATANT_BUFFER : register(b0)
{
    float4 diffuse_color;
}

struct PS_IN
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOOD0;
};

Texture2D tex; //テクスチャ
SamplerState samp; // テクスチャサンプラ

float4 main(PS_IN pi) : SV_TARGET
{
       return tex.Sample(samp, pi.uv) * diffuse_color;    
}