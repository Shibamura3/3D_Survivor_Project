/*==============================================================================

   3D描画用頂点シェーダー [shader_vertex_3d.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/00/10
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

cbuffer VS_CONSTANT_BUFFER : register(b3)
{
    float4x4 light_view_proj; //ライトポジション座標変換
};

struct VS_IN
{
    float4 posL    : POSITION0;
    float4 normalL : NORMAL0; // 法線情報 
    float4 color   : COLOR0;
    float2 uv      : TEXCOORD0;
};

struct VS_OUT
{
    float4 posH    : SV_POSITION;
    float4 posW    : POSITION0;
    float4 posLightWVP : POSITION1;
    float4 normalW : NORMAL0;
    float4 color   : COLOR0;
    float2 uv      : TEXCOORD0;
};

//=============================================================================
// 頂点シェーダ
//=============================================================================
//シェーダーは変数を見ない、セマンティクスを見る
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    // 座標変換
    float4x4 mtxWV = mul(world, view);
    float4x4 mtxWVP = mul(mtxWV, proj);
    vo.posH = mul(vi.posL, mtxWVP);
    
    // ライトビュープロジェクション空間へ座標変換
    vo.posLightWVP = mul(vi.posL, mul(world, light_view_proj));
    
    // ライト計算　ディレクションライトの強さ
    // 普通のワールド変換行列ではダメ
    // ワールド変換行列の転置逆行列なら良い
    float4 normalW = mul(float4(vi.normalL.xyz, 0.0f), world); // 線形保管でもらう
    vo.normalW = normalize(normalW); // 単位ベクトル可 誤差を減らす
    vo.posW = mul(vi.posL, world); // ローカル座標を使ってワールド変換行列を算出
    
    vo.color = vi.color; // パススルー カラーの計算は全てピクセルシェーダで行う
    vo.uv = vi.uv; // パススルー　関係ないので何もしない
    
    return vo;
}
