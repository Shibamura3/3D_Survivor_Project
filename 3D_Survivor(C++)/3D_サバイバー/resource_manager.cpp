/*
	リソースファイルの管理：resouce_manager.h

	2026/01/23	hibiki sakuma
*/

#include "resource_manager.h"
#include "texture.h"
#include "Audio.h"

// それぞれの素材情報を格納
static int Resouce_Tex[Tex_ID_MAX]{};
static int Resouce_Audio[Audio_ID_MAX]{};
static MODEL* Resouce_Model[Model_ID_MAX]{};

void Resouce_ManagerInitialize(){

	// 画像の読み込み
	Resouce_Tex[Mousu_Cursor_Red] = Texture_Load(L"resuce/img/Mouse/MouseCursor.png");
	Resouce_Tex[Mousu_Cursor_Blue] = Texture_Load(L"resuce/img/Mouse/MouseClick.png"); 
	Resouce_Tex[Title_Back] = Texture_Load(L"resuce/img/TITLE/Title_back.png");
	Resouce_Tex[Title_Logo] = Texture_Load(L"resuce/img/TITLE/TitleLogo.png");
	Resouce_Tex[Result_Back] = Texture_Load(L"resuce/img/RESULT/Result_back.png");
	Resouce_Tex[Achieve_Back] = Texture_Load(L"resuce/img/ACHIEVE/Achieve_back.png");
	Resouce_Tex[Start_Button] = Texture_Load(L"resuce/img/TITLE/Start_Button.png");
	Resouce_Tex[Achieve_Button] = Texture_Load(L"resuce/img/TITLE/Achieve_Button.png");
	Resouce_Tex[Title_Button] = Texture_Load(L"resuce/img/RESULT/Title_Button.png");
	Resouce_Tex[Achieve_Lock] = Texture_Load(L"resuce/img/ACHIEVE/lock.png");
	Resouce_Tex[Achieve_UnLock] = Texture_Load(L"resuce/img/ACHIEVE/UnLock.png");
	Resouce_Tex[UI_Case] = Texture_Load(L"resuce/img/UI/Case.png");
	Resouce_Tex[UI_Exp] = Texture_Load(L"resuce/img/UI/EXPText.png");
	Resouce_Tex[UI_Hp] = Texture_Load(L"resuce/img/UI/HPText.png");
	Resouce_Tex[Effect_Trajectory] = Texture_Load(L"resuce/img/effect000.jpg");
	Resouce_Tex[Effect_Explosion] = Texture_Load(L"resuce/img/Effect_0.png");
	Resouce_Tex[Icon_Pad] = Texture_Load(L"resuce/img/pad.png");
	Resouce_Tex[Icon_Keyboard] = Texture_Load(L"resuce/img/keyboard.png");
	Resouce_Tex[Pause] = Texture_Load(L"resuce/img/pause.png");
	Resouce_Tex[LevelUP] = Texture_Load(L"resuce/img/level.png");
	Resouce_Tex[LevelUP_HP] = Texture_Load(L"resuce/img/max_hp.png");
	Resouce_Tex[LevelUP_SPEED] = Texture_Load(L"resuce/img/speed.png");
	Resouce_Tex[LevelUP_Bullet] = Texture_Load(L"resuce/img/shotInterval.png");
	Resouce_Tex[Cube] = Texture_Load(L"resuce/img/box.png");
	Resouce_Tex[Color_Red] = Texture_Load(L"resuce/img/UI/red.png");
	Resouce_Tex[Color_White] = Texture_Load(L"resuce/img/w.png");
	Resouce_Tex[Color_Blue] = Texture_Load(L"resuce/img/UI/green.png");
	Resouce_Tex[Color_Black] = Texture_Load(L"resuce/img/black.png");
	Resouce_Tex[Color_Ground1] = Texture_Load(L"resuce/img/ground.png");
	Resouce_Tex[Color_Ground2] = Texture_Load(L"resuce/img/ground2.png");

	// モデルの読み込み
	Resouce_Model[Player1] = ModelLoad("resuce/3Dmodel/PlayerPic/player_bule.fbx", 0.5f);
	Resouce_Model[Bullet_Model] = ModelLoad("resuce/3Dmodel/bullet.fbx", 0.10f);
	Resouce_Model[Sky] = ModelLoad("resuce/3Dmodel/sky.fbx", 100.0f, true);
	Resouce_Model[Stone] = ModelLoad("resuce/3Dmodel/stone.fbx", 1.0f);
	Resouce_Model[Wood] = ModelLoad("resuce/3Dmodel/wood.fbx", 0.250f);

	// 音源の読み込み
	Resouce_Audio[Title_BGM] = LoadAudio("resuce/audio/bgm.wav");
	Resouce_Audio[Check_SE] = LoadAudio("resuce/audio/SE_check.wav");
	Resouce_Audio[Game_BGM] = LoadAudio("resuce/audio/bgm.wav");
	Resouce_Audio[Bullet_Hit_SE] = LoadAudio("resuce/audio/bullet_hit.wav");
	Resouce_Audio[Bullet_SE] = LoadAudio("resuce/audio/bullet.wav");
}

void Resouce_ManagerFinalize(){
	Texture_AllRelease();

	for (int i = 0; i < Audio_ID_MAX; i++) {
		UnloadAudio(Resouce_Audio[i]);
	}

	for (int i = 0; i < Model_ID_MAX; i++) {
		ModelRelease(Resouce_Model[i]);
	}
}

int Resouce_Manager_GetTexId(Tex_ID id){
	return Resouce_Tex[id];
}

int Resouce_Manager_GetAudioId(Audio_ID id){
	return Resouce_Audio[id];
}

MODEL* Resouce_Manager_GetModelId(Model_ID id){
	return Resouce_Model[id];
}
