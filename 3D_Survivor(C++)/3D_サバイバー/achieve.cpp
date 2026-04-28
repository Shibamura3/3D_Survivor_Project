/*
	実績画面の制御：achieve.h

	2025/12/19	hibiki sakuma
*/
#include "achieve.h"
#include "achievementmanager.h"
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
#include <SpriteFont.h>
#include <SpriteBatch.h>

//定数宣言
static constexpr float SELECT_SIZE_X = 600.0f;
static constexpr float SELECT_SIZE_Y = 100.0f;
static constexpr float STAGE_SIZE_X = 200.0f;
static constexpr float STAGE_SIZE_Y = 50.0f;
static constexpr float ICON_SIZE = 128.0f;

static constexpr float ACHIEVE_START_X = 200.0f; // 一覧の左端
static constexpr float ACHIEVE_START_Y = 200.0f; // 一覧の上端
static constexpr float ACHIEVE_STEP_Y = 80.0f;  // 実績ごとの縦間隔

// 音源読み込み用
static int g_Check_SE = -1;
static int g_BGM = -1;

// 変数宣言
static bool g_Button_Title = false;
// マウス用
static float g_ScrollY = 0.0f;          // 現在のスクロールオフセット
static constexpr float SCROLL_SPEED = 20.0f; // スクロール速度

// スマートポインタで安全に管理
static std::unique_ptr<DirectX::SpriteBatch> g_FontBatch;
static std::unique_ptr<DirectX::SpriteFont>  g_Font;

static XMFLOAT2 g_ButtonPos{}; // ボタンの中心座標
void Achieve_Initialize(){
	//画面遷移　フェードイン処理
	Fade_Start(1.0, false); // タイトルからつながっている
	// マウスを絶対座標に
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);

	//音源の読み込み
	g_Check_SE = LoadAudio("resuce/audio/SE_check.wav");
	g_BGM = LoadAudio("resuce/audio/bgm.wav");

	// カーソルボタンの接触判定
	g_Button_Title = false;

	// ボタンの座標の設定
	// タイトルボタン
	g_ButtonPos = { Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X, Direct3D_GetBackBufferHeight() * 0.75f };

	// フォント関連の初期化
	// SpriteBatchの生成
	g_FontBatch = std::make_unique<DirectX::SpriteBatch>(Direct3D_GetContext());
	// フォントファイルの読み込み
	// ※パスは自分のプロジェクト構成に合わせて調整してください
	g_Font = std::make_unique<DirectX::SpriteFont>(Direct3D_GetDevice(), L"resuce/data/font/myfont.spritefont");
	// 知らない文字を？で代用
	if (g_Font) {
		g_Font->SetDefaultCharacter(L'?');
	}
	//BGMの再生
	//PlayAudio(g_BGM, true);
	
	// マウスカーソルの設定
	MouseCursor_Initialize();

}

void Achieve_Finalize(){
	//Texture_AllRelease();
	// サウンドストップはココに入れる
	UnloadAudio(g_Check_SE);
	//UnloadAudio(g_Miss_SE);
	UnloadAudio(g_BGM);
	MouseCursor_Finalize();

	g_Font.reset();
	g_FontBatch.reset();

}

void Achieve_Update(double elapsed_time){
	// マウス状態の取得
	Mouse_State ms{};
	Mouse_GetState(&ms);

	// スクロール処理 (ホイール)
	// scrollWheelValue は 1ノッチにつき 120 (WHEEL_DELTA) という値が入るのが一般的です
	if (ms.scrollWheelValue != 0) {
		// 感度調整のため 0.5f などを掛ける
		g_ScrollY -= static_cast<float>(ms.scrollWheelValue) * 0.5f;

		// 値を読み取ったら累計値をリセットする（重要！）
		Mouse_ResetScrollWheelValue();
	}

	// スクロール処理 (ゲームパッドの右スティック等でも動かせると親切)
	if (PadLogger_IsConnected()) {
		XMFLOAT2 rightStick = PadLogger_GetRightThumbStick(0); // 右スティック
		g_ScrollY -= rightStick.y * 10.0f; // スティックの倒し具合で加算
	}

	// 3. スクロールの範囲制限（これがないと無限に上や下へ行ってしまう）
	const auto& achievements = AchievementManager::Instance().GetAll();
	float totalHeight = (achievements.size() * ACHIEVE_STEP_Y) + 150.0f; // 150.0f の余白を追加
	float windowHeight = Direct3D_GetBackBufferHeight() * 0.7f; // 表示領域の高さ

	if (g_ScrollY < 0.0f) g_ScrollY = 0.0f; // 上限
	if (g_ScrollY > totalHeight - windowHeight) {
		g_ScrollY = totalHeight - windowHeight; // 下限
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
	}
	else {
		// マウス：OSの座標でカーソル移動
		//Mouse_State ms{};
		//Mouse_GetState(&ms);
		MouseCursor_UpdateWithMouse(ms.x, ms.y);

		// 左クリックを決定にする
		isDecideTriggered = ms.leftButton;
	}

	// ボタンとカーソルの接触判定
	g_Button_Title = Collision_IsOverlapBox(Achieve_GetCollision(), MouseCursor_GetCollision());

	// カーソルがボタンに乗ったかどうかの通知
	MouseCursor_IsHit(g_Button_Title);

	// 決定操作が行われた場合
	if (isDecideTriggered && (g_Button_Title)) {
		Fade_Start(1.0, true);
		//PlayAudio(g_Check_SE);
	}

	if (Fade_GetState() == FADE_STATE_FINISHED_OUT) { // フェードが終わったらシーンチェンジ
		Scene_Change(SCENE_TITLE); // タイトル画面へ
	}
}

void Achieve_Draw(){
	Direct3D_SetDepthEnable(false);
	Sprite_Begin();// スプライト描き始め

	// 画面の背景
	Sprite_Draw(Resouce_Manager_GetTexId(Title_Back), 0.0f, 0.0f, (float)Direct3D_GetBackBufferWidth(), (float)Direct3D_GetBackBufferHeight());
	// ウィンドウ
	Sprite_Draw(Resouce_Manager_GetTexId(Color_Black), Direct3D_GetBackBufferWidth() * 0.1f, Direct3D_GetBackBufferHeight() * 0.1f, Direct3D_GetBackBufferWidth() * 0.8f, Direct3D_GetBackBufferHeight() * 0.8f, { 1, 1, 1, 0.5f });
	
	const auto& achievements = AchievementManager::Instance().GetAll();
	float currentY = ACHIEVE_START_Y - g_ScrollY; // スクロール分を引く

	for (const auto& ach : achievements) {
		if (currentY > 100.0f && currentY < 650.0f) {
			int texId = ach.isUnlocked ? Resouce_Manager_GetTexId(Achieve_UnLock): Resouce_Manager_GetTexId(Achieve_Lock);
			XMFLOAT4 iconColor = ach.isUnlocked ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
			Sprite_Draw(texId, ACHIEVE_START_X, currentY, 64.0f, 64.0f, iconColor);
		}
		currentY += ACHIEVE_STEP_Y;
	}

	currentY = ACHIEVE_START_Y - g_ScrollY;

	g_FontBatch->Begin();

	for (const auto& ach : achievements) {
		if (currentY > 100.0f && currentY < 650.0f) {
			XMVECTOR colorVec = ach.isUnlocked ? Colors::Gold : Colors::LightGray;
			g_Font->DrawString(
				g_FontBatch.get(),
				ach.title.c_str(),
				XMFLOAT2(ACHIEVE_START_X + 80.0f, currentY + 15.0f),
				colorVec
			);
		}
		currentY += ACHIEVE_STEP_Y;
	}
	g_FontBatch->End();
	
	// 背景のα情報を元に戻す
	Direct3D_SetAlphaBlendTransparent();
	// 選択対象
	if (!g_Button_Title) {
		Sprite_Draw(Resouce_Manager_GetTexId(Title_Button), Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X * 0.5f, Direct3D_GetBackBufferHeight() * 0.75f, SELECT_SIZE_X, SELECT_SIZE_Y);  // 選択された時は正常に表示
	}
	else {
		Sprite_Draw(Resouce_Manager_GetTexId(Title_Button), Direct3D_GetBackBufferWidth() * 0.5f - SELECT_SIZE_X * 0.5f, Direct3D_GetBackBufferHeight() * 0.75f, SELECT_SIZE_X, SELECT_SIZE_Y, { 1.0f,1.0f,1.0f,0.5f });
	} // 選択されていないときは半透明に
	
	// 操作アイコンの表示
	if (PadLogger_IsConnected()) {
		Sprite_Draw(Resouce_Manager_GetTexId(Icon_Pad), Direct3D_GetBackBufferWidth() * 0.9f, Direct3D_GetBackBufferHeight() * 0.85f, ICON_SIZE, ICON_SIZE);
	} else {
		Sprite_Draw(Resouce_Manager_GetTexId(Icon_Keyboard), Direct3D_GetBackBufferWidth() * 0.9f, Direct3D_GetBackBufferHeight() * 0.85f, ICON_SIZE, ICON_SIZE);
	}
	// マウスカーソルの描画
	MouseCursor_Draw();

#if defined(DEBUG) || defined(_DEBUG) 
	Collision_DebugDraw(Achieve_GetCollision());
#endif

}

Box Achieve_GetCollision()
{
	return { {g_ButtonPos.x + SELECT_SIZE_X, g_ButtonPos.y + SELECT_SIZE_Y * 0.5f}, SELECT_SIZE_X * 0.5f, SELECT_SIZE_Y * 0.5f };
}
