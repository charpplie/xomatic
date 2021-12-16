#pragma once 

struct IConfig;


// FUNCTION: CenterWindow (HWND, HWND) 
// PURPOSE:  Center one window over another 
// COMMENTS: 
//      Dialog boxes take on the screen position that they were designed at, 
//      which is not always appropriate. Centering the dialog over a particular 
//      window usually results in a better position. 
extern void CenterWindow( HWND hwndChild, HWND hwndParent=0 );

// redirect to the parent window
extern BOOL CALLBACK WndProcRedirect( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

// Arguments:
//   sInput - e.g. "/reduce=2 /space=tangent"
extern void UpdateOrCreateEntryFromString( IConfig *pOutCfg, const string sInput );


extern bool IsValidNameChar( unsigned char c );
