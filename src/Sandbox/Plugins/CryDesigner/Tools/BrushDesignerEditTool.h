#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerTool.h
//  Created:     8/12/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Core/BrushDesigner.h"
#include "Core/BaseBrush.h"
#include "Core/BrushDesignerGlobalSettings.h"
#include "Objects/DesignerBrushObject.h"

class CBrushDesigner;
class CBrushDesignerBaseTool;
class CSolidBrushCreatePanel;
class IBrushDesignerEditToolPanel;
class CBrushDesignerElementManager;

class CBrushDesignerEditTool : public CEditTool
{
	DECLARE_DYNCREATE(CBrushDesignerEditTool)
public:

	CBrushDesignerEditTool();

	static void RegisterTool( CRegistrationContext &rc );

	virtual bool Activate( CEditTool *pPreviousTool ) override;

	virtual void BeginEditParams( IEditor *ie,int flags ) override;
	virtual void EndEditParams() override;

	void Display( DisplayContext &dc ) override;
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags ) override;

	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;

	void OnManipulatorDrag( CViewport *view,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const Vec3 &value ) override;
	virtual void OnManipulatorMouseEvent( CViewport *view, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo = false ) override;

	void GetAffectedObjects( DynArray<CBaseObject*>& outAffectedObjects ) override;

	static BUtil::EDesignerMode GetDesignerMode() { return m_DesignerMode; }

	CBrushDesigner* GetDesigner() const;
	CBaseBrush* GetBrush() const;
	CBaseObject* GetBaseObject() const { return m_pBaseObject; }

	virtual void SetUserData( const char *key,void *userData ) override;
	virtual void Finish() override;

	void SetBaseObject( CBaseObject* pBaseObject );

	virtual bool IsNeedMoveTool() { return true; }

	void LeaveCurrentTool();
	void EnterCurrentTool();	

	void SelectAllElements();
	void SetSubMatID( int nSubMatID, CBrushDesigner* pDesigner );
	void MaterialChanged();

	static SDesignerEnvironmentInfo& GetGlobalEnvironmentInfo() { return m_DesignerGlobalSetting; }

	CBrushDesignerBaseTool* GetCurrentTool() const;
	void SetDesignerMode( BUtil::EDesignerMode designerMode, bool bForceChange = false );
	void GoToSelectDesignerMode() { SetDesignerMode(m_PreviousSelectMode); }
	void GoToPrevDesignerMode()
	{
		if( m_PreviousDesignerMode == BUtil::eDesigner_Mapping || m_PreviousDesignerMode == BUtil::eDesigner_SmoothingGroup )
			SetDesignerMode(m_PreviousDesignerMode); 
		else
			GoToSelectDesignerMode();
	}
	static BUtil::EDesignerMode GetPrevDesignerMode() { return m_PreviousDesignerMode; }

	void OnEditorNotifyEvent( EEditorNotifyEvent event );
	bool IsDisplayGrid() override;
	bool IsUpdateUIPanel() override;
	bool IsMoveToObjectModeAfterEnd() override { return false; }
	bool IsCircleTypeRotateGizmo() override;

	void SetPanelOwner( CDesignerBrushObject* pObj )	{ s_pDesignerObj = pObj; }
	CDesignerBrushObject* GetPanelOwner() const { return s_pDesignerObj; }

	std::set<CDesignerBrushObject*> GetSelectedDesignerObjects() const;

	void DeleteObjectIfEmpty();

	virtual int GetPanelIndex() const;

	virtual IBrushDesignerEditToolPanel* GetDesignerToolPanel() const { return CDesignerBrushObject::GetMenuPanel(); }

	CBrushDesignerElementManager* GetSelectedElements() { return m_pSelectedElements.get(); }
	void StoreSelectionUndo();
	
	void GetSelectedObjectList( std::vector<BUtil::SSelectedInfo>& selections ) const;

protected:
	
	virtual ~CBrushDesignerEditTool();
	// Delete itself.
	void DeleteThis() { delete this; };

protected:
	
	void UpdateStatusText();
	void RecordUndo();
	void InitializeEventHandlers();
	void ReleaseEventHandlers();
	void CreateMappingTableBetweenToolIdAndButtonId();

	bool SetDesignerModeToSelectElements( BUtil::EDesignerMode designerMode );

	CBrushDesignerBaseTool* GetTool( BUtil::EDesignerMode designerMode ) const
	{
		TOOLDESIGNER_MAP::const_iterator iTool = m_ToolMap.find(designerMode);
		if( iTool == m_ToolMap.end() )
			return NULL;
		return iTool->second;
	}
	void SetCheckButton( BUtil::EDesignerMode designerMode, int nCheck );

protected:

	typedef std::map<BUtil::EDesignerMode,_smart_ptr<CBrushDesignerBaseTool> > TOOLDESIGNER_MAP;
	TOOLDESIGNER_MAP m_ToolMap;

	static BUtil::EDesignerMode m_DesignerMode;
	static BUtil::EDesignerMode m_PreviousSelectMode;
	static BUtil::EDesignerMode m_PreviousDesignerMode;

	_smart_ptr<CBaseObject> m_pBaseObject;
	_smart_ptr<CDesignerBrushObject> s_pDesignerObj;

	std::unique_ptr<CBrushDesignerElementManager> m_pSelectedElements;

	static SDesignerEnvironmentInfo m_DesignerGlobalSetting;
};
