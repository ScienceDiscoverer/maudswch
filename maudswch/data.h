#pragma once

/*___________________________________________________________________
|  regChk:
|    Checks if specified value exists in the app's registry key
|
|  name: Name of the value to test
|
|  Return value:
|       Value exist -> true
|    No value exist -> false
|____________________________________________________________________*/
bool regChk(LPCWSTR name);


/*___________________________________________________________________
|  regGet:
|    Reads DWORD value from the Registry, first check with regChk!
|
|  name: Name of the value to retrieve
|  
|  Return value:
|       Value exist -> Data stored by the value
|    No value exist -> 0
|____________________________________________________________________*/
DWORD regGet(LPCWSTR name);


/*___________________________________________________________________
|  regSet:
|    Sets empty value in the Registry
|
|  name: Name of the value to set
|____________________________________________________________________*/
void regSet(LPCWSTR name);


/*___________________________________________________________________
|  regSet(dw):
|    Sets DWORD value in the Registry
|
|  name: Name of the value to set
|  data: Data to associate with the value
|____________________________________________________________________*/
void regSet(LPCWSTR name, DWORD data);

/*___________________________________________________________________
|  regSet(sz):
|    Sets SZ (null-terminated string) value in the Registry
|
|  name: Name of the value to set
|  data: Null terminated string to associate with the value
|____________________________________________________________________*/
void regSet(LPCWSTR name, LPCWSTR data);


/*___________________________________________________________________
|  regDel:
|    Removes value from the Registry
|
|  name: Name of the value to be removed
|____________________________________________________________________*/
void regDel(LPCWSTR name);


/*___________________________________________________________________
|  regAutoStChk:
|    Tests if application is scheduled to autorun on startup
|
|  Return value:
|    AR key exist -> true
|       No AR key -> false
|____________________________________________________________________*/
bool regAutoStChk();


/*___________________________________________________________________
|  regAutoStSet:
|    Switches the state of application autorun capabilities
|
|  state: New state for autorun (true -> on, false -> off)
|____________________________________________________________________*/
void regAutoStSet(bool state);