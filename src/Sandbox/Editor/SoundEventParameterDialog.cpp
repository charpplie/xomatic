// SoundEventParameterDialog.cpp : implementation file
//

#include "stdafx.h"
#include "SoundEventParameterDialog.h"


// CSoundEventParameterDialog dialog

IMPLEMENT_DYNAMIC(CSoundEventParameterDialog, CDialog)

CSoundEventParameterDialog::CSoundEventParameterDialog(CWnd* pParent /*=NULL*/)
:	CDialog(CSoundEventParameterDialog::IDD, pParent),
	m_pSound(NULL),
	m_nIndex(-1)
{
}

CSoundEventParameterDialog::~CSoundEventParameterDialog()
{
}

void CSoundEventParameterDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSoundEventParameterDialog, CDialog)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CSoundEventParameterDialog message handlers

void CSoundEventParameterDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl* pSlider	= (CSliderCtrl*)pScrollBar;
	float const fValue		= static_cast<float>(pSlider->GetPos())*0.01f; // Can't use "nPos", on button-up on the slider nPos always gets 0
	CWnd* pCurrentValue		= GetDlgItem(IDC_SOUND_PARAMETER_CURR);
	
	if(pCurrentValue)
	{
		char cTemp[16] = {'\0'};
		sprintf_s(cTemp, 16, "%.2f", fValue);
		pCurrentValue->SetWindowText(cTemp);
	}

	// Set value of this slider
	if(m_pSound && m_nIndex > -1)
		m_pSound->SetParam(m_nIndex, fValue);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

//////////////////////////////////////////////////////////////////////////
void CSoundEventParameterDialog::SetName(char const* pcName)
{
	CWnd* pParameterName = GetDlgItem(IDC_SOUND_PARAMETER_NAME);
	if(pParameterName)
		pParameterName->SetWindowText(pcName);
}

//////////////////////////////////////////////////////////////////////////
void CSoundEventParameterDialog::SetCurrentValue(float const fCurrentValue)
{
	CSliderCtrl* pSlider = (CSliderCtrl*)GetDlgItem(IDC_SOUND_PARAMETER_SLIDER);
	if(pSlider)
		pSlider->SetPos(static_cast<int>(fCurrentValue*100.0f));

	CWnd* pCurrentValue	= GetDlgItem(IDC_SOUND_PARAMETER_CURR);
	if(pCurrentValue)
	{
		char cTemp[16] = {'\0'};
		sprintf_s(cTemp,16,"%.2f",fCurrentValue);
		pCurrentValue->SetWindowText(cTemp);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundEventParameterDialog::SetRange(float const fRangeMin, float const fRangeMax)
{
	CSliderCtrl* pSlider = (CSliderCtrl*)GetDlgItem(IDC_SOUND_PARAMETER_SLIDER);

	if(CWnd* pRangeMin = GetDlgItem(IDC_SOUND_PARAMETER_MIN))
	{
		char cTemp[16] = {'\0'};
		sprintf_s(cTemp,16,"%.2f",fRangeMin);
		pRangeMin->SetWindowText(cTemp);

		if(pSlider)
			pSlider->SetRangeMin(static_cast<int>(fRangeMin*100.0f));
	}

	if(CWnd* pRangeMax = GetDlgItem(IDC_SOUND_PARAMETER_MAX))
	{
		char cTemp[16] = {'\0'};
		sprintf_s(cTemp,16,"%.2f",fRangeMax);
		pRangeMax->SetWindowText(cTemp);

		if(pSlider)
			pSlider->SetRangeMax(static_cast<int>(fRangeMax*100.0f));
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundEventParameterDialog::SetSound(ISound* const pSound)
{
	m_pSound = pSound;

	// Now update our parameter
	CSliderCtrl* pSlider = (CSliderCtrl*)GetDlgItem(IDC_SOUND_PARAMETER_SLIDER);
	if(pSlider)
	{
		float const fValue = static_cast<float>(pSlider->GetPos())*0.01f;
		if(m_pSound && m_nIndex > -1)
			m_pSound->SetParam(m_nIndex, fValue);
	}
}
