
#ifndef __controlmru_h__
#define __controlmru_h__

#if _MSC_VER > 1000
#pragma once
#endif


//////////////////////////////////////////////////////////////////////////
class CControlMRU : public CXTPControlRecentFileList
{
protected:
	virtual void OnCalcDynamicSize(DWORD dwMode);

private:
	DECLARE_XTP_CONTROL(CControlMRU)
	bool DoesFileExist(CString& sFileName);
};


////////////////////////////////////////////////////////////////////////////
//class CControlMRUEntry : public CXTPControlPopup
//{
//public:
//	void Initialize();
//
//protected:
//	virtual void OnClick(BOOL bKeyboard = FALSE, CPoint pt = CPoint(0, 0));
//	virtual void OnMouseHover();
//	virtual BOOL OnSetPopup(BOOL bPopup);
//
//private:
//		DECLARE_XTP_CONTROL(CControlMRUEntry)
//		bool m_bTmpDisabled;
//};

#endif // __controlmru_h__