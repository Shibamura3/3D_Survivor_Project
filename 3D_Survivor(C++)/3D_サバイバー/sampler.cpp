/*
	サンプラーの設定ユーティリティ：sampler.h

	2025/09/18	hibiki sakuma
*/


#include "sampler.h"

#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "shader.h"
#include "shader3d.h"

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11SamplerState* g_pSamplerPoint = nullptr;
static ID3D11SamplerState* g_pSamplerLiner = nullptr;
static ID3D11SamplerState* g_pSamplerAnisotropic = nullptr;

void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext){
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	//サンプラーステートの設定
	D3D11_SAMPLER_DESC sampler_desc{}; // 2Dと3Dで切り替えたい

	//フィルタリング
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

	//UV参照外の取り扱い(UVアドレッシングモード)
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; 
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; 
	sampler_desc.BorderColor[0] = 0.0f;
	sampler_desc.BorderColor[1] = 0.0f;
	sampler_desc.BorderColor[2] = 0.0f;
	sampler_desc.BorderColor[3] = 0.0f;

	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.MipLODBias = 0;
	sampler_desc.MaxAnisotropy = 16; // 周囲のｎピクセル
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerPoint);

	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerLiner);

	sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerAnisotropic);

}

void Sampler_Finalize(){
	SAFE_RELEASE(g_pSamplerAnisotropic);
	SAFE_RELEASE(g_pSamplerLiner);
	SAFE_RELEASE(g_pSamplerPoint);
	
}

void Sampler_SetFilterPoint(){
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerPoint);
}

void Sampler_SetFilterLinear(){
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerLiner);
}

void Sampler_SetFilterAnisotropic(){
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerAnisotropic);
}
