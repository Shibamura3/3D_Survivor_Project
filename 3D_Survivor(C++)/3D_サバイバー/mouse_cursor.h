/*
	マウスカーソルの表示　：mouse_cursor.h

	2025/12/18	hibiki sakuma
*/
#ifndef MOUSE_CURSOR_H
#define MOUSE_CURSOR_H

#include "collision.h"

void MouseCursor_Initialize();
void MouseCursor_Finalize();

// マウスモードでの更新：OSのマウス座標を強制同期
void MouseCursor_UpdateWithMouse(int x, int y);
// パッドモードでの更新：現在の座標から移動させる
void MouseCursor_UpdateWithStick(float stickX, float stickY, double elapsed_time);

//void MouseCursor_Update(double elapsed_time);
void MouseCursor_Draw();

void MouseCursor_IsHit(bool check);
Box MouseCursor_GetCollision();

#endif // !MOUSE_CURSOR_H
