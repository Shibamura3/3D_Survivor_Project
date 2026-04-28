/*
	リソースファイルの管理：resouce_manager.h

	2026/01/23	hibiki sakuma
*/
#ifndef RESOUCE_MANAGER_H
#define RESOUCE_MANAGER_H

#include "model.h"

enum Tex_ID
{
	Mousu_Cursor_Red,
	Mousu_Cursor_Blue,
	Title_Back,
	Title_Logo,
	Result_Back,
	Achieve_Back,
	Start_Button,
	Achieve_Button,
	Title_Button,
	Achieve_Lock,
	Achieve_UnLock,
	UI_Case,
	UI_Exp,
	UI_Hp,
	Effect_Trajectory,
	Effect_Explosion,
	Icon_Pad,
	Icon_Keyboard,
	Pause,
	Cube,
	LevelUP,
	LevelUP_HP,
	LevelUP_SPEED,
	LevelUP_Bullet,
	Color_Red,
	Color_White,
	Color_Blue,
	Color_Black,
	Color_Ground1,
	Color_Ground2,

	// 追加
	Tex_ID_MAX
};

enum Audio_ID
{
	Title_BGM,
	Game_BGM,
	Bullet_Hit_SE,
	Check_SE,
	Bullet_SE,
	// 追加
	Audio_ID_MAX
};

enum Model_ID
{
	Player1,
	Bullet_Model,
	Sky,
	Stone,
	Wood,

	// 追加
	Model_ID_MAX
};

void Resouce_ManagerInitialize();
void Resouce_ManagerFinalize();

int Resouce_Manager_GetTexId(Tex_ID id);
int Resouce_Manager_GetAudioId(Audio_ID id);
MODEL* Resouce_Manager_GetModelId(Model_ID id);

#endif // !RESOUCE_MANAGER_H
