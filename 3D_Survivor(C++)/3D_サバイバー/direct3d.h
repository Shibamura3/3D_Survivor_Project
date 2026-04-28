/*==============================================================================

   Direct3Dの初期化関連 [direct3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef DIRECT3D_H
#define DIRECT3D_H

#include <Windows.h>
#include <D3d11.h>
#include <DirectXMath.h>

// セーフリリースマクロ
#define SAFE_RELEASE(o) if (o) { (o)->Release(); o = NULL; }


bool Direct3D_Initialize(HWND hWnd); // Direct3Dの初期化
void Direct3D_Finalize(); // Direct3Dの終了処理

void Direct3D_Present(); // バックバッファの表示

//バックバッファの大きさの取得
unsigned int Direct3D_GetBackBufferWidth();  //幅
unsigned int Direct3D_GetBackBufferHeight();  //高さ

//Direct3Dデバイスの取得
ID3D11Device* Direct3D_GetDevice();

//Direct3Dデバイスコンテキストの取得
ID3D11DeviceContext* Direct3D_GetContext();

//Direct3D_アルファブレンドの切り替えよう
void Direct3D_SetAlphaBlendTransparent(); // 透過処理、乗算合成
void Direct3D_SetAlphaBlendAdd(); // 加算合成

//深度バッファの設定
void Direct3D_SetDepthEnable(bool enable);
void Direct3D_SetDepthWriteDisable();

//ビューポート行列
DirectX::XMMATRIX Direct3D_MatrixViewport();
//スクリーン座標から3D座標への変換
DirectX::XMFLOAT3 Dirext3D_ScreenToWorld(int x, int y, float depth, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);  // ベクトル型でも可能
//3D座標への変換からスクリーン座標
DirectX::XMFLOAT2 Dirext3D_WorldToScreen(const DirectX::XMFLOAT3& position, float depth, const DirectX::XMFLOAT4X4 & view, const DirectX::XMFLOAT4X4 & projection);  // ベクトル型でも可能

// バックバッファのクリア
void Direct3D_ClearBackBuffer();
// バックバッファレンダリングに切り替える
void Direct3D_SetBackBuffer();

// オフスクリーン関連
// バックバッファのクリア
void Direct3D_ClearOffscreen();
// テクスチャへのレンダリングへの切り替え
void Direct3D_SetOffscreen();
// オフスクリーンレンダリングテクスチャの設定
void Direct3D_SetOffscreenTexture(int slot);

// 深度情報バッファ関連
// 深度情報バッファバッファのクリア
void Direct3D_ClearDepth();
// 深度情報バッファへのレンダリングへの切り替え
void Direct3D_SetDepth();
// 深度情報バッファレンダリングテクスチャの設定
void Direct3D_SetDepthTexture(int slot);

// ライトビュープロジェクション行列の定数バッファへの登録と設定
void Direct3D_SetLightViewProjrctionMatrix(const DirectX::XMMATRIX & matrix);

// シャドウマップ関連
void Direct3D_ClearShadowMap();
void Direct3D_SetShadowMap();
void Direct3D_SetShadowMapTexture(int slot);

void Direct3D_DebugColorClear();

#endif // DIRECT3D_H
