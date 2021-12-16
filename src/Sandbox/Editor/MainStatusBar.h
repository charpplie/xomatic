// (c) 2001-2013 Crytek GmbH
// Created by Sergiy Shaykin

#pragma once


class CMainStatusBar: public CXTPStatusBar, public IMainStatusBar
{
public:
	void Init(CWnd* pParentWnd);

	// from IMainStatusBar
	virtual void SetStatusText(const char* text) override;
	virtual int SetItem(const char* indicatorName, const char* text, const char* tip, void* hIcon) override;

	int Set(const char* indicatorName, const char* text, const char* tip, int nIconIndex);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);

private:
	std::map<string, int> m_indexMap;

};
