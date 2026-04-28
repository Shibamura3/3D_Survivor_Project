/*==============================================================================

   3Dフィールド描画用ピクセルシェーダー [shader_pixel_3d.hlsl]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
// 定数バッファ
cbuffer PS_CONATANT_BUFFER : register(b0)
{
    float4 diffuse_color;
}


cbuffer PS_CONSTANT_BUFFER : register(b1) // light用 アンビエントライト
{
    float4 ambient_color; //アンビエントライトのカラー
};

cbuffer PS_CONSTANT_BUFFER : register(b2) // light用 ディレクションライト
{
    float4 directonal_vector; //ディレクションの方向
    float4 directonal_color = { 1.0f, 1.0f, 1.0f, 1.0f }; //ディレクションのカラー
}

cbuffer PS_CONSTANT_BUFFER : register(b3) // light用 フォンライト
{
    float3 eye_posW; // 視線ベクトル　カメラ座標
    float specular_power = 30.0f; // 反射のつよさ（テカリ）
    float4 supeculer_color = { 0.4f, 0.4f, 0.4f, 1.0f }; // 反射光の色
};

struct PointLight
{
    float3 posW; // ポイントライトのワールド座標
    float range; // 有効範囲
    float3 color; // ポイントライトの色
};

cbuffer PS_CONSTANT_BUFFER : register(b4) // light用 ポイントライト
{
    PointLight pointlight[4]; // ポイントライトの数分の構造体
    int point_light_count; // ポイントライトの配置数
    float dummy[3];
};

struct PS_IN
{
    float4 posH        : SV_POSITION;
    float4 posW        : POSITION0;
    float4 posLightWVP : POSITION1;
    float4 normalW     : NORMAL0;
    float4 blend       : COLOR0;
    float2 uv          : TEXCOORD0;
};

Texture2D tex0 : register(t0); //テクスチャ
Texture2D tex1 : register(t1); //テクスチャ
Texture2D tex2 : register(t2); //深度情報テクスチャ

SamplerState samp : register(s0); // テクスチャサンプラー

float4 main(PS_IN pi) : SV_TARGET
{
    // uvを使用した回転 半々でブレンド
    float2 uv;
    float angle = 3.1415f * 45.0f / 180.0f;
    uv.x = pi.uv.x * cos(angle) + pi.uv.y * sin(angle);
    uv.y = -pi.uv.x * sin(angle) + pi.uv.y * cos(angle);
   
    // ２枚のテクスチャをブレンドパラメータを元にブレンドするサンプルコード
    // Rの値を重みとして、lerpで合成する
    float4 texA = tex0.Sample(samp, pi.uv); // 草
    float4 texB = tex1.Sample(samp, pi.uv); // 道
    float4 tex_color = lerp(texA, texB, pi.blend.r);
    
    //ライティング関係
    // 材質の光　マテリアル
    float3 material_color = tex_color.rgb * diffuse_color.rgb;
   
    // 平行光源 ディフューズライト
    float4 normalW = normalize(pi.normalW);
    float dl = (dot(-directonal_vector, pi.normalW) + 1.0f) * 0.5f; // ハーフランバート
    float3 diffuse = material_color * directonal_color.rgb * dl; // 材質にかかわる光＊平行光
    
    // 環境光　アンビエントカラー　
    float3 ambient = material_color * ambient_color.rgb; // 材質にかかわる光＊一律の光源

    // スペキュラー
    float3 toEye = normalize(eye_posW - pi.posW.xyz); // 視線ベクトル
    float3 toReflect = reflect(directonal_vector, normalW).xyz; // 反射ベクトルを算出
    float t = pow(max(dot(toReflect, toEye), 0.0f), specular_power); // 内積の計算でスペキュラーの強さを求める    
    float3 specular = supeculer_color * t; // 反射の光のみを設定可能
        
      // 最終的に目に入ってくる光の色
//    float alpha = tex.Sample(samp, pi.uv).a * diffuse_color.a; // 地面に透明はないので」α成分は計算しない
    float3 color = ambient + diffuse + specular; // それぞれの光は加算　スペキュラーは光っている部分のみの加算
    
 
    for (int i = 0; i < point_light_count; i++)
    {
        // ポイントライトのサンプル
        // 点光源から面（ピクセル）へのベクトルを算出
        float lightTopixel = pi.posW.xyz - pointlight[i].posW;
        // 面（ピクセル）とライトとの距離を測る
        float D = length(lightTopixel); // ベクトルの引き算で距離を算出
        
        //三段階の減衰率でやるのがきれい
        // 影響力
        float A = pow(max(1.0f - 1.0f / pointlight[i].range * D, 0.0f), 2.0f); // 1-1/影響範囲×距離 指数計算と０以下を０に
       
        // 点光源と面（ピクセル）との向きを考慮に入れる→並行光源と同じ考え
        float dl = max(0.0f, dot(-normalize(lightTopixel), normalW.xyz));
        float diffuse = material_color * directonal_color.rgb * dl;
        
        // 点光源の影響を加算する
        color += material_color * pointlight[i].color.rgb * A * dl;
        
        // 点光源のスペキュラを計算
        float3 toRef = reflect(normalize(lightTopixel), normalW.xyz); // 反射ベクトルを算出
        float PL_t = pow(max(dot(toRef, toEye), 0.0f), specular_power); // 内積の計算でスペキュラーの強さを求める    
        float3 PL_specular = pointlight[i].color.rgb * PL_t; // 反射の光のみを設定可能

        color += PL_specular;
    }
    
    // 影の計算
    //float2 shadowmap_uv = pi.posLightWVP.xy / pi.posLightWVP.w;
    //shadowmap_uv = shadowmap_uv * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    //
    //float depthmap_z = tex2.Sample(samp, shadowmap_uv).r;
    //
    //float shadowmap_z = pi.posLightWVP.z / pi.posLightWVP.w;
    //
    //if (shadowmap_z - 0.0001f < depthmap_z)
    //{
    //    color *= 0.5f;
    //}
   // 影の計算
    float3 shadow_pos = pi.posLightWVP.xyz / pi.posLightWVP.w;
    float2 shadow_uv = shadow_pos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    
    // 影マップから取得した距離
    float depthmap_z = tex2.Sample(samp, shadow_uv).r;
    // 今のピクセルのライトからの距離
    float shadowmap_z = shadow_pos.z;

    // 判定範囲内かつ、今の場所が影マップより「奥」なら影にする
    if (shadow_uv.x >= 0.0f && shadow_uv.x <= 1.0f &&
        shadow_uv.y >= 0.0f && shadow_uv.y <= 1.0f)
    {
        // 0.0005f は「マッハバンド（縞々模様）」を防ぐための微調整値（バイアス）です
        if (shadowmap_z - 0.0005f > depthmap_z)
        {
            color *= 0.5f; // 影にする
        }
    }
    // テクスチャの色にマテリアルの色を掛ける
    return float4(color, 1.0f);
    // デバッグ用：シャドウマップの内容をそのまま色として表示
    //float depth = tex2.Sample(samp, shadow_uv).r;
    //return float4(depth, depth, depth, 1.0f);
}
