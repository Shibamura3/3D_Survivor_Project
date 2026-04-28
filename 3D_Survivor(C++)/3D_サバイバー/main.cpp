//ウィンドウの表示　[main.cpp]
//2025/06/03---2025/06/06
//
#ifndef WINVER
#define WINVER 0x0A00          // Windows 10 を指定
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00    // Windows 10 を指定
#endif

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cri_adx2le.h>
#include "game_window.h"
#include "direct3d.h"
#include "shader.h"
#include "shader3d.h"
#include "shader_depth.h"
#include "shader3d_unlit.h"
#include "shader3d_instance.h"
#include "sampler.h"
#include "sprite.h"
#include "texture.h"
#include "sprite_animu.h"
#include "achievementmanager.h"
#include "fade.h"
#include "collision.h"
#include "debug_text.h"
#include "system_timer.h"
#include "Audio.h"
#include "key_logger.h" //キーボード入力
#include "resource_manager.h"
//キーパッド入力
#include <Xinput.h>
#include "pad_logger.h"
#include "mouse.h"
#pragma comment(lib,"xinput.lib")
#include "scene.h"

#include "cube.h"
#include "light.h"
#include "meshfield.h"
#include <DirectXMath.h>
using namespace DirectX;

//ウィンドウの表示メイン文
int APIENTRY WinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{

	//一箇所描いておけばok ないと音と画像など読み込むときに使えなくなる
	(void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	// カーソルを1度だけ呼ぶ
	ShowCursor(FALSE);
	//DPIスケーリング
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	//分割したウィンドウ表示の呼び出し
	HWND hWnd = GameWindow_Create(hinstance);

	//Direct3dの初期化
	SystemTimer_Initialize();
	KeyLogger_Initialize();
	PadLogger_Initalize();
	Mouse_Initialize(hWnd);
	InitAudio(); // Audio_Initialize

	Direct3D_Initialize(hWnd);
	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Shader3d_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Shader3dUnlit_Initialize();
	ShaderDepth_Initialize();
	Shader3d_Instance_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Sampler_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Texture_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	AchievementManager::Instance().Initialize();
	Sprite_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	SpriteAnim_Initialize();
	Fade_Initialize();
	Resouce_ManagerInitialize();
	//ウィンドウ上にマウスを表示しなくなる
	//Mouse_SetVisible(false);
	//相対座標モード
	//Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);

	Scene_Initialize(); // ゲームのシーン全体の処理が入っている
	Light_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Cube_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
#if defined(DEBUG) || defined(_DEBUG) //リリースの時は無視される
	//Direct3D_Initialize(hWnd);以降にかく
	//フォントの初期化
	hal::DebugText dt(Direct3D_GetDevice(),
		Direct3D_GetContext(),
		L"consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(),
		Direct3D_GetBackBufferHeight(),
		0.0f, 0.0f,
		0, 0,
		0.0f, 20.0f);

	Collision_DebudInitialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Collision_Debud3DInitialize(Direct3D_GetDevice(), Direct3D_GetContext());
#endif

	//ウィンドウの表示
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//fps.実行フレーム計測用
	double exec_last_time = SystemTimer_GetTime();
	double fps_last_time = exec_last_time;
	double current_time = exec_last_time;
	ULONG frame_count = 0;
	double fps = 0.0;
	double elapsed_time = 0.0f;

	//メッセージループ
	MSG msg;

	do {
		//peekメッセージを貯めてから処理
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) { // ウィンドウメッセージが来ていたら 
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}
		else { // ゲームの処理 
			// ウィンドウが最小化されている、またはアクティブでない場合は少し待機してループを回すだけにする
			if (IsIconic(hWnd) || GetForegroundWindow() != hWnd) {
				Sleep(10); // CPU負荷を抑える
				SystemTimer_GetElapsedTime(); // 溜まった時間を空読みして捨てる
				continue;
			}
			//fps計測
			current_time = SystemTimer_GetTime(); //システム時刻を取得
			double fps_elapsed_time = current_time - fps_last_time; // fps計測用の経過時間こ計算

			if (fps_elapsed_time >= 1.0) {// 1秒ごとに計測
				fps = frame_count / fps_elapsed_time;
				fps_last_time = current_time; //  FPSを測定した時刻を取得
				frame_count = 0; // フレームカウントをクリア

			}

			// 実行フレームの計測 1/60ごとに実行
			//elapsed_time += SystemTimer_GetElapsedTime();
			
			//elapsed_time = SystemTimer_GetElapsedTime();
			//elapsed_time = std::clamp(elapsed_time, 0.0, 0.1);

			elapsed_time += SystemTimer_GetElapsedTime();
			if (elapsed_time >= 1.0 / 15.0/*処理落ちフレーム数*/) {
				elapsed_time = 1.0 / 15.0; // 時間を固定する
			}

			//if (elapsed_time >= (1.0 / 60.0)) {
			{
				//if(true){
				//exec_last_time = current_time; // 処理した時刻を保存

				// ゲームの更新
				KeyLogger_Update();
				PadLogger_Update();
				Scene_Update(elapsed_time);// ゲームのアップデート
				SpriteAnim_Update(elapsed_time);
				Fade_Update(elapsed_time);

				//ゲームの描画
				Direct3D_SetBackBuffer();// 画用紙を白（クリアー）する
				Direct3D_ClearBackBuffer();
				Scene_Draw();// ゲーム本体の描画
				Fade_Draw();
#if defined(DEBUG) || defined(_DEBUG) 
				std::stringstream ss;//coutと同じ
				ss << "fps:" << fps << std::endl;
				dt.SetText(ss.str().c_str());
				dt.Draw();
				dt.Clear();
#endif

				Direct3D_Present();// 画用紙にプレゼントする

				Scene_Refresh();

				frame_count++;
				elapsed_time = 0.0;

			}
		}
	} while (msg.message != WM_QUIT);

	// 描画とは逆順に後片付け
#if defined(DEBUG) || defined(_DEBUG) 
	Collision_Debud3DFinalize();
	Collision_DebudFinalize();
#endif

	Resouce_ManagerInitialize();
	Mouse_Finalize(); 
	MeshField_Finalize();
	Light_Finalize();
	Cube_Finalize();
	Scene_Finalize();

	Fade_Finalize();
	SpriteAnim_Finalize();
	Sprite_Finalize();
	Texture_Finalize();
	Sampler_Finalize();
	Shader3d_Instance_Finalize();
	ShaderDepth_Finalize();
	Shader3dUnlit_Finalize();
	Shader3d_Finalize();
	Shader_Finalize();
	Direct3D_Finalize();
	UninitAudio(); // Audio__Finalize

	CoUninitialize();

	return (int)msg.wParam;

}