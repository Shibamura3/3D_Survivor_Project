/*
	スコアの制御：fade.h

	2025/07/09	hibiki sakuma
*/

#ifndef FADE_H
#define FADE_H

#include <DirectXMath.h>

void Fade_Initialize(); 
void Fade_Finalize(); 
void Fade_Update(double elapsed_time);
void Fade_Draw();
void Fade_Start(double time, bool isFadeOut, DirectX::XMFLOAT3 color = { 0.0f,0.0f,0.0f });

enum FadeState : int {
	FADE_STATE_NONE,
	FADE_STATE_FINISHED_IN,
	FADE_STATE_FINISHED_OUT,
	FADE_STATE_IN,
	FADE_STATE_OUT,
	FADE_STATE_MAX
};
FadeState Fade_GetState();

#endif // !SCORE_H
