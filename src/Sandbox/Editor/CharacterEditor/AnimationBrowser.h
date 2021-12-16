
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:  AnimationBrowser.h
//  Created:     27/6/2008 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  07/06/2009 by Xiaomao: Changed from CE_Panel_Animations to AnimationBrowser,
//              to avoid comfusion with CharPanel_Animation
////////////////////////////////////////////////////////////////////////////

#ifndef __AnimationBrowser_h__
#define __AnimationBrowser_h__
#pragma once

#include "..\ToolbarDialog.h"
#include "ICryAnimation.h"

class CModelViewportCE;
class CCharacterEditor;
class CAnimationGraphDialog;

class CAnimationBrowser : public CDialog, public IEditorNotifyListener
{
public:
	typedef std::vector<CString>		TDSelectedAnimations;
	// ids of attachments in the attachment browser and the combobox
	struct SAttachmentIDs{
		int32 iIndexAttachBrowser;
		int32 iIndexComboBox;
	};


public:
	CAnimationBrowser();
	virtual ~CAnimationBrowser();

	enum 
	{ 
		IDD = IDD_DB_ENTITY,
		IDD_STANDALONE = IDD_ANIMATION_BROWSER,
	};

	BOOL CreateBitmap(CBitmap& bitmap, const int32 width, const int32 height, const BYTE* pData);
	BOOL LoadAndCombineTrueColorImages( CImageList& imageList, std::vector<UINT>& nIDResources, int nIconWidth, int nIconHeight, COLORREF colMaskColor);
	BOOL CreateBitmapToolbarDynamic(CImageList& imageList);
	void SetCharacterEditor(CCharacterEditor *pCE ) {m_pCharacterEditor = pCE;}
	void SetAnimationGraphDialog(CAnimationGraphDialog* pAnimDialog) {m_pAnimationGraphDialog = pAnimDialog;}
	void SetModelViewportCE(CModelViewportCE* pModelViewCE) {m_pModelViewportCE = pModelViewCE;}

	CModelViewportCE* GetModelViewportCE() const
	{
		return m_pModelViewportCE;
	}
	
	void ReloadAnimations();
	void UpdateAnimations(ICharacterInstance* characterInstantce);
	void RemoveAll();

	static CAnimationBrowser*&	GetCurrentInstance();

	bool GetSelectedAnimations( TDSelectedAnimations &anims );

	// Update comboBox
	void UpdateCharacterComboBox(const std::vector<const char*>& attachmentNames, const std::vector<int>& posInAttachBrowser);
	void UpdateCharacterComboBoxSelection(const int ind);
	
	std::vector<SAttachmentIDs>& GetAttachmentIDs(){return m_attachmentIDs;}

	CXTPToolBar* GetToolBar(){return &m_wndToolBar;}
	
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK() {};
	virtual void OnCancel() {};
	int32 GetAnimIcon(const int32 nAnimId, IAnimationSet* pAnimations);
	void PlaySelectedAnimations(std::vector<CString>& anims);
	void OnSelectedPMG(const std::vector<CString> anims);

	//////////////////////////////////////////////////////////////////////////
	// Messages
	//////////////////////////////////////////////////////////////////////////
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg void OnReportSelChange( NMHDR * pNotifyStruct, LRESULT *result );
	afx_msg void OnReportItemDblClick( NMHDR * pNotifyStruct, LRESULT *result );
	afx_msg void OnFilterText(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelectCharacters(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelectMotionFromHistory(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReportItemRClick( NMHDR* pNotifyStruct, LRESULT* result );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEditorNotifyListener
	//////////////////////////////////////////////////////////////////////////
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );
	//////////////////////////////////////////////////////////////////////////

	void LayOutControls();

	CXTPReportControl m_wndReport;
	CImageList m_imageList;
	CXTPToolBar m_wndToolBar;
	CModelViewportCE* m_pModelViewportCE;
	CCharacterEditor *m_pCharacterEditor;
	CAnimationGraphDialog* m_pAnimationGraphDialog;
	ICharacterInstance *m_pCharacterInstance;

	CString m_filterText;
	CString m_filterTextPrev;
private:
	std::vector<SAttachmentIDs> m_attachmentIDs;
	CString GenerateToolTips(IAnimationSet* pAnimations, const int animId);
	bool bSelMotionFromHistory;
};


#endif
