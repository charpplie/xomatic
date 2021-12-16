////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: ScreenFaderKeyUIControls.cpp
//  Version:   v1.00
//  Created:   28-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "Stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

#include "TrackViewAnimNode.h"
#include "TrackViewTrack.h"

//-----------------------------------------------------------------------------
//!
class CScreenFaderKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CScreenFaderKeyUIControls)

	CSmartVariableArray		mv_table;
	CSmartVariable<float>	mv_fadeTime;
	CSmartVariable<Vec3>	mv_fadeColor;
	CSmartVariable<CString> mv_strTexture;
	CSmartVariable<bool>	mv_bUseCurColor;
	CSmartVariableEnum<int> mv_fadeType;
	CSmartVariableEnum<int> mv_fadechangeType;

public:

	//-----------------------------------------------------------------------------
	//!
	virtual bool SupportTrackType( const CAnimParamType &paramType, EAnimCurveType trackType, EAnimValue valueType ) const
	{
		return paramType == eAnimParamType_ScreenFader;
	}

	//-----------------------------------------------------------------------------
	//!
	virtual void OnCreateVars()
	{
		AddVariable( mv_table, "Key Properties" );

		mv_fadeType->SetEnumList(NULL);
		mv_fadeType->AddEnumItem("FadeIn", IScreenFaderKey::eFT_FadeIn);
		mv_fadeType->AddEnumItem("FadeOut", IScreenFaderKey::eFT_FadeOut);
		AddVariable( mv_table, mv_fadeType, "Type");

		mv_fadechangeType->SetEnumList(NULL);
		mv_fadechangeType->AddEnumItem("Linear", IScreenFaderKey::eFCT_Linear);
		mv_fadechangeType->AddEnumItem("Square", IScreenFaderKey::eFCT_Square);
		mv_fadechangeType->AddEnumItem("Cubic Square", IScreenFaderKey::eFCT_CubicSquare);
		mv_fadechangeType->AddEnumItem("Square Root", IScreenFaderKey::eFCT_SquareRoot);
		mv_fadechangeType->AddEnumItem("Sin", IScreenFaderKey::eFCT_Sin);
		AddVariable( mv_table, mv_fadechangeType, "ChangeType" );

		AddVariable( mv_table, mv_fadeColor, "Color", IVariable::DT_COLOR);

		mv_fadeTime->SetLimits(0.f, 100.f);
		AddVariable( mv_table, mv_fadeTime, "Duration");
		AddVariable( mv_table, mv_strTexture, "Texture",IVariable::DT_TEXTURE);
		AddVariable( mv_table, mv_bUseCurColor,"Use Current Color");
	}

	//-----------------------------------------------------------------------------
	//!
	virtual bool OnKeySelectionChange(CTrackViewKeyBundle &keys);

	//-----------------------------------------------------------------------------
	//!
	virtual void OnUIChange( IVariable *pVar, CTrackViewKeyBundle &keys );

	virtual unsigned int GetPriority() const { return 1; }
};

IMPLEMENT_DYNCREATE(CScreenFaderKeyUIControls, CTrackViewKeyUIControls)

//-----------------------------------------------------------------------------
bool CScreenFaderKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle &keys)
{
	if (!keys.AreAllKeysOfSameType())
		return false;

	bool bAssigned = false;
	if (keys.GetKeyCount() == 1)
	{
		CTrackViewKeyHandle &keyHandle = keys.GetKey(0);

		CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
		if (paramType == eAnimParamType_ScreenFader )
		{
			IScreenFaderKey screenFaderKey;
			keyHandle.GetKey(&screenFaderKey);

			mv_fadeTime = screenFaderKey.m_fadeTime;
			mv_fadeColor = Vec3(screenFaderKey.m_fadeColor.x,screenFaderKey.m_fadeColor.y,screenFaderKey.m_fadeColor.z);
			mv_strTexture = screenFaderKey.m_strTexture;
			mv_bUseCurColor = screenFaderKey.m_bUseCurColor;
			mv_fadeType = (int)screenFaderKey.m_fadeType;
			mv_fadechangeType = (int)screenFaderKey.m_fadeChangeType;

			bAssigned = true;
		}
	}
	return bAssigned;
}

//-----------------------------------------------------------------------------
void CScreenFaderKeyUIControls::OnUIChange(IVariable *pVar, CTrackViewKeyBundle &selectedKeys)
{
	if (!selectedKeys.AreAllKeysOfSameType())
		return;

	for (size_t keyIndex = 0,num = selectedKeys.GetKeyCount(); keyIndex < num; ++keyIndex)
	{
		CTrackViewKeyHandle selectedKey = selectedKeys.GetKey(keyIndex);
		
		CAnimParamType paramType = selectedKey.GetTrack()->GetParameterType();
		if (paramType == eAnimParamType_ScreenFader)
		{
			IScreenFaderKey screenFaderKey;
			selectedKey.GetKey(&screenFaderKey);

			SyncValue( mv_fadeTime, screenFaderKey.m_fadeTime, false, pVar);

			SyncValue( mv_bUseCurColor, screenFaderKey.m_bUseCurColor, false, pVar);
			
			if(pVar == mv_fadeTime.GetVar())
			{
				screenFaderKey.m_fadeTime = MAX((float)mv_fadeTime, 0.f);

			}
			else if (pVar == mv_strTexture.GetVar())
			{
				CString sTexture = mv_strTexture;
				strcpy_s(screenFaderKey.m_strTexture, sTexture);
			}
			else if (pVar == mv_fadeType.GetVar())
			{
				screenFaderKey.m_fadeType = IScreenFaderKey::EFadeType((int)mv_fadeType);
			}
			else if( pVar == mv_fadechangeType.GetVar())
			{
				screenFaderKey.m_fadeChangeType = IScreenFaderKey::EFadeChangeType((int)mv_fadechangeType);
			}
			else if (pVar == mv_fadeColor.GetVar())
			{
				Vec3 color = mv_fadeColor;
				screenFaderKey.m_fadeColor = Vec4(color, screenFaderKey.m_fadeType == IScreenFaderKey::eFT_FadeIn ? 1.f : 0.f);
			}
			
			selectedKey.SetKey(&screenFaderKey);
		}
	}
}

//-----------------------------------------------------------------------------
//!
class CScreenFaderKeyUIControls_Class : public IClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {FBBC2407-C36B-45b2-9A54-0CF9CD3908FD}
		static const GUID guid = 
		{ 0xfbbc2407, 0xc36b, 0x45b2, { 0x9a, 0x54, 0xc, 0xf9, 0xcd, 0x39, 0x8, 0xfd } };
		return guid;
	}

	virtual const char* ClassName() { return "TrackView.KeyUI.ScreenFader"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CScreenFaderKeyUIControls); };
};
REGISTER_CLASS_DESC(CScreenFaderKeyUIControls_Class);