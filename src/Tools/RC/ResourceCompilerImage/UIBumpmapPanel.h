#pragma once

#include "SliderView.h"
#include "FloatEditView.h"
#include "CheckBoxView.h"
#include <set>

class CBumpProperties;


class CUIBumpmapPanel
{
public:

	// constructor
	CUIBumpmapPanel();

	void InitDialog( HWND hWndParent, const bool bShowBumpmapFileName );

	void GetDataFromDialog( CBumpProperties &rBumpProp );

	void SetDataToDialog( const CBumpProperties &rBumpProp, const bool bEnableCustoms, const bool bInitalUpdate );

	HWND GetHWND() const;

	void ChooseAddBump( HWND hParentWnd );

	TinyDocument<float> m_bumpStrengthValue;
	TinyDocument<float> m_bumpBlurValue;
	TinyDocument<bool>  m_bumpInvertValue;

private: // ---------------------------------------------------------------------------------------

	HWND									m_hTab_Normalmapgen;			// window handle
	SliderView						m_bumpStrengthSlider;
	FloatEditView					m_bumpStrengthEdit;
	SliderView						m_bumpBlurSlider;
	FloatEditView					m_bumpBlurEdit;
	CheckBoxView					m_bumpInvertCheckBox;


	void UpdateBumpStrength( const CBumpProperties &rBumpProp );
	void UpdateBumpBlur( const CBumpProperties &rBumpProp );
	void UpdateBumpInvert( const CBumpProperties &rBumpProp );

	static BOOL CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ReflectNotifications(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled, const std::set<int>& reflectIDs);
	std::set<int> reflectIDs;
};
