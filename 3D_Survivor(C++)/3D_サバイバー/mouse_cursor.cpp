/*
	マウスカーソルの表示　：mouse_cursor.cpp

	2025/12/18	hibiki sakuma
*/
#include "mouse_cursor.h"
#include "sprite.h"
#include "resource_manager.h"
#include "mouse.h"
#include "pad_logger.h"
#include "direct3d.h"
#include <DirectXMath.h>
#include <iostream>
using namespace DirectX;

// 定数宣言
static constexpr int CURSOR_SIZE = 64;
const float CURSOR_SPEED = 1000.0f; // 1秒間に動くピクセル数

// 変数宣言
static XMFLOAT2 g_Position{};
static bool g_IsClick = false;

void MouseCursor_Initialize(){
	g_Position = { Direct3D_GetBackBufferWidth() * 0.5f, Direct3D_GetBackBufferHeight() * 0.5f }; // 画面中央を初期位置
}

void MouseCursor_Finalize(){

}

// マウスモードでの更新：OSのマウス座標を強制同期
void MouseCursor_UpdateWithMouse(int x, int y) {
	g_Position.x = (float)x;
	g_Position.y = (float)y;
}

// パッドモードでの更新：現在の座標から移動させる
void MouseCursor_UpdateWithStick(float stickX, float stickY, double elapsed_time) {

	g_Position.x += stickX * CURSOR_SPEED * (float)elapsed_time;
	// SDLのスティックYは上がマイナスなので、ここではそのまま加算（上がマイナス＝画面上方向）
	g_Position.y -= stickY * CURSOR_SPEED * (float)elapsed_time;

	// 画面外に行かないように制限（Direct3Dから解像度を取得してクランプ）
	g_Position.x = std::max(0.0f, std::min((float)Direct3D_GetBackBufferWidth(), g_Position.x));
	g_Position.y = std::max(0.0f, std::min((float)Direct3D_GetBackBufferHeight(), g_Position.y));

}

void MouseCursor_Draw(){
	if (!g_IsClick) {
		Sprite_Draw(Resouce_Manager_GetTexId(Mousu_Cursor_Blue), g_Position.x - CURSOR_SIZE * 0.25f, g_Position.y - CURSOR_SIZE * 0.25f, CURSOR_SIZE, CURSOR_SIZE);
	} else {
		Sprite_Draw(Resouce_Manager_GetTexId(Mousu_Cursor_Red), g_Position.x - CURSOR_SIZE * 0.25f, g_Position.y - CURSOR_SIZE * 0.25f, CURSOR_SIZE, CURSOR_SIZE);
	}

#if defined(DEBUG) || defined(_DEBUG) 
	Collision_DebugDraw(MouseCursor_GetCollision());
#endif
}

void MouseCursor_IsHit(bool check){
	g_IsClick = check;
}

Box MouseCursor_GetCollision(){
	return { {g_Position.x,g_Position.y}, CURSOR_SIZE * 0.25f, CURSOR_SIZE * 0.25f};
}
