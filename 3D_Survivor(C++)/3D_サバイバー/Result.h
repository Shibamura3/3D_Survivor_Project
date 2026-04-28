/*
	リザルト画面の制御：Result.h

	2025/09/04	hibiki sakuma
*/


#ifndef RESULT_H
#define RESULT_H

#include "collision.h"

struct GameResultData {
	char name[16]; // ランキングに名前を入力
    int level;
    int killCount;
	// 追加記入
};

void Result_Initialize();
void Result_Finalize();
void Result_Update(double elapsed_time);
void Result_Draw();

// リザルト情報の取得
void Result_SetData(int Lv, int count);

Box Result_GetCollision();

#endif // !RESULT_H
