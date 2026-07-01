/*
	‹ó‚Ì•`‰æ@

	2025/11/21	hibiki sakuma
*/

#include "skydome.h"
#include "model.h"
#include "player.h"
#include "resource_manager.h"
#include "shader3d_unlit.h"
using namespace DirectX;

static XMFLOAT3 g_position{};

void Skydome_Initialize(){
}

void Skydome_Finalize(){
}

void Skydonm_SetPosition(const DirectX::XMFLOAT3& position){
	g_position = position;
}

void Skydome_Draw() {
    Shader3dUnlit_Begin();

    XMFLOAT3 playerPos = GetPlayer()->GetPosition();

    g_position.x = playerPos.x;
    g_position.z = playerPos.z;
    float targetY = playerPos.y * 0.2f;
    g_position.y += (targetY - g_position.y) * 0.05f;

    ModelUnlitDraw( Resouce_Manager_GetModelId(Sky),
        XMMatrixTranslationFromVector(XMLoadFloat3(&g_position))
    );
}