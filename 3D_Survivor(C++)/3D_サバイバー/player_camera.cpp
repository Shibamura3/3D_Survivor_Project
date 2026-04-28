/*
	プレイヤーカメラ制御：player_camera.cpp

	2025/10/31	hibiki sakuma
*/
// サードパーソン用　ファーストパーソンの場合はプレイヤーのモデルを非表示にしたり手のみ映す
#include "player_camera.h"
#include "player.h"
#include "direct3d.h"
#include "Key_logger.h"
#include "pad_logger.h"

#include <DirectXMath.h>
using namespace DirectX;

//変数宣言
static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; 
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; 

static XMFLOAT3 g_Player_CameraFront = { 0.0f, 0.0f, 1.0 }; 
static XMFLOAT3 g_Player_CameraPosition = { 0.0f, 0.0f, 0.0f };
static XMFLOAT4X4 g_Player_CameraMatrix{};
static XMFLOAT4X4 g_Player_CameratPerspectMatrix{};

// 回転制御用の静的変数
static float g_CameraAngleX = 0.0f;       // 水平回転角
static const float g_CameraAngleY = 0.5f;   // 見下ろし角度（固定）
static const float g_CameraDistance = 15.0f; // プレイヤーとの距離
static const float g_sensitivity = 1.0f; // カメラ感度

// 前回のフレームでのマウス座標を保持（絶対座標モードの場合に使用）
static int g_PrevMouseX = 0;

// 定数宣言
static constexpr float CAMERA_ROTATION_KEY = 3.0f; 

void Player_Camera_Initialize() {
	g_CameraAngleX = 0.0f;
	g_PrevMouseX = 0;

	g_Player_CameraFront = { 0.0f, 0.0f, 1.0 };
	g_Player_CameraPosition = { 0.0f, 0.0f, 0.0f };
}

void Player_Camera_Finalize(){

}

void Player_Camera_Update(double elapsed_time){
	//Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE); // 一時的に画面外に行かないようにマウスを相対モード（中央固定）にする
	float rotationInput = 0.0f;

	// パッドが刺さっている間はマウスを無視
	if (PadLogger_IsConnected()) {
		// パッド入力
		XMFLOAT2 rightStick = PadLogger_GetRightThumbStick(0);

		rotationInput -= rightStick.x * 5.0f;
	} else {
		// キー入力（パッドがない時だけ実行）
		if (KeyLogger_IsPressed(KK_LEFT)) {
			rotationInput += CAMERA_ROTATION_KEY; // 左回転
		}
		if (KeyLogger_IsPressed(KK_RIGHT)) {
			rotationInput -= CAMERA_ROTATION_KEY; // 右回転
		}
	}

	// 感度と経過時間を考慮して角度を更新
	g_CameraAngleX += rotationInput * g_sensitivity * (float)elapsed_time;

	// --- 2. カメラ座標の算出（水平回転） ---
	XMVECTOR playerPos = XMLoadFloat3(&GetPlayer()->GetPosition());

	// プレイヤーの座標から「現在の高さ」を抜き出し、
	// ジャンプしても変わらない「基準となる高さ（地面の高さなど）」に差し替える
	float baseHeight = 0.0f; // 地面の高さが0なら0.0f、ステージに合わせて調整
	XMVECTOR stablePos = XMVectorSet(XMVectorGetX(playerPos), baseHeight, XMVectorGetZ(playerPos), 0.0f);

	// 注視点：プレイヤーの少し上（1.5m上）
	XMVECTOR target = stablePos + XMVectorSet(0.0f, 1.5f, 0.0f, 0.0f);

	// 球面座標から座標を計算
	// Y軸固定の円運動
	float offsetX = g_CameraDistance * cosf(g_CameraAngleY) * sinf(g_CameraAngleX);
	float offsetY = g_CameraDistance * sinf(g_CameraAngleY);
	float offsetZ = g_CameraDistance * cosf(g_CameraAngleY) * cosf(g_CameraAngleX);

	XMVECTOR position = target + XMVectorSet(offsetX, offsetY, -offsetZ, 0.0f);

	// --- 3. 行列の作成と保存 ---
	XMVECTOR front = XMVector3Normalize(target - position);
	XMStoreFloat3(&g_Player_CameraPosition, position);
	XMStoreFloat3(&g_Player_CameraFront, front);

	// ビュー行列
	XMMATRIX mtmView = XMMatrixLookAtLH(position, target, { 0.0f, 1.0f, 0.0f });
	XMStoreFloat4x4(&g_Player_CameraMatrix, mtmView);

	// プロジェクション行列
	float aspectRatio = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 1000.0f);
	XMStoreFloat4x4(&g_Player_CameratPerspectMatrix, mtxPerspective);
}

const DirectX::XMFLOAT3& Player_Camera_GetPosition(){
	return g_Player_CameraPosition;
}

const DirectX::XMFLOAT3& Player_Camera_GetFront(){
	return g_Player_CameraFront;
}

const DirectX::XMFLOAT4X4& Player_Camera_GetViewMatrix(){
	return g_Player_CameraMatrix;
}

const DirectX::XMFLOAT4X4& Player_Camera_GetPerspectiveMatrix(){
	return g_Player_CameratPerspectMatrix;
}