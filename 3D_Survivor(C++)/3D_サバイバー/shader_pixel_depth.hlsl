/*==============================================================================

  ê[ìxèÓïÒèëÇ´çûÇ›óp [shader_pixel_depth.hlsl]
														 Author : hibiki sakuma
														 Date   : 2025/12/10
--------------------------------------------------------------------------------

==============================================================================*/
struct PS_IN
{
    float4 posH : SV_POSITION;
};

float4 main(PS_IN pi) : SV_TARGET
{
    return float4(pi.posH.z, pi.posH.z, pi.posH.z, 1.0f);
    //return float4(1.0f,1.0f,1.0f, 1.0f);

}
