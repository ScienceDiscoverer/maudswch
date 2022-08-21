#include <sstream>
#include <windows.h>
#include "gui.h"

// Controls IDs - - - - - - - - -
#define CHBX_REG_AUTOSTART	0x45
#define CHBX_GLOB_KEY		0x2A
#define CHBX_BLOCK_INPUT	0x22

#define BUTT_SET_KEY		0x42
#define BUTT_TERMINATE		0x34
// - - - - - - - - - - - - - - - -

struct WinPosSize
{
	int x;
	int y;
	int w;
	int h;
};

HINSTANCE exec_adress;
ATOM main_class;
float dpi_scale;
WinPosSize wmain;
HFONT default_font;
UINT UWM_GUI_ACTION;
HWND main_wnd;
HWND setkey_butt;
HWND oinput_chbx;

const int child_h = 22;

// Control statuses
bool autost_checked;
bool keybh_checked;
bool blockinp_checked;
// - - - - - - - -

DWORD kvirt_code;
DWORD kscan_code;




/*___________________________________________________________________
|  spawnCheckbox:
|    Creates checkbox centered and reletive to client area top
|
|  top_pos: Checkbox index from top to bottom
|     text: Checkbox label
|       id: Checkbox ID
|      par: Parent HWND
|
|  Return value:
|          Checkbox created -> HWND of new Checkbox
|    Checkbox creation fail -> NULL
|____________________________________________________________________*/
HWND spawnCheckbox(int top_pos, LPCWSTR text, WORD id, HWND par);


/*___________________________________________________________________
|  spawnButton:
|    Creates push button
|
|  top_pos: Button index from top to bottom
|     text: Button initial text
|       id: Button ID
|      par: Parent HWND
|
|  Return value:
|          Edit created -> HWND of new Button
|    Edit creation fail -> NULL
|____________________________________________________________________*/
HWND spawnButton(int top_pos, LPCWSTR text, WORD id, HWND par);


/*___________________________________________________________________
|  buttClickHandler:
|    Responds to every possible button interaction
|
|    b_id: Button ID
|  b_hwnd: Handle to Button window
|____________________________________________________________________*/
void buttClickHandler(int b_id, HWND b_hwnd);


/*___________________________________________________________________
|  flipBool:
|    Flips boolean variable
|
|  (IN/OUT) b_id: Button ID
|         b_hwnd: Handle to Button window
|
|  Return value:
|    Always -> Previous bool state
|____________________________________________________________________*/
bool flipBool(bool* b, HWND b_hwnd);




LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		DeleteObject(default_font);
		PostMessage(NULL, UWM_GUI_ACTION, GUI_WINDOWWAS_DEST, 0);
		break;
	case WM_COMMAND:
		if(HW(wp) == BN_CLICKED)
		{
			buttClickHandler(LW(wp), (HWND)lp);
			break;
		}
		else
		{
			return DefWindowProc(hwnd, msg, wp, lp);
		}
	default:
		return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}

void initGUI(HINSTANCE hinst, UINT gui_msg)
{
	exec_adress = hinst;
	UWM_GUI_ACTION = gui_msg;

	// Create Fake Window to get active monitor's DPI
	HWND hwnd = CreateWindowEx(0, L"Button", 0, 0,
		0, 0, 0, 0, 0, 0, hinst, 0);
	dpi_scale = (float)GetDpiForWindow(hwnd) / 96.0f;
	DestroyWindow(hwnd);

	// Calculate main window position/size
	RECT work_a;
	SystemParametersInfo(SPI_GETWORKAREA, NULL, &work_a, NULL);

	wmain.w = wmain.h = S(300);
	wmain.x = (work_a.right - work_a.left)/2 - wmain.w/2 + work_a.left;
	wmain.y = (work_a.bottom - work_a.top)/2 - (wmain.h + S(30))/2 + work_a.top + S(30)/2;

	// Create main font
	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, NULL);

	metrics.lfMessageFont.lfHeight = S(12) * -1; // 12 seems to be default on 96 DPI

	default_font = CreateFontIndirect(&metrics.lfMessageFont);
}

HWND spawnMainWnd()
{
	HICON ico = LoadIcon(exec_adress, L"MAINICON");
	
	// Register Window Class Atom ==============================================================
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);					// Structure size in bytes
	wc.lpszClassName = L"superSimpleWindow";		// Name of the Class (Atom)
	wc.lpfnWndProc = wndProc;						// Window callback function
	wc.hInstance = exec_adress;						// Handle to module's instance
	// Optional, 0 if unused
	wc.style = 0;									// Class style, any combination
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);		// Def. cursor for all windows
	wc.hIcon = ico;									// Def. icon for all windows
	wc.hIconSm = ico;								// Def. small icon for all w.
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);	// Def. brush for WM_ERASEBKGND
	wc.lpszMenuName = NULL;							// Def. menu name for all w.
	wc.cbClsExtra = 0;								// Extra Class Memory (max 40B)
	wc.cbWndExtra = 0;								// Extra Window Memory (max 40B)

	main_class = main_class ? main_class : RegisterClassEx(&wc);
	// =========================================================================================

	// Create Window ===========================================================================
	main_wnd = CreateWindowEx(
		WS_EX_COMPOSITED,					//   [I]  Extended window style
		(LPCWSTR)main_class,				// [I|O]  Class Atom / Class Atom String
		L"Micro Audio Switcher",			// [I|O]  Window name String (Title)
		WS_SYSMENU,							//   [I]  Window style (WS_OVERLAPPED = 0x0)
		wmain.x, wmain.y, wmain.w, wmain.h,	//   [I]  PosX, PoxY, Width, Height
		NULL,								// [I|O]  Handle to this window's parent
		NULL,								// [I|O]  Handle to menu / Child-window ID
		exec_adress,						// [I|O]  Handle to instance of the module
		NULL);								// [I|O]  CREATESTRUCT ptr. for lParam of WM_CREATE
	// =========================================================================================

	HWND autost = spawnCheckbox(0, L"Add autostart registry entry", CHBX_REG_AUTOSTART, main_wnd);
	HWND kbhook = spawnCheckbox(1, L"Enable global keyboard key", CHBX_GLOB_KEY, main_wnd);
	oinput_chbx = spawnCheckbox(4, L"Block original input", CHBX_BLOCK_INPUT, main_wnd);

	setkey_butt = spawnButton(3, L"Prsess any key...", BUTT_SET_KEY, main_wnd);
	spawnButton(8, L"EXIT", BUTT_TERMINATE, main_wnd);

	if(!autost_checked)
	{
		SendMessage(autost, BM_SETCHECK, BST_UNCHECKED, NULL);
	}
	else
	{
		SendMessage(autost, BM_SETCHECK, BST_CHECKED, NULL);
	}

	if(!keybh_checked)
	{
		SendMessage(kbhook, BM_SETCHECK, BST_UNCHECKED, NULL);
		EnableWindow(setkey_butt, FALSE);
		EnableWindow(oinput_chbx, FALSE);
	}
	else
	{
		SendMessage(kbhook, BM_SETCHECK, BST_CHECKED, NULL);
		if(kvirt_code != 0xFFFFFFFF && kscan_code != 0xFFFFFFFF)
		{
			EnableWindow(setkey_butt, FALSE);
		}
	}

	if(!blockinp_checked)
	{
		SendMessage(oinput_chbx, BM_SETCHECK, BST_UNCHECKED, NULL);
	}
	else
	{
		SendMessage(oinput_chbx, BM_SETCHECK, BST_CHECKED, NULL);
	}

	setButtText(kvirt_code, kscan_code);

	UpdateWindow(main_wnd);
	ShowWindow(main_wnd, SW_SHOWNORMAL);

	return main_wnd;
}

void setControls(bool astrt, bool kh, bool binp)
{
	autost_checked = astrt;
	keybh_checked = kh;
	blockinp_checked = binp;
}

void setButtText(DWORD kvirt, DWORD kscan)
{
	kvirt_code = kvirt;
	kscan_code = kscan;
	
	std::wstringstream ss;
	if(kvirt == 0xFFFFFFFF || kscan == 0xFFFFFFFF)
	{
		ss << L"Prsess any key...";
	}
	else
	{
		ss << std::hex << std::uppercase << L"VIRT: " << kvirt << L" | SCAN: " << kscan;
	}

	if(oinput_chbx != NULL)
	{
		SendMessage(setkey_butt, WM_SETTEXT, NULL, (LPARAM)ss.str().c_str());
	}
}

void setButtAndDiscardStatus(bool butt, bool discard)
{
	EnableWindow(setkey_butt, butt);
	EnableWindow(oinput_chbx, discard);
}

HWND spawnCheckbox(int top_pos, LPCWSTR text, WORD id, HWND par)
{
	RECT cli_r;

	//GetWindowRect(hwnd, &wnd_r); // Use to calc exact client area, if needed
	GetClientRect(par, &cli_r);

	int w = cli_r.right - cli_r.left;
	int h = cli_r.bottom - cli_r.top;

	int cx = w/2 - S(165)/2 + cli_r.left;
	int cy = S(child_h) * (top_pos+1) + cli_r.top;

	// Create Window =======================================================================
	HWND child = CreateWindowEx(
		0,										// Extended window style
		L"Button",								// Class Atom / Class Atom String
		text,									// Window name String (Title)
		WS_VISIBLE | WS_CHILD | BS_CHECKBOX,	// Window style (WS_OVERLAPPED = 0x0)
		cx, cy, S(165), S(child_h),				// PosX, PoxY, Width, Height
		par,									// Handle to this window's parent
		(HMENU)id,								// Handle to menu / Child-window ID
		exec_adress,							// Handle to instance of the module
		NULL									// CREATESTRUCT ptr. for lParam of WM_CREATE
	);
	// =====================================================================================

	SendMessage(child, WM_SETFONT, (WPARAM)default_font, TRUE);

	return child;
}

HWND spawnButton(int top_pos, LPCWSTR text, WORD id, HWND par)
{
	RECT cli_r;

	//GetWindowRect(hwnd, &wnd_r); // Use to calc exact client area, if needed
	GetClientRect(par, &cli_r);

	int w = cli_r.right - cli_r.left;
	int h = cli_r.bottom - cli_r.top;

	int cx = w/2 - S(165)/2 + cli_r.left;
	int cy = S(child_h) * (top_pos+1) + cli_r.top;

	int hc = top_pos == 8 ? child_h * 2 : child_h;

	// Create Window =======================================================================
	HWND child = CreateWindowEx(
		0,										// Extended window style
		L"Button",								// Class Atom / Class Atom String
		text,									// Window name String (Title)
		WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,	// Window style (WS_OVERLAPPED = 0x0)
		cx, cy, S(165), S(hc),					// PosX, PoxY, Width, Height
		par,									// Handle to this window's parent
		(HMENU)id,								// Handle to menu / Child-window ID
		exec_adress,							// Handle to instance of the module
		NULL									// CREATESTRUCT ptr. for lParam of WM_CREATE
	);
	// =====================================================================================

	SendMessage(child, WM_SETFONT, (WPARAM)default_font, TRUE);

	return child;
}

void buttClickHandler(int b_id, HWND b_hwnd)
{
	switch(b_id)
	{
	case CHBX_REG_AUTOSTART:
		flipBool(&autost_checked, b_hwnd);
		PostMessage(NULL, UWM_GUI_ACTION, GUI_AUTOSTART_CHBX, (LPARAM)autost_checked);
		break;
	case CHBX_GLOB_KEY:
		flipBool(&keybh_checked, b_hwnd);
		PostMessage(NULL, UWM_GUI_ACTION, GUI_GLOB_KEYB_CHBX, (LPARAM)keybh_checked);
		break;
	case CHBX_BLOCK_INPUT:
		flipBool(&blockinp_checked, b_hwnd); 
		PostMessage(NULL, UWM_GUI_ACTION, GUI_BLOCK_INP_CHBX, (LPARAM)blockinp_checked);
		break;
	case BUTT_SET_KEY:
		if(kvirt_code != 0xFFFFFFFF || kscan_code != 0xFFFFFFFF)
		{
			PostMessage(NULL, UWM_GUI_ACTION, GUI_SAVE_KEYC_BUTT, 0);
		}
		break;
	case BUTT_TERMINATE:
		if(MessageBox(main_wnd, L"This will terminate MAS process, instead of just closing this window.\n"
			"Are you sure about that?", L"Termination Confirmation",
			MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2) == IDYES)
		{
			PostMessage(NULL, UWM_GUI_ACTION, GUI_TERMINATE_BUTT, 0);
			DestroyWindow(main_wnd);
		}
		break;
	default:
		break;
	}
}

bool flipBool(bool* b, HWND b_hwnd)
{
	bool tmp = *b;
	*b = !*b;
	if(tmp)
	{
		SendMessage(b_hwnd, BM_SETCHECK, BST_UNCHECKED, NULL);
	}
	else
	{
		SendMessage(b_hwnd, BM_SETCHECK, BST_CHECKED, NULL);
	}

	return tmp;
}