/*==============================================================================

   Direct3Dの初期化関連 [direct3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include "direct3d.h"
#include "debug_ostream.h"
using namespace DirectX;

#pragma comment(lib, "d3d11.lib")
// #pragma comment(lib, "dxgi.lib")

/* 各種インターフェース */
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11BlendState* g_pBlendStateMultiply = nullptr;
static ID3D11BlendState* g_pBlendStateAdd = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilStateDepthDisable = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilStateDepthEnable = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilStateDepthWriteDisable = nullptr;

static ID3D11RasterizerState* g_pRasterizerState = nullptr;

/* バックバッファ関連 */
static ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
static ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_BackBufferDesc{};
static D3D11_VIEWPORT g_Viewport{}; //ビューポート設定
static bool configureBackBuffer(); // バックバッファの設定・生成
static void releaseBackBuffer(); // バックバッファの解放

// オフスクリーンレンダリング関係
static ID3D11Texture2D* g_pOffscreenBuffer = nullptr;
static ID3D11RenderTargetView* g_pOffscreenRenderTargetView = nullptr;
static ID3D11ShaderResourceView* g_pOffscreenShaderResourceView = nullptr;
static ID3D11Texture2D* g_pOffscreenDepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pOffscreenDepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_OffscreenDesc{};
static D3D11_VIEWPORT g_OffscreenViewport{}; //ビューポート設定
static bool configureOffscreenBackBuffer(); // オフスクリーン用バックバッファの設定・生成
static void releaseOffscreenBackBuffer(); // オフスクリーン用バックバッファの解放

// 深度情報レンダリング関係
static ID3D11Texture2D* g_pDepthBuffer = nullptr;
static ID3D11RenderTargetView* g_pDepthRenderTargetView = nullptr;
static ID3D11ShaderResourceView* g_pDepthShaderResourceView = nullptr;
static ID3D11Texture2D* g_pDepthDepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pDepthDepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_DepthDesc{};
static D3D11_VIEWPORT g_DepthViewport{}; //ビューポート設定
static ID3D11Buffer* g_pVSConstantBuffer3 = nullptr; // register(b3)用

static bool configureDepthBackBuffer(); // 深度情報用バックバッファの設定・生成
static void releaseDepthBackBuffer(); // 深度情報用バックバッファの解放

bool Direct3D_Initialize(HWND hWnd)
{
    /* デバイス、スワップチェーン、コンテキスト生成 */
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.Windowed = TRUE;
    swap_chain_desc.BufferCount = 2;
    // swap_chain_desc.BufferDesc.Width = 0;
    // swap_chain_desc.BufferDesc.Height = 0;
	// ⇒ ウィンドウサイズに合わせて自動的に設定される
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    //fpsの解放しない場合、する場合
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	//swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.OutputWindow = hWnd;

	UINT device_flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	//たまに動かない場合は下をコメントアウト
    device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
 
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        device_flags,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &g_pSwapChain,//絵を描くのに重要
        &g_pDevice,//絵を描くのに重要
        &feature_level,
        &g_pDeviceContext);//絵を描くのに重要

    if (FAILED(hr)) {
		MessageBox(hWnd, "Direct3Dの初期化に失敗しました", "エラー", MB_OK);
        return false;
    }

	if (!configureBackBuffer()) {
		MessageBox(hWnd, "バックバッファの設定に失敗しました", "エラー", MB_OK);
		return false;
	}
	// オフスクリーン関連のリソース作成
	if (!configureOffscreenBackBuffer()) {
		MessageBox(hWnd, "オフスクリーン用バックバッファの設定に失敗しました", "エラー", MB_OK);
		return false;
	}
	// 深度バッファ関連のリソース作成
	if (!configureDepthBackBuffer()) {
		MessageBox(hWnd, "オフスクリーン用バックバッファの設定に失敗しました", "エラー", MB_OK);
		return false;
	}

	// αブレンド...処理が重いので注意
	// 色:RGBにAを足す。
	// A好きに使ってよい。基本は透明の表現に使われる
	
	// ブレンドステート設定
	D3D11_BLEND_DESC bd = {};
	// アルファテスト
	bd.AlphaToCoverageEnable = FALSE; //TRUE:フェードインがうまく行かなくなる がきれいに透明びょうができる
	bd.IndependentBlendEnable = FALSE;
	bd.RenderTarget[0].BlendEnable = TRUE;//αブレンドするしない
	
	/*----透過ブレンドの設定----*/
	//RGB
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;//OP:演算子
	//srcソース、今から描く絵　destもともと書いてある絵
	//srcRGB * srcA + DestRGB * (1 - srcA) = RGBの計算結果
	//乗算合成

	//srcRGB * srcA + DestRGB * 1 = だんだん値があだり白に近づく、黒は透明に
	//加算合成
	
	//α
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;//1
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;//0
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;//演算子
	
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	//srcA * 1 + DestA * 0
	//SRC...ソース：今から描く絵：色
	//dest...すでに描かれた絵：色

	g_pDevice->CreateBlendState(&bd, &g_pBlendStateMultiply);

	/*----透過ブレンドの設定----*/

	Direct3D_SetAlphaBlendTransparent(); // デフォルトのプレステート

	/*----加算ブレンドの設定----*/

	//RGB
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	//srcRGB * srcA + DestRGB * 1 = だんだん値があだり白に近づく、黒は透明に
	//加算合成

	//α
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;//1

	g_pDevice->CreateBlendState(&bd, &g_pBlendStateAdd);
	/*----加算ブレンドの設定----*/

	// 深度ステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthFunc = D3D11_COMPARISON_LESS;
	dsd.StencilEnable = FALSE;
	dsd.DepthEnable = FALSE; // 無効にする
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthDisable);

	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthEnable);

	dsd.StencilEnable = FALSE;
	dsd.DepthFunc = D3D11_COMPARISON_ALWAYS;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthWriteDisable);

	Direct3D_SetDepthEnable(true);

	// ラスタライザステートの作成
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_SOLID;
	//rd.FillMode = D3D11_FILL_WIREFRAME; // ワイヤーフレームでの表示
	rd.CullMode = D3D11_CULL_BACK;
	//rd.CullMode = D3D11_CULL_NONE;
	rd.DepthClipEnable = TRUE;
	rd.MultisampleEnable = FALSE;
	g_pDevice->CreateRasterizerState(&rd, &g_pRasterizerState);

	// デバイスコンテキストにラスタライザーステートを設定
	g_pDeviceContext->RSSetState(g_pRasterizerState);

    return true;
}

void Direct3D_Finalize()
{
	SAFE_RELEASE(g_pDepthStencilStateDepthDisable);
	SAFE_RELEASE(g_pDepthStencilStateDepthEnable);
	//SAFE_RELEASE(g_pDepthStencilStateDepthWriteDisable);
	SAFE_RELEASE(g_pBlendStateMultiply);
	SAFE_RELEASE(g_pRasterizerState);

	releaseBackBuffer();
	releaseOffscreenBackBuffer();
	releaseDepthBackBuffer();

	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pDeviceContext);
	SAFE_RELEASE(g_pDevice);
}

void Direct3D_Present()
{
	// スワップチェーンの表示
	//表画面と裏画面の切り替え
	//g_pDeviceContextはすぐに実行せず命令をためる
	//GPUはif文に弱いが繰り返しにはとても強い
	g_pSwapChain->Present(1, 0);//0にすると交信できる速度で頑張る
	//ベンチマークをとるときhs第一引数を0にする
}

unsigned int Direct3D_GetBackBufferWidth()
{
	return g_BackBufferDesc.Width;
}

unsigned int Direct3D_GetBackBufferHeight()
{
	return g_BackBufferDesc.Height;
}

ID3D11Device* Direct3D_GetDevice()
{
	return g_pDevice;
}

ID3D11DeviceContext* Direct3D_GetContext()
{
	return g_pDeviceContext;
}

void Direct3D_SetAlphaBlendTransparent(){
	float blend_factor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
	g_pDeviceContext->OMSetBlendState(g_pBlendStateMultiply, blend_factor, 0xffffffff);

}

void Direct3D_SetAlphaBlendAdd(){
	float blend_factor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
	g_pDeviceContext->OMSetBlendState(g_pBlendStateAdd, blend_factor, 0xffffffff);

}

void Direct3D_SetDepthEnable(bool enable){
	if (enable) {
		g_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilStateDepthEnable, NULL);
	} else {
		g_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilStateDepthDisable, NULL);
	}
}

void Direct3D_SetDepthWriteDisable(){
	g_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilStateDepthWriteDisable, NULL);
}

DirectX::XMMATRIX Direct3D_MatrixViewport(){
	float half_width = Direct3D_GetBackBufferWidth() * 0.5f;
	float half_height = Direct3D_GetBackBufferHeight() * 0.5f;
	float max_depth = g_Viewport.MaxDepth;
	float min_depth = g_Viewport.MinDepth;

	return DirectX::XMMATRIX(
		half_width,         0.0f,                    0.0f, 0.0f,
		      0.0f, -half_height,                    0.0f, 0.0f,
		      0.0f,         0.0f, (max_depth - min_depth), 0.0f,
		half_width,  half_height,               min_depth, 1.0f
	);

}

DirectX::XMFLOAT3 Dirext3D_ScreenToWorld(int x, int y, float depth, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection){
	XMMATRIX xview = XMLoadFloat4x4(&view);
	XMMATRIX xproj = XMLoadFloat4x4(&projection);
	XMVECTOR xpoint{ x, y, depth, 1.0f };
	// 合成行列の逆行列を生成
	XMMATRIX inv{ XMMatrixInverse(nullptr, xview * xproj *Direct3D_MatrixViewport()) };
	// 座標返還した結果をもう使わないxpointに入れる
	xpoint = XMVector3TransformCoord(xpoint, inv);

	XMFLOAT3 ret;

	XMStoreFloat3(&ret, xpoint);

	return ret;
}

DirectX::XMFLOAT2 Dirext3D_WorldToScreen(const DirectX::XMFLOAT3& position, float depth, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection){
	XMMATRIX xview = XMLoadFloat4x4(&view);
	XMMATRIX xproj = XMLoadFloat4x4(&projection);
	XMVECTOR xpoint{ XMLoadFloat3(&position)};
	// 座標返還した結果をもう使わないxpointに入れる
	xpoint = XMVector3TransformCoord(xpoint, xview * xproj * Direct3D_MatrixViewport());

	XMFLOAT2 ret;

	XMStoreFloat2(&ret, xpoint);

	return ret;
}

void Direct3D_ClearBackBuffer() {
	float clear_color[4] = { 0.1f, 0.2f, 0.1f, 1.0f };//背景色RGB
	g_pDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clear_color);//画面の画用紙、指定した色で塗りつぶす
	g_pDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Direct3D_SetBackBuffer(){
	// ビューポートの設定
	g_pDeviceContext->RSSetViewports(1, &g_Viewport); 
	// レンダーターゲットビューとデプスステンシルビューの設定 
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

}

void Direct3D_ClearOffscreen(){
	float clear_color[4] = { 0.0f,0.0f,0.0f,1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pOffscreenRenderTargetView, clear_color);
	g_pDeviceContext->ClearDepthStencilView(g_pOffscreenDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Direct3D_SetOffscreen(){
	g_pDeviceContext->RSSetViewports(1, &g_OffscreenViewport); // ビューポートの設定

	// レンダーターゲットビューとデプスステンシルビューの設定
	g_pDeviceContext->OMSetRenderTargets(1, &g_pOffscreenRenderTargetView, g_pOffscreenDepthStencilView);
}

void Direct3D_SetOffscreenTexture(int slot) {
	//テクスチャ設定
	g_pDeviceContext->PSSetShaderResources(slot, 1, &g_pOffscreenShaderResourceView);
}

void Direct3D_ClearDepth(){
	float clear_color[4] = { 0.0f,0.0f,0.0f,1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pDepthRenderTargetView, clear_color);
	g_pDeviceContext->ClearDepthStencilView(g_pDepthDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Direct3D_SetDepth(){
	g_pDeviceContext->RSSetViewports(1, &g_DepthViewport); // ビューポートの設定

	// レンダーターゲットビューとデプスステンシルビューの設定
	g_pDeviceContext->OMSetRenderTargets(1, &g_pDepthRenderTargetView, g_pDepthDepthStencilView);
}

void Direct3D_SetDepthTexture(int slot){
	//テクスチャ設定
	g_pDeviceContext->PSSetShaderResources(slot, 1, &g_pDepthShaderResourceView);
}

void Direct3D_SetLightViewProjrctionMatrix(const DirectX::XMMATRIX& matrix){
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファに行列をセット
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer3, 0, nullptr, &transpose, 0, 0);

	// 定数バッファを描画パイプラインに設定
	Direct3D_GetContext()->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer3);
}

bool configureBackBuffer()
{
    HRESULT hr;

    ID3D11Texture2D* back_buffer_pointer = nullptr;

	// バックバッファの取得
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer_pointer);

    if (FAILED(hr)) {
		hal::dout << "バックバッファの取得に失敗しました" << std::endl;
        return false;
    }

	// バックバッファのレンダーターゲットビューの生成
	hr = g_pDevice->CreateRenderTargetView(back_buffer_pointer, nullptr, &g_pRenderTargetView);
	//Createがついたものは大体返さなきゃいけない

    if (FAILED(hr)) {
        back_buffer_pointer->Release();
        hal::dout << "バックバッファのレンダーターゲットビューの生成に失敗しました" << std::endl;
        return false;
    }

	// バックバッファの状態（情報）を取得
    back_buffer_pointer->GetDesc(&g_BackBufferDesc);

	back_buffer_pointer->Release(); // バックバッファのポインタは不要なので解放

	// デプスステンシルバッファの生成
	D3D11_TEXTURE2D_DESC depth_stencil_desc{};
	depth_stencil_desc.Width = g_BackBufferDesc.Width;
	depth_stencil_desc.Height = g_BackBufferDesc.Height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;
	hr = g_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &g_pDepthStencilBuffer);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルバッファの生成に失敗しました" << std::endl;
		return false;
	}

	// デプスステンシルビューの生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = depth_stencil_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;
	depth_stencil_view_desc.Flags = 0;
	hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &depth_stencil_view_desc, &g_pDepthStencilView);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルビューの生成に失敗しました" << std::endl;
		return false;
	}

	// ビューポートの設定 
	g_Viewport.TopLeftX = 0.0f;
	g_Viewport.TopLeftY = 0.0f;
	g_Viewport.Width = static_cast<FLOAT>(g_BackBufferDesc.Width);
	g_Viewport.Height = static_cast<FLOAT>(g_BackBufferDesc.Height);
	g_Viewport.MinDepth = 0.0f;
	g_Viewport.MaxDepth = 1.0f;

    return true;
}

void releaseBackBuffer()
{
	SAFE_RELEASE(g_pRenderTargetView);
	SAFE_RELEASE(g_pDepthStencilBuffer);
	SAFE_RELEASE(g_pDepthStencilView);
}

bool configureOffscreenBackBuffer(){
	HRESULT hr;
	
	g_OffscreenDesc.Width = 512;
	g_OffscreenDesc.Height = 512;
	g_OffscreenDesc.MipLevels = 1;
	g_OffscreenDesc.ArraySize = 1;
	g_OffscreenDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	g_OffscreenDesc.SampleDesc.Count = 1;
	g_OffscreenDesc.SampleDesc.Quality = 0;
	g_OffscreenDesc.Usage = D3D11_USAGE_DEFAULT;
	g_OffscreenDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	g_OffscreenDesc.CPUAccessFlags = 0;
	g_OffscreenDesc.MiscFlags = 0;

	g_pDevice->CreateTexture2D(&g_OffscreenDesc, nullptr, &g_pOffscreenBuffer);
	g_pDevice->CreateRenderTargetView(g_pOffscreenBuffer, nullptr, &g_pOffscreenRenderTargetView);
	g_pDevice->CreateShaderResourceView(g_pOffscreenBuffer, nullptr, &g_pOffscreenShaderResourceView);
	
	// デプスステンシルバッファの生成
	D3D11_TEXTURE2D_DESC depth_stencil_desc{};
	depth_stencil_desc.Width = g_OffscreenDesc.Width;
	depth_stencil_desc.Height = g_OffscreenDesc.Height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D32_FLOAT; // !!!!!!!!!!!!!!
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;
	g_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &g_pOffscreenDepthStencilBuffer);


	// デプスステンシルビューの生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = depth_stencil_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;
	depth_stencil_view_desc.Flags = 0;
	g_pDevice->CreateDepthStencilView(g_pOffscreenDepthStencilBuffer, &depth_stencil_view_desc, &g_pOffscreenDepthStencilView);

	// ビューポートの設定 
	g_OffscreenViewport.TopLeftX = 0.0f;
	g_OffscreenViewport.TopLeftY = 0.0f;
	g_OffscreenViewport.Width = static_cast<FLOAT>(g_OffscreenDesc.Width);
	g_OffscreenViewport.Height = static_cast<FLOAT>(g_OffscreenDesc.Height);
	g_OffscreenViewport.MinDepth = 0.0f;
	g_OffscreenViewport.MaxDepth = 1.0f;
	
	//g_pDeviceContext->RSSetViewports(1, &g_OffscreenViewport); // ビューポートの設定
	
	return true;
}

void releaseOffscreenBackBuffer() {
	SAFE_RELEASE(g_pOffscreenBuffer);
	SAFE_RELEASE(g_pOffscreenRenderTargetView);
	SAFE_RELEASE(g_pOffscreenShaderResourceView);
	SAFE_RELEASE(g_pOffscreenDepthStencilBuffer);
	SAFE_RELEASE(g_pOffscreenDepthStencilView);
}

bool configureDepthBackBuffer() {
	HRESULT hr;

	g_DepthDesc.Width = 4096;
	g_DepthDesc.Height = 4096;
	g_DepthDesc.MipLevels = 1;
	g_DepthDesc.ArraySize = 1;
//	g_DepthDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	g_DepthDesc.Format = DXGI_FORMAT_R32_FLOAT;
	g_DepthDesc.SampleDesc.Count = 1;
	g_DepthDesc.SampleDesc.Quality = 0;
	g_DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	g_DepthDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	g_DepthDesc.CPUAccessFlags = 0;
	g_DepthDesc.MiscFlags = 0;

	g_pDevice->CreateTexture2D(&g_DepthDesc, nullptr, &g_pDepthBuffer);
	g_pDevice->CreateRenderTargetView(g_pDepthBuffer, nullptr, &g_pDepthRenderTargetView);
	g_pDevice->CreateShaderResourceView(g_pDepthBuffer, nullptr, &g_pDepthShaderResourceView);

	// デプスステンシルバッファの生成
	D3D11_TEXTURE2D_DESC depth_stencil_desc{};
	depth_stencil_desc.Width = g_DepthDesc.Width;
	depth_stencil_desc.Height = g_DepthDesc.Height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D32_FLOAT; // !!!!!!!!!!!!!!
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;
	g_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &g_pDepthDepthStencilBuffer);


	// デプスステンシルビューの生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = depth_stencil_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;
	depth_stencil_view_desc.Flags = 0;
	g_pDevice->CreateDepthStencilView(g_pDepthDepthStencilBuffer, &depth_stencil_view_desc, &g_pDepthDepthStencilView);

	// ビューポートの設定 
	g_DepthViewport.TopLeftX = 0.0f;
	g_DepthViewport.TopLeftY = 0.0f;
	g_DepthViewport.Width = static_cast<FLOAT>(g_DepthDesc.Width);
	g_DepthViewport.Height = static_cast<FLOAT>(g_DepthDesc.Height);
	g_DepthViewport.MinDepth = 0.0f;
	g_DepthViewport.MaxDepth = 1.0f;

	// 定数バッファを作る
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer3);

	return true;
}

void releaseDepthBackBuffer() {
	SAFE_RELEASE(g_pVSConstantBuffer3);
	SAFE_RELEASE(g_pDepthBuffer);
	SAFE_RELEASE(g_pDepthRenderTargetView);
	SAFE_RELEASE(g_pDepthShaderResourceView);
	SAFE_RELEASE(g_pDepthDepthStencilBuffer);
	SAFE_RELEASE(g_pDepthDepthStencilView);
}



void Direct3D_DebugColorClear() {
	// 影用テクスチャを「真っ赤」に塗りつぶす
	float debug_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pDepthRenderTargetView, debug_color);
}