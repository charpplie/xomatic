////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   CSoundEventParameterDialog
//  Version:     v1.00
//  Created:     26/10/2009 by Thomas.
//  Description: This the implementation of the CSoundEventParameterDialog
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __soundeventparameter_h__
#define __soundeventparameter_h__

#if _MSC_VER > 1000
#pragma once
#endif


// CSoundEventParameterDialog dialog

class CSoundEventParameterDialog : public CDialog
{
	DECLARE_DYNAMIC(CSoundEventParameterDialog)

public:
	CSoundEventParameterDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSoundEventParameterDialog();

// Dialog Data
	enum { IDD = IDD_SOUND_EVENTPARAMETER };

	void								SetName(char const* pcName);
	void								SetCurrentValue(float const iCurrentValue);
	void								SetRange(float const fRangeMin, float const fRangeMax);
	void								SetSound(ISound* const pSound);
	inline void					SetIndex(int const nIndex){m_nIndex = nIndex;}

private:

	_smart_ptr<ISound>	m_pSound;
	int									m_nIndex;

protected:
	virtual void				DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void				OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

#endif //__soundeventparameter_h__