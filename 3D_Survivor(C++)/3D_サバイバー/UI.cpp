/*
	UIの制御

	2025/12/18	hibiki sakuma
*/
#include <sstream>
#include "UI.h"
#include "player.h"
#include "pad_logger.h"
#include "sprite.h"
#include "resource_manager.h"
#include "debug_text.h"
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;


// 定数宣言
static constexpr float TEXT_IMG_SIZE = 50.0f;
static constexpr float PLAYERDATA_IMG_HEIGHT = 50.0f;
static constexpr float PLAYERDATA_IMG_WIDTH = 300.0f;
static constexpr float START_IMG_POSX = 20.0f;
static constexpr float START_IMG_POSY = 20.0f;
static constexpr float REGULATE_PIXEL = 6.0f;
static constexpr float ICON_SIZE = 128.0f;
static constexpr double END_TIME = 150.0; // 制限時間（秒）：変更要素

// 変数宣言
static float g_HpDataWidth = 0.0f;
static float g_ExpDataWidth = 0.0f;

static hal::DebugText* g_pDT = nullptr;
static hal::DebugText* g_pLv = nullptr;
static double g_AccumulatedTime = 0.0;
static double g_ClearTime = 0.0;

void UI_Initialize(){
	g_ClearTime = END_TIME;
	//フォントの初期化
	g_pDT = new hal::DebugText(Direct3D_GetDevice(),
		Direct3D_GetContext(),
		L"consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(),
		Direct3D_GetBackBufferHeight(),
		Direct3D_GetBackBufferWidth() * 0.4f, 50.0f,
		0, 0,
		0.0f, 20.0f);
	g_pLv = new hal::DebugText(Direct3D_GetDevice(),
		Direct3D_GetContext(),
		L"consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(),
		Direct3D_GetBackBufferHeight(),
		Direct3D_GetBackBufferWidth() * 0.06f, 80.0f,
		0, 0,
		0.0f, 20.0f);
}

void UI_Finalize(){
	delete g_pDT;
}

void UI_Update(double elapsed_time){
	// ステータスの描画関連
	g_HpDataWidth = (GetPlayer()->GetHp() * PLAYERDATA_IMG_WIDTH) / GetPlayer()->GetMaxHp();
	g_ExpDataWidth = (GetPlayer()->GetExp() * PLAYERDATA_IMG_WIDTH) / GetPlayer()->GetNextExp();

	// クリアタイマー関連
	g_AccumulatedTime += elapsed_time;
	if (g_AccumulatedTime > 1.0) {
		g_ClearTime--;
		g_AccumulatedTime = 0;
	}
}

void UI_Draw(){
	// バー横のテキストの表示
	Sprite_Draw(Resouce_Manager_GetTexId(UI_Hp), START_IMG_POSX, START_IMG_POSY, TEXT_IMG_SIZE, TEXT_IMG_SIZE);
	Sprite_Draw(Resouce_Manager_GetTexId(UI_Exp), START_IMG_POSX, START_IMG_POSY + TEXT_IMG_SIZE, TEXT_IMG_SIZE, TEXT_IMG_SIZE);
	// バーのケースを中身の表示
	Sprite_Draw(Resouce_Manager_GetTexId(Color_Red),  START_IMG_POSX + TEXT_IMG_SIZE + REGULATE_PIXEL, START_IMG_POSY + REGULATE_PIXEL                , g_HpDataWidth  - REGULATE_PIXEL * 2.0f, PLAYERDATA_IMG_HEIGHT - REGULATE_PIXEL * 2);
	Sprite_Draw(Resouce_Manager_GetTexId(Color_Blue), START_IMG_POSX + TEXT_IMG_SIZE + REGULATE_PIXEL, START_IMG_POSY + REGULATE_PIXEL + TEXT_IMG_SIZE, g_ExpDataWidth - REGULATE_PIXEL * 2.0f, PLAYERDATA_IMG_HEIGHT - REGULATE_PIXEL * 2);
	// バーのケースを表示
	Sprite_Draw(Resouce_Manager_GetTexId(UI_Case), START_IMG_POSX + TEXT_IMG_SIZE, START_IMG_POSY                , PLAYERDATA_IMG_WIDTH, PLAYERDATA_IMG_HEIGHT);
	Sprite_Draw(Resouce_Manager_GetTexId(UI_Case), START_IMG_POSX + TEXT_IMG_SIZE, START_IMG_POSY + TEXT_IMG_SIZE, PLAYERDATA_IMG_WIDTH, PLAYERDATA_IMG_HEIGHT);
	// 操作アイコンの表示
	if (PadLogger_IsConnected()) {
		Sprite_Draw(Resouce_Manager_GetTexId(Icon_Pad), Direct3D_GetBackBufferWidth() * 0.9f, Direct3D_GetBackBufferHeight() * 0.85f, ICON_SIZE, ICON_SIZE);
	} else {
		Sprite_Draw(Resouce_Manager_GetTexId(Icon_Keyboard), Direct3D_GetBackBufferWidth() * 0.9f, Direct3D_GetBackBufferHeight() * 0.85f, ICON_SIZE, ICON_SIZE);
	}
	// 時間の表示
	std::stringstream ss;//coutと同じ
	// 時間表示用変数
	int minute = (int)g_ClearTime / 60;
	int second = (int)g_ClearTime % 60;
	ss << "LIMIT TIME:  " << minute << ":" << second <<std::endl;
	g_pDT->SetText(ss.str().c_str(), { 1.0f, 1.0f, 0.0f, 1.0f });
	g_pDT->Draw();
	g_pDT->Clear();
	// レベルの表示
	std::stringstream sslv;
	sslv << "Level : " << GetPlayer()->GetLevel() << std::endl;
	g_pLv->SetText(sslv.str().c_str(), { 1.0f, 1.0f, 0.0f, 1.0f });
	g_pLv->Draw();
	g_pLv->Clear();
}

double UI_GetTime(){
	return g_ClearTime;
}
