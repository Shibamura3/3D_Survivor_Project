/*
	空の描画　

	2025/11/21	hibiki sakuma
*/

#include "skydome.h"
#include "model.h"
#include "resource_manager.h"
#include "shader3d_unlit.h"
using namespace DirectX;

static XMFLOAT3 g_position{};

void Skydome_Initialize(){
}

void Skydome_Finalize(){
}

void Skydone_SetPosition(const DirectX::XMFLOAT3& position){
	g_position = position;
}

void Skydome_Draw(){
	Shader3dUnlit_Begin();
	g_position.y = 0.0f; // スカイドームがキャラと連動して飛ばないために
	ModelUnlitDraw(Resouce_Manager_GetModelId(Sky), XMMatrixTranslationFromVector(XMLoadFloat3(&g_position)));
}
