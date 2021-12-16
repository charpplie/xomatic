// Copyright (c) 1999-2014 Crytek.

#define VC_EXTRALEAN
#define _WIN32_WINNT 0x0600
#include <afxwin.h>
#include <afxext.h>
#include <platform.h>

// this creates a dependency on ICryPak from the RC utilities in CryCommon, and we don't want it
#ifdef USE_GAMESTREAM
#undef USE_GAMESTREAM 
#endif

// don't allow implicit string conversions in this project
#define QT_NO_CAST_FROM_ASCII 
#include <qglobal.h>
#include <QtUtil.h>

// common Qt headers referenced by almost every file
#include <QString>
#include <QList>
