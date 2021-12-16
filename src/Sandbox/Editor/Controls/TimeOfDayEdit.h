#ifndef __TimeOfDayEdit_h__
#define __TimeOfDayEdit_h__

#define TIMEOFDAYN_CHANGE  0x0800

class CTimeOfDayEdit : public CXTTimeEdit
{
public:
	CTimeOfDayEdit();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual bool ProcessMask(UINT& nChar,int nEndPos);

	COLORREF GetTextColor();	
	bool IsCompleteTimeStr(CString szText);
	CString ForceMakeComleteValue(CString szText);

	void SetTime(float time);
	float GetTime() const { return m_fCurrentTime; }

	// Use SetTime(float) instead
	virtual void SetTime(int hours, int minutes) { assert(false); };
	
protected:
		DECLARE_MESSAGE_MAP()

		afx_msg void OnPaint();
		afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);

		bool m_bValidTimeValue;
		CBrush m_EditBrush;
		float m_fCurrentTime;
};

//////////////////////////////////////////////////////////////////////////
#endif // __TimeOfDayEdit_h__