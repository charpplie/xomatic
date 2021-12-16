////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: CommentKeyUIControls.cpp
//  Version:   v1.00
//  Created:   29-03-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "Stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CCommentKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CCommentKeyUIControls)
public:
	CSmartVariableArray mv_table;
	CSmartVariable<CString> mv_comment;
	CSmartVariable<float> mv_duration;
	CSmartVariable<float> mv_size;
	CSmartVariable<Vec3> mv_color;
	//CSmartVariable<CString> mv_font;
	CSmartVariableEnum<int> mv_align;
	CSmartVariableEnum<CString> mv_font;
	
	
	virtual void OnCreateVars()
	{
		AddVariable( mv_table, "Key Properties" );
		AddVariable( mv_table, mv_comment, "Comment" );
		AddVariable( mv_table, mv_duration, "Duration" );
		
		mv_size->SetLimits(1.f, 10.f);
		AddVariable( mv_table, mv_size, "Size");

		AddVariable( mv_table, mv_color, "Color", IVariable::DT_COLOR );
		
		mv_align->SetEnumList(NULL);
		mv_align->AddEnumItem("Left", ICommentKey::eTA_Left);
		mv_align->AddEnumItem("Center",ICommentKey::eTA_Center);
		mv_align->AddEnumItem("Right",ICommentKey::eTA_Right);
		AddVariable( mv_table, mv_align, "Align");

		mv_font->SetEnumList(NULL);
		CFileUtil::FileArray fa;
		CFileUtil::ScanDirectory(Path::GetGameFolder()+"/Fonts/", "*.xml", fa, true);
		for(size_t i=0; i<fa.size();++i)
		{
			string name = fa[i].filename;
			PathUtil::RemoveExtension(name);
			mv_font->AddEnumItem(name.c_str(),name.c_str());
		}
		AddVariable( mv_table, mv_font, "Font");
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if(type == ATRACK_COMMENT_TEXT)
			return true;
		else
			return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CCommentKeyUIControls, CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CCommentKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_COMMENT_TEXT )
		{
			ICommentKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_comment = key.m_strComment.c_str();
			mv_duration = key.m_duration;
			mv_size = key.m_size;
			mv_font = key.m_strFont;
			mv_color = key.m_color;
			mv_align = key.m_align;

			bAssigned = true;
		}
	}
	return bAssigned;
}
//////////////////////////////////////////////////////////////////////////
// Called when UI variable changes.
void CCommentKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (size_t keyIndex = 0,num = selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_COMMENT_TEXT)
		{
			ICommentKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if(!pVar || pVar == mv_comment.GetVar())
			{
				key.m_strComment = (CString)mv_comment;				
			}

			if (!pVar || pVar == mv_font.GetVar())
			{
				CString sFont = (CString)mv_font;
				strcpy_s(key.m_strFont, sFont);
			}

			if (!pVar || pVar == mv_align.GetVar())
			{
				key.m_align = (ICommentKey::ETextAlign)((int)mv_align);
			}

			SyncValue( mv_duration,key.m_duration,false,pVar );
			SyncValue( mv_color, key.m_color, false, pVar);
			SyncValue( mv_size, key.m_size, false, pVar);
			
			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}
	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CCommentKeyUIControls_Class : public IClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {FA250B8B-FC2A-43b1-AF7A-8C3B6672B49D}
		static const GUID guid = 
		{ 0xfa250b8b, 0xfc2a, 0x43b1, { 0xaf, 0x7a, 0x8c, 0x3b, 0x66, 0x72, 0xb4, 0x9d } };
		return guid;
	}

	virtual const char* ClassName() { return "TrackView.KeyUI.Comment"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CCommentKeyUIControls); };
};
//////////////////////////////////////////////////////////////////////////

REGISTER_CLASS_DESC(CCommentKeyUIControls_Class);