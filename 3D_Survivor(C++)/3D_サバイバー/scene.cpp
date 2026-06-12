/*
	画面遷移の制御：scene.h

	2025/07/11	hibiki sakuma
*/

#include "scene.h"
#include "Game.h"
#include "Result.h"
#include "title_scene.h"
#include "achieve.h"
static Scene g_Scene_now = SCENE_TITLE; // デバックする場合は該当シーンに書き換える
//static Scene g_Scene_now = SCENE_GAME; // デバックする場合は該当シーンに書き換える
static Scene g_Scene_next = g_Scene_now;

void Scene_Initialize(){
	switch (g_Scene_now) {
	case SCENE_TITLE:
		TitleScene_Initialize();
		break;
	case SCENE_ACHIEVE:
		Achieve_Initialize();
		break;
	case SCENE_GAME:
		Game_Initialize();
		break;
	case SCENE_RESULT:
		Result_Initialize();
		break;
	case SCENE_MAX:

		break;
	default:

		break;
	}
}

void Scene_Finalize(){
	switch (g_Scene_now) {
	case SCENE_TITLE:
		TitleScene_Finalize();
		break;
	case SCENE_ACHIEVE:
		Achieve_Finalize();
		break;
	case SCENE_GAME:
		Game_Finalize();
		break;
	case SCENE_RESULT:
		Result_Finalize();
		break;
	case SCENE_MAX:

		break;
	default:

		break;
	}
}

void Scene_Update(double elapsed_time){
	switch (g_Scene_now) {
	case SCENE_TITLE:
		TitleScene_Update(elapsed_time);
		break;
	case SCENE_ACHIEVE:
		Achieve_Update(elapsed_time);
		break;
	case SCENE_GAME:
		Game_UpDate(elapsed_time);
		break;
	case SCENE_RESULT:
		Result_Update(elapsed_time);
		break;
	case SCENE_MAX:

		break;
	default:

		break;
	}
}

void Scene_Draw(){
	switch (g_Scene_now) {
	case SCENE_TITLE:
		TitleScene_Draw();
		break;
	case SCENE_ACHIEVE:
		Achieve_Draw();
		break;
	case SCENE_GAME:
		Game_Draw();
		break;
	case SCENE_RESULT:
		Result_Draw();
		break;
	case SCENE_MAX:
		
		break;
	default:
		
		break;
	}
}

void Scene_Refresh(){
	if (g_Scene_now != g_Scene_next) { // シーンチェンジをするタイミング
		// 現在のシーンのあと片付け
		Scene_Finalize();
		// 現在のシーンを次のシーンに移す
		g_Scene_now = g_Scene_next;
		// 次のシーンの初期化
		Scene_Initialize();
	}
}


void Scene_Change(Scene scene){
	g_Scene_next = scene;
}
