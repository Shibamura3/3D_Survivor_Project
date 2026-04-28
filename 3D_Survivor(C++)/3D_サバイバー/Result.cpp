/*
	リザルト画面の制御：Result.h

	2025/09/04	hibiki sakuma
*/

#include "Result.h"
#include "resource_manager.h"
#include "sprite.h"
#include "Key_logger.h"
#include "fade.h"
#include "scene.h"
//#include "Audio.h"
#include "direct3d.h"
#include "mouse.h"
#include "pad_logger.h"
#include "mouse_cursor.h"
#include "UI.h"
#include "player.h"
#include "game.h"	
#include "achievementmanager.h"
#include "debug_text.h"

#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <SpriteFont.h>
#include <SpriteBatch.h>
#include <DirectXMath.h>
using namespace DirectX;

//定数宣言
static constexpr float SELECT_SIZE_X = 600.0f;
static constexpr float SELECT_SIZE_Y = 100.0f;
static constexpr float STAGE_SIZE_X = 200.0f;
static constexpr float STAGE_SIZE_Y = 50.0f;
static constexpr float ACHIEVE_SIZE = 100.0f;
static constexpr float ICON_SIZE = 128.0f;
static constexpr int MAX_RANKING = 5; // ランキングに表示する件数
static constexpr float PLAY_TEXT_X = 200.0f;
static constexpr float RANKING_TEXT_X = 900.0f;


// 変数宣言
static bool g_Button_Title = false;
static bool g_IsClear = false;
static hal::DebugText* g_pDT = nullptr;
static XMFLOAT2 g_ButtonPos{}; // ボタンの中心座標
static GameResultData g_FinalResult;
static bool g_IsNaming = true; // 名前入力中かどうかの判定
static std::string g_InputName = ""; // 入力中の名前
static std::vector<GameResultData> g_RankingList;
// DirectXTK用
static std::unique_ptr<DirectX::SpriteBatch> g_FontBatch;
static std::unique_ptr<DirectX::SpriteFont>  g_Font;

// 内部関数のプロトタイプ宣言
void LoadRanking();
void SaveRanking();
void AddToRanking(const GameResultData& data);

void Result_Initialize(){
	// 変数の初期化
	g_IsNaming = true;
	g_InputName = "";

	//画面遷移　フェードイン処理
	Fade_Start(1.0, false); // タイトルからつながっている
	// マウスを絶対座標に
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);

	// フラグの初期化
	g_Button_Title = false;
	g_IsClear = false;

	// フォント初期化
	g_FontBatch = std::make_unique<DirectX::SpriteBatch>(Direct3D_GetContext());
	g_Font = std::make_unique<DirectX::SpriteFont>(Direct3D_GetDevice(), L"resuce/data/font/myfont.spritefont");

	// ボタンの座標の設定
	// タイトルボタン
	g_ButtonPos = { Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X, Direct3D_GetBackBufferHeight() * 0.7f };

	//BGMの再生
	//PlayAudio(g_Title_BGM, true);

	// resultに来た時からクリア判定
	if (UI_GetTime() <= 0.0) {
		g_IsClear = true;
	} else {
		g_IsClear = false;
	}

	// マウスカーソルの設定
	MouseCursor_Initialize();

	g_pDT = new hal::DebugText(Direct3D_GetDevice(),
		Direct3D_GetContext(),
		L"consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(),
		Direct3D_GetBackBufferHeight(),
		Direct3D_GetBackBufferWidth() * 0.2f, Direct3D_GetBackBufferHeight() * 0.3f,
		0, 0,
		0.0f, 20.0f);

	// ランキング読み込み
	LoadRanking();
}

void Result_Finalize(){
	//Texture_AllRelease();
	// サウンドストップはココに入れる
	//UnloadAudio(g_Check_SE);
	//UnloadAudio(g_Miss_SE);
	//UnloadAudio(g_Title_BGM);
	MouseCursor_Finalize();
	g_Font.reset();
	g_FontBatch.reset();
}

void Result_Update(double elapsed_time){
	if (g_IsNaming) { // ランキングの入力
		// --- 名前入力ロジック ---
		// A-Zまでの入力を簡易的にチェック（Key_loggerを使用）
		for (int i = (int)KK_A; i <= (int)KK_Z; ++i) {
			if (KeyLogger_IsTrigger((Keyboard_Keys)i)) {
				if (g_InputName.length() < 15) g_InputName += (char)('A' + (i - (int)KK_A));
			}
		}
		// バックスペースで消去
		if (KeyLogger_IsTrigger(KK_BACK) && !g_InputName.empty()) {
			g_InputName.pop_back();
		}
		// エンターで確定
		if (KeyLogger_IsTrigger(KK_ENTER)) {
			strcpy_s(g_FinalResult.name, g_InputName.empty() ? "NO NAME" : g_InputName.c_str());
			AddToRanking(g_FinalResult);
			SaveRanking();
			g_IsNaming = false;
		}
		return; // 入力中は下のボタン判定などをさせない
	}

	// モード判定
	InputMode mode = PadLogger_IsConnected() ? InputMode::Controller : InputMode::KeyboardMouse;

	bool isDecideTriggered = false; // 共通の決定フラグ

	if (mode == InputMode::Controller) {
		// パッド：スティックでカーソル移動
		XMFLOAT2 stick = PadLogger_GetLeftThumbStick(0);
		MouseCursor_UpdateWithStick(stick.x, stick.y, elapsed_time);

		// Aボタンを決定にする
		isDecideTriggered = PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_A);
	} else {
		// マウス：OSの座標でカーソル移動
		Mouse_State ms{};
		Mouse_GetState(&ms);
		MouseCursor_UpdateWithMouse(ms.x, ms.y);

		// 左クリックを決定にする
		isDecideTriggered = ms.leftButton;
	}

	// ボタンとカーソルの接触判定
	g_Button_Title = Collision_IsOverlapBox(Result_GetCollision(), MouseCursor_GetCollision());
	
	// カーソルがボタンに乗ったかどうかの通知
	MouseCursor_IsHit(g_Button_Title);

	// 決定操作が行われた場合
	if (isDecideTriggered && (g_Button_Title)) {
		Fade_Start(1.0, true);
	}

	if (Fade_GetState() == FADE_STATE_FINISHED_OUT) { // フェードが終わったらシーンチェンジ
		Scene_Change(SCENE_TITLE); // タイトル画面へ
	}
}

void Result_Draw(){
	Direct3D_SetDepthEnable(false);
	Sprite_Begin(); // スプライト描き始め

	// 共通背景の描画
	Sprite_Draw(Resouce_Manager_GetTexId(Result_Back), 0.0f, 0.0f, (float)Direct3D_GetBackBufferWidth(), (float)Direct3D_GetBackBufferHeight());
	// 半透明ウィンドウ
	Sprite_Draw(Resouce_Manager_GetTexId(Color_Black), Direct3D_GetBackBufferWidth() * 0.1f, Direct3D_GetBackBufferHeight() * 0.1f, Direct3D_GetBackBufferWidth() * 0.8f, Direct3D_GetBackBufferHeight() * 0.8f, { 1, 1, 1, 0.5f });

	// テキスト描画 (DirectXTK)
	g_FontBatch->Begin();

	// 今回のリザルト表示
	g_Font->DrawString(g_FontBatch.get(), L"[ CURRENT RESULT ]", XMFLOAT2(PLAY_TEXT_X, 200), Colors::Cyan);

	std::wstring wLv = L"Player Level   : " + std::to_wstring(g_FinalResult.level);
	std::wstring wKill = L"Defeated Enemy : " + std::to_wstring(g_FinalResult.killCount);
	g_Font->DrawString(g_FontBatch.get(), wLv.c_str(), XMFLOAT2(PLAY_TEXT_X, 280), Colors::White);
	g_Font->DrawString(g_FontBatch.get(), wKill.c_str(), XMFLOAT2(PLAY_TEXT_X, 330), Colors::White);

	// 名前入力 または ランキング ---
	if (g_IsNaming) {
		// 名前入力画面
		g_Font->DrawString(g_FontBatch.get(), L"ENTER YOUR NAME", XMFLOAT2(RANKING_TEXT_X, 200), Colors::Yellow);
		std::wstring wName = L"> " + std::wstring(g_InputName.begin(), g_InputName.end()) + L"_";
		g_Font->DrawString(g_FontBatch.get(), wName.c_str(), XMFLOAT2(RANKING_TEXT_X, 300), Colors::White);
		g_Font->DrawString(g_FontBatch.get(), L"PRESS ENTER TO REGISTER", XMFLOAT2(RANKING_TEXT_X, 400), Colors::Gray);
	} else {
		// ランキング表示
		g_Font->DrawString(g_FontBatch.get(), L"[ TOP RANKING ]", XMFLOAT2(RANKING_TEXT_X, 150), Colors::Gold);
		for (int i = 0; i < (int)g_RankingList.size(); ++i) {
			std::string rankStr = std::to_string(i + 1) + ". " + g_RankingList[i].name + " : " + std::to_string(g_RankingList[i].killCount);
			std::wstring wRank(rankStr.begin(), rankStr.end());

			// 自分の記録が今ランクインしたものなら色を変えると分かりやすい
			XMVECTOR color = (strcmp(g_RankingList[i].name, g_FinalResult.name) == 0 && g_RankingList[i].killCount == g_FinalResult.killCount)
				? Colors::Lime : Colors::White;

			g_Font->DrawString(g_FontBatch.get(), wRank.c_str(), XMFLOAT2(RANKING_TEXT_X, 220.0f + i * 50.0f), color);
		}
	}
	g_FontBatch->End();

	// UI・ボタン描画
	Direct3D_SetAlphaBlendTransparent(); // α設定を戻す

	// 名前入力が終わった後だけ「タイトルへ」ボタンを出す
	if (!g_IsNaming) {
		float alpha = g_Button_Title ? 0.5f : 1.0f;
		Sprite_Draw(Resouce_Manager_GetTexId(Title_Button),
			Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X * 0.5f,
			Direct3D_GetBackBufferHeight() * 0.75f,
			SELECT_SIZE_X, SELECT_SIZE_Y, { 1, 1, 1, alpha });
	}

	// 操作アイコン
	if (PadLogger_IsConnected()) {
		Sprite_Draw(Resouce_Manager_GetTexId(Icon_Pad), Direct3D_GetBackBufferWidth() * 0.9f, Direct3D_GetBackBufferHeight() * 0.85f, ICON_SIZE, ICON_SIZE);
	} else {
		Sprite_Draw(Resouce_Manager_GetTexId(Icon_Keyboard), Direct3D_GetBackBufferWidth() * 0.9f, Direct3D_GetBackBufferHeight() * 0.85f, ICON_SIZE, ICON_SIZE);
	}

	MouseCursor_Draw();

#if defined(DEBUG) || defined(_DEBUG) 
	if (!g_IsNaming) Collision_DebugDraw(Result_GetCollision());
#endif
}

void Result_SetData(int Lv, int count){
	g_FinalResult.level = Lv;
	g_FinalResult.killCount = count;
}

Box Result_GetCollision()
{
	return { {g_ButtonPos.x + SELECT_SIZE_X, g_ButtonPos.y + SELECT_SIZE_Y * 0.5f}, SELECT_SIZE_X * 0.5f, SELECT_SIZE_Y * 0.5f };
}

void LoadRanking() {
	g_RankingList.clear();
	std::ifstream ifs("resuce/data/ranking.dat", std::ios::binary);
	if (ifs) {
		GameResultData data;
		while (ifs.read((char*)&data, sizeof(GameResultData))) {
			g_RankingList.push_back(data);
		}
	}
}

void SaveRanking() {
	std::ofstream ofs("resuce/data/ranking.dat", std::ios::binary);
	for (const auto& data : g_RankingList) {
		ofs.write((char*)&data, sizeof(GameResultData));
	}
}

void AddToRanking(const GameResultData& data) {
	g_RankingList.push_back(data);
	// スコア順にソート（降順）
	std::sort(g_RankingList.begin(), g_RankingList.end(), [](const GameResultData& a, const GameResultData& b) {
		return a.killCount >= b.killCount;
		});
	// 最大数を超えたら削除
	if (g_RankingList.size() > MAX_RANKING) {
		g_RankingList.resize(MAX_RANKING);
	}
}