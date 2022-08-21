#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define EXP extern "C" __declspec(dllexport)




EXP LRESULT CALLBACK MouseHookProc(int code, WPARAM wp, LPARAM lp);
EXP LRESULT CALLBACK llKbHookProc(int code, WPARAM wp, LPARAM lp);


/*___________________________________________________________________
|  setMasterThreadId:			 
|    Initialises shared DLL memory with host program's thread ID
|  
|  m_thread_id: Host thread ID
|       mb_msg: Registered Windows User mess. for mouse mbutton
|		mw_msg: Registered Windows User mess. for mouse wheel
|        k_msg: Registered Windows User mess. for keyboad events
|
|  Return value:
|        Host thread ID not set -> 0
|    Host thread ID already set -> Current host thread ID
|____________________________________________________________________*/
EXP DWORD setMasterThreadId(DWORD m_thread_id, UINT mb_msg, UINT mw_msg, UINT k_msg);


/*___________________________________________________________________
|  setKbFilter:
|    Sets specific keyboard key to hook for
|
|     virt: Virtual key code to filter for the hook 
|     scan: Scan key cose to filter for the hook
|     Note: Use MAX_INT on virt or scan to hook all the keys
|____________________________________________________________________*/
EXP void setKbFilter(UINT virt, UINT scan);


/*___________________________________________________________________
|  setDiscardInp:
|    Sets flag that controls original input discard status
|
|  discard: TRUE if orig. input needs to be discarded, FALSE if not
|____________________________________________________________________*/
EXP void setDiscardInp(UINT discard);