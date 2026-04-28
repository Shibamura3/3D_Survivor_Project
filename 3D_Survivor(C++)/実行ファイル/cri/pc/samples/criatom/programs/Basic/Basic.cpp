/****************************************************************************
 *
 * CRI ADX2 LE SDK
 *
 * Copyright (c) 2013-2014 CRI Middleware Co., Ltd.
 *
 * Title   : CRI ADX2 LE sample program
 * Module  : 基本再生制御サンプル(HCA-MX再生対応)
 * File    : Basic.cpp
 *
 ****************************************************************************/

/**************************************************************************
 * インクルード
 * Header files
 **************************************************************************/
#include <cri_adx2le.h>
#include <CriSmpFramework.h>

/* インゲームプレビューを有効にする場合、定義する */
#define USE_INGAME_PREVIEW

/**************************************************************************
 * 定数マクロ定義
 * Constant macro definitions
 **************************************************************************/
/* ACF/ACBのマクロ定義ヘッダ */
#include "../../data/Public/Basic.h"
#include "../../data/Public/ADX2_samples_acf.h"

/* データディレクトリへのパス */
#define PATH	"../../../data/Public/"

/* サンプルで使用するファイル名 */
#define ACF_FILE			"ADX2_samples.acf"
#define ACB_FILE			"Basic.acb"
#define AWB_FILE			"Basic.awb"

/* 最大ボイス数を増やすための関連パラメータ */
#define MAX_VOICE			(24)
#define MAX_VIRTUAL_VOICE	(64)		/* ざっくり多め(通常ボイス＋HCA-MXボイス＋α) */
#define MAX_CRIFS_LOADER	(64)		/* ざっくり多め(通常ボイス＋HCA-MXボイス＋α) */

/* 最大サンプリングレート（ピッチ変更含む） */
#define MAX_SAMPLING_RATE	(48000*2)

/* HCA-MXコーデックのサンプリングレート */
#define SAMPLINGRATE_HCAMX		(32000)


#define X_POS_OFFSET_FOR_MAP_DRAW	(20)
#define PITCH_CHANGE_VALUE			(-200.0f)

/**************************************************************************
 * 型定義
 * Type definitions
 **************************************************************************/

/* キューリストアイテム */
typedef struct AppCueListItemTag {
	CriAtomExCueId id;
} AppCueListItem;

/* アプリケーション構造体 */
typedef struct AppTag{
	CriAtomExPlayerHn		player;		/* 再生プレーヤ */
	CriAtomExVoicePoolHn	standard_voice_pool;	/* ボイスプール(ADX/HCAコーデック用) */
	CriAtomExVoicePoolHn	hcamx_voice_pool;		/* ボイスプール(HCA-MX用) */
	CriAtomExAcbHn			acb_hn;		/* ACBハンドル(音声データ) */
	CriAtomDbasId			dbas_id;	/* D-BASの作成*/

	CriAtomExPlaybackId		playback_id;	/* VoiceキューのプレイバックID(再生開始時に保持する) */

	CriSint32 ui_cue_idnex;				/* ユーザ選択中のキュー   */

} AppObj;

/***************************************************************************
 * 関数宣言
 * Function declarations
 ***************************************************************************/
static void user_error_callback_func(const CriChar8 *errid, CriUint32 p1, CriUint32 p2, CriUint32 *parray);
static void *user_alloc_func(void *obj, CriUint32 size);
static void user_free_func(void *obj, void *ptr);

static void app_display_info(AppObj *app_obj);
static CriBool app_atomex_initialize(AppObj *app_obj);
static CriBool app_atomex_finalize(AppObj *app_obj);
static CriBool app_execute_main(AppObj *app_obj);

static CriBool app_atomex_start(AppObj *app_obj);
static CriBool app_atomex_stop_player(AppObj *app_obj);
static CriBool app_atomex_stop_cue(AppObj *app_obj);

/**************************************************************************
 * 変数定義
 * Variable definitions
 **************************************************************************/

/* キュー一覧 */
static AppCueListItem g_cue_list[] = {
	CRI_BASIC_MUSIC1,
	CRI_BASIC_MUSIC2,
	CRI_BASIC_VOICE_RANDOM,
	CRI_BASIC_VOICE_EFFECT,
	CRI_BASIC_KALIMBA,
	CRI_BASIC_GUNSHOT,
	CRI_BASIC_BOMB_LIMIT,
	CRI_BASIC_PITCH_UP_M2,
	CRI_BASIC_PITCH_DEF_M2
};
static CriUint32 g_num_cue_items = sizeof(g_cue_list)/sizeof(AppCueListItem);

static CriChar8* g_playback_status_description[] = {
	"--",
	"Prepare",
	"Playing",
	"Removed",
};

/***************************************************************************
 * 関数定義
 * Function definitions
 ***************************************************************************/

int main(void) {
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

	for (;;) {
		/* ユーザのキー入力情報を更新 */
		idev->Update();
		{
			/* 入力1: キュー再生 */
			if ( idev->IsPressed(idev->BUTTON1) ) {
				app_atomex_start(&app_obj);
			}
			/* 入力2: プレーヤ停止 (全体停止) */
			if ( idev->IsPressed(idev->BUTTON2) ) {
				app_atomex_stop_player(&app_obj);
			}
			/* 入力3: 特定の再生音に対する停止 (本サンプルではVoiceキューのみ停止) */
			if ( idev->IsPressed(idev->BUTTON3) ) {
				app_atomex_stop_cue(&app_obj);
			}

			/* 入力4: キュー選択（上) */
			if ( idev->IsPressed(idev->BUTTON_UP) ) {
				app_obj.ui_cue_idnex--;
				if (app_obj.ui_cue_idnex<0) {
					app_obj.ui_cue_idnex = g_num_cue_items - 1;
				}
			}
			/* 入力5: キュー選択（下) */
			if ( idev->IsPressed(idev->BUTTON_DOWN) ) {
				app_obj.ui_cue_idnex++;
				if (app_obj.ui_cue_idnex == g_num_cue_items) {
					app_obj.ui_cue_idnex = 0;
				}
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

	return 0;
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

static void app_display_info(AppObj *app_obj)
{
	CriSint32 i = 1;
	CriSmpPrintf( 2, i++, "CRI ADX2 LE Sample Program: Basic");i++;

	/* ボイスプール内のボイス使用状況を表示 */
	CriSint32 current_vnum, vlimit;
	criAtomExVoicePool_GetNumUsedVoices(app_obj->standard_voice_pool, &current_vnum, &vlimit);
	CriSmpPrintf( 2, i++, "Standard Voice (Playing / Max): %d/%d", current_vnum, vlimit);
	criAtomExVoicePool_GetNumUsedVoices(app_obj->hcamx_voice_pool, &current_vnum, &vlimit);
	CriSmpPrintf( 2, i++, "HCA-MX Voice   (Playing / Max): %d/%d", current_vnum, vlimit);i+=2;

	/* キューのリスト情報を表示 */
	CriSmpPrintf( 2, i++, "-- Cue List --");
	for (CriUint32 n=0;n<g_num_cue_items;n++) {

		if (n==app_obj->ui_cue_idnex) {
			CriSmpPrintf(2, i+n, "*");
		}

		CriSmpPrintf( 4, i+n, "%s", criAtomExAcb_GetCueNameById(app_obj->acb_hn, g_cue_list[n].id));

		/* Voiceキューの名前の隣にだけ、そのキューの再生状態と時刻を表示する */
		if (g_cue_list[n].id == CRI_BASIC_VOICE_RANDOM) {
			CriAtomExPlaybackStatus playback_state = criAtomExPlayback_GetStatus(app_obj->playback_id);
			CriSint64 playback_time = criAtomExPlayback_GetTime(app_obj->playback_id);
			CriSmpPrintf( 4 + X_POS_OFFSET_FOR_MAP_DRAW, i+n, "[Status: %s,  Time:%lld msec.]",
				g_playback_status_description[playback_state], playback_time);
		}
	}

	/* キー入力情報の表示 */
	i=24;
	CriSmpPrintf( 2, i++, "1       key: Start");
	CriSmpPrintf( 2, i++, "2       key: Stop All");
	CriSmpPrintf( 2, i++, "3       key: Stop Only Voice");
	CriSmpPrintf( 2, i++, "UP/DOWN Key: Select Preview Index");
	CriSmpPrintf( 2, i++, "ESC     key: EXIT");
}

CriBool app_atomex_initialize(AppObj *app_obj)
{
	/* カーソルのリセット */
	app_obj->ui_cue_idnex = 0;
	/* 未取得なプレイバックID(Voiceキュー再生時のみ取得) */
	app_obj->playback_id = 0;

	/* エラーコールバック関数の登録 */
	criErr_SetCallback(user_error_callback_func);

	/* メモリアロケータの登録 */
	criAtomEx_SetUserAllocator(user_alloc_func, user_free_func, NULL);

	/* ライブラリの初期化（最大ボイス数変更） */
	CriAtomExConfig_WASAPI lib_config;
	CriFsConfig fs_config;
	criAtomEx_SetDefaultConfig_WASAPI(&lib_config);
	criFs_SetDefaultConfig(&fs_config);
	lib_config.atom_ex.max_virtual_voices = MAX_VIRTUAL_VOICE;
	lib_config.hca_mx.output_sampling_rate = SAMPLINGRATE_HCAMX;
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
	CriAtomExStandardVoicePoolConfig standard_vpool_config;
	criAtomExVoicePool_SetDefaultConfigForStandardVoicePool(&standard_vpool_config);
	standard_vpool_config.num_voices = MAX_VOICE;
	standard_vpool_config.player_config.max_sampling_rate = MAX_SAMPLING_RATE;
	standard_vpool_config.player_config.streaming_flag = CRI_TRUE;
	app_obj->standard_voice_pool = criAtomExVoicePool_AllocateStandardVoicePool(&standard_vpool_config, NULL, 0);

	/* HCA-MX再生用：ボイスプールの作成 */
	CriAtomExHcaMxVoicePoolConfig hcamx_vpool_config;
	criAtomExVoicePool_SetDefaultConfigForHcaMxVoicePool(&hcamx_vpool_config);
	hcamx_vpool_config.num_voices = MAX_VOICE;
	hcamx_vpool_config.player_config.max_sampling_rate = MAX_SAMPLING_RATE;
	hcamx_vpool_config.player_config.streaming_flag = CRI_TRUE;
	app_obj->hcamx_voice_pool = criAtomExVoicePool_AllocateHcaMxVoicePool(&hcamx_vpool_config, NULL, 0);

	/* ACBファイルを読み込み、ACBハンドルを作成 */
	app_obj->acb_hn = criAtomExAcb_LoadAcbFile(NULL, PATH ACB_FILE, NULL, PATH AWB_FILE, NULL, 0);

	/* プレーヤの作成 */
	app_obj->player = criAtomExPlayer_Create(NULL, NULL, 0);

	return CRI_TRUE;
}

CriBool app_atomex_finalize(AppObj *app_obj)
{
	/* DSPのデタッチ */
	criAtomEx_DetachDspBusSetting();

	/* プレーヤハンドルの破棄 */
	criAtomExPlayer_Destroy(app_obj->player);
	
	/* ボイスプールの破棄 */
	criAtomExVoicePool_Free(app_obj->hcamx_voice_pool);
	criAtomExVoicePool_Free(app_obj->standard_voice_pool);

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

	return CRI_TRUE;
}

static CriBool app_atomex_start(AppObj *app_obj)
{
	CriAtomExCueId start_cue_id = g_cue_list[app_obj->ui_cue_idnex].id;

	/* キューIDの指定 */
	criAtomExPlayer_SetCueId(app_obj->player, app_obj->acb_hn, start_cue_id);

	/* MEMO: 特定の音だけピッチを変えて再生したい場合。      */
	/* (1) プレーヤにピッチを設定。                          */
	/* (2) 再生開始。                                        */
	/* (3) プレーヤのピッチを戻す。                          */
	/* {                                                     */
	/*   criAtomExPlayer_SetPitch(player, pitch);          */
	/*   criAtomExPlayer_SetCueId(player, acb, cue_id);  */
	/*   criAtomExPlayer_Start(player, pitch);             */
	/*   criAtomExPlayer_SetPitch(player, 0.0f);           */
	/* {                                                     */
	/* 補足: HCA-MXコーデックの場合はピッチ変更は無効。      */

	/* 再生の開始 */
	CriAtomExPlaybackId playback_id = criAtomExPlayer_Start(app_obj->player);

	/* Voiceキューのみ、再生時刻取得のためプレイバックIDを保存する */
	if (start_cue_id == CRI_BASIC_VOICE_RANDOM) {
		app_obj->playback_id = playback_id;
	}

	return CRI_TRUE;
}

static CriBool app_atomex_stop_player(AppObj *app_obj)
{
	/* プレーヤの停止 */
	criAtomExPlayer_Stop(app_obj->player);

	return CRI_TRUE;
}

static CriBool app_atomex_stop_cue(AppObj *app_obj)
{
	/* 特定の再生音のみ停止 */
	criAtomExPlayback_Stop(app_obj->playback_id);

	return CRI_TRUE;
}
