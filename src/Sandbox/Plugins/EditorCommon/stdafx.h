#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

// These redicilous dependencies are needed just to be able to use
// Sandbox gizmos drawing and hit-testing code =(
#pragma warning(disable: 4266)
#include <SDKDDKVer.h>
#include <afxwin.h>
#include <afxdlgs.h>
#include "CryModuleDefs.h"
#include "platform.h"

#include "ISystem.h"
#include "IEditor.h"
#include "Util/EditorUtils.h"

