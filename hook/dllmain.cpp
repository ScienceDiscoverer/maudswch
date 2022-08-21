#include "exports.h"

#define IGNORE_FILTER 0xFFFFFFFF

#pragma data_seg(".shared")
DWORD master_thread_id = NULL;
UINT UWM_MBUTTON = NULL;
UINT UWM_MWHEEL = NULL;
UINT UWM_KEYPRESS = NULL;
UINT kb_filter_virt = 0xFFFFFFFF;
UINT kb_filter_scan = 0xFFFFFFFF;
UINT kb_discard_orig = TRUE;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,RWS")

using namespace std;

BOOL APIENTRY DllMain(
	_In_	HMODULE hm,			// "Handle" to "Module" in fact its base adress of DLL
	_In_	DWORD creason,		// Reason for calling this function by the OS
	_In_	LPVOID reserved)	// Dynamic/Statc link flag or FreeLibrary/Process term.
{
	switch(creason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hm);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

LRESULT MouseHookProc(int code, WPARAM wp, LPARAM lp)
{
	switch (code)
	{
	case HC_NOREMOVE:
	case HC_ACTION:
	{
		switch(wp)
		{
		case WM_MOUSEWHEEL:
		{
			MOUSEHOOKSTRUCTEX* mh = (MOUSEHOOKSTRUCTEX*)lp;
			PostThreadMessage(master_thread_id, UWM_MWHEEL, mh->mouseData,
				((LPARAM)mh->pt.y << 16) | mh->pt.x); 
			goto exit;
		}
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		{
			MOUSEHOOKSTRUCT* mh = (MOUSEHOOKSTRUCT*)lp;
			PostThreadMessage(master_thread_id, UWM_MBUTTON, wp == WM_MBUTTONDOWN,
				((LPARAM)mh->pt.y << 16) | mh->pt.x);
			goto exit;
		}
		default:
			break;
		}
		break;
	}
	default:
		break;
	}

	exit:
	// `code` -> send < 0 to stop other hookz (if they don't ignore `code`)
	// Nonzero return value to prevent the system from passing
	// the message to the hooks AND target window procedure
	return CallNextHookEx(NULL, code, wp, lp);
}

LRESULT CALLBACK llKbHookProc(int code, WPARAM wp, LPARAM lp)
{
	switch(code)
	{
	case HC_ACTION:
	{
		KBDLLHOOKSTRUCT* khs = (KBDLLHOOKSTRUCT*)lp;

		if((kb_filter_virt == khs->vkCode && kb_filter_scan == khs->scanCode) ||
			kb_filter_virt == IGNORE_FILTER /* Assume that other filter is IGNORE too */)
		{
			LPARAM lpar = 0;
			lpar = khs->vkCode;
			lpar = ((LPARAM)khs->scanCode << 16) | lpar;
			
			PostThreadMessage(master_thread_id, UWM_KEYPRESS, wp, lpar);
			if(kb_discard_orig && kb_filter_virt != IGNORE_FILTER) // Prevent in. block if filter not set
			{
				return -1; // Prevent the system from passing the message to the rest of the hook chain
			}              // or (and?) the target window procedure
		}
		break;
	}
	default:
		break;
	}

	return CallNextHookEx(NULL, code, wp, lp);
}

DWORD setMasterThreadId(DWORD m_thread_id, UINT mb_msg, UINT mw_msg, UINT k_msg)
{
	if(master_thread_id != NULL)
	{
		return master_thread_id;
	}
	
	master_thread_id = m_thread_id;
	UWM_MBUTTON = mb_msg;
	UWM_MWHEEL = mw_msg;
	UWM_KEYPRESS = k_msg;
	return 0;
}

EXP void setKbFilter(UINT virt, UINT scan)
{
	kb_filter_virt = virt;
	kb_filter_scan = scan;
}

EXP void setDiscardInp(UINT discard)
{
	kb_discard_orig = discard;
}