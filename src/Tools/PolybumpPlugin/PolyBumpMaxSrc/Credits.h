#pragma once


// CCredits dialog

class CCredits : public CDialog
{
	DECLARE_DYNAMIC(CCredits)

public:
	CCredits(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCredits();

// Dialog Data
	enum { IDD = IDD_CREDITS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
