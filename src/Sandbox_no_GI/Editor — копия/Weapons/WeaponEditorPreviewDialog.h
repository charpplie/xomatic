////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2008.
// -------------------------------------------------------------------------
//  File name:   WeaponEditorPreviewDialog.h
//  Version:     v1.00
//  Created:     18-02-2008 by Evgeny Strokov
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __WeaponEditorPreviewDialog_h__
#define __WeaponEditorPreviewDialog_h__

#include "ToolbarDialog.h"
#include "../CharacterEditor/ModelViewportCE.h"

struct IItem;
class CAxisHelper;
class CArcBall3D;
class CWeaponEditorSettingsDialog;

class CWeaponViewport : public CModelViewportCE
{
public:
	CWeaponViewport();
	~CWeaponViewport() {}

	void OnRender();
	void SetWeapon(IItem* pWeapon);
	void SetFPCamera(bool isFP) { m_isFPCamera = isFP; }
	void SetZoom(bool isZommed) { m_isZoomed = isZommed; }

	void SetSettingsDialog(CWeaponEditorSettingsDialog* pSettingsDialog);

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(		UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(		UINT nFlags, CPoint point); 

	IItem* m_pWeapon;
	bool m_isFPCamera;
	bool m_isZoomed;

	std::auto_ptr<CAxisHelper> m_pAxisHelper;
	std::auto_ptr<CArcBall3D>  m_pArcBall;
	CWeaponEditorSettingsDialog* m_pSettingsDialog;
};

//////////////////////////////////////////////////////////////////////////
class CWeaponEditorPreviewDialog : public CToolbarDialog
{
	DECLARE_DYNAMIC(CWeaponEditorPreviewDialog)
public:
	static void RegisterViewClass();

	CWeaponEditorPreviewDialog();
	~CWeaponEditorPreviewDialog();

	enum { IDD = IDD_DATABASE };

	void RedrawPreview();

	//void SetCharacter(const char* szCharacterName);
	//ICharacterInstance* GetCharacter();
	CWeaponViewport* GetModelViewport() { return m_pModelViewport; }

protected:
	virtual void OnOK() {};
	virtual void OnCancel() {};

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);

private:
	CWeaponViewport* m_pModelViewport;
};

#endif __WeaponEditorPreviewDialog_h__
