#pragma once

#include "EditorCommonAPI.h"

struct IEditor;

// Can fail if AfxGetApp() returns NULL, i.e. MFC app wasn't created yet or
// plugin has a different instance of MFC DLL, caused by plugin having
// different project configuration from Editor.
bool EDITOR_COMMON_API InitializeQt(IEditor* editor);
void EDITOR_COMMON_API FinalizeQt();
