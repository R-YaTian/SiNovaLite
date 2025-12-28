// PubNova.cpp : Defines the exported functions for the DLL application.
#include "stdafx.h"
#include <cstring>
#include <atlbase.h>

// const
#define VERSION  10005
#define KEY_MAX  190
#define DKEYs    19
#define SetKeys  12
#define TYPEIN_W 1
#define WCHAR_l  20

typedef DWORD(*EVALFUNC)(LPCSTR);

// core
HWND
	RGSS_hWnd = 0,
	TypeIn_hWnd = 0 ;

WNDPROC
	RGSS_Proc,
	TypeIn_Proc ;

HMODULE hRGSS;
EVALFUNC hRGSSEval;

// variable
int
	sm_border_W, //窗口边框宽
	sm_title_H,  //窗口标题高
	sm_cxscreen, //系统分辨率x
	sm_cyscreen, //系统分辨率y
	mouseZ ;     //鼠标滚轮状态

unsigned int
	*key_states,
	*dkey_states,
	dkey_map[SetKeys] ;

bool
	key_press[KEY_MAX],
	dkey_press[DKEYs],
	ProcessEnd = false ;

// temp
WCHAR WCHAR_temp[WCHAR_l];
TCHAR szPath[MAX_PATH] = {0};


// functions
VOID rgss_screen_rect(RECT* rect)
{
	GetWindowRect( RGSS_hWnd, rect ) ;
	rect->left   += sm_border_W ;
	rect->right  -= sm_border_W ;
	rect->top    += sm_border_W + sm_title_H ;
	rect->bottom -= sm_border_W ;
}

VOID getPath(TCHAR szPath[MAX_PATH]) //获取模块所在目录
{
	GetModuleFileName(NULL, szPath, MAX_PATH);
	PathRemoveFileSpec(szPath);
	PathAddBackslash(szPath);
}

INT endex()
{
	char* str = "SceneManager.exit";
	getPath(szPath);
	PathAppend(szPath, _T("System\\RGSS301.dll"));
	hRGSS = LoadLibrary(szPath);
	if (hRGSS == NULL) {
		getPath(szPath);
		PathAppend(szPath, _T("RGSS103J.dll")); //为了兼容RGDXP
		hRGSS = LoadLibrary(szPath);
	}
	if (hRGSS != NULL) {
		hRGSSEval = (EVALFUNC)GetProcAddress(hRGSS, "RGSSEval");
		hRGSSEval((LPCSTR)str);
		return 1;
	}
	else
		return 0;
}

VOID lockMouse()
{
	RECT rect ;
	rgss_screen_rect(&rect) ;
	ClipCursor( &rect ) ;
}

//读取系统窗口信息
VOID load_rgssWND_SysMetr()
{
	sm_border_W = GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXEDGE) ;
	sm_title_H  = GetSystemMetrics(SM_CYCAPTION) ;
	sm_cxscreen = GetSystemMetrics(SM_CXSCREEN) ;
	sm_cyscreen = GetSystemMetrics(SM_CYSCREEN) ;
}

// 处理按键按下
inline VOID process_keydown(WPARAM wParam)
{
	key_press[wParam] = true;
}

// TypeIn 窗口过程
LRESULT CALLBACK TypeInWinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CHAR: // 屏蔽当当当
		if ( wParam == 0x0D || wParam == 0x1B || wParam == 0x09 ) // Enter Esc Tab
			return true ;
		else
			break ;
	case WM_KEYUP: case WM_SYSKEYUP:
		if ( wParam >= 0x08 && wParam <= 0x2E ) // 响应功能键
			key_press[wParam] = false ;
		break ;
	case WM_KEYDOWN: case WM_SYSKEYDOWN:
		if ( wParam >= 0x08 && wParam <= 0x2E || wParam >= 0x70 && wParam <= 0x7B)
			process_keydown( wParam ) ;
		break ;
	}
	return CallWindowProc( TypeIn_Proc, hWnd, uMsg, wParam, lParam ) ;
}

// RGSS 窗口过程
// 切换窗口后重置按键状态
inline VOID reset_key_states()
{
	for ( int i = 0; i != KEY_MAX; ++i )
	{
		key_states[i] = 0 ;
	}
	for ( int i = 0; i != DKEYs; ++i )
	{
		dkey_states[i] = 0 ;
	}
}
// 主过程
LRESULT CALLBACK HookWinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_ACTIVATE:
		if ( wParam && TypeIn_hWnd ) {
			SetFocus( TypeIn_hWnd ) ;
			return true ;
		} else
			break ;
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
		key_press[1] = true ;
		break ;
	case WM_LBUTTONUP:
		key_press[1] = false ;
		break ;
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
		key_press[2] = true ;
		break ;
	case WM_RBUTTONUP:
		key_press[2] = false ;
		break ;
	case WM_MBUTTONDOWN: // 鼠标中键
		key_press[4] = true ;
		return true ;
	case WM_MBUTTONUP:
		key_press[4] = false ;
		return true ;
	case WM_ACTIVATEAPP:
		reset_key_states() ;
		return true ;
	case WM_CLOSE:
		if (ProcessEnd == true) {
			if (endex() == 1)
				return 0;
		}
	case WM_KEYUP: case WM_SYSKEYUP://按键松开
		if (wParam >= KEY_MAX) return true ;
		key_press[wParam] = false ;
		break ;
	case WM_KEYDOWN: case WM_SYSKEYDOWN:
		if (wParam < KEY_MAX)
			process_keydown(wParam) ;
		break ;
	case WM_MOUSEWHEEL:
		if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
			++mouseZ;
		else
			--mouseZ;
		return true ;
	}
	return CallWindowProc(RGSS_Proc, hWnd, uMsg, wParam, lParam) ;
}
LONG set_hook()
{
	LONG hwnd = SetWindowLong( RGSS_hWnd, GWL_WNDPROC, (LONG)HookWinProc );
	if (hwnd == 0) return 0;
	RGSS_Proc = (WNDPROC)hwnd;
	return 1;
}



// 查找游戏窗口句柄
BOOL CALLBACK findRgssProc(HWND hwnd, LPARAM thread_id)
{
	WCHAR cn[12];
	GetClassName(hwnd, cn, 12);
	if (wcscmp(cn, L"RGSS Player") == 0 &&
		GetWindowThreadProcessId(hwnd, 0) == thread_id
		)
	{
		// 获取窗口标题，终止 Enmu
		GetWindowText( hwnd, WCHAR_temp, WCHAR_l ) ;
		RGSS_hWnd = hwnd ;
		return false ;
	} else {
		return true ;
	}
}
// 判断游戏是否已经运行
BOOL CALLBACK findDupProc(HWND hwnd, LPARAM thread_id)
{
	WCHAR cn[12], tn[WCHAR_l];
	GetClassName(hwnd, cn, 12);
	GetWindowText(hwnd, tn, WCHAR_l);
	if (wcscmp(cn, L"RGSS Player") == 0 &&
		wcscmp(tn, WCHAR_temp) == 0 &&
		GetWindowThreadProcessId(hwnd, 0) != thread_id
		)
	{
		// 发现同名窗口，终止 Enmu
		RGSS_hWnd = 0;
		return false;
	} else {
		return true;
	}
}
// 获取游戏窗口
VOID GetHWND()
{
	LONG thread_id = GetCurrentThreadId() ;
	EnumWindows(findRgssProc, thread_id) ;
	EnumWindows(findDupProc,  thread_id) ;
}



// 设置输入窗位置
inline VOID move_typeIn_window( int x, int y )
{
	MoveWindow( TypeIn_hWnd, x, y, TYPEIN_W, TYPEIN_W, false ) ;
}
// 创建输入窗
LONG create_typeIn_window()
{
	RECT rect ;
	rgss_screen_rect( &rect ) ;
	int x = 40,
		y = rect.bottom - rect.top - 10 ;
	TypeIn_hWnd = CreateWindow( L"edit", NULL,
		WS_CHILD |
		ES_AUTOHSCROLL |
		WS_HSCROLL, x, y, TYPEIN_W, TYPEIN_W,
		RGSS_hWnd, (HMENU)0, GetModuleHandle(NULL), NULL ) ;
	SetFocus( TypeIn_hWnd ) ;
	TypeIn_Proc = ( WNDPROC )SetWindowLong( TypeIn_hWnd, GWL_WNDPROC, (LONG)TypeInWinProc ) ;
	return ( LONG )TypeIn_hWnd ;
}





// 导出函数 ----------------------------------------------
LONG __stdcall NovaVersion()
{
	return VERSION ;
}
LONG __stdcall Hook() // 1 success, 0 hook fail, -1 already run
{	
	GetHWND() ;
	if (RGSS_hWnd == 0) return -1 ;
	load_rgssWND_SysMetr() ;
	return set_hook();
}

// 加载按键指针
VOID __stdcall LoadKeyBuf(unsigned int* ks, unsigned int* dks)
{
	key_states = ks ;
	dkey_states = dks ;
}
// 读取按键设置
VOID __stdcall LoadDKeySet(unsigned int* keysets)
{
	for(int i = 0; i != SetKeys; ++i)
		dkey_map[i] = keysets[i] ;
}

// 更新按键状态
inline VOID update_dkeys()
{
	// :A - L&R Shift
	dkey_press[0] = ( key_press[0x10] || key_press[dkey_map[0]] ) ;
	// :B - Esc
	dkey_press[1] = ( key_press[0x1B] || key_press[dkey_map[1]] ) ;
	// :C - Enter, Spacebar
	dkey_press[2] = ( key_press[0x0D] || key_press[0x20] || key_press[dkey_map[2]] ) ;
	// :X -
	dkey_press[3] = ( key_press[dkey_map[3]] ) ;
	// :Y
	dkey_press[4] = ( key_press[dkey_map[4]] ) ;
	// :Z
	dkey_press[5] = ( key_press[dkey_map[5]] ) ;
	// :L - PageUp
	dkey_press[6] = ( key_press[0x21] || key_press[dkey_map[6]] ) ;
	// :R - PageDown
	dkey_press[7] = ( key_press[0x22] || key_press[dkey_map[7]] ) ;
	// :L :U :R :D
	dkey_press[8] = ( key_press[0x25] || key_press[dkey_map[8]] ) ;
	dkey_press[9] = ( key_press[0x26] || key_press[dkey_map[9]] ) ;
	dkey_press[10] = ( key_press[0x27] || key_press[dkey_map[10]] ) ;
	dkey_press[11] = ( key_press[0x28] || key_press[dkey_map[11]] ) ;
	// :F5
	dkey_press[12] = ( key_press[0x74] ) ;
	// :F6
	dkey_press[13] = ( key_press[0x75] ) ;
	// :F7
	dkey_press[14] = ( key_press[0x76] ) ;
	// :F8
	dkey_press[15] = ( key_press[0x77] ) ;
	// :F9
	dkey_press[16] = ( key_press[0x78] ) ;
	// :CTRL - L&R Ctrl
	dkey_press[17] = ( key_press[0x11] ) ;
	// :ALT - 
	dkey_press[18] = ( key_press[0x12] ) ;
}

VOID __stdcall UpdateKeys()
{
	update_dkeys() ; 
	for(int i = 0; i != KEY_MAX; ++i)
	{
		if (key_press[i])
			++key_states[i] ;
		else
			key_states[i] = 0 ;
	}
	for(int i = 0; i != DKEYs; ++i)
	{
		if ( dkey_press[i] )
			++dkey_states[i] ;
		else
			dkey_states[i] = 0 ;
	}
}

// 创建输入窗 * 返回窗口句柄
LONG __stdcall TypeInActivate()
{
	return create_typeIn_window() ;
}
// 设置输入窗位置
VOID __stdcall SetTypeInPos(int x, int y)
{
	move_typeIn_window(x, y) ;
}
// 获取输入窗文本并清空该窗口
char* __stdcall GetTypeInText()
{
	static char cStr[128] ;
	wchar_t wBuf[64];
	LONG iLen = GetWindowTextLength(TypeIn_hWnd) + 1 ;
	if ( iLen > 64 )
		iLen = 64 ;
	GetWindowText( TypeIn_hWnd, wBuf, iLen) ;
	LONG nLen = WideCharToMultiByte( CP_ACP, 0, wBuf, -1, NULL, 0, NULL, NULL) ;
	WideCharToMultiByte( CP_ACP, 0, wBuf, -1, cStr, nLen, NULL, NULL) ;
	SetWindowText( TypeIn_hWnd, L"") ;
	return cStr ;
}
// 销毁输入窗口
VOID __stdcall TypeInDeactivate()
{
	DestroyWindow( TypeIn_hWnd ) ;
	TypeIn_hWnd = 0 ;
	SetFocus ( RGSS_hWnd ) ;
}

VOID __stdcall MouseLock()
{
	lockMouse() ;
}
VOID __stdcall MouseUnLock()
{
	ClipCursor( NULL ) ;
}
VOID __stdcall GetMousePos(int* p)
{
	POINT pos ;
	GetCursorPos( &pos ) ;
	ScreenToClient(RGSS_hWnd, &pos) ;
	p[0] = pos.x ;
	p[1] = pos.y ;
}
INT  __stdcall MouseZ()
{
	return mouseZ ;
}
VOID __stdcall MouseZReset()
{
	mouseZ = 0 ;
}
VOID __stdcall SetMousePos(int x, int y)
{
	POINT pos ;
	pos.x = 0 ;
	pos.y = 0 ;
	ScreenToClient(RGSS_hWnd, &pos) ;
	SetCursorPos( (x - pos.x), (y - pos.y) ) ;
}
VOID __stdcall ProcessEndEx(int pe)
{
	if (pe)
		ProcessEnd = true;
	else
		ProcessEnd = false;
}