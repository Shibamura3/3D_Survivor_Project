/*
	カメラ制御：camera.h

	2025/09/09	hibiki sakuma
*/

#include "camera.h"

#include <DirectXMath.h>
#include <sstream>
using namespace DirectX;
#include "direct3d.h"
#include "Key_logger.h"
#include "mouse.h"
#include "debug_text.h"

static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; //
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; //

//変数宣言
static XMFLOAT3 g_CameraPosition = { 0.0f,5.0f,-10.0 };
static Mouse_State g_MouseState{};
// 三方向のベクトル
// たくさんのカメラを使用する際は構造体やクラスを使用
static XMFLOAT3 g_CameraFront = { 0.0f, 0.0f, 1.0 }; // 前 Z軸の＋
static XMFLOAT3 g_CameraUp    = { 0.0f, 1.0f, 0.0 }; // 上 Y軸の＋
static XMFLOAT3 g_CameraRight = { 1.0f, 0.0f, 0.0 }; // 右 X軸の＋

static XMFLOAT4X4 g_CameraMatrix;
static XMFLOAT4X4 g_Perspective;

static float g_Fov = XMConvertToRadians(60);

static hal::DebugText* g_pDT = nullptr;

// 定数宣言
static constexpr float CAMERA_MOVE_SPEED = 3.0f;
static constexpr float CAMERA_ROTATION_SPEED = XMConvertToRadians(30); // 一秒間に30回転

void Camera_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, const DirectX::XMFLOAT3& right) {
	// 念のためInitializeでも初期化

	Camera_Initialize();

	g_CameraPosition = position;

	XMVECTOR I_front = XMVector3Normalize(XMLoadFloat3(&front));
	XMVECTOR I_right = XMVector3Normalize(XMLoadFloat3(&right) * XMVECTOR { 1.0f, 0.0f, 1.0f });
	XMVECTOR up = XMVector3Cross(I_right, I_front);

	XMStoreFloat3(&g_CameraFront, I_front);
	XMStoreFloat3(&g_CameraRight, I_right);
	XMStoreFloat3(&g_CameraUp, up);


	//XMStoreFloat4x4(&g_CameraMatrix, XMMatrixIdentity());
	//XMStoreFloat4x4(&g_Perspective, XMMatrixIdentity());

}

void Camera_Initialize(){
	// 念のためInitializeでも初期化
	g_CameraPosition = { 0.0f,5.0f,-10.0 };
	g_CameraFront = { 0.0f, 0.0f, 1.0 };
	g_CameraUp = { 0.0f, 1.0f, 0.0 }; 
	g_CameraRight = { 1.0f, 0.0f, 0.0 };
	g_Fov = XMConvertToRadians(60);

	XMStoreFloat4x4(&g_CameraMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&g_Perspective, XMMatrixIdentity());

	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

	//本来は容量を再定義する
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1);
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2);

#if defined(DEBUG) || defined(_DEBUG) //リリースの時は無視される
	//Direct3D_Initialize(hWnd);以降にかく
	//フォントの初期化
	g_pDT = new hal::DebugText(Direct3D_GetDevice(),
		Direct3D_GetContext(),
		L"consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(),
		Direct3D_GetBackBufferHeight(),
		0.0f, 28.0f,
		0, 0,
		0.0f, 20.0f);
#endif
}

void Camera_Finalize(){
	
	SAFE_RELEASE(g_pVSConstantBuffer2);
	SAFE_RELEASE(g_pVSConstantBuffer1);

	delete g_pDT;
}

void Camera_Update(double elapsed_time){
	// XMVECTOR に変換
	XMVECTOR front = XMLoadFloat3(&g_CameraFront);
	XMVECTOR up    = XMLoadFloat3(&g_CameraUp);
	XMVECTOR right = XMLoadFloat3(&g_CameraRight);

	XMVECTOR position = XMLoadFloat3(&g_CameraPosition);

	Mouse_GetState(&g_MouseState);

	// 下を向く
	if (KeyLogger_IsPressed(KK_DOWN)) { // ↓キー
		XMMATRIX rotation = XMMatrixRotationAxis(right, CAMERA_ROTATION_SPEED * elapsed_time); // 右ベクトルを主軸に他２ベクトルを回転
		front = XMVector3TransformNormal(front, rotation);
		front = XMVector3Normalize(front); // floatの誤差が少し出るため１に戻す
		// 2本のベクトルを外積で＊と直角になる
		up = XMVector3Cross(front, right); // 外積はかける順番で符号が変わる

	}
	// 上を向く
	if (KeyLogger_IsPressed(KK_UP)) { // ↑キー
		XMMATRIX rotation = XMMatrixRotationAxis(right, -CAMERA_ROTATION_SPEED * elapsed_time); // 右ベクトルを主軸に他２ベクトルを回転
		front = XMVector3TransformNormal(front, rotation);
		front = XMVector3Normalize(front); // floatの誤差が少し出るため１に戻す
		// 2本のベクトルを外積で＊と直角になる
		up = XMVector3Cross(front, right); // 外積はかける順番で符号が変わる

	}
	// 右を向く
	if (KeyLogger_IsPressed(KK_RIGHT)) { // →キー
		//XMMATRIX rotation = XMMatrixRotationAxis(up, CAMERA_ROTATION_SPEED * elapsed_time); // 右ベクトルを主軸に他２ベクトルを回転
		XMMATRIX rotation = XMMatrixRotationY(CAMERA_ROTATION_SPEED * elapsed_time); // 3つのベクトル全てを軸を中心に回転
		up = XMVector3Normalize(XMVector3TransformNormal(up, rotation)); // 増えた分もノーマライズ
		front = XMVector3TransformNormal(front, rotation);
		front = XMVector3Normalize(front); // floatの誤差が少し出るため１に戻す
		// 2本のベクトルを外積で＊と直角になる
		right = XMVector3Normalize(XMVector3Cross(up, front)); // 外積はかける順番で符号が変わる

	}
	// ←を向く
	if (KeyLogger_IsPressed(KK_LEFT)) { // ←キー
		//XMMATRIX rotation = XMMatrixRotationAxis(up, CAMERA_ROTATION_SPEED * elapsed_time); // 右ベクトルを主軸に他２ベクトルを回転
		XMMATRIX rotation = XMMatrixRotationY(-CAMERA_ROTATION_SPEED * elapsed_time); // 3つのベクトル全てを軸を中心に回転
		up = XMVector3TransformNormal(up, rotation); // 増えた分もノーマライズ
		front = XMVector3TransformNormal(front, rotation);
		front = XMVector3Normalize(front); // floatの誤差が少し出るため１に戻す
		// 2本のベクトルを外積で＊と直角になる
		right = XMVector3Normalize(XMVector3Cross(up, front)); // 外積はかける順番で符号が変わる

	}

	// 前進
	if (KeyLogger_IsPressed(KK_W)) {
		position += front * CAMERA_MOVE_SPEED * elapsed_time;
	}
	// 後退
	if (KeyLogger_IsPressed(KK_S)) {
		position += -front * CAMERA_MOVE_SPEED * elapsed_time;
	}
	// 左
	if (KeyLogger_IsPressed(KK_A)) {
		position += -right * CAMERA_MOVE_SPEED * elapsed_time;
	}
	// 右
	if (KeyLogger_IsPressed(KK_D)) {
		position += right * CAMERA_MOVE_SPEED * elapsed_time;
	}
	// 上
	if (KeyLogger_IsPressed(KK_Z)) {
		position += up * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVECTOR{0.0f, 1.0f, 0.0f} * CAMERA_MOVE_SPEED * elapsed_time; // FPSの動き
	}
	// 下
	if (KeyLogger_IsPressed(KK_X)) {
		position += -up * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVECTOR{ 0.0f, -1.0f, 0.0f } *CAMERA_MOVE_SPEED * elapsed_time; // FPSの動き
	}
	// 画角
	if (KeyLogger_IsPressed(KK_V)) {
		g_Fov += XMConvertToRadians(10) * elapsed_time;
	}
	// 
	if (KeyLogger_IsPressed(KK_C)) {
		g_Fov -= XMConvertToRadians(10) * elapsed_time;
	}

	// 座標の更新結果の保存
	XMStoreFloat3(&g_CameraPosition, position);

	XMStoreFloat3(&g_CameraFront, front);
	XMStoreFloat3(&g_CameraUp, up);
	XMStoreFloat3(&g_CameraRight, right);

	//ビュー座標変換行列の作成(カメラ) 
	XMMATRIX mtmView = XMMatrixLookAtLH(
		position, // カメラの位置 左右　xを＋-　上下　yを+- 表裏　zを+=
		position + front,  // 視点 現在地から前方向を足す　
		up //  カメラの向き
	);

	// 頂点シェーダーにビュー座標変換行列を設定
	// ビュー変換行列を保存
	XMStoreFloat4x4(&g_CameraMatrix, mtmView);
 	//Shader3d_SetViewMatrix(mtmView); // test

	// パースペクティブ行列の作成
	float aspecctRatio = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
	float nearz = 0.1f;
	float farz = 200.0f;

	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(g_Fov, aspecctRatio, nearz, farz);

	// 頂点シェーダーに変換行列を設定
	//Shader3d_SetProjectionMatrix(mtxPerspective); // test
	// パースペクティブ変換行列を保存
	XMStoreFloat4x4(&g_Perspective, mtxPerspective);

}

const DirectX::XMFLOAT4X4& Camera_GetViewMatrix(){
	return g_CameraMatrix;
}

const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix(){
	return g_Perspective;
}

const DirectX::XMFLOAT3& Camera_GetFront() {
	return g_CameraFront;
}

const DirectX::XMFLOAT3& Camera_GetPosition(){
	return g_CameraPosition;
}

float Camera_GetFov()
{
	return g_Fov;
}

void Camera_SetMatrix(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection){
	
	XMFLOAT4X4 trans_v, trans_pro;
	XMStoreFloat4x4(&trans_v, XMMatrixTranspose(view));
	XMStoreFloat4x4(&trans_pro, XMMatrixTranspose(projection));
	// 定数バッファにビュー変換行列とプロジェクション変換行列をセット
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &trans_v, 0, 0);
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &trans_pro, 0, 0);
	Direct3D_GetContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
	Direct3D_GetContext()->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);
}

void Camera_DebugDraw(){
	// 昔のやり方、新しいのを調べる
#if defined(DEBUG) || defined(_DEBUG) 
	std::stringstream ss;//coutと同じ
	ss << "CAMERA FRONT : x = " << g_CameraFront.x;
	ss << " y = " << g_CameraFront.y ;
	ss << " z = " << g_CameraFront.z << std::endl;

	ss << "CAMERA UP    : x = " << g_CameraUp.x;
	ss << " y = " << g_CameraUp.y;
	ss << " z = " << g_CameraUp.z << std::endl;

	ss << "CAMERA RIGHT : x = " << g_CameraRight.x;
	ss << " y = " << g_CameraRight.y;
	ss << " z = " << g_CameraRight.z << std::endl;

	ss << "CAMERA FOV   : FOV = " << g_Fov << std::endl;
	
	ss << std::endl;
	ss << "CAMERA POSITION : x = " << g_CameraPosition.x ;
	ss << " y = " << g_CameraPosition.y;
	ss << " z = " << g_CameraPosition.z << std::endl;
	ss << std::endl;
	ss << "MOUSE POSITION : x = " << g_MouseState.x;
	ss << " y = " << g_MouseState.y;

	g_pDT->SetText(ss.str().c_str(), {1.0f,1.0f,0.0f,1.0f});

	g_pDT->Draw();
	g_pDT->Clear();
#endif

}
