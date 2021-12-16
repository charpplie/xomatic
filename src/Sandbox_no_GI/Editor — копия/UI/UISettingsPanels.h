////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UISettingsPanels.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __UISettingsPanels__H_
#define __UISettingsPanels__H_

#include "PropertiesPanel.h"

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
struct ICVarSmartVar;

class CUISettingsPropertiesPanel : public CPropertiesPanel
{
public:
	CUISettingsPropertiesPanel();

	virtual void UpdateChanges();

protected:
	void RegisterVar( ICVarSmartVar* pVar, const char* description );
	void AddVariables()	{	UpdateVars(); SetVarBlock( m_pVarBlock, functor(*this,&CUISettingsPropertiesPanel::OnVarChange) ); }

private:
	void UpdateVars();
	void OnVarChange( IVariable *pVar );
	inline CVarBlock* GetVarBlock() const { return m_pVarBlock; }

private:
	_smart_ptr<CVarBlock> m_pVarBlock;
	std::list<ICVarSmartVar*> m_Vars;
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
struct ICVarSmartVar
{
	virtual ~ICVarSmartVar() {}

	virtual CVariableBase& GetVar() = 0;
	virtual void Read() = 0;
	virtual void Write() = 0;
	virtual void ReadIfChanged() = 0;
};

template <class T>
struct SCVarSmartVar : public ICVarSmartVar
{
	SCVarSmartVar(const char* cvar, int inactive = 0, int active = 1, bool isFlag = false)
 		: m_ActiveVal(active)
 		, m_InctiveVal(inactive)
		, m_bIsFlag(isFlag)
	{
		if (gEnv->pConsole) m_pCVar = gEnv->pConsole->GetCVar(cvar);
		assert(m_pCVar);
	}

	CVariableBase& GetVar() { return mv_Value; }
	void Read()	{	if (m_pCVar) { T data; GetData(data); mv_Value = data; } }
	void Write() { if (m_pCVar)	{	T data;	mv_Value->Get(data); SetData(data);	} }
	void ReadIfChanged()	{	if (m_pCVar) { T data; GetData(data); if (mv_Value != data) mv_Value = data; } }

private:
	void GetData(T& data)	{ assert(false); } // must be specialized...
	void SetData(const T& data) { m_pCVar->Set(data); }

	CSmartVariable<T> mv_Value;
	ICVar* m_pCVar;
	const int m_ActiveVal;
	const int m_InctiveVal;
	const bool m_bIsFlag;
};

template <>
void SCVarSmartVar<bool>::GetData(bool& data)
{
	if (m_bIsFlag)
		data = m_pCVar->GetIVal() & m_ActiveVal;
	else
		data = m_pCVar->GetIVal() == m_ActiveVal;
}

template <>
void SCVarSmartVar<bool>::SetData(const bool& data)
{
	if (m_bIsFlag)
		m_pCVar->Set( data ? m_pCVar->GetIVal() | m_ActiveVal : m_pCVar->GetIVal() & ~m_ActiveVal );
	else
		m_pCVar->Set( data ? m_ActiveVal : m_InctiveVal );
}

template <>
void SCVarSmartVar<int>::GetData(int& data)
{
	data = m_pCVar->GetIVal();
}

template <>
void SCVarSmartVar<float>::GetData(float& data)
{
	data = m_pCVar->GetFVal();
}

template <>
void SCVarSmartVar<CString>::GetData(CString& data)
{
	data = m_pCVar->GetString();
}

template <>
void SCVarSmartVar<CString>::SetData(const CString& data)
{
	m_pCVar->Set( data.GetString() );
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUISettingsPropertiesPanelDebug : public CUISettingsPropertiesPanel
{
public:
	CUISettingsPropertiesPanelDebug();

private:
	SCVarSmartVar<bool> mv_FlashInfo;
	SCVarSmartVar<bool> mv_UIElementInfo;
	SCVarSmartVar<bool> mv_UIActionInfo;
	SCVarSmartVar<bool> mv_DebugDraw;
	SCVarSmartVar<bool> mv_FlashAutoReload;
	SCVarSmartVar<float> mv_WaitForFlash;
	SCVarSmartVar<bool> mv_LogUIAction;
	SCVarSmartVar<CString> mv_LogUIActionFilter;
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUISettingsPropertiesPanelFlashInfo : public CUISettingsPropertiesPanel
{
public:
	CUISettingsPropertiesPanelFlashInfo();

private:
	SCVarSmartVar<float>   sys_flash_info_peak_tolerance;
	SCVarSmartVar<CString> sys_flash_info_peak_exclude;
	SCVarSmartVar<float>   sys_flash_info_histo_scale;
	SCVarSmartVar<float>   sys_flash_curve_tess_error;
	SCVarSmartVar<bool>   sys_flash_check_filemodtime;
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUISettingsPropertiesPanelFlashRenderInfo : public CUISettingsPropertiesPanel
{
public:
	CUISettingsPropertiesPanelFlashRenderInfo();

private:
	SCVarSmartVar<bool>  sys_flash_debugdraw;
	SCVarSmartVar<bool>  sys_flash_newstencilclear;
	SCVarSmartVar<bool>  sys_flash_edgeaa;
	SCVarSmartVar<float> sys_flash_stereo_maxparallax;
};

#endif //__UISettingsPanels__H_