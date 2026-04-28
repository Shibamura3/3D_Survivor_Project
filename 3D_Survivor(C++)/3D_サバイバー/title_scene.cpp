/*
	タイトル画面の制御：title_scene.h

	2025/12/19	hibiki sakuma
*/

#include "title_scene.h"
#include "resource_manager.h"
#include "sprite.h"
#include "Key_logger.h"
#include "fade.h"
#include "scene.h"
#include "Audio.h"
#include "direct3d.h"
#include "mouse.h"
#include "mouse_cursor.h"
#include "pad_logger.h"
#include <DirectXMath.h>
using namespace DirectX;

//定数宣言
static constexpr float TITLE_LOGO_X = 480.0f;
static constexpr float TITLE_LOGO_Y = 96.0f;
static constexpr float SELECT_SIZE_X = 600.0f;
static constexpr float SELECT_SIZE_Y = 100.0f;
static constexpr float STAGE_SIZE_X = 200.0f;
static constexpr float STAGE_SIZE_Y = 50.0f;
static constexpr float ICON_SIZE = 128.0f;

static constexpr int BUTTON_NUM = 2;

// 変数宣言
static double g_AccumulatedTime = 0.0; // 経過時間
static double g_scene_timer = 0;
static bool g_Button_Start = false;
static bool g_Button_Achieve = false;
static bool g_Scene_Start = false;
static bool g_Scene_Achieve = false;

static XMFLOAT2 g_ButtonPos[BUTTON_NUM]{}; // ボタンの中心座標

void TitleScene_Initialize() {
	//画面遷移　フェードイン処理
	Fade_Start(1.0, false); // タイトルからつながっている
	// マウスを絶対座標に
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);

	// カーソルボタンの接触判定
	g_Button_Start = false;
	g_Button_Achieve = false;
	// ボタンごとの決定判定
	g_Scene_Start = false;
	g_Scene_Achieve = false;
	
	// ボタンの座標の設定
	// スタートボタン
	g_ButtonPos[0] = { Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X, Direct3D_GetBackBufferHeight() * 0.5f };
	// 実績ボタン
	g_ButtonPos[1] = { Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X, Direct3D_GetBackBufferHeight() * 0.7f };
	// タイマーの初期化
	g_scene_timer = 0;
	//BGMの再生
	PlayAudio(Resouce_Manager_GetAudioId(Title_BGM), true);

	// マウスカーソルの設定
	MouseCursor_Initialize();
}

void TitleScene_Finalize() {
	// タイトルのBGMを止める
	StopAudio(Resouce_Manager_GetAudioId(Title_BGM));
	MouseCursor_Finalize();
}

void TitleScene_Update(double elapsed_time) {
	g_scene_timer += elapsed_time;

	// シーン開始から0.3〜0.5秒くらいは決定操作を無視する
	if (g_scene_timer < 0.5) {
		return;
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

	// ボタンとカーソルの接触判定（MouseCursor_GetCollision は更新後の g_Position を使うのでそのまま使える）
	g_Button_Start = Collision_IsOverlapBox(TitleScene_GetButton(0), MouseCursor_GetCollision());
	g_Button_Achieve = Collision_IsOverlapBox(TitleScene_GetButton(1), MouseCursor_GetCollision());

	// カーソルがボタンに乗ったかどうかの通知
	MouseCursor_IsHit(g_Button_Start || g_Button_Achieve);

	// 決定操作が行われた場合
	if (isDecideTriggered && (g_Button_Start || g_Button_Achieve)) {
		Fade_Start(1.0, true);
		PlayAudio(Resouce_Manager_GetAudioId(Check_SE));
		if (g_Button_Start) g_Scene_Start = true;
		if (g_Button_Achieve) g_Scene_Achieve = true;
	}

	// シーン遷移
	if (Fade_GetState() == FADE_STATE_FINISHED_OUT) {
		if (g_Scene_Start)   Scene_Change(SCENE_GAME);
		if (g_Scene_Achieve) Scene_Change(SCENE_ACHIEVE);
	}

}

void TitleScene_Draw() {
	Direct3D_SetDepthEnable(false);
	Sprite_Begin();// スプライト描き始め

	//タイトル画面の背景移動
	Sprite_Draw(Resouce_Manager_GetTexId(Title_Back), 0.0f, 0.0f, (float)Direct3D_GetBackBufferWidth(), (float)Direct3D_GetBackBufferHeight());
	
	// タイトルロゴ
	Sprite_Draw(Resouce_Manager_GetTexId(Title_Logo), Direct3D_GetBackBufferWidth() * 0.5f - TITLE_LOGO_X, Direct3D_GetBackBufferHeight() * 0.1f, TITLE_LOGO_X * 2.0f, TITLE_LOGO_Y * 2.0f);
	
	// 選択対象
	if (!g_Button_Start) {
		Sprite_Draw(Resouce_Manager_GetTexId(Start_Button), Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X * 0.5f, Direct3D_GetBackBufferHeight() * 0.5f, SELECT_SIZE_X, SELECT_SIZE_Y);  // 選択された時は正常に表示
	} else {
		Sprite_Draw(Resouce_Manager_GetTexId(Start_Button), Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X * 0.5f, Direct3D_GetBackBufferHeight() * 0.5f, SELECT_SIZE_X, SELECT_SIZE_Y, { 1.0f,1.0f,1.0f,0.5f });
	} // 選択されていないときは半透明に
	
	if (!g_Button_Achieve) {
		Sprite_Draw(Resouce_Manager_GetTexId(Achieve_Button), Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X * 0.5f, Direct3D_GetBackBufferHeight() * 0.7f, SELECT_SIZE_X, SELECT_SIZE_Y);
	} else {
		Sprite_Draw(Resouce_Manager_GetTexId(Achieve_Button), Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X * 0.5f, Direct3D_GetBackBufferHeight() * 0.7f, SELECT_SIZE_X, SELECT_SIZE_Y, { 1.0f,1.0f,1.0f,0.5f });
	}
	// 操作アイコンの表示
	if (PadLogger_IsConnected()) {
		Sprite_Draw(Resouce_Manager_GetTexId(Icon_Pad), Direct3D_GetBackBufferWidth() * 0.9f, Direct3D_GetBackBufferHeight() * 0.85f, ICON_SIZE, ICON_SIZE);
	} else {
		Sprite_Draw(Resouce_Manager_GetTexId(Icon_Keyboard), Direct3D_GetBackBufferWidth() * 0.9f, Direct3D_GetBackBufferHeight() * 0.85f, ICON_SIZE, ICON_SIZE);
	}
	// マウスカーソルの描画
	MouseCursor_Draw();

#if defined(DEBUG) || defined(_DEBUG) 
	Collision_DebugDraw(TitleScene_GetButton(0));
	Collision_DebugDraw(TitleScene_GetButton(1));
#endif
}

Box TitleScene_GetButton(int index){
	return { {g_ButtonPos[index].x + SELECT_SIZE_X, g_ButtonPos[index].y + SELECT_SIZE_Y * 0.5f}, SELECT_SIZE_X * 0.5f, SELECT_SIZE_Y * 0.5f};
}

