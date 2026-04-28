/*==============================================================================

   深度情報書き込み用 [shader_vertex_depth.hlsl]
														 Author : hibiki sakuma
														 Date   : 2025/12/10
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
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
};

//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    vo.posH = mul(vi.posL, mul(world, mul(view, proj)));
    
    return vo;
}