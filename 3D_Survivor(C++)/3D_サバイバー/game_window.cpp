//*****************************************************************
//
//
//
//
//******************************************************************

#include "game_window.h"
#include <algorithm>//真ん中にウィンドウをだすために必要
#include "keyboard.h"
#include "mouse.h"
#include "system_timer.h"

//ウィンドウの情報
static constexpr char WINDOW_CLASS[] = "GameWindows";//メインウィンドウクラス
static constexpr char TITLE[] = "3D-Survivar";//タイトルバーのテキスト

//メインウィンドウのサイズ計算　16:9
static constexpr int SCREEN_WIDTH = 16 * 100;
static constexpr int SCREEN_HIGHT = 9 * 100;

//ウィンドウプロシージャのプロトタイプ宣言
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND GameWindow_Create(HINSTANCE hinstance)
{
	//ウィンドウクラスの登録
	WNDCLASSEX wcex{};

	//初期化
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hinstance;
	wcex.hIcon = LoadIcon(hinstance, IDI_APPLICATION);
//	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hCursor = nullptr;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcex.lpszMenuName = nullptr;//メニュバーの表示、今回は使わない
	wcex.lpszClassName = WINDOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	RegisterClassEx(&wcex);


	// メインウィンドウの作成
	RECT window_rect{ 0, 0, SCREEN_WIDTH, SCREEN_HIGHT };
	DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

	AdjustWindowRect(&window_rect, style, FALSE);

	const int WINDOW_WIDTH = window_rect.right - window_rect.left;
	const int WINDOW_HEIGHT = window_rect.bottom - window_rect.top;

	//デスクトップのサイズを取得
	//プライマリモニターの画面解像度取得
	int desktop_width = GetSystemMetrics(SM_CXSCREEN);
	int desktop_height = GetSystemMetrics(SM_CYSCREEN);

	//ウィンドウの表示位置を真ん中に調整する　 	
	const int WINDOW_X = std::max((desktop_width - WINDOW_WIDTH) / 2, 0);
	const int WINDOW_Y = std::max((desktop_height - WINDOW_HEIGHT) / 2, 0);

	HWND hWnd = CreateWindow(
		WINDOW_CLASS,
		TITLE,
		style,
		//WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX),
		//WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
		WINDOW_X,
		WINDOW_Y,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		nullptr, nullptr, hinstance, nullptr);

	return hWnd;
}

//ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_ACTIVATE:
		if (wParam != WA_INACTIVE) {
			//ShowCursor(TRUE);
			SystemTimer_Start();
		}else{
			SystemTimer_Stop();
		}

		break;
	case WM_INPUT:
		return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_ACTIVATEAPP:
		//Keyboard_ProcessMessage(message, wParam, lParam);
		//Mouse_ProcessMessage(message, wParam, lParam);
		SystemTimer_Stop();
		break;
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		Mouse_ProcessMessage(message, wParam, lParam);
		break;
	//case WM_SETCURSOR:
	//	// クライアント領域に入ったらカーソルを消す
	//	if (LOWORD(lParam) == HTCLIENT) {
	//		SetCursor(nullptr);
	//		return TRUE; // Windowsに処理させない
	//	}
	//	break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {//エスケープキーの場合
			SendMessage(hWnd, WM_CLOSE, 0, 0); //sendmessageはオリジナルのメッセージを飛ばすこともできる
		}
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Keyboard_ProcessMessage(message, wParam, lParam);
		break;

	case WM_CLOSE://ウィンドウを閉じようとしたとき、右上の×など
		if (MessageBox(hWnd, "終了してよろしいですか", "確認", MB_YESNO | MB_DEFBUTTON2) == IDYES) {
			DestroyWindow(hWnd);
		}
		break;

	case WM_DESTROY: //ウィンドウの破棄メッセージ
		PostQuitMessage(0);//WM_Quitメッセージの送信
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}