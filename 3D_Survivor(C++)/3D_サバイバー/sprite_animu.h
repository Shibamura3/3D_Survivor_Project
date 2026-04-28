/************************************************
*スプライト表示　sprite_animu.h
* 
* 
*************************************************/
#ifndef SPRITE_ANIMU_H
#define SPRITE_ANIMU_H

#include <DirectXMath.h>

void SpriteAnim_Initialize();
void SpriteAnim_Finalize();

void SpriteAnim_Update(double elapsed_time);
void SpriteAnim_Draw(int playid,float dx,float dy,float dw,float dh);
void BillboardAnim_Draw(int playid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot = {0.0f,0.0f});

int Spriteanim_RegisterPattern(int texId, int patternMax, int h_patternMax, double seconds_par_pattern, const DirectX::XMUINT2& pattern_size, const DirectX::XMUINT2& start_position, bool is_looped, bool is_invert);

int SpriteAnim_CreatePlayer(int anim_pattern_id);

void SpriteAnim_DestroyPlayer(int index);

bool SpriteAnim_IsStopped(int index);

#endif // SPRITE_ANIMU_H