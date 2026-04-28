#pragma once

#include <unordered_map>

#include <d3d11.h>
#include <DirectXMath.h>
#include "assimp\cimport.h"
#include "assimp\scene.h"
#include "assimp\postprocess.h"
#include "assimp\matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

#include "collision.h"

struct MODEL
{
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer = nullptr;
	ID3D11Buffer** IndexBuffer = nullptr;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;

	AABB local_aabb; // 動かしていない状態の当たり判定
};

MODEL* ModelLoad(const char* FileName, float scale, bool bBlender = false);
MODEL* ModelLoad(const char* FileName, const DirectX::XMFLOAT3& scale, bool bBlender = false);

void ModelRelease(MODEL* model);

void ModelDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld); // モデルのポインターを渡す→ModelLoadを呼び出すと手に入る
void ModelDepthDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld); 
void ModelUnlitDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld);
// 複数のインスタンスを一度に描画する
void ModelDrawInstanced(MODEL* model, const DirectX::XMMATRIX* mtxWorlds, int instanceCount);

AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3& position);