    /*==============================================================================

   インスタンシング対応の描画用頂点シェーダー [shader_vertex_instance.hlsl]
														 Author : Hibiki Sakuma
														 Date   : 2026/01/12
--------------------------------------------------------------------------------
ピクセルシェーダは既存のshader_pixel_3d.hlsl をそのまま使い回す
==============================================================================*/
cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 view;
}
cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 proj;
}
cbuffer VS_CONSTANT_BUFFER : register(b3)
{
    float4x4 light_view_proj;
}

struct VS_IN
{
    float4 posL : POSITION; 
    float4 normalL : NORMAL; 
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    // スロット1から送られてくる行列データ
    float4x4 instWorld : INSTANCE_WORLD;
};

struct VS_OUT
{
    float4 posH : SV_POSITION; 
    float4 posW : TEXCOORD1; 
    float4 posLightWVP : TEXCOORD2;
    float4 normalW : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    // インスタンス行列を使用してワールド座標を計算
    float4x4 targetWorld = vi.instWorld;

    // 1. 画面上の位置 (SV_POSITION)
    float4x4 mtxWVP = mul(mul(targetWorld, view), proj);
    vo.posH = mul(vi.posL, mtxWVP);
    
    // 2. ライト計算用ワールド座標
    vo.posW = mul(vi.posL, targetWorld);
        
    // 3. 法線
    vo.normalW = normalize(mul(float4(vi.normalL.xyz, 0.0f), targetWorld));
    
    // 4. その他パススルー
    vo.color = vi.color;
    vo.uv = vi.uv;
    return vo;
}
