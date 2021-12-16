// Credits.cpp : implementation file
//

#include "stdafx.h"
#include "Max.h"
#include "PolyBumpPlugin.h"
#include "Credits.h"


// CCredits dialog

IMPLEMENT_DYNAMIC(CCredits, CDialog)
CCredits::CCredits(CWnd* pParent /*=NULL*/)
	: CDialog(CCredits::IDD, pParent)
{
}

CCredits::~CCredits()
{
}

void CCredits::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCredits, CDialog)
END_MESSAGE_MAP()


// CCredits message handlers
