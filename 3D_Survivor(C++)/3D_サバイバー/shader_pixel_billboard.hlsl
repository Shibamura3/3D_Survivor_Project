/*==============================================================================

   ビルボード描画用ピクセルシェーダー [shader_pixel_billboard.hlsl]
														 Author : hibiki sakuma
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/
// 定数バッファ
cbuffer PS_CONATANT_BUFFER : register(b0)
{
    float4 diffuse_color;
}

struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOOD0;
};

Texture2D tex; //テクスチャ
SamplerState samp; // テクスチャサンプラ

float4 main(PS_IN pi) : SV_TARGET
{
    float4 color = tex.Sample(samp, pi.uv) * pi.color * diffuse_color;
    // アルファテストの検証
    // clip(pi.uv.x * 100); // 縦に格子状になる
    //if (color.a < 0.01f){  
    //    discard; // これ以降を書かなくなる
    //}
    return color;
}
