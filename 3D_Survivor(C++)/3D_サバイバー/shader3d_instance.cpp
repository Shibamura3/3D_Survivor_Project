/*==============================================================================

   シェーダー 3D用[shader3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/09/10
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "shader.h"
#include "sampler.h"
#include "shader3d_instance.h"


static ID3D11VertexShader* g_pInstVertexShader = nullptr;
static ID3D11InputLayout* g_pInstInputLayout = nullptr;
static ID3D11Buffer* g_pInstVSConstantBuffer0 = nullptr; // 定数バッファ
static ID3D11Buffer* g_pInstPSConstantBuffer0 = nullptr; // ピクセルシェーダの定数バッファb0
static ID3D11PixelShader* g_pInstPixelShader = nullptr;

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pInstDevice = nullptr;
static ID3D11DeviceContext* g_pInstContext = nullptr;

bool Shader3d_Instance_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr; // 戻り値格納用

	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Shader_Instance_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return false;
	}

	// デバイスとデバイスコンテキストの保存
	g_pInstDevice = pDevice;
	g_pInstContext = pContext;


	// 事前コンパイル済み頂点シェーダーの読み込み
	std::ifstream ifs_vs("shader_vertex_instance.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_3d.cso", "エラー", MB_OK);
		return false;
	}

	// ファイルサイズを取得
	ifs_vs.seekg(0, std::ios::end); // ファイルポインタを末尾に移動
	std::streamsize filesize = ifs_vs.tellg(); // ファイルポインタの位置を取得（つまりファイルサイズ）
	ifs_vs.seekg(0, std::ios::beg); // ファイルポインタを先頭に戻す

	// バイナリデータを格納するためのバッファを確保
	unsigned char* vsbinary_pointer = new unsigned char[filesize];

	ifs_vs.read((char*)vsbinary_pointer, filesize); // バイナリデータを読み込む
	ifs_vs.close(); // ファイルを閉じる

	// 頂点シェーダーの作成
	hr = g_pInstDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pInstVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader_Instance_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer; // メモリリークしないようにバイナリデータのバッファを解放
		return false;
	}


	// 頂点レイアウトの定義
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// Slot 1 (Instance Buffer) から行列を読み込む設定
		{ "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};//				0を送る		情報のビット数　32はビット

	UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

	// 頂点レイアウトの作成
	hr = g_pInstDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInstInputLayout);

	delete[] vsbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_Instance_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}

	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

	//本来は容量を再定義する
	g_pInstDevice->CreateBuffer(&buffer_desc, nullptr, &g_pInstVSConstantBuffer0);

	// 事前コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("shader_pixel_3d.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_2d.cso", "エラー", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// ピクセルシェーダーの作成
	hr = g_pInstDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pInstPixelShader);

	delete[] psbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_Instance_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}

	// ピクセルシェーダ用定数バッファの作成
	//D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // バッファのサイズ

	//本来は容量を再定義する
	g_pInstDevice->CreateBuffer(&buffer_desc, nullptr, &g_pInstPSConstantBuffer0);

	return true;
}

void Shader3d_Instance_Finalize()
{
	SAFE_RELEASE(g_pInstPixelShader);
	SAFE_RELEASE(g_pInstPSConstantBuffer0);
	SAFE_RELEASE(g_pInstVSConstantBuffer0);
	SAFE_RELEASE(g_pInstInputLayout);
	SAFE_RELEASE(g_pInstVertexShader);
}

void Shader3d_Instance_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファに行列をセット
	g_pInstContext->UpdateSubresource(g_pInstVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);

}

void Shader3d_Instance_SetColor(const XMFLOAT4& color) {
	g_pInstContext->UpdateSubresource(g_pInstPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void Shader3d_Instance_Begin()
{
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	g_pInstContext->VSSetShader(g_pInstVertexShader, nullptr, 0);
	g_pInstContext->PSSetShader(g_pInstPixelShader, nullptr, 0);

	// 頂点レイアウトを描画パイプラインに設定
	g_pInstContext->IASetInputLayout(g_pInstInputLayout);

	// 定数バッファを描画パイプラインに設定
	g_pInstContext->VSSetConstantBuffers(0, 1, &g_pInstVSConstantBuffer0);
	g_pInstContext->PSSetConstantBuffers(0, 1, &g_pInstPSConstantBuffer0);
	

}
