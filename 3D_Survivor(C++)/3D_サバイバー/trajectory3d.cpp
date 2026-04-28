/*
	軌跡エフェクトの描画　

	2025/11/21	hibiki sakuma
*/

#include "trajectory3d.h"
#include "resource_manager.h"
#include "billboard.h"
#include "sheder_billboard.h"
#include "direct3d.h"
// デバック用
#include <sstream>
#include "debug_text.h"

using namespace DirectX;

struct Trajectory3d { // エフェクトの構造体
	XMFLOAT3 position; // 表示場所
	XMFLOAT4 color; // 色
	float size; // 表示サイズ
	double lifeTime; // 表示時間
	double birthTime; // 発生時間 ０以下の時は生まれてない

};

//定数宣言
static constexpr unsigned int TRAJECTORY_MAX = 1024; // 軌跡の最大数

//変数宣言
static Trajectory3d g_Trajectorys[TRAJECTORY_MAX] = {};
static double g_Time = 0.0; // 現在の時間
static unsigned int g_NextIndex = 0;
static hal::DebugText* g_pDT = nullptr;

void Trajectory3d_Initialize() {
	for (Trajectory3d& t : g_Trajectorys) {
		t.birthTime = 0.0; // 発生時間を０にする事で使用していない判定を出す
	}
	g_Time = 0.0;
}

void Trajectory3d_Finalize() {

}

void Trajectory3d_Update(double elapsed_time) {

	for (Trajectory3d& t : g_Trajectorys) {
		if (t.birthTime == 0.0) continue; // 使用していないので実行しない

		double time = g_Time - t.birthTime;

		if (time > t.lifeTime) {
			t.birthTime = 0.0; // 寿命が尽きた
		}
	}

	g_Time += elapsed_time; // 時間の更新
}

void Trajectory3d_Draw() {
	Direct3D_SetAlphaBlendAdd(); // 加算合成に変更

	for (const Trajectory3d& t : g_Trajectorys) {
		if (t.birthTime == 0.0) continue; // 使用していないので実行しない
		double time = g_Time - t.birthTime; // 生存時間(経過時間) = 現在時間 - 発生時間
		float ratio = (float)(time / t.lifeTime); // 比率　寿命から何割経過しているか
		float size = t.size * (1.0f - ratio); // 生まれた最初のサイズを比率を小さくする
		XMFLOAT4 color = t.color; // 色の編集 RGBA → WXYZ
		color.w = t.color.w * (1.0f - ratio); // 最後になるにつれて透明になる
		ShaderBillboard_SetColor(color);

		Billboard_Draw(Resouce_Manager_GetTexId(Effect_Trajectory), t.position, {size,size}, color);

	}

	Direct3D_SetAlphaBlendTransparent(); // 乗算合成に戻す
}

void Trajectory3d_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, float size, double lifeTime) {
//	// 新しく書き込む前に、念のため「使用中」フラグをリセットするイメージ
	if (g_NextIndex >= TRAJECTORY_MAX) g_NextIndex = 0;
	g_Trajectorys[g_NextIndex].birthTime = g_Time;
	g_Trajectorys[g_NextIndex].lifeTime = lifeTime;
	g_Trajectorys[g_NextIndex].color = color;
	g_Trajectorys[g_NextIndex].position = position;
	g_Trajectorys[g_NextIndex].size = size;
	g_Trajectorys[g_NextIndex].position.y -= 0.15f; // 生成位置を少し下げる

	g_NextIndex = (g_NextIndex + 1) % TRAJECTORY_MAX;

}
