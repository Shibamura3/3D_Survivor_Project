
#include <assert.h>
#include "direct3d.h"
#include "texture.h"
#include "shader3d.h"
#include "shader_depth.h"
#include "shader3d_unlit.h"
#include "shader3d_instance.h"
#include "model.h"

#include <DirectXMath.h>
using namespace DirectX;
#include "WICTextureLoader11.h"

// 頂点構造体
struct Vertex3d
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT3 normal;   // 法線
	XMFLOAT4 color;    // 色　
	XMFLOAT2 texcoord; // UV
};
// インスタンス用構造体
struct InstanceData {
	DirectX::XMMATRIX world;
};
//変数宣言
static int g_TexId_White = -1;
static ID3D11Buffer* g_pInstanceBuffer = nullptr;
// 定数宣言
static constexpr int MAX_INSTANCES = 500; // 一度に描画できる最大数

MODEL* ModelLoad(const char* FileName, float scale, bool bBlender)
{
	MODEL* model = new MODEL;

	//const std::string modelPath(FileName);

	model->AiScene = aiImportFile(FileName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
	assert(model->AiScene);

	model->VertexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];
	model->IndexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];

	//model->local_aabb.min
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// 頂点バッファ生成
		{
			Vertex3d* vertex = new Vertex3d[mesh->mNumVertices];

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				if (!bBlender) {
					// position に　スケールを掛け算することで初めからサイズを調整できる
					vertex[v].position = XMFLOAT3(mesh->mVertices[v].x * scale, mesh->mVertices[v].y * scale, mesh->mVertices[v].z * scale);
					vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
				}
				else {
					vertex[v].position = XMFLOAT3(mesh->mVertices[v].x * scale, -mesh->mVertices[v].z * scale, mesh->mVertices[v].y * scale);
					vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, -mesh->mNormals[v].z, mesh->mNormals[v].y);
				}
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 頂点カラーを白にしておく
				vertex[v].texcoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);

				// aabbの取得
				if (v == 0 && m == 0) {
					model->local_aabb.min = vertex[v].position;
					model->local_aabb.max = vertex[v].position;
				}
				else {
					model->local_aabb.min.x = std::min(model->local_aabb.min.x, vertex[v].position.x);
					model->local_aabb.min.y = std::min(model->local_aabb.min.y, vertex[v].position.y);
					model->local_aabb.min.z = std::min(model->local_aabb.min.z, vertex[v].position.z);
					model->local_aabb.max.x = std::max(model->local_aabb.max.x, vertex[v].position.x);
					model->local_aabb.max.y = std::max(model->local_aabb.max.y, vertex[v].position.y);
					model->local_aabb.max.z = std::max(model->local_aabb.max.z, vertex[v].position.z);

				}
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(Vertex3d) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = vertex;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

			delete[] vertex;
		}


		// インデックスバッファ生成
		{
			unsigned int* index = new unsigned int[mesh->mNumFaces * 3];

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];

				assert(face->mNumIndices == 3);

				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[1];
				index[f * 3 + 2] = face->mIndices[2];
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * mesh->mNumFaces * 3;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = index;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

			delete[] index;
		}

	}

	g_TexId_White = Texture_Load(L"resuce/img/w.png"); // 念の為、白いテクスチャを貼る

	// FBXにテクスチャが内包している場合
	for (unsigned int i = 0; i < model->AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];

		ID3D11ShaderResourceView* texture;
		ID3D11Resource* resource;
		/*
		TexMetadata metadata;
		ScratchImage image;
		LoadFromWICMemory(aitexture->pcData, aitexture->mWidth, WIC_FLAGS_NONE, &metadata, image);
		CreateShaderResourceView(DirectXGetDevice(), image.GetImages(), image.GetImageCount(), metadata, &texture);
		*/

		CreateWICTextureFromMemory( // 内包されたテクスチャの場合のみ
			Direct3D_GetDevice(),				//_In_ ID3D11Device * d3dDevice,
			Direct3D_GetContext(),				//_In_opt_ ID3D11DeviceContext * d3dContext,
			(const uint8_t*)aitexture->pcData,  //_In_reads_bytes_(wicDataSize) const uint8_t * wicData,
			(size_t)aitexture->mWidth,			//_In_ size_t wicDataSize,
			&resource,							//_Outptr_opt_ ID3D11Resource * *texture,
			&texture							//_Outptr_opt_ ID3D11ShaderResourceView * *textureView,
		);										//_In_ size_t maxsize = 0) noexcept;

		assert(texture);

		resource->Release(); // !!!!

		model->Texture[aitexture->mFilename.data] = texture;
	}

	// fbxのファイルパスだけを取得
	const std::string modelPath(FileName);

	// 最後の'/'または'\\'の位置を探す 
	size_t pos = modelPath.find_last_of("/\\");
	std::string directory;

	if (pos != std::string::npos) {
		directory = modelPath.substr(0, pos); // ファイル名を除いた部分
	}
	else {
		directory = ""; // パスに区切りがない場合(ファイル名のみ)
	}

	// テクスチャがFBXとは別に用意されている場合
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) {
		aiString filename;
		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];

		if (filename.length == 0) { // テクスチャが内包されている場合
			continue; // 実行しない(フィルター①)
		}

		// テクスチャパスに余計なフォルダ名が在ったらカット
		std::string str_filename = filename.C_Str();
		size_t n = str_filename.find_last_of("\\/");
		str_filename = str_filename.substr(n + 1);


		if (model->Texture.count(filename.C_Str())) { // すでにファイル名が登録されているか
			continue; // 実行しない(フィルター②)
		}

		ID3D11ShaderResourceView* texture;
		ID3D11Resource* resource;

		std::string texfilename = directory + "/" + str_filename; // ホームから該当ファイルまでのパス＋/＋テクスチャのファイル名

		int len = MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, nullptr, 0);
		wchar_t* pWideFilename = new wchar_t[len]; //  new:一時的に使用
		MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, pWideFilename, len);

		CreateWICTextureFromFile( // Unicode対応なので上で計算
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			pWideFilename,
			&resource,
			&texture
		);

		delete[] pWideFilename;

		assert(texture);

		resource->Release(); // !!!

	}

	return model;
}

MODEL* ModelLoad(const char* FileName, const DirectX::XMFLOAT3& scale, bool bBlender){
	MODEL* model = new MODEL;

	//const std::string modelPath(FileName);

	model->AiScene = aiImportFile(FileName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
	assert(model->AiScene);

	model->VertexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];
	model->IndexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];

	//model->local_aabb.min
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// 頂点バッファ生成
		{
			Vertex3d* vertex = new Vertex3d[mesh->mNumVertices];

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				if (!bBlender) {
					// position に　スケールを掛け算することで初めからサイズを調整できる
					vertex[v].position = XMFLOAT3(mesh->mVertices[v].x * scale.x, mesh->mVertices[v].y * scale.y, mesh->mVertices[v].z * scale.z);
					vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
				}
				else {
					vertex[v].position = XMFLOAT3(mesh->mVertices[v].x * scale.x, -mesh->mVertices[v].z * scale.y, mesh->mVertices[v].y * scale.z);
					vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, -mesh->mNormals[v].z, mesh->mNormals[v].y);
				}
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 頂点カラーを白にしておく
				vertex[v].texcoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);

				// aabbの取得
				if (v == 0 && m == 0) {
					model->local_aabb.min = vertex[v].position;
					model->local_aabb.max = vertex[v].position;
				}
				else {
					model->local_aabb.min.x = std::min(model->local_aabb.min.x, vertex[v].position.x);
					model->local_aabb.min.y = std::min(model->local_aabb.min.y, vertex[v].position.y);
					model->local_aabb.min.z = std::min(model->local_aabb.min.z, vertex[v].position.z);
					model->local_aabb.max.x = std::max(model->local_aabb.max.x, vertex[v].position.x);
					model->local_aabb.max.y = std::max(model->local_aabb.max.y, vertex[v].position.y);
					model->local_aabb.max.z = std::max(model->local_aabb.max.z, vertex[v].position.z);

				}
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(Vertex3d) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = vertex;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

			delete[] vertex;
		}


		// インデックスバッファ生成
		{
			unsigned int* index = new unsigned int[mesh->mNumFaces * 3];

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];

				assert(face->mNumIndices == 3);

				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[1];
				index[f * 3 + 2] = face->mIndices[2];
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * mesh->mNumFaces * 3;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = index;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);
			//DirectXGetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

			delete[] index;
		}

	}

	g_TexId_White = Texture_Load(L"resuce/img/w.png"); // 念の為、白いテクスチャを貼る

	// FBXにテクスチャが内包している場合
	for (unsigned int i = 0; i < model->AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];

		ID3D11ShaderResourceView* texture;
		ID3D11Resource* resource;
		/*
		TexMetadata metadata;
		ScratchImage image;
		LoadFromWICMemory(aitexture->pcData, aitexture->mWidth, WIC_FLAGS_NONE, &metadata, image);
		CreateShaderResourceView(DirectXGetDevice(), image.GetImages(), image.GetImageCount(), metadata, &texture);
		*/

		CreateWICTextureFromMemory( // 内包されたテクスチャの場合のみ
			Direct3D_GetDevice(),				//_In_ ID3D11Device * d3dDevice,
			Direct3D_GetContext(),				//_In_opt_ ID3D11DeviceContext * d3dContext,
			(const uint8_t*)aitexture->pcData,  //_In_reads_bytes_(wicDataSize) const uint8_t * wicData,
			(size_t)aitexture->mWidth,			//_In_ size_t wicDataSize,
			&resource,							//_Outptr_opt_ ID3D11Resource * *texture,
			&texture							//_Outptr_opt_ ID3D11ShaderResourceView * *textureView,
		);										//_In_ size_t maxsize = 0) noexcept;

		assert(texture);

		resource->Release(); // !!!!

		model->Texture[aitexture->mFilename.data] = texture;
	}

	// fbxのファイルパスだけを取得
	const std::string modelPath(FileName);

	// 最後の'/'または'\\'の位置を探す 
	size_t pos = modelPath.find_last_of("/\\");
	std::string directory;

	if (pos != std::string::npos) {
		directory = modelPath.substr(0, pos); // ファイル名を除いた部分
	}
	else {
		directory = ""; // パスに区切りがない場合(ファイル名のみ)
	}

	// テクスチャがFBXとは別に用意されている場合
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) {
		aiString filename;
		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];

		if (filename.length == 0) { // テクスチャが内包されている場合
			continue; // 実行しない(フィルター①)
		}

		// テクスチャパスに余計なフォルダ名が在ったらカット
		std::string str_filename = filename.C_Str();
		size_t n = str_filename.find_last_of("\\/");
		str_filename = str_filename.substr(n + 1);


		if (model->Texture.count(filename.C_Str())) { // すでにファイル名が登録されているか
			continue; // 実行しない(フィルター②)
		}

		ID3D11ShaderResourceView* texture;
		ID3D11Resource* resource;

		std::string texfilename = directory + "/" + str_filename; // ホームから該当ファイルまでのパス＋/＋テクスチャのファイル名

		int len = MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, nullptr, 0);
		wchar_t* pWideFilename = new wchar_t[len]; //  new:一時的に使用
		MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, pWideFilename, len);

		CreateWICTextureFromFile( // Unicode対応なので上で計算
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			pWideFilename,
			&resource,
			&texture
		);

		delete[] pWideFilename;

		assert(texture);

		resource->Release(); // !!!

	}

	return model;

}


void ModelRelease(MODEL* model)
{
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		model->VertexBuffer[m]->Release();
		model->IndexBuffer[m]->Release();
	}

	delete[] model->VertexBuffer;
	delete[] model->IndexBuffer;


	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		pair.second->Release();
	}

	aiReleaseImport(model->AiScene);

	delete model;
}

void ModelDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld){
	// シェーダーを描画パイプラインに設定
	Shader3d_Begin();
	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点シェーダにワールド座標変換行列に設定
	Shader3d_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) // モデルのマテリアル分繰り返す
	{
		aiString texture;
		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texture);

		if (texture.length != 0) { // 下と同じ結果
			//if (texture != aiString("")) { // テクスチャの貼られてない場所を探す
			Direct3D_GetContext()->PSSetShaderResources(0, 1, &model->Texture[texture.data]); // テクスチャを貼る
			// マテリアルの設定
			Shader3d_SetColor({ 1.0f, 1.0f, 1.0f , 1.0f });

		} else {
			Texture_SetTexture(g_TexId_White);
			// マテリアルの設定
			aiColor3D diffuse;
			aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
			Shader3d_SetColor({ diffuse.r, diffuse.g, diffuse.b , 1.0f });
		}

		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);

		// インデックスバッファを描画パイプラインに設定
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		// ポリゴン描画命令発行
		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0); // model->AiScene->mMeshes[m]->mNumFacesが三角形のはずなので * 3　で調点数を求める
	}
}

void ModelDepthDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld) {
	// シェーダーを描画パイプラインに設定
	ShaderDepth_Begin();
	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点シェーダにワールド座標変換行列に設定
	ShaderDepth_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) // モデルのマテリアル分繰り返す
	{
		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);

		// インデックスバッファを描画パイプラインに設定
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		// ポリゴン描画命令発行
		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0); // model->AiScene->mMeshes[m]->mNumFacesが三角形のはずなので * 3　で調点数を求める
	}
}

void ModelUnlitDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld) {
	// シェーダーを描画パイプラインに設定
	Shader3dUnlit_Begin();
	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点シェーダにワールド座標変換行列に設定
	Shader3dUnlit_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) // モデルのマテリアル分繰り返す
	{

		// テクスチャの設定
		if (model->AiScene->mNumTextures) { // モデルにテクスチャが１つでも埋め込まれていれば

			aiString texture;
			aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
			aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texture);

			if (texture.length != 0) { // 下と同じ結果
				//if (texture != aiString("")) { // テクスチャの貼られてない場所を探す
				Direct3D_GetContext()->PSSetShaderResources(0, 1, &model->Texture[texture.data]); // テクスチャを貼る

				// マテリアルの設定
				Shader3dUnlit_SetColor({ 1.0f, 1.0f, 1.0f , 1.0f });

			}
			else {
				Texture_SetTexture(g_TexId_White);
				// マテリアルの設定
				aiColor3D diffuse;
				aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
				Shader3dUnlit_SetColor({ diffuse.r, diffuse.g, diffuse.b , 1.0f });
			}
		}
		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);

		// インデックスバッファを描画パイプラインに設定
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		// ポリゴン描画命令発行
		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0); // model->AiScene->mMeshes[m]->mNumFacesが三角形のはずなので * 3　で調点数を求める
	}
}

void ModelDrawInstanced(MODEL* model, const DirectX::XMMATRIX* mtxWorlds, int instanceCount){
	if (instanceCount <= 0 || !model) return;
	// インスタンスバッファを１度だけ作成
	if (g_pInstanceBuffer == nullptr) {
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(XMMATRIX) * MAX_INSTANCES; // 1000体分
		desc.Usage = D3D11_USAGE_DYNAMIC;                 // CPUから書き込む設定
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;      // Mapを許可する設定

		HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&desc, nullptr, &g_pInstanceBuffer);
		if (FAILED(hr)) {
			assert(!"インスタンスバッファの作成に失敗しました");
			return;
		}
	}

	// 安全装置：最大数を超えないよう
	if (instanceCount > MAX_INSTANCES) instanceCount = MAX_INSTANCES;

	// 1. インスタンスバッファの更新 (これはOK)
	D3D11_MAPPED_SUBRESOURCE ms;
	if (SUCCEEDED(Direct3D_GetContext()->Map(g_pInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
		memcpy(ms.pData, mtxWorlds, sizeof(XMMATRIX) * instanceCount);
		Direct3D_GetContext()->Unmap(g_pInstanceBuffer, 0);
	}

	// 2. インスタンス専用のシェーダー開始
	Shader3d_Instance_Begin();
	
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++) {
		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];

		// 色の更新
		Texture_SetTexture(g_TexId_White);
		// マテリアルの設定
		aiColor3D diffuse;
		aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		Shader3d_Instance_SetColor({ diffuse.r, diffuse.g, diffuse.b , 1.0f });

		// ... 頂点バッファセット ...
		ID3D11Buffer* vbs[2] = { model->VertexBuffer[m], g_pInstanceBuffer };
		UINT strides[2] = { sizeof(Vertex3d), sizeof(InstanceData) };
		UINT offsets[2] = { 0, 0 };
		Direct3D_GetContext()->IASetVertexBuffers(0, 2, vbs, strides, offsets);

		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		Direct3D_GetContext()->DrawIndexedInstanced(
			model->AiScene->mMeshes[m]->mNumFaces * 3, // 1体あたりのインデックス数
			instanceCount,                             // 【ここ！】描画する体数
			0,                                         // 開始インデックス位置
			0,                                         // 開始頂点位置
			0                                          // 開始インスタンス位置
		);
	}
}

AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3& position){
	return {
		{position.x + model->local_aabb.min.x, position.y + model->local_aabb.min.y, position.z + model->local_aabb.min.z},
		{position.x + model->local_aabb.max.x, position.y + model->local_aabb.max.y, position.z + model->local_aabb.max.z}
	};

}






