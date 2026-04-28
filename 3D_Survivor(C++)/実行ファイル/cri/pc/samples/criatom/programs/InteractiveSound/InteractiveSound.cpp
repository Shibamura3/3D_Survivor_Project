/****************************************************************************
 *
 * CRI ADX2 LE SDK
 *
 * Copyright (c) 2013-2014 CRI Middleware Co., Ltd.
 *
 * Title   : CRI ADX2 LE sample program
 * Module  : インタラクティブ サウンド
 * File    : InteractiveSound.cpp
 *
 ****************************************************************************/

/**************************************************************************
 * インクルード
 * Header files
 **************************************************************************/
#include <windows.h>
#include <cri_adx2le.h>
#include <CriSmpFramework.h>

/* インゲームプレビューを有効にする場合、定義する */
#define USE_INGAME_PREVIEW

/**************************************************************************
 * 定数マクロ定義
 * Constant macro definitions
 **************************************************************************/
/* ACBファイルのマクロ定義ヘッダ */
#include "../../data/Public/InteractiveSound.h"
/* ACFファイルのマクロ定義ヘッダ */
#include "../../data/Public/ADX2_samples_acf.h"

/* データディレクトリへのパス */
/* Header file for path definitions to data directory */
#define PATH				"../../../data/Public/"

/* サンプルで使用するファイル名 */
/* File names used in this sample */
#define ACF_FILE			"ADX2_samples.acf"
#define ACB_FILE			"InteractiveSound.acb"
#define AWB_FILE			"InteractiveSound.awb"

/* 最大ボイス数を増やすための関連パラメータ */
#define MAX_VOICE			(24)
#define MAX_VIRTUAL_VOICE	(MAX_VOICE + 8)		/* バーチャルボイスは多め */
#define MAX_CRIFS_LOADER	(MAX_VOICE + 8)		/* 読み込み数も多めに */

/* 最大サンプリングレート（ピッチ変更含む） */
#define MAX_SAMPLING_RATE	(48000*2)

/* AISACコントロール値の閾値 */
#define X_POS_OFFSET_FOR_SLIDER_DRAW	(30)
#define Z_POS_OFFSET_FOR_SLIDER_DRAW	(0)

#define X_POS_SCALE_VAL	(50.0f)

#define SLIDER_WIDTH	(51)

/* AISACコントロール値の単位変化量 */
#define DELTA_AISAC_VALUE (0.02f) // 1 ÷ X_POS_SCALE_VAL (単位変化量あたり、スライダー配列の１要素分に相当するつもり）

/**************************************************************************
 * 型定義
 * Type definitions
 **************************************************************************/
/* AISACプレビュー項目 */
typedef struct App3dPositioningPreviewItemTag {
	CriAtomExCueId cue_id;
} App3dPositioningPreviewItem;

/* アプリケーション構造体 */
typedef struct AppTag{
	/* 音声リソースオブジェクト */
	CriAtomExVoicePoolHn voice_pool;
	CriAtomExAcbHn	acb_hn;
	CriAtomExPlayerHn player;			/* 再生プレーヤ */
	CriAtomExAisacControlId  aisac_control_id;
	CriAtomDbasId dbas_id;

	/* 再生キューのプレビューインデックス */
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
static void app_decrease_aisac(AppObj *app_obj);
static void app_increase_aisac(AppObj *app_obj);
static void app_reset_aisac(AppObj *app_obj);

/**************************************************************************
 * 変数定義
 * Variable definitions
 **************************************************************************/
/* AISAC のプレビューリスト */
static App3dPositioningPreviewItem g_preview_list[] = {
	CRI_INTERACTIVESOUND_MULTI_TRACK,
	CRI_INTERACTIVESOUND_IMPACT_SE,
};

/* AISACスライダーの初期状態 */
static const CriChar8 g_slider_default[SLIDER_WIDTH + 1] = {
	"---------------------------------------------------",
};

/* スライダー表示用 */
static CriChar8 g_slider[SLIDER_WIDTH + 1];

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
			/* 入力3: 再生モードを前に移行 */
			if ( idev->IsPressed(idev->BUTTON_UP) ) {
				app_goto_prev_preview(&app_obj);
			}
			/* 入力4: 再生モードを次に移行 */
			if ( idev->IsPressed(idev->BUTTON_DOWN) ) {
				app_goto_next_preview(&app_obj);
			}

			/* 入力5: AISACコントロール値を減少 */
			if ( idev->IsOn(idev->BUTTON_LEFT) ) {
				app_decrease_aisac(&app_obj);
			}
			/* 入力6: AISACコントロール値を増加 */
			if ( idev->IsOn(idev->BUTTON_RIGHT) ) {
				app_increase_aisac(&app_obj);
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
	CriSmpPrintf( 2, i++, "CRI ADX2 LE Sample Program: InteractiveSound");i+=2;

	/* 再生情報の表示 */
	CriSmpPrintf( 2, i++, "<< AISAC Preview List >>");
	for (CriSint32 n=0;n<sizeof(g_preview_list)/sizeof(App3dPositioningPreviewItem);n++) {
		if (n==app_obj->selecting_preview_index) {
			CriSmpPrintf( 1, i, "*");
		}
		CriSmpPrintf( 3, i, "%d: Cue Name=%s", n, criAtomExAcb_GetCueNameById(app_obj->acb_hn, g_preview_list[n].cue_id));
		i++;
	}
	i++;
	CriSmpPrintf( 2, i++, "Current Preview Index : %d", app_obj->current_preview_index);

	CriFloat32 aisac_val = criAtomExPlayer_GetAisacById(app_obj->player, app_obj->aisac_control_id);
	CriSmpPrintf( 2, i++, "Current AISAC Value   : %1.2f", aisac_val);

	/* キー入力情報の表示 */
	i=22;
	CriSmpPrintf( 2, i++, "1          key: Start");
	CriSmpPrintf( 2, i++, "2          key: Stop");
	CriSmpPrintf( 2, i++, "UP/DOWN    Key: Select Preview Index");
	CriSmpPrintf( 2, i++, "LEFT/RIGHT Key: Change AISAC Control Value");
	CriSmpPrintf( 2, i++, "ESC        key: EXIT");

	/* 現在のAISACコントロール値をスライダー上に表す */
	i=9;
	CriSint32 mapped_aisac_cv = (CriSint32)(criAtomExPlayer_GetAisacById(app_obj->player, app_obj->aisac_control_id) * X_POS_SCALE_VAL);
	memcpy(g_slider, g_slider_default, SLIDER_WIDTH + 1);
	if ((i < SLIDER_WIDTH + 1) && (i >= 0)) {
		g_slider[mapped_aisac_cv] = '#';
	}
	CriSmpPrintf(X_POS_OFFSET_FOR_SLIDER_DRAW, i + Z_POS_OFFSET_FOR_SLIDER_DRAW, "%s", g_slider);
}

static CriBool app_atomex_initialize(AppObj *app_obj)
{
	/* アプリケーションの初期値設定 */
	app_obj->selecting_preview_index = 0;
	app_obj->current_preview_index = 0;
	app_obj->aisac_control_id = CRI_INTERACTIVESOUND_AISACCONTROL_ANY;

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
	vpool_config.num_voices = MAX_VOICE;
	vpool_config.player_config.max_sampling_rate = MAX_SAMPLING_RATE;
	vpool_config.player_config.streaming_flag = CRI_TRUE;
	app_obj->voice_pool = criAtomExVoicePool_AllocateStandardVoicePool(&vpool_config, NULL, 0);

	/* ACBファイルを読み込み、ACBハンドルを作成 */
	app_obj->acb_hn = criAtomExAcb_LoadAcbFile(NULL, PATH ACB_FILE, NULL, PATH AWB_FILE, NULL, 0);

	/* プレーヤの作成 */
	app_obj->player = criAtomExPlayer_Create(NULL, NULL, 0);
	app_reset_aisac(app_obj);

	return CRI_TRUE;
}

static CriBool app_atomex_finalize(AppObj *app_obj)
{
	/* DSPのデタッチ */
	criAtomEx_DetachDspBusSetting();

	/* Atomハンドルの破棄 */
	criAtomExPlayer_Destroy(app_obj->player);
	
	/* ボイスプールの破棄 */
	criAtomExVoicePool_Free(app_obj->voice_pool);

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

	return CRI_TRUE;
}

static CriBool app_atomex_start(AppObj *app_obj)
{
	/* 前の音を停止 */
	criAtomExPlayer_Stop(app_obj->player);

	/* キューIDの指定 */
	criAtomExPlayer_SetCueId(app_obj->player, app_obj->acb_hn, g_preview_list[app_obj->selecting_preview_index].cue_id);

	/* 再生の開始 */
	criAtomExPlayer_Start(app_obj->player);

	app_obj->current_preview_index = app_obj->selecting_preview_index;

	return CRI_TRUE;
}

static CriBool app_atomex_stop(AppObj *app_obj)
{
	criAtomExPlayer_Stop(app_obj->player);
	app_reset_aisac(app_obj);

	return CRI_TRUE;
}

void app_reset_aisac(AppObj *app_obj)
{
	/* AISACコントロール値をゼロ初期化 */
	criAtomExPlayer_SetAisacById(app_obj->player, app_obj->aisac_control_id, 0.0f);
	return;
}

static void app_decrease_aisac(AppObj *app_obj)
{
	/* AISACコントロール値を減少 */
	CriFloat32 aisac_val = criAtomExPlayer_GetAisacById(app_obj->player, app_obj->aisac_control_id) - DELTA_AISAC_VALUE;
	criAtomExPlayer_SetAisacById(app_obj->player, app_obj->aisac_control_id, aisac_val);
	criAtomExPlayer_UpdateAll(app_obj->player);
}

static void app_increase_aisac(AppObj *app_obj)
{
	/* AISACコントロール値を減少 */
	CriFloat32 aisac_val = criAtomExPlayer_GetAisacById(app_obj->player, app_obj->aisac_control_id) + DELTA_AISAC_VALUE;
	criAtomExPlayer_SetAisacById(app_obj->player, app_obj->aisac_control_id, aisac_val);
	criAtomExPlayer_UpdateAll(app_obj->player);
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
