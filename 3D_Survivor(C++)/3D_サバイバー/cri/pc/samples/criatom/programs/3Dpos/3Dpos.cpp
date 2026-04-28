/****************************************************************************
 *
 * CRI ADX2 LE SDK
 *
 * Copyright (c) 2013-2014 CRI Middleware Co., Ltd.
 *
 * Title   : CRI ADX2 LE sample program
 * Module  : 3Dポジショニング
 * File    : 3Dpos.cpp
 *
 ****************************************************************************/

/**************************************************************************
 * インクルード
 * Header files
 **************************************************************************/
#include <windows.h>
#include <math.h> 

#include <cri_adx2le.h>
#include <CriSmpFramework.h>

/* インゲームプレビューを有効にする場合、定義する */
#define USE_INGAME_PREVIEW

/**************************************************************************
 * 定数マクロ定義
 * Constant macro definitions
 **************************************************************************/
/* ACF/ACBのマクロ定義ヘッダ */
#include "../../data/Public/3Dpos.h"
#include "../../data/Public/ADX2_samples_acf.h"

/* データディレクトリへのパス */
/* Header file for path definitions to data directory */
#define PATH				"../../../data/Public/"

/* サンプルで使用するファイル名 */
#define ACF_FILE			"ADX2_samples.acf"
#define ACB_FILE			"3Dpos.acb"

/* 最大ボイス数を増やすための関連パラメータ */
#define MAX_STANDARD_VOICE	(8)
#define MAX_HCAMX_VOICE		(8)
#define MAX_VOICE			(MAX_STANDARD_VOICE + MAX_HCAMX_VOICE)
#define MAX_VIRTUAL_VOICE	(MAX_VOICE + 8)		/* バーチャルボイスは多め */
#define MAX_CRIFS_LOADER	(MAX_VOICE + 1)		/* 読み込み数も多めに */

/* 最大サンプリングレート（ピッチ変更含む） */
#define MAX_SAMPLING_RATE	(48000*2)

/* HCA-MXコーデックのサンプリングレート */
#define HCAMX_SAMPLING_RATE		(32000)

/* 音源の移動域の最小値と最大値 (200 x 200 x 200 の立方体状空間内を移動する) */
#define SOURCE_POS_MIN				(-100)
#define SOURCE_POS_MAX				(100)

#define X_POS_OFFSET_FOR_MAP_DRAW	(60)
#define Z_POS_OFFSET_FOR_MAP_DRAW	(7)

#define X_POS_SCALE_VAL				(0.2f)
#define Z_POS_SCALE_VAL				(0.08f)

#define MAP_2D_WIDTH				(41)
#define MAP_2D_HEIGHT				(17)

#define GM	(-44.0f)

/**************************************************************************
 * 型定義
 * Type definitions
 **************************************************************************/
/* 3D音源の移動パターン */
typedef enum {
	APP_MORTION_CROSS_FRONT_OF_LISTENER = 0,
	APP_MOTION_PASS_LISTENER_FRONT_TO_REAR,
	APP_MOTION_GO_AROUND_LISTENER,
	APP_MOTION_APPEAR_AT_RANDOM,
} AppSourceMotionPattern;

/* 3Dポジショニングプレビュー項目 */
typedef struct App3dPositioningPreviewItemTag {
	CriAtomExCueId cue_id;
	AppSourceMotionPattern motion_pattern;
	const CriChar8 *description;
} App3dPositioningPreviewItem;

/* アプリケーション構造体 */
typedef struct AppTag{
	/* 音声リソースオブジェクト */
	CriAtomExVoicePoolHn voice_pool;
	CriAtomExVoicePoolHn hcamx_voice_pool; /* ボイスプールハンドル(HCA-MXコーデック用) 	*/
	CriAtomExAcbHn acb_hn;
	CriAtomDbasId dbas_id;

	/* 3D音源 */
	CriAtomExPlayerHn player;			/* 再生プレーヤ */
	CriAtomEx3dSourceHn source;			/* 音源オブジェクト */
	CriAtomExVector source_pos;			/* 音源の位置 */
	CriAtomExVector source_velocity;	/* 音源の速度 */

	/* リスナー */
	CriAtomEx3dListenerHn listener;
	CriAtomExVector listener_pos;		/* リスナーの位置 */
	CriAtomExVector listener_front;		/* リスナーの向いている方向(前方ベクトル) */
	CriAtomExVector listener_top;		/* リスナーの向いている方向(上方ベクトル) */

	/* 3Dポジショニングのプレビューインデックス */
	CriSint32 selecting_preview_index;
	CriSint32 current_preview_index;
} AppObj;

/***************************************************************************
 * 関数定義
 * Function definitions
 ***************************************************************************/
static void user_error_callback_func(const CriChar8 *errid, CriUint32 p1, CriUint32 p2, CriUint32 *parray);
void *user_alloc_func(void *obj, CriUint32 size);
void user_free_func(void *obj, void *ptr);

/* アプリケーション側の実装 */
static void app_display_info(AppObj *app_obj);
static CriBool app_atomex_initialize(AppObj *app_obj);
static CriBool app_atomex_finalize(AppObj *app_obj);
static CriBool app_execute_main(AppObj *app_obj);
static CriBool app_atomex_start(AppObj *app_obj);
static CriBool app_atomex_stop(AppObj *app_obj);
static void app_goto_next_preview(AppObj *app_obj);
static void app_goto_prev_preview(AppObj *app_obj);
static void app_reset_source(AppObj *app_obj);
static void app_update_source(AppObj *app_obj);
static void app_atomex_switch_binaural(AppObj* app_obj);

/**************************************************************************
 * 変数定義
 * Variable definitions
 **************************************************************************/

/* 3D Positioningのプレビューリスト */
static App3dPositioningPreviewItem g_preview_list[] = {
	{CRI_3DPOS_HELI_LOOP,		APP_MORTION_CROSS_FRONT_OF_LISTENER,    "Cross front of Listener"},
	{CRI_3DPOS_FOOTSTEP_LOOP,	APP_MOTION_PASS_LISTENER_FRONT_TO_REAR, "Pass Listener from front to rear"},
	{CRI_3DPOS_FOOTSTEP,	APP_MOTION_GO_AROUND_LISTENER,        "Go around Listener"},
	{CRI_3DPOS_BOMB2,			APP_MOTION_APPEAR_AT_RANDOM,          "Appears at random"},
};

/* 2Dマップの初期状態 */
static const CriChar8 g_map_2d_default[MAP_2D_HEIGHT][MAP_2D_WIDTH + 1] = {
	"                    |z                   ", // 0
	"                    |                    ", // 1
	"                    |                    ", // 2
	"                    |                    ", // 3
	"                    |                    ", // 4
	"                    |                    ", // 5
	"                    |                    ", // 6
	"                    |                    ", // 7
	"-------------------(L)-------------------", // 8 原点にListenerが在る
	"                    |                   x", // 9
	"                    |                    ", //10
	"                    |                    ", //11
	"                    |                    ", //12
	"                    |                    ", //13
	"                    |                    ", //14
	"                    |                    ", //15
	"                    |                    ", //16
};

/* 実際に描画される2Dマップ。3D音源のx, z方向の移動を2D表示する */
static CriChar8 g_map_2d[MAP_2D_HEIGHT][MAP_2D_WIDTH + 1];

/***************************************************************************
 * 関数定義
 * Function definitions
 ***************************************************************************/

/* サンプルのエントリポイント */
int main(void)
{
	/* アプリケーションオブジェクトの定義 */
	AppObj app_obj = {0};
	/* サンプルフレームワークオブジェクト (ウィンドウ管理・デバッグ文字表示用) */
	CriSmpFramework *smpfw = NULL;
	/* キー入力判定用オブジェクト */
	CriSmpInputDevice* idev = NULL;

	/* サンプルフレームワークの作成 */
	smpfw = CriSmpFramework::Create();
	idev = smpfw->GetInputDevice();

	/* アプリケーションの初期化 */
	app_atomex_initialize(&app_obj);

	/* メインループの開始 */
	for (;;) {
		/* ユーザのキー入力情報を更新 */
		idev->Update();
		{
			/* 入力1: 音源の再生開始 */
			if ( idev->IsPressed(idev->BUTTON1) ) {
				app_atomex_start(&app_obj);
			}
			/* 入力2: 音源の再生停止 */
			if ( idev->IsPressed(idev->BUTTON2) ) {
				app_atomex_stop(&app_obj);
			}
			/* 入力3: バイノーラル ON/OFF */
			if (idev->IsPressed(idev->BUTTON3)) {
				app_atomex_switch_binaural(&app_obj);
			}
			/* 入力3: 再生モードを前に移行 */
			if ( idev->IsPressed(idev->BUTTON_UP) ) {
				app_goto_prev_preview(&app_obj);
			}
			/* 入力4: 再生モードを次に移行 */
			if ( idev->IsPressed(idev->BUTTON_DOWN) ) {
				app_goto_next_preview(&app_obj);
			}
		}

		/* アプリケーション終了判定 (ESCキーかウィンドウを閉じられた場合) */
		if ( idev->IsPressed(idev->BUTTON_CANCEL) || smpfw->IsTerninated() == TRUE) {
			break;
		}

		/* アプリケーションの更新 */
		if (app_execute_main(&app_obj) != CRI_TRUE) {
			break;
		}

		/* アプリケーションの情報を表示 */
		smpfw->BeginDraw();
		app_display_info(&app_obj);
		smpfw->EndDraw();

		/* 仮想V同期待ち(タイマーによるスリープ処理) */
		smpfw->SyncFrame();
	}

	/* アプリケーションの終了 */
	app_atomex_finalize(&app_obj);

	/* サンプルフレームワークの破棄 */
	smpfw->Destroy(); smpfw = NULL;

	return CRI_OK;
}

static void app_display_info(AppObj *app_obj)
{
	CriSint32 i = 1;
	CriSmpPrintf( 2, i++, "CRI ADX2 LE Sample Program: 3D Positioning");i+=2;

	/* 再生情報の表示 */
	CriSmpPrintf( 2, i++, "<< 3D Positioning Preview List >>");
	for (CriSint32 n=0; n<sizeof(g_preview_list)/sizeof(App3dPositioningPreviewItem); n++) {
		if (n==app_obj->selecting_preview_index) {
			CriSmpPrintf( 1, i, "*");
		}
		CriSmpPrintf( 3, i, "%d: Cue Name=%s", n, criAtomExAcb_GetCueNameById(app_obj->acb_hn, g_preview_list[n].cue_id));
		CriSmpPrintf(27, i, "Motion Type=%s", g_preview_list[n].description);
		i++;
	}

	i++;
	CriSmpPrintf( 2, i++, "Current Preview Index : %d", app_obj->current_preview_index);

	i++;
	i++;
	CriSmpPrintf( 2, i++, "<Source Info>");
	CriSmpPrintf( 2, i++, "  Motion Type  : %s", g_preview_list[app_obj->current_preview_index].description);
	CriSmpPrintf( 2, i++, "  Position     : (%7.2f, %7.2f, %7.2f)", app_obj->source_pos.x, app_obj->source_pos.y, app_obj->source_pos.z);
	CriSmpPrintf( 2, i++, "  Velocity     : (%7.2f, %7.2f, %7.2f)", app_obj->source_velocity.x, app_obj->source_velocity.y, app_obj->source_velocity.z);

	/* キー入力情報の表示 */
	i=25;
	CriSmpPrintf( 2, i++, "1       key: Start");
	CriSmpPrintf( 2, i++, "2       key: Stop");
	CriSmpPrintf( 2, i++, "3       key: Switch Binaural");
	CriSmpPrintf( 2, i++, "UP/DOWN Key: Select Preview Index");
	CriSmpPrintf( 2, i++, "ESC     key: EXIT");

	/* 3D音源の位置を2Dマップ上に描画する */
	CriSint32 mapped_source_pos_x		= (CriSint32)((app_obj->source_pos.x + SOURCE_POS_MAX) * X_POS_SCALE_VAL);
	CriSint32 mapped_source_pos_z		= (MAP_2D_HEIGHT-1) - (CriSint32)((app_obj->source_pos.z + SOURCE_POS_MAX) * Z_POS_SCALE_VAL);
	memcpy(g_map_2d, g_map_2d_default, MAP_2D_HEIGHT * (MAP_2D_WIDTH + 1));	/* マップ上のガイド情報 */
	for (int i=0; i<MAP_2D_HEIGHT; i++) {
		if (i == mapped_source_pos_z) {
			g_map_2d[i][mapped_source_pos_x] = 'S';
		}
		CriSmpPrintf(X_POS_OFFSET_FOR_MAP_DRAW, i + Z_POS_OFFSET_FOR_MAP_DRAW, "%s", g_map_2d[i]);
	}
	CriSmpPrintf(90, i-5 , "(L:Listener)"); i++;
	CriSmpPrintf(90, i-5 , "(S:Source )");
}

static CriBool app_atomex_initialize(AppObj *app_obj)
{
	/* アプリケーションの初期値設定 */
	app_obj->selecting_preview_index = 0;
	app_obj->current_preview_index = 0;

	/* 音源の初期位置を設定(この時点ではまだ実際に反映されていない) */
	app_obj->source_pos.x	 = 0.0f;	app_obj->source_pos.y	 = 0.0f;	app_obj->source_pos.z	 = 20.0f;

	/* リスナーの初期位置を設定(この時点ではまだ実際に反映されていない) */
	app_obj->listener_pos.x	 = 0.0f;	app_obj->listener_pos.y	 = 0.0f;	app_obj->listener_pos.z	 = 0.0f;
	app_obj->listener_front.x = 0.0f;	app_obj->listener_front.y = 0.0f;	app_obj->listener_front.z = 1.0f;
	app_obj->listener_top.x	 = 0.0f;	app_obj->listener_top.y	 = 1.0f;	app_obj->listener_top.z	 = 0.0f;

	/* エラーコールバック関数の登録 */
	criErr_SetCallback(user_error_callback_func);

	/* メモリアロケータの登録 */
	criAtomEx_SetUserAllocator(user_alloc_func, user_free_func, NULL);

	/* Sound xR インターフェースの登録 */
	criAtomExAsr_RegisterSoundxRInterface(criSoundxR_GetInterface());

	/* ライブラリの初期化設定（最大ボイス数変更） */
	CriAtomExConfig_WASAPI lib_config;
	CriFsConfig fs_config;
	criAtomEx_SetDefaultConfig_WASAPI(&lib_config);
	criFs_SetDefaultConfig(&fs_config);
	lib_config.atom_ex.max_virtual_voices = MAX_VIRTUAL_VOICE;
	lib_config.hca_mx.output_sampling_rate = HCAMX_SAMPLING_RATE;
	fs_config.num_loaders = MAX_CRIFS_LOADER;
	lib_config.atom_ex.fs_config = &fs_config;
	criAtomEx_Initialize_WASAPI(&lib_config, NULL, 0);

	/* D-Basの作成（最大ストリーム数はここで決まります） */
	app_obj->dbas_id = criAtomDbas_Create(NULL, NULL, 0);

	#if defined(USE_INGAME_PREVIEW)
		CriAtomExAsrBusAnalyzerConfig analyze_config;
		CriSint32 bus_no;
		/* インゲームプレビュー用のモニタライブラリを初期化 */
		criAtomExMonitor_Initialize(NULL, NULL, 0);
		/* レベル測定機能を追加 */
		criAtomExAsr_SetDefaultConfigForBusAnalyzer(&analyze_config);
		for ( bus_no=0;bus_no<8;bus_no++) {
			criAtomExAsr_AttachBusAnalyzer(bus_no, &analyze_config);
		}
	#endif

	/* ACFファイルの読み込みと登録 */
	criAtomEx_RegisterAcfFile(NULL, PATH ACF_FILE, NULL, 0);
	
	/* DSP設定のアタッチ */
	criAtomEx_AttachDspBusSetting(CRI_ADX2_SAMPLES_ACF_DSPSETTING_DSPBUSSETTING_0, NULL, 0);

	/* ボイスプールの作成（最大ボイス数変更／最大ピッチ変更／ストリーム再生対応） */
	CriAtomExStandardVoicePoolConfig vpool_config;
	criAtomExVoicePool_SetDefaultConfigForStandardVoicePool(&vpool_config);
	vpool_config.num_voices							= MAX_STANDARD_VOICE;
	vpool_config.player_config.max_sampling_rate	= MAX_SAMPLING_RATE;
	vpool_config.player_config.streaming_flag		= CRI_TRUE;
	app_obj->voice_pool = criAtomExVoicePool_AllocateStandardVoicePool(&vpool_config, NULL, 0);

	/* HCA-MX再生用のボイスプール作成 */
	CriAtomExHcaMxVoicePoolConfig hcamx_config;
	criAtomExVoicePool_SetDefaultConfigForHcaMxVoicePool(&hcamx_config);
	hcamx_config.num_voices							= MAX_HCAMX_VOICE;
	hcamx_config.player_config.max_sampling_rate	= HCAMX_SAMPLING_RATE;
	hcamx_config.player_config.streaming_flag		= CRI_TRUE;
	app_obj->hcamx_voice_pool = criAtomExVoicePool_AllocateHcaMxVoicePool(&hcamx_config, NULL, 0);

	/* ACBファイルを読み込み、ACBハンドルを作成 */
	app_obj->acb_hn = criAtomExAcb_LoadAcbFile(NULL, PATH ACB_FILE, NULL, NULL, NULL, 0);

	/* プレーヤの作成 */
	app_obj->player = criAtomExPlayer_Create(NULL, NULL, 0);

	/* 3Dリスナーハンドルと3D音源ハンドルを作成 */
	app_obj->listener = criAtomEx3dListener_Create(NULL, NULL, 0);
	app_obj->source = criAtomEx3dSource_Create(NULL, NULL, 0);

	/* プレーヤにリスナーと音源のハンドルを登録 */
	criAtomExPlayer_Set3dListenerHn(app_obj->player, app_obj->listener);
	criAtomExPlayer_Set3dSourceHn(app_obj->player, app_obj->source);

	/* リスナーの位置の設定 */
	criAtomEx3dListener_SetPosition(app_obj->listener, &(app_obj->listener_pos)); 

	/* リスナーの向きの設定 */
	criAtomEx3dListener_SetOrientation(app_obj->listener, &(app_obj->listener_front), &(app_obj->listener_top));

	/* リスナーのパラメータを実際に反映 */
	criAtomEx3dListener_Update(app_obj->listener);

	/* 音源の位置の設定＆反映 */	
	criAtomEx3dSource_SetPosition(app_obj->source, &(app_obj->source_pos));

	/* 音源のパラメータを実際に反映 */
	criAtomEx3dSource_Update(app_obj->source);

	return CRI_TRUE;
}

static CriBool app_atomex_finalize(AppObj *app_obj)
{
	/* DSPのデタッチ */
	criAtomEx_DetachDspBusSetting();

	/* プレーヤハンドルの破棄 */
	criAtomExPlayer_Destroy(app_obj->player);
	
	/* ボイスプールの破棄 */
	criAtomExVoicePool_Free(app_obj->voice_pool);
	criAtomExVoicePool_Free(app_obj->hcamx_voice_pool);

	/* ACBハンドルの破棄 */
	criAtomExAcb_Release(app_obj->acb_hn);

	/* ACFの登録解除 */
	criAtomEx_UnregisterAcf();

	#if defined(USE_INGAME_PREVIEW)
		/* インゲームプレビュー関連機能の終了処理 */
		CriSint32 bus_no;
		for ( bus_no=0;bus_no<8;bus_no++) {
			criAtomExAsr_DetachBusAnalyzer(bus_no);
		}
		criAtomExMonitor_Finalize();
	#endif

	/* D-BASの破棄 */
	criAtomDbas_Destroy(app_obj->dbas_id);

	/* ライブラリの終了 */
	criAtomEx_Finalize_WASAPI();

	return CRI_TRUE;
}

CriBool app_execute_main(AppObj *app_obj)
{
	CriAtomExPlayerStatus player_status;

	/* サーバ処理の実行 */
	criAtomEx_ExecuteMain();

#if defined(USE_INGAME_PREVIEW)
	/* バス解析情報の取得 */
	CriAtomExAsrBusAnalyzerInfo analyze_info;
	CriSint32 bus_no;
	for ( bus_no=0;bus_no<8;bus_no++) {
		criAtomExAsr_GetBusAnalyzerInfo(bus_no, &analyze_info);
	}
#endif

	/* Exプレーヤのステータス確認 */
	player_status = criAtomExPlayer_GetStatus(app_obj->player);

	/* 再生エラーになった場合はアプリケーションを終了 */
	if (player_status == CRIATOMEXPLAYER_STATUS_ERROR) {
		return CRI_FALSE;		
	}
		
	/* 音源の位置の更新 */
	if (player_status == CRIATOMEXPLAYER_STATUS_PLAYING) {
		app_update_source(app_obj);
	}
	
	return CRI_TRUE;
}

static CriBool app_atomex_start(AppObj *app_obj)
{
	/* 前の音を停止 */
	//criAtomExPlayer_Stop(app_obj->player);
	criAtomExPlayer_StopWithoutReleaseTime(app_obj->player);

	/* キューIDの指定 */
	criAtomExPlayer_SetCueId(app_obj->player, app_obj->acb_hn, g_preview_list[app_obj->selecting_preview_index].cue_id);

	/* 再生の開始 */
	criAtomExPlayer_Start(app_obj->player);
	
	/* 移動パターンを変更して音源を初期位置に置く */
	app_obj->current_preview_index = app_obj->selecting_preview_index;
	app_reset_source(app_obj);

	return CRI_TRUE;
}

static CriBool app_atomex_stop(AppObj *app_obj)
{
	criAtomExPlayer_Stop(app_obj->player);
	app_reset_source(app_obj);

	return CRI_TRUE;
}

void app_reset_source(AppObj *app_obj)
{
	/* 音源の初期位置を設定 */
	switch (g_preview_list[app_obj->current_preview_index].motion_pattern) {
		case	APP_MORTION_CROSS_FRONT_OF_LISTENER:
			app_obj->source_pos.x	 =-100.0f;		app_obj->source_pos.y	 = 0.0f;		app_obj->source_pos.z	 = 40.0f;
			app_obj->source_velocity.x=0.25f;		app_obj->source_velocity.y= 0.0f;		app_obj->source_velocity.z= 0.0f;
			break;
		case	APP_MOTION_PASS_LISTENER_FRONT_TO_REAR:
			app_obj->source_pos.x	 =   0.0f;		app_obj->source_pos.y	 = 0.0f;		app_obj->source_pos.z	 =100.0f;
			app_obj->source_velocity.x=   0.0f;		app_obj->source_velocity.y= 0.0f;		app_obj->source_velocity.z= -0.25f;
			break;
		case	APP_MOTION_GO_AROUND_LISTENER:
			app_obj->source_pos.x	 =50.0f;		app_obj->source_pos.y	 = 0.0f;		app_obj->source_pos.z	 = 0.0f;
			app_obj->source_velocity.x=0.0f;			app_obj->source_velocity.y= 0.0f;		app_obj->source_velocity.z= 1.0f;
			break;
		case	APP_MOTION_APPEAR_AT_RANDOM:				
			app_obj->source_pos.x	 = (CriFloat32)(rand() % (SOURCE_POS_MAX*2) - SOURCE_POS_MAX);
			app_obj->source_pos.y	 = 0.0f;
			app_obj->source_pos.z	 = (CriFloat32)(rand() % (SOURCE_POS_MAX*2) - SOURCE_POS_MAX);
			app_obj->source_velocity.x=   0.0f;		app_obj->source_velocity.y= 0.0f;		app_obj->source_velocity.z= 0.0f;
			break;
		default:
			app_obj->source_pos.x	 =   0.0f;		app_obj->source_pos.y	 = 0.0f;		app_obj->source_pos.z	 =20.0f;
			app_obj->source_velocity.x= 0.0f;		app_obj->source_velocity.y= 0.0f;		app_obj->source_velocity.z= 0.0f;
			break;
	}

	/* 上で設定したパラメータを実際に登録＆反映させる */
	criAtomEx3dSource_SetVelocity (app_obj->source, &(app_obj->source_velocity)); 
	criAtomEx3dSource_SetPosition(app_obj->source, &(app_obj->source_pos));
	criAtomEx3dSource_Update(app_obj->source);
	return;
}

void app_update_source(AppObj *app_obj)
{
	switch (g_preview_list[app_obj->current_preview_index].motion_pattern) {
		case	APP_MORTION_CROSS_FRONT_OF_LISTENER:
			/* リスナーの前を横切る */
			if (app_obj->source_pos.x < SOURCE_POS_MAX) {
				app_obj->source_pos.x = app_obj->source_pos.x + app_obj->source_velocity.x;
			} else {
				/* 移動域の端まで辿り着いたら初期位置に戻す */
				app_reset_source(app_obj);
			}
			break;
		case	APP_MOTION_PASS_LISTENER_FRONT_TO_REAR:
			/* リスナーの正面から背後へと通り抜ける */
			if (app_obj->source_pos.z >= SOURCE_POS_MIN) {
				app_obj->source_pos.z = app_obj->source_pos.z + app_obj->source_velocity.z;
			} else {
				app_reset_source(app_obj);
			}
			break;
		case	APP_MOTION_GO_AROUND_LISTENER:
			/* リスナー(原点)の周囲を回る */
			{
				CriFloat32 rr = static_cast<CriFloat32>(pow(app_obj->source_pos.x, 2.0f) + pow(app_obj->source_pos.z, 2.0f));
				CriFloat32 acceleration = GM / rr;
				CriFloat32 r = static_cast<CriFloat32>(sqrt(rr));

				app_obj->source_velocity.x = app_obj->source_velocity.x + (acceleration * app_obj->source_pos.x / r);
				app_obj->source_velocity.z = app_obj->source_velocity.z + (acceleration * app_obj->source_pos.z / r);

				app_obj->source_pos.x = app_obj->source_pos.x + app_obj->source_velocity.x;
				app_obj->source_pos.z = app_obj->source_pos.z + app_obj->source_velocity.z;
			}
			break;
		case APP_MOTION_APPEAR_AT_RANDOM:
			/* このモードでは移動しない。初期位置がランダムに決まるだけ */
			break;
		default:
			break;
	}

	/* 上で設定したパラメータを実際に登録＆反映させる */
	criAtomEx3dSource_SetVelocity (app_obj->source, &(app_obj->source_velocity)); 
	criAtomEx3dSource_SetPosition(app_obj->source, &(app_obj->source_pos));
	criAtomEx3dSource_Update(app_obj->source);

	return;
}

static void app_atomex_switch_binaural(AppObj* app_obj)
{
	CriBool is_binaural;

	is_binaural = criAtomExAsr_IsEnabledBinauralizer();

	if (is_binaural == CRI_FALSE) {
		is_binaural = CRI_TRUE;
	} else {
		is_binaural = CRI_FALSE;
	}

	criAtomExAsr_EnableBinauralizer(is_binaural);
}

static void app_goto_next_preview(AppObj *app_obj)
{
	CriSint32 max_items = sizeof(g_preview_list)/sizeof(App3dPositioningPreviewItem);

	app_obj->selecting_preview_index++;
	if (app_obj->selecting_preview_index >= max_items) {
		app_obj->selecting_preview_index = 0;
	}
}

static void app_goto_prev_preview(AppObj *app_obj)
{
	CriSint32 max_items = sizeof(g_preview_list)/sizeof(App3dPositioningPreviewItem);

	app_obj->selecting_preview_index--;
	if (app_obj->selecting_preview_index < 0) {
		app_obj->selecting_preview_index = max_items -1;
	}
}


/* エラーコールバック関数 */
static void user_error_callback_func(const CriChar8 *errid, CriUint32 p1, CriUint32 p2, CriUint32 *parray)
{
	const CriChar8 *errmsg;

	/* エラー文字列の表示 */
	errmsg = criErr_ConvertIdToMessage(errid, p1, p2);
	CriSmpDebugPrintf("%s\n", errmsg);

	return;
}

void *user_alloc_func(void *obj, CriUint32 size)
{
	void *ptr;
	ptr = malloc(size);
	return ptr;
}

void user_free_func(void *obj, void *ptr)
{
	free(ptr);
}
/* --- end of file --- */
