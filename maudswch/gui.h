#pragma once

#define HW(x) (((unsigned int)x) >> 16)
#define LW(x) (((unsigned int)x) & 0xFFFF)
#define S(x) ((int)(x * dpi_scale)) // Scale value for DPI

// GUI Actions - - - - - - - - -
#define GUI_AUTOSTART_CHBX 0x0
#define GUI_GLOB_KEYB_CHBX 0x1
#define GUI_BLOCK_INP_CHBX 0x2
#define GUI_SAVE_KEYC_BUTT 0x3
#define GUI_TERMINATE_BUTT 0x4
#define GUI_WINDOWWAS_DEST 0x5
// - - - - - - - - - - - - - - - 




LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);


/*___________________________________________________________________
|  initGUI:
|    Initialises some important global GUI parameters
|
|    hinst: Pointer to executable first byte in virutal memory
|  gui_msg: Unique registered user message code for UI actions
|____________________________________________________________________*/
void initGUI(HINSTANCE hinst, UINT gui_msg);


/*___________________________________________________________________
|  spawnMainWnd:
|    Registers Main Class and creates Main Window using this class
|
|  Return value:
|          Main Window created -> HWND of the Main Widnow
|    Main Window creation fail -> NULL
|____________________________________________________________________*/
HWND spawnMainWnd();


/*___________________________________________________________________
|  setControls:
|    Tells GUI module how to set up all the config elements
|
|  astrt: Autostart status
|     kh: Keyboard Hook status
|   binp: Blocking of the original input
|____________________________________________________________________*/
void setControls(bool astrt, bool kh, bool binp);


/*___________________________________________________________________
|  setButtText:
|    Changes key information displayed on Confirm Key button
|
|  kvirt: Global key Virtual Code
|  kscan: Global key Scan Code
|____________________________________________________________________*/
void setButtText(DWORD kvirt, DWORD kscan);


/*___________________________________________________________________
|  setButtAndDiscardStatus:
|    Disables or enables key confirm button and/or discard checkbox
|
|     butt: Confirm key button status (true - on, false - off)
|  discard: Discard inp. checkbox status (true - on, false - off)
|____________________________________________________________________*/
void setButtAndDiscardStatus(bool butt, bool discard);