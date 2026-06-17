/*
	ゲーム本体：game.cpp

	2025/06/27		hibiki sakuma
*/

#include "Game.h"
#include "camera.h"
#include "meshfield.h"
#include "Audio.h"
#include "light.h"
#include "achievementmanager.h"
#include "sampler.h"
#include "shader3d.h"
#include "shaderfield.h"
#include "billboard.h"
#include "direct3d.h"
#include "Key_logger.h"
#include "pad_logger.h"
#include "mouse.h"
#include "mouse_cursor.h"
#include "player.h"
#include "player_camera.h"
#include "UI.h"
#include "bullet.h"
#include "bullet_enemy.h"
#include "sprite_animu.h" // UIで使う場合はmainで呼ぶ
#include "bullet_hit_effect.h"
#include "trajectory3d.h"
#include "map_camera.h"
#include "light_camera.h"
#include "texture.h"
#include "sprite.h"
#include "model.h"
#include "map.h"
#include "skydome.h"
#include "enemymanager.h"
#include "fade.h"
#include "scene.h"
#include "resource_manager.h"
#include "Result.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <random>
using namespace DirectX;

// 定数宣言
static constexpr int LEVEL_UP_CHOICE = 3;
// ミニマップ用定数
static constexpr int MIN_MAP_SIZE = 256;
// ポーズメニュー用定数
static constexpr float LOGO_SIZE = 516.0f;
static constexpr float BTN_WIDTH = 400.0f;
static constexpr float BTN_HEIGHT = 80.0f;
// ダメージ値
static constexpr int PLAYER_BULLET_DAMAGE = 10;
static constexpr int ENEMY_BULLET_DAMAGE = 25;
static constexpr int ENEMY_CONTACT_DAMAGE = 50;
static constexpr int FORCE_KILL_DAMAGE = 999;
// 位置補正
static constexpr float HIT_EFFECT_Y_OFFSET = 0.5f;

// 変数宣言
static double g_AccumulatedTime = 0.0;
static bool g_IsEnd = false; // ゲーム終了検知
static bool g_IsPause = false; // ポーズ中かどうか
static int g_DefeatedEnemy = 0; // 倒した敵の数を保存
static double g_gameTimer = 0.0; // ゲームタイマー
static double g_totalSurvivedSeconds = 0.0; // 生存時間
static Player* g_pPlayer = nullptr; // プレーヤーの情報
// ボタンの接触判定用フラグ
static bool g_Pause_Button_ToTitle = false;
static bool g_LevelUp_Button[LEVEL_UP_CHOICE] = { false,false,false };
static XMFLOAT2 g_LevelUp_Btn_Pos[LEVEL_UP_CHOICE]{}; // 3つのボタン用
static Tex_ID g_LevelUp_Tex[LEVEL_UP_CHOICE] = { LevelUP_HP, LevelUP_SPEED, LevelUP_Bullet };
static XMFLOAT2 g_TitleButton_Position{};

// 内部関数の宣言
namespace
{
	void UpdatePauseInput();
	void UpdatePauseMenu(double elapsed_time);
	bool UpdateLevelUpSelection(double elapsed_time);

	void UpdateWorld(double elapsed_time);
	void UpdateEffects(double elapsed_time);

	void UpdateCombatCollision();
	void UpdatePlayerBulletsVsEnemies();
	void UpdateEnemyBulletsVsPlayer();
	void UpdatePlayerVsEnemies();

	void UpdateGameEnd();
	void HandleResultTransition();

	bool IsPauseTriggered();
	bool IsDecideTriggered();

	void MapRendering();
	Box GetButtonBox(DirectX::XMFLOAT2 position);
}

// 内部関数の実装
namespace
{
	// ポーズ関係
	bool IsPauseTriggered(){
		if (PadLogger_IsConnected()){
			return PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_START);
		}

		return KeyLogger_IsTrigger(KK_TAB);
	}
	void UpdatePauseInput() {
		bool pauseTriggered = false;

		if (PadLogger_IsConnected()) {
			pauseTriggered = PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_START);
		} else {
			pauseTriggered = KeyLogger_IsTrigger(KK_TAB);
		}

		if (pauseTriggered){
			g_IsPause = !g_IsPause;

			if (g_IsPause) {
				g_Pause_Button_ToTitle = false;
				Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
				Mouse_SetVisible(true);
			} else if (!g_pPlayer->IsLevelUpPending()) {
				Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
				Mouse_SetVisible(false);
			}

		}
	}

	void UpdatePauseMenu(double elapsed_time)
	{
		bool isDecideTriggered = false;

		if (PadLogger_IsConnected()) {
			XMFLOAT2 stick = PadLogger_GetLeftThumbStick(0);
			MouseCursor_UpdateWithStick(stick.x, stick.y, elapsed_time);
			isDecideTriggered = PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_A);
		}
		else {
			Mouse_State ms{};
			Mouse_GetState(&ms);
			MouseCursor_UpdateWithMouse(ms.x, ms.y);
			isDecideTriggered = ms.leftButton;
		}

		g_Pause_Button_ToTitle =
			Collision_IsOverlapBox(GetButtonBox(g_TitleButton_Position), MouseCursor_GetCollision());

		MouseCursor_IsHit(g_Pause_Button_ToTitle);

		if (isDecideTriggered && g_Pause_Button_ToTitle) {
			g_Pause_Button_ToTitle = true;
			Fade_Start(1.0, true);
			PlayAudio(Resouce_Manager_GetAudioId(Check_SE));
		}

		if (Fade_GetState() == FADE_STATE_FINISHED_OUT && g_Pause_Button_ToTitle) {
			Scene_Change(SCENE_TITLE);
		}
	}


	// レベルアップ関係
	bool IsDecideTriggered() {
		if (PadLogger_IsConnected()) {
			return PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_A);
		}

		Mouse_State ms{};
		Mouse_GetState(&ms);
		return ms.leftButton;
	}

	bool UpdateLevelUpSelection(double elapsed_time)
	{
		if (!GetPlayer()->IsLevelUpPending()) {
			return false;
		}

		// レベルアップ中はマウスを自由移動モードにする
		Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		Mouse_SetVisible(false);

		Mouse_State ms{};
		bool anyHit = false;

		// 入力モードに応じてカーソル更新
		if (PadLogger_IsConnected()) {
			XMFLOAT2 stick = PadLogger_GetLeftThumbStick(0);
			MouseCursor_UpdateWithStick(stick.x, stick.y, elapsed_time);
		} else {
			Mouse_GetState(&ms);
			MouseCursor_UpdateWithMouse(ms.x, ms.y);
		}

		// 3つのボタンとの当たり判定を更新
		for (int i = 0; i < LEVEL_UP_CHOICE; i++) {
			g_LevelUp_Button[i] = Collision_IsOverlapBox(GetButtonBox(g_LevelUp_Btn_Pos[i]), MouseCursor_GetCollision());

			if (g_LevelUp_Button[i]) {
				anyHit = true;
			}
		}

		// カーソルの見た目更新
		MouseCursor_IsHit(anyHit);

		if (IsDecideTriggered()) {
			if (g_LevelUp_Button[0]) g_pPlayer->ApplyUpgrade(UpgradeOption::MaxHP);
			if (g_LevelUp_Button[1]) g_pPlayer->ApplyUpgrade(UpgradeOption::MoveSpeed);
			if (g_LevelUp_Button[2]) g_pPlayer->ApplyUpgrade(UpgradeOption::ShotInterval);
		
			// 選択されたらマウスを相対モードに戻す
			if (!g_pPlayer->IsLevelUpPending()) {
				Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
				Mouse_SetVisible(false);
			}
		}
		return true;
	}

	// 通常更新
	void UpdateWorld(double elapsed_time) {
		GetPlayer()->Update(elapsed_time);
		Player_Camera_Update(elapsed_time);
		UI_Update(elapsed_time);

		Map_UpDate(elapsed_time);

		Bullet_Update(elapsed_time);
		Bullet_Enemy_Update(elapsed_time);
		EnemyManager::Update(elapsed_time);
	}

	void UpdateEffects(double elapsed_time) {
		Trajectory3d_Update(elapsed_time);
		Bullet_HitEffect_Update();

		// 実績：生存時間
		g_gameTimer += elapsed_time;
		if (g_gameTimer >= 1.0f) {
			g_totalSurvivedSeconds++;
			g_gameTimer = 0.0;

			AchievementManager::Instance().OnNotify(
				"SESSION_TIME",
				(int)g_totalSurvivedSeconds
			);
		}


		// 実績：クリアプレイヤー（今回は1人）
		if (UI_GetTime() <= 0.0) {
			AchievementManager::Instance().OnNotify("CLEAR_PLAYER", 1);
		}

	}


	// 戦闘判定
	void UpdateCombatCollision() {
		UpdatePlayerBulletsVsEnemies();
		UpdateEnemyBulletsVsPlayer();
		UpdatePlayerVsEnemies();
	}

	void UpdatePlayerBulletsVsEnemies() {
		for (int i = 0; i < Bullet_GetBulletCount(); i++) {
			if (!Bullet_IsActive(i)) continue;

			BulletRay ray = Bullet_GetRay(i);
			bool hit_something = false;

			float nearestT_map = 1.0f;
			for (int j = 0; j < Map_GetObjectsCount(); j++) {
				AABB object = Map_GetObject(j)->Aabb_collision;
				float t;
				XMFLOAT3 normal;
				if (Collision_RaycastAABB(ray.start, ray.end, object, t, normal)) {
					if (t < nearestT_map) {
						nearestT_map = t;
						hit_something = true;
					}
				}
			}
			if (hit_something) {
				XMFLOAT3 hitPos = {
					ray.start.x + (ray.end.x - ray.start.x) * nearestT_map,
					ray.start.y + (ray.end.y - ray.start.y) * nearestT_map - HIT_EFFECT_Y_OFFSET,
					ray.start.z + (ray.end.z - ray.start.z) * nearestT_map
				};
				Bullet_HitEffect_Create(hitPos);
				Bullet_Deactivate(i);
				PlayAudio(Resouce_Manager_GetAudioId(Bullet_Hit_SE));
				continue; // この弾は消えたので次の弾（i+1）へ
			}

			float nearestT_enemy = 1.0f;
			int hitEnemyIdx = -1;
			for (int j = 0; j < EnemyManager::GetMaxCount(); j++) {
				Enemy* e = EnemyManager::GetEnemy(j);
				if (!e || !e->IsActive()) continue;
				Sphere enemy = e->GetCollision();
				float t;
				XMFLOAT3 normal;
				if (Collision_RaycastSphere(ray.start, ray.end, enemy, t, normal)) {
					if (t < nearestT_enemy) {
						nearestT_enemy = t;
						hitEnemyIdx = j;
					}
				}
			}

			if (hitEnemyIdx != -1) {
				XMFLOAT3 hitPos = {
					ray.start.x + (ray.end.x - ray.start.x) * nearestT_enemy,
					ray.start.y + (ray.end.y - ray.start.y) * nearestT_enemy - HIT_EFFECT_Y_OFFSET,
					ray.start.z + (ray.end.z - ray.start.z) * nearestT_enemy
				};
				Bullet_HitEffect_Create(hitPos);
				Bullet_Deactivate(i);
				PlayAudio(Resouce_Manager_GetAudioId(Bullet_Hit_SE));

				Enemy* enemy = EnemyManager::GetEnemy(hitEnemyIdx);
				int prevHP = enemy->GetHP();
				enemy->Damage(PLAYER_BULLET_DAMAGE);

				if (prevHP > 0 && enemy->GetHP() <= 0) {
					g_DefeatedEnemy++;
					g_pPlayer->AddExp(1);
					AchievementManager::Instance().OnNotify("ENEMY_KILLED", 1);
				}

				continue; // この弾は消えたので次の弾（i+1）へ
			}
		}
	}
	void UpdateEnemyBulletsVsPlayer() {
		for (int i = 0; i < Bullet_Enemy_GetBulletCount(); i++) {
			if (!Bullet_Enemy_IsActive(i)) continue;

			// 簡易的に点と球（プレイヤー）の判定、またはレイキャスト
			if (Collision_IsHitCapsuleVsSphere(g_pPlayer->GetCapsule(), Bullet_Enemy_GetSphere(i).center_position, Bullet_Enemy_GetSphere(i).radius).isHit) {
				g_pPlayer->Damage(ENEMY_BULLET_DAMAGE);    // プレイヤーにダメージ
				Bullet_Enemy_Deactivate(i);  // 弾を消す
			}
		}
	}
	void UpdatePlayerVsEnemies() {
		for (int j = 0; j < EnemyManager::GetMaxCount(); j++) { // マップの当たり判定を総当たり
			Enemy* e = EnemyManager::GetEnemy(j);
			if (!e || !e->IsActive()) continue;
			Sphere enemy = e->GetCollision();
			if (Collision_IsOverlapSphere(enemy, g_pPlayer->GetPosition())) {
				e->Damage(FORCE_KILL_DAMAGE);
				g_pPlayer->Damage(ENEMY_CONTACT_DAMAGE);
			}
		}
	}

	// ゲーム終了
	void UpdateGameEnd() {
		if (g_IsEnd) return;

		if (GetPlayer()->GetHp() <= 0.0f) {
			g_IsEnd = true;
			Fade_Start(1.0, true);
		}

		if (UI_GetTime() <= 0.0) {
			g_IsEnd = true;
			Fade_Start(1.0, true);
		}
	}

	void HandleResultTransition()
	{
		if (Fade_GetState() == FADE_STATE_FINISHED_OUT && g_IsEnd) {
			Result_SetData(GetPlayer()->GetLevel(), g_DefeatedEnemy);
			Scene_Change(SCENE_RESULT);
		}
	}

	// マップ描画用
	void MapRendering()	{
		// レンダーターゲットをテクスチャへ
		Direct3D_SetOffscreen();
		Direct3D_ClearOffscreen();

		// マップカメラ行列の設定
		XMFLOAT3 position = g_pPlayer->GetPosition();
		position.y = 75.0f;
		MapCamera_SetPosition(position);
		MapCamera_SetFront(Player_Camera_GetFront());
		XMFLOAT4X4 mtxView = MapCamera_GetViewMatrix();
		XMFLOAT4X4 mtxProj = MapCamera_GetPerspectiveMatrix();
		XMMATRIX view = XMLoadFloat4x4(&mtxView);
		XMMATRIX proj = XMLoadFloat4x4(&mtxProj);

		//カメラに関する行列をシェーダーに設定する
		Camera_SetMatrix(view, proj);

		// テクスチャ―サンプラーの設定
		Sampler_SetFilterAnisotropic();

		// 並行光を黒にして環境光のみにするか、並行光を下向きにするか、ライトはゲームのままか
		Light_SetAmbient({ 1.0f,1.0f,1.0f });
		Light_SetDirectionalWorld({ 0.0f,0.0f,0.0f,1.0f }, { 0.0f,0.0f,0.0f,0.0f });

		// 深度有効
		Direct3D_SetDepthEnable(true);

		EnemyManager::Draw();
		g_pPlayer->Draw();
		Map_Draw();
	}

	Box GetButtonBox(DirectX::XMFLOAT2 position) {
		return { {position.x + BTN_WIDTH * 0.5f, position.y + BTN_HEIGHT * 0.5f}, BTN_WIDTH * 0.5f, BTN_HEIGHT * 0.5f };
	}

}


void Game_Initialize() {
	g_pPlayer = new Player();
	g_pPlayer->Initialize({ 3.0f, 1.0f, 2.0f }, { 0.0f, 0.0f, -1.0f }, Player1);
	UI_Initialize();
	EnemyManager::Initialize();
	Camera_Initialize(); // デバックカメラ　座標検索用
	Player_Camera_Initialize();
	Map_Initialize();
	MeshField_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Bullet_Initialize({ 0.0f, 1.0f, 3.0f }, { 0.0f,0.0f,-1.0f });
	Bullet_Enemy_Initialize();
	Skydome_Initialize();
	Billboard_Initialize();
	Bullet_HitEffect_Initialize();
	Trajectory3d_Initialize();
	LightCamera_Initialize({-1.0f,-1.0f,1.0f},{100.0f,100.0f,-100.0f});
	MouseCursor_Initialize();

	// 変数初期化
	g_IsEnd = false;
	g_IsPause = false;
	g_DefeatedEnemy = 0;
	g_gameTimer = 0.0;
	g_totalSurvivedSeconds = 0.0;
	// ポーズ中のボタンは配置の初期化
	g_TitleButton_Position = { Direct3D_GetBackBufferWidth() * 0.5f - BTN_WIDTH * 0.5f,Direct3D_GetBackBufferHeight() * 0.7f}; // タイトル画面へ
	// レベルアップボタンの配置の初期化
	float startX = Direct3D_GetBackBufferWidth() * 0.5f;
	float startY = Direct3D_GetBackBufferHeight() * 0.5f;
	for (int i = 0; i < LEVEL_UP_CHOICE; i++) {
		// 縦並びに配置
		g_LevelUp_Btn_Pos[i] = { startX - BTN_WIDTH * 0.5f, startY - BTN_WIDTH * 0.5f + (i * Direct3D_GetBackBufferHeight() * 0.2f) };
	}
	// BGM再生
	PlayAudio(Resouce_Manager_GetAudioId(Game_BGM), true);

	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);

	//画面遷移　フェードイン処理
	Fade_Start(0.1, false); // タイトルからつながっている

}

void Game_Finalize() {
	// BGMを止める
	StopAudio(Resouce_Manager_GetAudioId(Game_BGM));
	// ゲーム終了時の結果を送信
	Result_SetData(GetPlayer()->GetLevel(), g_DefeatedEnemy);
	MouseCursor_Finalize();
	EnemyManager::Finalize();
	Trajectory3d_Finalize();
	Bullet_HitEffect_Finalize();
	Billboard_Finalize();
	Map_Finalize();
	Bullet_Enemy_Finalize();
	Bullet_Finalize();
	Player_Camera_Finalize();
	Camera_Finalize();	
	UI_Finalize();
	g_pPlayer->Finalize();
	delete g_pPlayer;
	Skydome_Finalize();
}

void Game_UpDate(double elapsed_time){

	UpdatePauseInput();

	if (g_IsPause){
		UpdatePauseMenu(elapsed_time);
		Fade_Update(elapsed_time);
		HandleResultTransition();
		return;
	}

	// レベルアップ3択中は通常ゲーム進行を止める
	if (UpdateLevelUpSelection(elapsed_time)){
		Fade_Update(elapsed_time);
		HandleResultTransition();
		return;
	}

	UpdateWorld(elapsed_time);
	UpdateEffects(elapsed_time);
	UpdateCombatCollision();
	UpdateGameEnd();

	Fade_Update(elapsed_time);
	HandleResultTransition();
}

void Game_Draw() {
	MapRendering(); // オフスクリーンレンダリング
	
	// レンダーターゲットをバックバッファへ
	Direct3D_SetBackBuffer();
	Direct3D_ClearBackBuffer();

	XMFLOAT4X4 mtxView = Player_Camera_GetViewMatrix();
	XMMATRIX view = XMLoadFloat4x4(&mtxView);
	XMMATRIX proj = XMLoadFloat4x4(&Player_Camera_GetPerspectiveMatrix());
	// スペキュラ用
	XMFLOAT3 camera_pos = Player_Camera_GetPosition();
	
	// カメラに関する行列をシェーダーに設定する
	Camera_SetMatrix(view, proj);
	
	// ビルボードにビュー行列をセットする
	Billboard_SetViewMatrix(mtxView);

	//テクスチャーサンプラーの設定
	Sampler_SetFilterAnisotropic();
	
	// 空の表示
	Direct3D_SetDepthEnable(false);
	Skydome_Draw();
	Direct3D_SetDepthEnable(true);

	// 各種ライトの設定
	// 環境光
	XMFLOAT3 amb_c = { 0.2f,0.2f,0.2f };
	Light_SetAmbient(amb_c); // 空間全体、ほかのライトで当たらないところのイメージ
	// 並行光源
	XMVECTOR v{ -1.0f,-1.0f,1.0f };
	v = XMVector3Normalize(v);
	XMFLOAT4 dir;
	XMStoreFloat4(&dir, v);
	Light_SetDirectionalWorld(dir, {1.0f,0.9f,0.7f,1.0f }/*色*/);
	// スペキュラー
	Light_SetSpecularWorld(camera_pos, 50.0f, { 0.8f,0.8f,0.8f,1.0f });
	
	// 敵の描画
	EnemyManager::Draw();
	// マップ描画
	Map_Draw();
	// プレイヤー描画
	g_pPlayer->Draw();
#if defined(DEBUG) || defined(_DEBUG) 
	Collision_DebugDraw_Execute();
#endif
	// アルファテスト
	Direct3D_SetDepthWriteDisable();
	// 弾のエフェクトの描画
	Bullet_HitEffect_Draw();
	Trajectory3d_Draw();
	Direct3D_SetDepthEnable(true);

	// ココから2DのUI表示 //
	Direct3D_SetDepthEnable(false);

	Sprite_Begin();// スプライト描き始め

	// UIの表示
	UI_Draw();
	// ミニマップの描画
	Direct3D_SetOffscreenTexture(0);
	Sprite_Draw((float)Direct3D_GetBackBufferWidth() * 0.8f, (float)Direct3D_GetBackBufferHeight() * 0.04f, MIN_MAP_SIZE, MIN_MAP_SIZE);
	
	// レベルアップ時の表示
	if (g_pPlayer->IsLevelUpPending()) {
		// 半透明の黒いパネルを出す
		Sprite_Draw(Resouce_Manager_GetTexId(Color_Black), 0.0f, 0.0f, (float)Direct3D_GetBackBufferWidth(), (float)Direct3D_GetBackBufferHeight(), { 1.0f, 1.0f, 1.0f, 0.5f });
		// 「LEVEL UP」という画像を表示
		Sprite_Draw(Resouce_Manager_GetTexId(LevelUP), (float)Direct3D_GetBackBufferWidth() * 0.5f - LOGO_SIZE * 0.5f, Direct3D_GetBackBufferHeight() * 0.1f - LOGO_SIZE * 0.5f, LOGO_SIZE, LOGO_SIZE);
		// 三択の選択肢の表示
		for (int i = 0; i < LEVEL_UP_CHOICE; i++) {
			XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			if (g_LevelUp_Button[i]) color.w = 0.5f; // マウスが乗っていたら半透明に
			Sprite_Draw(Resouce_Manager_GetTexId(g_LevelUp_Tex[i]), g_LevelUp_Btn_Pos[i].x, g_LevelUp_Btn_Pos[i].y, BTN_WIDTH, BTN_HEIGHT, color);
		}

		MouseCursor_Draw();
	}

	// ポーズ中だけ上に重ねて表示
	if (g_IsPause) {
		// 半透明の黒いパネルを出す
		Sprite_Draw(Resouce_Manager_GetTexId(Color_Black), 0.0f, 0.0f, (float)Direct3D_GetBackBufferWidth(), (float)Direct3D_GetBackBufferHeight(), {1.0f, 1.0f, 1.0f, 0.5f});
		// 「PAUSE」という画像を表示
		Sprite_Draw(Resouce_Manager_GetTexId(Pause), (float)Direct3D_GetBackBufferWidth() * 0.5f - LOGO_SIZE * 0.5f, Direct3D_GetBackBufferHeight() * 0.5f - LOGO_SIZE * 0.5f, LOGO_SIZE, LOGO_SIZE);
		// タイトルに戻るボタンの描画
		if (g_Pause_Button_ToTitle) {
			Sprite_Draw(Resouce_Manager_GetTexId(Title_Button), (float)Direct3D_GetBackBufferWidth() * 0.5f - BTN_WIDTH * 0.5f, Direct3D_GetBackBufferHeight() * 0.7f, BTN_WIDTH, BTN_HEIGHT, { 1.0f,1.0f,1.0f,0.5f });
		} else {
			Sprite_Draw(Resouce_Manager_GetTexId(Title_Button), (float)Direct3D_GetBackBufferWidth() * 0.5f - BTN_WIDTH * 0.5f, Direct3D_GetBackBufferHeight() * 0.7f, BTN_WIDTH, BTN_HEIGHT);
		}
		MouseCursor_Draw();
	}
}

