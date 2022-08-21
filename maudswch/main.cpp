#include <windows.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <setupapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include "PolicyConfig.h"
#include "gui.h"
#include "data.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifdef UNICODE
#define TOWSTR(x) L ## x
#else
#define TOWSTR(x) x
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK_CALL(x) if((x) != S_OK) \
	{ MessageBox(NULL, TOWSTR(#x) ## " line: " TOSTRING(__LINE__), \
	TOWSTR("maudswch error"), MB_OK); return -1; }

#define APP_GUID "-{428B9585-6BF5-48C2-AABE-B2CA47CC3988}"
#define REGISTER_MESSAGE(n) \
	static const UINT n = RegisterWindowMessage(TOWSTR(#n APP_GUID))

// Supported hook types - -
#define HOOK_TASKBAR_MB 0x1
#define HOOK_GLOBAL_KB  0x2
// - - - - - - - - - - - -


typedef DWORD(__stdcall* MASTERPROC)(DWORD master_thread_id, UINT mb_msg, UINT mw_msg, UINT k_msg);
typedef void(__stdcall* FILTERPROC)(UINT virt, UINT scan);
typedef void(__stdcall* DISCARDPROC)(UINT discard);

REGISTER_MESSAGE(UWM_MBUTTON);     // WP: T:MD F:MU   | LP: LW: Mouse X HW: Mouse Y
REGISTER_MESSAGE(UWM_MWHEEL);	   // WP: Wheel Delta | LP: LW: Mouse X HW: Mouse Y
REGISTER_MESSAGE(UWM_KEYPRESS);    // WP: Key State   | LP: LW: Virt.C. HW: Scan C.
REGISTER_MESSAGE(UWM_NEWINSTANCE); // WP: Not Used    | LP: Not Used
REGISTER_MESSAGE(UWM_GUI_ACTION);  // WP: GUI_ACTION  | LP: New Value

// Config variables - -
bool autostart = false;
bool keyb_hook = false;
bool block_inp = true;
DWORD key_virtc = 0xFFFFFFFF;
DWORD key_scanc = 0xFFFFFFFF;
// - - - - - - - - - -

// Config variable names - - - - - -
LPCWSTR keyb_hook_n = L"keyb_hook";
LPCWSTR block_inp_n = L"block_inp";
LPCWSTR key_virtc_n = L"key_virtc";
LPCWSTR key_scanc_n = L"key_scanc";
// - - - - - - - - - - - - - - - - -

bool window_is_alive;
bool filter_is_setting_up; // User is choosing global key

HINSTANCE dll_start_addr; // A.K.A. "The Instance"
HHOOK mouse_hook;
HHOOK ll_kb_hook;

FILTERPROC setKbFilter;
DISCARDPROC setDiscardInp;




/*___________________________________________________________________
|  toggleDefaultDevice:
|    Switches default audio output device in a closed loop
|
|  Return value:
|            Audio device switched -> 0
|    Failed to switch audio device -> -1
|____________________________________________________________________*/
int toggleDefaultDevice();


/*___________________________________________________________________
|  adjustMasterVolume:
|    Brings current def. snd. device volume up or down
|
|  vol_up: Volume is brought up if true, down if false
|
|  Return value:
|         Volume adjusted -> 0
|    Failed adjust volume -> -1
|____________________________________________________________________*/
int adjustMasterVolume(bool vol_up);


/*___________________________________________________________________
|  toggleMasterMute:
|    Toggles default audio device from muted to unmuted state
|
|  Return value:
|        Device muted -> 0
|    Operation failed -> -1
|____________________________________________________________________*/
int toggleMasterMute();


/*___________________________________________________________________
|  cursorInSndIcon:
|    Checks if mouse is hovering over taskbar notification
|    area sound icon
|
|  x: Cursor x position
|  y: Cursor y position
|
|  Return value:
|    Cursor is inside sound icon -> true
|       Cursor is somewhere else -> false
|____________________________________________________________________*/
bool cursorInSndIcon(ULONGLONG x, ULONGLONG y);


/*___________________________________________________________________
|  respondToGUIaction:
|    Handles any possible GUI action taken
|
|   a_type: One of GUI_ACTION codes
|  new_val: New value of the global config passed by GUI
|
|  Return value:
|    Action handled successfully -> 0
|           Something went worng -> -1
|____________________________________________________________________*/
int respondToGUIaction(int a_type, int new_val);


/*___________________________________________________________________
|  startHooking:
|    Sets up Windows Hooks specified by input parameter
|
|  whats_hooking: One or multiple s. HOOK_ types (use | to combine)
|
|  Return value:
|    Hook(s) hooked -> 0
|    Hook(s) missed -> -1
|____________________________________________________________________*/
int startHooking(int whats_hooking);


/*___________________________________________________________________
|  stopHooking:
|    Unhooks one or more of the already running Windows Hooks
|
|  whats_unhooking: One or multiple s. HOOK_ types (use | to combine)
|
|  Return value:
|    Hook(s) unhooked -> 0
|       Hook(s) stuck -> -1
|____________________________________________________________________*/
int stopHooking(int whats_unhooking);




// HINSTANCE -> "handle" to "instance" aka "module".
// It's NOT a handle. And not to "instance" or "module".
// I's all 16 bit Windows legacy backwards compatability nonsense.
// Since 16-bit Windows had a common address space, the GetInstanceData function was really
// nothing more than a hmemcpy, and many programs relied on this and just used raw hmemcpy
// instead of using the documented API.
// In Win32/Win64 it's actually executable (DLL or EXE) image.
// HINSTANCE points to actual virtual adress where first byte of
// executable's code is located: cout << (const char*)hinst ---> MZ? (? = 0x90/0x00)
int WINAPI wWinMain(
	_In_		HINSTANCE hinst,	// "Handle" to "instance"
	_In_opt_	HINSTANCE phinst,	// "Handle" to "previous instance", always NULL
	_In_		PWSTR cmd,			// Command line arguments
	_In_		int show)			// Default user preference for ShowWindow()
{
	// Seems that only local thread spesific hooks require this
	ChangeWindowMessageFilter(UWM_MBUTTON, MSGFLT_ADD);
	// I will still add other custom messages, just in case!
	ChangeWindowMessageFilter(UWM_KEYPRESS, MSGFLT_ADD);
	ChangeWindowMessageFilter(UWM_NEWINSTANCE, MSGFLT_ADD);
	ChangeWindowMessageFilter(UWM_MWHEEL, MSGFLT_ADD);
	ChangeWindowMessageFilter(UWM_GUI_ACTION, MSGFLT_ADD);

	// Check user settings and init GUI
	autostart = regAutoStChk();
	keyb_hook = regChk(keyb_hook_n);
	block_inp = !regChk(block_inp_n);

	setControls(autostart, keyb_hook, block_inp);

	key_virtc = regChk(key_virtc_n) ? regGet(key_virtc_n) : key_virtc;
	key_scanc = regChk(key_scanc_n) ? regGet(key_scanc_n) : key_scanc;

	setButtText(key_virtc, key_scanc);

	if(startHooking(HOOK_TASKBAR_MB | (keyb_hook ? HOOK_GLOBAL_KB : 0x0)))
	{
		return -1; // Silent Error
	}

	setKbFilter(key_virtc, key_scanc);
	setDiscardInp(block_inp);

	if(wcscmp(cmd, L"-silent"))
	{
		initGUI(hinst, UWM_GUI_ACTION);
		CHECK_CALL(!spawnMainWnd());
		window_is_alive = true;
	}

	ULONGLONG stime = 0, etime = 0;
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if(msg.message == UWM_MBUTTON)
		{
			
			if(msg.wParam) // Button is pressed
			{
				stime = GetTickCount64();
				continue;
			}
			else // Button is released
			{
				etime = GetTickCount64();
				if(cursorInSndIcon(LW(msg.lParam), HW(msg.lParam)))
				{
					if(etime - stime > 300)
					{
						toggleMasterMute();
					}
					else
					{
						toggleDefaultDevice();
					}
				}
			}
		}
		else if(msg.message == UWM_MWHEEL)
		{
			// Only High Word is used in mouseData, LW is reserved and never used
			if(cursorInSndIcon(LW(msg.lParam), HW(msg.lParam)))
			{
				adjustMasterVolume((short)HW(msg.wParam) > 0);
			}
		}
		else if(msg.message == UWM_NEWINSTANCE)
		{
			if(!window_is_alive)
			{
				initGUI(hinst, UWM_GUI_ACTION);
				CHECK_CALL(!spawnMainWnd());
				window_is_alive = true;
			}
		}
		else if(msg.message == UWM_KEYPRESS && (msg.wParam == WM_KEYUP || msg.wParam == WM_SYSKEYUP))
		{
			if(filter_is_setting_up)
			{
				key_virtc = LW(msg.lParam);
				key_scanc = HW(msg.lParam);

				setButtText(key_virtc, key_scanc);
			}
			else
			{
				toggleDefaultDevice();
			}

		}
		else if(msg.message == UWM_GUI_ACTION)
		{
			respondToGUIaction((int)msg.wParam, (int)msg.lParam);
		}

		// Only needed when Edit input boxes are used
		//TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	stopHooking(HOOK_TASKBAR_MB | HOOK_GLOBAL_KB);

	return (int)msg.wParam;
}

int toggleDefaultDevice()
{
	// Initialise COM concurrency...
	CHECK_CALL(CoInitialize(NULL));

	IMMDeviceEnumerator* enumer = NULL;
	CHECK_CALL(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumer));

	// Enumerate devices...
	IMMDeviceCollection* devs = NULL;
	CHECK_CALL(enumer->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devs));

	// Get current default audio output device...
	IMMDevice* def_dev = NULL;
	CHECK_CALL(enumer->GetDefaultAudioEndpoint(eRender, eMultimedia, &def_dev));
	LPWSTR def_dev_id = NULL;
	def_dev->GetId(&def_dev_id);
	def_dev->Release();
	
	// Get device count...
	unsigned int dev_num = 9001; // ITS OVER 9000!!!!
	CHECK_CALL(devs->GetCount(&dev_num));

	IMMDevice* dev = NULL;
	LPWSTR new_dd_id = NULL; // New Default Device ID

	for(unsigned int i = 0; i < dev_num; ++i)
	{
		devs->Item(i, &dev);

		LPWSTR dev_id = NULL;
		dev->GetId(&dev_id);

		// Check if current device in a list is default
		if(!lstrcmp(def_dev_id, dev_id))
		{
			unsigned int indx = i < dev_num-1 ? i + 1 : 0;
			dev->Release();
			devs->Item(indx, &dev);
			dev->GetId(&new_dd_id);
			CoTaskMemFree(dev_id);
			dev->Release();
			break;
		}

		CoTaskMemFree(dev_id);
		dev->Release();
	}

	// Use esoteric undocumented PolicyConfig.h and Vista configs
	IPolicyConfigVista* vista_config = NULL;

	CHECK_CALL(CoCreateInstance(__uuidof(CPolicyConfigVistaClient),
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (void**)&vista_config));

	// Finally, set default audio device for all 3 roles
	CHECK_CALL(vista_config->SetDefaultEndpoint(new_dd_id, eConsole));
	CHECK_CALL(vista_config->SetDefaultEndpoint(new_dd_id, eMultimedia));
	CHECK_CALL(vista_config->SetDefaultEndpoint(new_dd_id, eCommunications));

	vista_config->Release();

	enumer->Release();
	devs->Release();
	CoUninitialize();

	return 0;
}

int adjustMasterVolume(bool vol_up)
{
	// Sample output from API
	// ID: 00000250E49C90C0 vol: -9.71875DB 0.521074 min: -65.25 max: 0 inc: 0.03125
	// ID: 00000250E49C8B40 vol: -19.8958DB 0.26     min: -65.25 max: 0 inc: 0.03125

	// Initialise COM concurrency...
	CHECK_CALL(CoInitialize(NULL));

	IMMDeviceEnumerator* enumer = NULL;
	CHECK_CALL(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumer));

	// Enumerate devices...
	IMMDeviceCollection* devs = NULL;
	CHECK_CALL(enumer->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devs));

	// Get current default audio output device...
	IMMDevice* def_dev = NULL;
	CHECK_CALL(enumer->GetDefaultAudioEndpoint(eRender, eMultimedia, &def_dev));

	void* vol_void = NULL;
	CHECK_CALL(def_dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, &vol_void));
	IAudioEndpointVolume* vol = (IAudioEndpointVolume*)vol_void;

	float vscalar = 0.0f;
	vol->GetMasterVolumeLevelScalar(&vscalar);

	float delta = vol_up ? 0.02f : -0.02f; // Standart Windows 2% sound volume change step

	// Use this for super fine tuning the sound level
	//vol->GetVolumeRange(&min, &max, &inc);
	//vol->GetMasterVolumeLevel(&volume);
	//vol->SetMasterVolumeLevel(volume + delta, NULL);
	vol->SetMasterVolumeLevelScalar(vscalar + delta, NULL);

	vol->Release();
	def_dev->Release();
	enumer->Release();
	devs->Release();
	CoUninitialize();

	return 0;
}

int toggleMasterMute()
{
	// Initialise COM concurrency...
	CHECK_CALL(CoInitialize(NULL));

	IMMDeviceEnumerator* enumer = NULL;
	CHECK_CALL(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumer));

	// Enumerate devices...
	IMMDeviceCollection* devs = NULL;
	CHECK_CALL(enumer->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devs));

	// Get current default audio output device...
	IMMDevice* def_dev = NULL;
	CHECK_CALL(enumer->GetDefaultAudioEndpoint(eRender, eMultimedia, &def_dev));

	void* vol_void = NULL;
	CHECK_CALL(def_dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, &vol_void));
	IAudioEndpointVolume* vol = (IAudioEndpointVolume*)vol_void;

	BOOL mute;
	vol->GetMute(&mute);

	HRESULT res = vol->SetMute(mute == 0, NULL);

	vol->Release();
	def_dev->Release();
	enumer->Release();
	devs->Release();
	CoUninitialize();

	return 0;
}

bool cursorInSndIcon(ULONGLONG x, ULONGLONG y)
{
	NOTIFYICONIDENTIFIER snd_icon;
	ZeroMemory(&snd_icon, sizeof(NOTIFYICONIDENTIFIER));
	snd_icon.cbSize = sizeof(NOTIFYICONIDENTIFIER);
	memcpy(&snd_icon.guidItem,
		"\x73\xAE\x20\x78\xE3\x23\x29\x42\x82\xC1\xE4\x1C\xB6\x7D\x5B\x9C", sizeof(GUID));

	RECT sr;
	if(Shell_NotifyIconGetRect(&snd_icon, &sr) != S_OK)
	{
		return false;
	}

	if(x > sr.left && x < sr.right && y > sr.top && y < sr.bottom)
	{
		return true;
	}

	return false;
}

int respondToGUIaction(int a_type, int new_val)
{
	switch(a_type)
	{
	case GUI_AUTOSTART_CHBX:
		if(new_val)
		{
			autostart = true;
			regAutoStSet(true);
		}
		else
		{
			autostart = false;
			regAutoStSet(false);
		}
		break;
	case GUI_GLOB_KEYB_CHBX:
		if(new_val)
		{
			keyb_hook = true;
			filter_is_setting_up = true;
			regSet(keyb_hook_n);
			startHooking(HOOK_GLOBAL_KB);
			setButtAndDiscardStatus(true, false);
		}
		else
		{
			keyb_hook = false;
			regDel(keyb_hook_n);
			regDel(key_virtc_n);
			regDel(key_scanc_n);
			key_virtc = key_scanc = 0xFFFFFFFF;
			setButtText(key_virtc, key_scanc);
			stopHooking(HOOK_GLOBAL_KB);
			setKbFilter(0xFFFFFFFF, 0xFFFFFFFF); // Reset filter
			setButtAndDiscardStatus(false, false);
		}
		break;
	case GUI_BLOCK_INP_CHBX:
		if(new_val)
		{
			block_inp = true;
			regDel(block_inp_n);
			setDiscardInp(TRUE);
		}
		else
		{
			block_inp = true;
			regSet(block_inp_n);
			setDiscardInp(FALSE);
		}
		break;
	case GUI_SAVE_KEYC_BUTT:
		setKbFilter(key_virtc, key_scanc);
		regSet(key_virtc_n, key_virtc);
		regSet(key_scanc_n, key_scanc);
		setButtAndDiscardStatus(false, true);
		filter_is_setting_up = false;
		break;
	case GUI_TERMINATE_BUTT:
		PostQuitMessage(0);
		break;
	case GUI_WINDOWWAS_DEST:
		window_is_alive = false;
		break;
	default:
		return -1;
	}

	return 0;
}

int startHooking(int whats_hooking)
{
	bool first = true; // First hook setup after launch of the instance
	if(dll_start_addr != NULL)
	{
		first = false;
	}
	
	// HACKERMAN MODE: ON
	// Obtain taskbar thread_id by class name
	HWND taskb_hwnd = FindWindow(L"Shell_TrayWnd", NULL);
	DWORD proc_id = NULL;
	DWORD thread_id = GetWindowThreadProcessId(taskb_hwnd, &proc_id);

	// Load hooking DLL, get function pointers
	MASTERPROC setMasterThreadId = NULL;
	HOOKPROC mouseHookCback = NULL;
	HOOKPROC llKbHookCback = NULL;

	if(first)
	{
		dll_start_addr = LoadLibrary(L"hook");
	}

	if(dll_start_addr != NULL)
	{
		setMasterThreadId = (MASTERPROC)GetProcAddress(dll_start_addr, "setMasterThreadId");
		mouseHookCback = (HOOKPROC)GetProcAddress(dll_start_addr, "MouseHookProc");
		llKbHookCback = (HOOKPROC)GetProcAddress(dll_start_addr, "llKbHookProc");
		setKbFilter = (FILTERPROC)GetProcAddress(dll_start_addr, "setKbFilter");
		setDiscardInp = (DISCARDPROC)GetProcAddress(dll_start_addr, "setDiscardInp");
	}
	else
	{
		MessageBox(NULL, L"GetProcAddress(dll_inst, \"mouseHookProc\") line: 65", L"maudswch error", MB_OK);
		return -1;
	}

	if(mouseHookCback != NULL)
	{
		// Will this multi-instance check work with shared memory? No idea (._. )
		// YES, IT ACTUALLY DOES! ( o_0 )
		DWORD res = setMasterThreadId(GetCurrentThreadId(), UWM_MBUTTON, UWM_MWHEEL, UWM_KEYPRESS);
		if(first && res) // Only check for multi-instance at first hooking
		{
			PostThreadMessage(res, UWM_NEWINSTANCE, 0, 0);
			return -1;
		}

		if(whats_hooking & HOOK_TASKBAR_MB)
		{
			mouse_hook = SetWindowsHookEx(WH_MOUSE, mouseHookCback, dll_start_addr, thread_id);
		}
		// Oh boi, here it goes... The real hackin begins! Hook em' up, ladies'!
		if(whats_hooking & HOOK_GLOBAL_KB)
		{
			ll_kb_hook = SetWindowsHookEx(WH_KEYBOARD_LL, llKbHookCback, dll_start_addr, 0);
		}
		// But will the global hook work?..
		// Yes it does!
	}
	else
	{
		MessageBox(NULL, L"SetWindowsHookEx", L"maudswch error", MB_OK);
		return -1;
	}

	return 0;
}

int stopHooking(int whats_unhooking)
{
	bool res = true;
	if(whats_unhooking & HOOK_TASKBAR_MB)
	{
		res = (bool)UnhookWindowsHookEx(mouse_hook);
	}
	if(whats_unhooking & HOOK_GLOBAL_KB)
	{
		res = (bool)UnhookWindowsHookEx(ll_kb_hook);
	}

	return !res;
}