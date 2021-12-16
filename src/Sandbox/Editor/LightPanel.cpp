#include "stdafx.h"
#include "LightPanel.h"

static const char *LIGHT_COLOR_DIFFUSE = "clrDiffuse";
static const char *LIGHT_COLOR_DIFFUSE_MULTIPLIER = "fDiffuseMultiplier";
static const char *LIGHT_COLOR_SPECULAR_MULTIPLIER = "fSpecularMultiplier";
static const char *LIGHT_COLOR_HDR_DYNAMIC = "fHDRDynamic";

static const char *LIGHT_PROJECTOR_FOV = "fProjectorFov";
static const char *LIGHT_PROJECTOR_NEAR_PLANE = "fProjectorNearPlane";

static const char *LIGHT_STYLE = "nLightStyle";

/*

  CLightPanel

*/

IMPLEMENT_DYNAMIC(CLightPanel, CDialog)

BEGIN_MESSAGE_MAP(CLightPanel, CDialog)
/*
	ON_CPN_XT_SELENDOK(IDC_LIGHT_COLOR, &CLightPanel::OnColor)

	ON_EN_CHANGE(IDC_LIGHT_DIFFUSE_MULTIPLIER, OnColorDiffuseMultiplier)
	ON_CONTROL(WMU_FS_CHANGED, IDC_LIGHT_DIFFUSE_MULTIPLIER_SLIDER, OnColorDiffuseMultiplierSlider)

	ON_EN_CHANGE(IDC_LIGHT_SPECULAR_MULTIPLIER, OnColorSpecularMultiplier)
	ON_CONTROL(WMU_FS_CHANGED, IDC_LIGHT_SPECULAR_MULTIPLIER_SLIDER, OnColorSpecularMultiplierSlider)

	ON_EN_CHANGE(IDC_LIGHT_HDR_DYNAMIC, OnColorHDRDynamic)
	ON_CONTROL(WMU_FS_CHANGED, IDC_LIGHT_HDR_DYNAMIC_SLIDER, OnColorHDRDynamicSlider)

	ON_EN_CHANGE(IDC_LIGHT_PROJECTOR_FOV, OnProjectorFov)
	ON_CONTROL(WMU_FS_CHANGED, IDC_LIGHT_PROJECTOR_FOV_SLIDER, OnProjectorFovSlider)

	ON_EN_CHANGE(IDC_LIGHT_PROJECTOR_NEAR_PLANE, OnProjectorNearPlane)
	ON_CONTROL(WMU_FS_CHANGED, IDC_LIGHT_PROJECTOR_NEAR_PLANE_SLIDER, OnProjectorNearPlaneSlider)

	ON_CONTROL(WMU_FS_CHANGED, IDC_LIGHT_COLOR_H, OnColorHSV)
	ON_CONTROL(WMU_FS_CHANGED, IDC_LIGHT_COLOR_S, OnColorHSV)
	ON_CONTROL(WMU_FS_CHANGED, IDC_LIGHT_COLOR_V, OnColorHSV)

	ON_CBN_SELCHANGE(IDC_LIGHT_STYLE, &CLightPanel::OnStyle)
*/
END_MESSAGE_MAP()

//

void CLightPanel::SetPropertyInteger(const char *name, int32 value)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return;
	if (pVariable->GetType() != IVariable::INT)
		return;

	pVariable->Set(value);
}

void CLightPanel::SetPropertyFloat(const char *name, f32 value)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return;
	if (pVariable->GetType() != IVariable::FLOAT)
		return;

	pVariable->Set(value);
}

void CLightPanel::SetPropertyVec3(const char *name, const Vec3 &value)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return;
	if (pVariable->GetType() != IVariable::VECTOR)
		return;

	pVariable->Set(value);
}

void CLightPanel::SetPropertyCOLORREF(const char *name, COLORREF value)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return;
	if (pVariable->GetType() != IVariable::VECTOR)
		return;

	pVariable->Set(ColorF((unsigned int)value).toVec3());
}

void CLightPanel::SetPropertyString(const char *name, const char *value)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return;
	if (pVariable->GetType() != IVariable::STRING)
		return;

	pVariable->Set(value);
}

int32 CLightPanel::GetPropertyInteger(const char *name)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return 0;
	if (pVariable->GetType() != IVariable::INT)
		return 0;

	int32 value;
	pVariable->Get(value);
	return value;
}

f32 CLightPanel::GetPropertyFloat(const char *name)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return 0.0f;
	if (pVariable->GetType() != IVariable::FLOAT)
		return 0.0f;

	f32 value;
	pVariable->Get(value);
	return value;
}

Vec3 CLightPanel::GetPropertyVec3(const char *name)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return Vec3(0.0f, 0.0f, 0.0f);
	if (pVariable->GetType() != IVariable::VECTOR)
		return Vec3(0.0f, 0.0f, 0.0f);

	Vec3 value;
	pVariable->Get(value);
	return value;
}

COLORREF CLightPanel::GetPropertyCOLORREF(const char *name)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return RGB(0, 0, 0);
	if (pVariable->GetType() != IVariable::VECTOR)
		return RGB(0, 0, 0);

	Vec3 value;
	pVariable->Get(value);
	return ColorF(value, 1.0f).pack_bgr888();
}

CString CLightPanel::GetPropertyString(const char *name)
{
	IVariable *pVariable = m_pEntity->GetProperties()->FindVariable(name);
	if (!pVariable)
		return "";
	if (pVariable->GetType() != IVariable::STRING)
		return "";

	CString value;
	pVariable->Get(value);
	return value;
}

//

BOOL CLightPanel::OnInitDialog()
{
	__super::OnInitDialog();
/*
	m_color.SetColor(RGB(255, 255, 255));
	m_color.SetWindowText("");

	m_colorH.SubclassDlgItem(IDC_LIGHT_COLOR_H, this);
	m_colorH.SetRangeFloat(0.0f, 1.0f);
	m_colorH.SetFillStyle(CFillSliderCtrl::eFillStyle_ColorHueGradient | CFillSliderCtrl::eFillStyle_Background);

	m_colorS.SubclassDlgItem(IDC_LIGHT_COLOR_S, this);
	m_colorS.SetRangeFloat(0.0f, 1.0f);
	m_colorS.SetFillStyle(CFillSliderCtrl::eFillStyle_Gradient | CFillSliderCtrl::eFillStyle_Background);

	m_colorV.SubclassDlgItem(IDC_LIGHT_COLOR_V, this);
	m_colorV.SetRangeFloat(0.0f, 1.0f);
	m_colorV.SetFillStyle(CFillSliderCtrl::eFillStyle_Gradient | CFillSliderCtrl::eFillStyle_Background);

	m_colorSpecularMultiplier.Create(this, IDC_LIGHT_SPECULAR_MULTIPLIER);
	m_colorSpecularMultiplier.SetRange(0.0f, 10.0f);
	m_colorSpecularMultiplier.SetInternalPrecision(3);
	m_colorSpecularMultiplier.SetValue(1.0f);
	m_colorSpecularMultiplier.EnableWindow(true);
	m_colorSpecularMultiplierSlider.SubclassDlgItem(IDC_LIGHT_SPECULAR_MULTIPLIER_SLIDER, this);
	m_colorSpecularMultiplierSlider.SetRangeFloat(0.0f, 10.0f);
	m_colorSpecularMultiplierSlider.SetValue(1.0f);
	m_colorSpecularMultiplierSlider.EnableWindow(true);

	m_colorDiffuseMultiplier.Create(this, IDC_LIGHT_DIFFUSE_MULTIPLIER);
	m_colorDiffuseMultiplier.SetRange(0.0f, 10.0f);
	m_colorDiffuseMultiplier.SetInternalPrecision(3);
	m_colorDiffuseMultiplier.SetValue(1.0f);
	m_colorDiffuseMultiplier.EnableWindow(true);
	m_colorDiffuseMultiplierSlider.SubclassDlgItem(IDC_LIGHT_DIFFUSE_MULTIPLIER_SLIDER, this);
	m_colorDiffuseMultiplierSlider.SetRangeFloat(0.0f, 10.0f);
	m_colorDiffuseMultiplierSlider.SetValue(1.0f);
	m_colorDiffuseMultiplierSlider.EnableWindow(true);

	m_colorHDRDynamic.Create(this, IDC_LIGHT_HDR_DYNAMIC);
	m_colorHDRDynamic.SetRange(-10.0f, 10.0f);
	m_colorHDRDynamic.SetInternalPrecision(3);
	m_colorHDRDynamic.SetValue(0.0f);
	m_colorHDRDynamic.EnableWindow(true);
	m_colorHDRDynamicSlider.SubclassDlgItem(IDC_LIGHT_HDR_DYNAMIC_SLIDER, this);
	m_colorHDRDynamicSlider.SetRangeFloat(-10.0f, 10.0f);
	m_colorHDRDynamicSlider.SetValue(0.0f);
	m_colorHDRDynamicSlider.EnableWindow(true);

	m_projectorFov.Create(this, IDC_LIGHT_PROJECTOR_FOV);
	m_projectorFov.SetRange(0.0f, 180.0f);
	m_projectorFov.SetInternalPrecision(3);
	m_projectorFov.SetValue(90.0f);
	m_projectorFov.EnableWindow(true);
	m_projectorFovSlider.SubclassDlgItem(IDC_LIGHT_PROJECTOR_FOV_SLIDER, this);
	m_projectorFovSlider.SetRangeFloat(0.0f, 180.0f);
	m_projectorFovSlider.SetValue(0.0f);
	m_projectorFovSlider.EnableWindow(true);

	m_projectorNearPlane.Create(this, IDC_LIGHT_PROJECTOR_NEAR_PLANE);
	m_projectorNearPlane.SetRange(0.0f, 100.0f);
	m_projectorNearPlane.SetInternalPrecision(3);
	m_projectorNearPlane.SetValue(90.0f);
	m_projectorNearPlane.EnableWindow(true);
	m_projectorNearPlaneSlider.SubclassDlgItem(IDC_LIGHT_PROJECTOR_NEAR_PLANE_SLIDER, this);
	m_projectorNearPlaneSlider.SetRangeFloat(0.0f, 100.0f);
	m_projectorNearPlaneSlider.SetValue(0.0f);
	m_projectorNearPlaneSlider.EnableWindow(true);

	m_style.AddString("Style0");
	m_style.AddString("Style1");
	m_style.AddString("Style2");
	m_style.AddString("Style3");
*/
	return true;
}

void CLightPanel::SetEntity(CEntity *pEntity)
{
	m_pEntity = pEntity;
	if (!m_pEntity)
		return;
/*
	m_color.SetColor(GetPropertyCOLORREF(::LIGHT_COLOR_DIFFUSE));
	OnColor();
	m_colorDiffuseMultiplier.SetValue(GetPropertyFloat(::LIGHT_COLOR_DIFFUSE_MULTIPLIER));
	OnColorDiffuseMultiplier();
	m_colorSpecularMultiplier.SetValue(GetPropertyFloat(::LIGHT_COLOR_SPECULAR_MULTIPLIER));
	OnColorSpecularMultiplier();
	m_colorHDRDynamic.SetValue(GetPropertyFloat(::LIGHT_COLOR_HDR_DYNAMIC));
	OnColorHDRDynamic();

	m_projectorFov.SetValue(GetPropertyFloat(::LIGHT_PROJECTOR_FOV));
	OnProjectorFov();
	m_projectorNearPlane.SetValue(GetPropertyFloat(::LIGHT_PROJECTOR_NEAR_PLANE));
	OnProjectorNearPlane();

	m_style.SetCurSel(GetPropertyInteger(::LIGHT_STYLE));
/*
	CString value = GetPropertyString("texture_Texture");
	SetPropertyString("texture_Texture", "textures/sprites/blackcore.dds");
	value = GetPropertyString("texture_Texture");
	const char *v = value;
*/
}

void CLightPanel::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTerrainPanel)
//	DDX_Control(pDX, IDC_LIGHT_COLOR, m_color);
//	DDX_Control(pDX, IDC_LIGHT_STYLE, m_style);
	//}}AFX_DATA_MAP
}

//

void CLightPanel::SetupColorHSV()
{
	ColorF colorStart, colorEnd;
	colorStart.fromHSV(m_colorH.GetValue(), 0.0f, m_colorV.GetValue());
	colorEnd.fromHSV(m_colorH.GetValue(), 1.0f, m_colorV.GetValue());
	m_colorS.SetFillColors(colorStart.pack_bgr888(), colorEnd.pack_bgr888());
	m_colorS.Invalidate();

	colorStart.fromHSV(m_colorH.GetValue(), m_colorS.GetValue(), 0.0f);
	colorEnd.fromHSV(m_colorH.GetValue(), m_colorS.GetValue(), 1.0f);
	m_colorV.SetFillColors(colorStart.pack_bgr888(), colorEnd.pack_bgr888());
	m_colorV.Invalidate();
}

void CLightPanel::OnColor()
{
	SetPropertyCOLORREF(::LIGHT_COLOR_DIFFUSE, m_color.GetColor());

	f32 h, s, v;
	ColorF(GetPropertyVec3(::LIGHT_COLOR_DIFFUSE), 1.0f).toHSV(h, s, v);
	m_colorH.SetValue(h);
	m_colorS.SetValue(s);
	m_colorV.SetValue(v);
	SetupColorHSV();
}

void CLightPanel::OnColorHSV()
{
	ColorF color;
	color.fromHSV(m_colorH.GetValue(), m_colorS.GetValue(), m_colorV.GetValue());
	SetPropertyVec3(::LIGHT_COLOR_DIFFUSE, color.toVec3());

	m_color.SetColor(color.pack_bgr888());
	SetupColorHSV();
}

void CLightPanel::OnColorDiffuseMultiplier()
{
	m_colorDiffuseMultiplierSlider.SetValue(m_colorDiffuseMultiplier.GetValue());
	SetPropertyFloat(::LIGHT_COLOR_DIFFUSE_MULTIPLIER,  m_colorDiffuseMultiplier.GetValue());
}

void CLightPanel::OnColorDiffuseMultiplierSlider()
{
	m_colorDiffuseMultiplier.SetValue(m_colorDiffuseMultiplierSlider.GetValue());
	OnColorDiffuseMultiplier();
}

void CLightPanel::OnColorSpecularMultiplier()
{

	m_colorSpecularMultiplierSlider.SetValue(m_colorSpecularMultiplier.GetValue());
	SetPropertyFloat(::LIGHT_COLOR_SPECULAR_MULTIPLIER, m_colorSpecularMultiplier.GetValue());
}

void CLightPanel::OnColorSpecularMultiplierSlider()
{
	m_colorSpecularMultiplier.SetValue(m_colorSpecularMultiplierSlider.GetValue());
	OnColorSpecularMultiplier();
}

void CLightPanel::OnColorHDRDynamic()
{
	m_colorHDRDynamicSlider.SetValue(m_colorHDRDynamic.GetValue());
	SetPropertyFloat(::LIGHT_COLOR_HDR_DYNAMIC, m_colorHDRDynamic.GetValue());
}

void CLightPanel::OnColorHDRDynamicSlider()
{
	m_colorHDRDynamic.SetValue(m_colorHDRDynamicSlider.GetValue());
	OnColorHDRDynamic();
}

void CLightPanel::OnProjectorFov()
{
	m_projectorFovSlider.SetValue(m_projectorFov.GetValue());
	SetPropertyFloat(::LIGHT_PROJECTOR_FOV, m_projectorFov.GetValue());
}

void CLightPanel::OnProjectorFovSlider()
{
	m_projectorFov.SetValue(m_projectorFovSlider.GetValue());
	OnProjectorFov();
}

void CLightPanel::OnProjectorNearPlane()
{
	m_projectorNearPlaneSlider.SetValue(m_projectorNearPlane.GetValue());
	SetPropertyFloat(::LIGHT_PROJECTOR_NEAR_PLANE, m_projectorNearPlane.GetValue());
}

void CLightPanel::OnProjectorNearPlaneSlider()
{
	m_projectorNearPlane.SetValue(m_projectorNearPlaneSlider.GetValue());
	OnProjectorNearPlane();
}

void CLightPanel::OnStyle()
{
	SetPropertyInteger(::LIGHT_STYLE, m_style.GetCurSel());
}
