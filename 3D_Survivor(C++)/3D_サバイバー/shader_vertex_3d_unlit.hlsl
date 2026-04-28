/*==============================================================================

   光なし3D描画用頂点シェーダー [shader_vertex_unlit.hlsl]
														 Author : hibiki sakuma
														 Date   : 2025/11/21
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 world; //ワールド座標変換
}
cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 view; //ビュー座標変換
};
cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 proj; //ポジション座標変換
};


struct VS_IN
{
    float4 posL : POSITION0;
    float2 uv : TEXCOORD0;
    
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    float4x4 mtxWV = mul(world, view);
    float4x4 mtxWVP = mul(mtxWV, proj);
    vo.posH = mul(vi.posL, mtxWVP);
      
    vo.uv = vi.uv;
    
    return vo;
}