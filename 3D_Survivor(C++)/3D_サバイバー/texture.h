//テクスチャ管理
//texture.h

#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H

#include <D3d11.h>
#include "sprite.h"

void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Texture_Finalize(void);

//テクスチャ画像の読み込み
//
//戻り値 : 管理番号 読み込めなかったら -1
//
int Texture_Load(const wchar_t* pFilename);
//部分開放は今回使わない、画像1つだけ解放する

//今回は全部の画像を一度にすべて解放する
void Texture_AllRelease();

void Texture_SetTexture(int texid, int slot = 0);

//テクスチャの縦横を取得する
unsigned int Texture_Width(int texid);
unsigned int Texture_Height(int texid);

#endif // !TEXTURE_H
