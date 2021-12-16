#ifndef __LightPanel_h__
#define __LightPanel_h_
#pragma once

#include "resource.h"

#include "Objects/Entity.h"

#include "Controls/ColorPickerButton.h"
#include "Controls/FillSliderCtrl.h"

class CLightPanel :
	public CDialog
{
	DECLARE_DYNAMIC(CLightPanel)

public:
	void SetEntity(CEntity *pEntity);

public:
	enum { IDD = IDD_PANEL_LIGHT };

public:
	virtual BOOL OnInitDialog();

	virtual void DoDataExchange(CDataExchange *pDX);

public:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnColor();
	afx_msg void OnColorHSV();
	afx_msg void OnColorDiffuseMultiplier();
	afx_msg void OnColorDiffuseMultiplierSlider();
	afx_msg void OnColorSpecularMultiplier();
	afx_msg void OnColorSpecularMultiplierSlider();
	afx_msg void OnColorHDRDynamic();
	afx_msg void OnColorHDRDynamicSlider();

	afx_msg void OnProjectorFov();
	afx_msg void OnProjectorFovSlider();
	afx_msg void OnProjectorNearPlane();
	afx_msg void OnProjectorNearPlaneSlider();

	afx_msg void OnStyle();

private:
	void SetPropertyInteger(const char *name, int32 value);
	void SetPropertyFloat(const char *name, f32 value);
	void SetPropertyVec3(const char *name, const Vec3 &value);
	void SetPropertyCOLORREF(const char *name, COLORREF value);
	void SetPropertyString(const char *name, const char *value);

	int32 GetPropertyInteger(const char *name);
	f32 GetPropertyFloat(const char *name);
	Vec3 GetPropertyVec3(const char *name);
	COLORREF GetPropertyCOLORREF(const char *name);
	CString GetPropertyString(const char *name);

	void SetupColorHSV();

private: 
	CEntity *m_pEntity;

	CColorPickerButton m_color;
	CFillSliderCtrl m_colorH;
	CFillSliderCtrl m_colorS;
	CFillSliderCtrl m_colorV;

	CNumberCtrl m_colorDiffuseMultiplier;
	CFillSliderCtrl m_colorDiffuseMultiplierSlider;
	CNumberCtrl m_colorSpecularMultiplier;
	CFillSliderCtrl m_colorSpecularMultiplierSlider;
	CNumberCtrl m_colorHDRDynamic;
	CFillSliderCtrl m_colorHDRDynamicSlider;

	CNumberCtrl m_projectorFov;
	CFillSliderCtrl m_projectorFovSlider;
	CNumberCtrl m_projectorNearPlane;
	CFillSliderCtrl m_projectorNearPlaneSlider;

	CComboBox m_style;
};

#endif // __LightPanel_h_
