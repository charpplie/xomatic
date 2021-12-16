#pragma once

class CBrushDesignerCreateBoxTool;
class CBrushDesignerCreateCylinderTool;
class CBrushDesignerCreateConeTool;
class CBrushDesignerCreateSphereTool;
class CBrushDesignerDrawRectangleTool;
class CBrushDesignerDrawDiscTool;
class CBrushDesignerDrawCurveTool;
class CBrushDesignerStairTool;
class CBrushDesignerStairProfileTool;
class CBrushDesignerSubdivisionTool;
class CBrushDesignerExportTool;
class CBrushDesignerMirrorTool;
class CBrushDesignerPivotTool;
class CBrushDesignerRemoveDoublesTool;
class CBrushDesignerResetXFormTool;
class CBrushDesignerSliceTool;
class CBrushDesignerSmoothingGroupTool;
class CBrushDesignerTextureMappingTool;
class CBrushDesignerCubeEditor;
class CBrushDesignerBooleanTool;
class CBrushDesignerCloneTool;
class CBrushRegion;
class CDesignerBrushObject;
class CBrushDesignerEditTool;

class IBaseToolPanel
{
public:
	virtual void DestroyPanel() = 0;
};

class IBrushDesignerEditToolPanel : public IBaseToolPanel
{
public:
	virtual void SetEditTool( CBrushDesignerEditTool* pTool, BUtil::EDesignerMode designerMode = BUtil::eDesigner_Max ) = 0;
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) = 0;
	virtual void UpdateBackFaceCheckBox( CBrushDesigner* pDesigner ) = 0;
	virtual void UpdateCloneArrayButtons() = 0;
	virtual void DisableButton( int nButtonID ) = 0;
	virtual void SetButtonCheck( int nButtonID, int nCheckID ) = 0;
	virtual int GetPanelIndex() = 0;
};

class ICreateBoxToolPanel : public IBaseToolPanel
{
public:
	virtual void Update( const BrushVec2& p0, const BrushVec2& p1, BrushFloat fHeight ) = 0;
};

class ICreateCylinderConeToolPanel : public IBaseToolPanel
{
public:
	virtual void Update( float fRadius, float fHeight ) = 0;
	virtual int GetSubdivisionNum() const = 0;
	virtual float GetRadius() const = 0;
};

class ICreateSphereDiscCurveToolPanel : public IBaseToolPanel
{
public:
	virtual void Update( float fRadius ) = 0;
	virtual int GetSubdivisionNum() const = 0;
};

class ICreateRectangleToolPanel : public IBaseToolPanel
{
public:
	virtual void Update( float fWidth, float fDepth ) = 0;
};

class ICreateStairToolPanel : public IBaseToolPanel
{
public:
	virtual void Update( BrushFloat fWidth, BrushFloat fHeight, BrushFloat fDepth ) = 0;
	virtual void SetRotateBy90Degree( bool bRotateBy90Degree ) = 0;
	virtual bool IsRotateBy90Degree() const = 0;
	virtual bool IsMirrored() const = 0;
	virtual void SetMirrored( bool bMirrored ) = 0;
	virtual BrushFloat GetStepRise() const = 0;
};

class ICreateStairProfileToolPanel : public IBaseToolPanel
{
public:
	virtual BrushFloat GetStepRise() const = 0;
};

class IMirrorToolPanel : public IBaseToolPanel
{
public:
	virtual void ToggleWndEnableDisable() = 0;
};

class IRemoveDoubleToolPanel : public IBaseToolPanel
{
public:
	virtual BrushFloat GetDistance() const = 0;
};

class ISmoothingGroupToolPanel : public IBaseToolPanel
{
public:
	virtual void ShowAllNumbers() = 0;
	virtual void HideNumber( int nNumber ) = 0;
	virtual void ClearAllSelectionsOfNumbers( int nExcludedID = -1 ) = 0;
};

class ITextureMappingToolPanel : public IBaseToolPanel
{
public:
	virtual void SetTexInfo( const BUtil::STexInfo& texInfo ) = 0;
	virtual void SetMatID( int nMatID ) = 0;
	virtual bool IsPicking() const = 0;	
};

class IDesignerRegionDebuggerDlg : public IBaseToolPanel
{
public:
	virtual void AddRegion( CBrushRegion* pRegion, const char* name ) = 0;
	virtual int GetRegionCount() const = 0;
	virtual void Open() = 0;
};

class ICubeEditorPanel : public IBaseToolPanel
{
public:
	virtual BrushFloat GetCubeSize() const = 0;
	virtual bool IsSidesMerged() const = 0;
	virtual bool IsAddButtonChecked() const = 0;
	virtual bool IsRemoveButtonChecked() const = 0;
	virtual bool IsPaintButtonChecked() const = 0;
	virtual int GetSubMatID() const = 0;
	virtual void SetSubMatID( int nID ) const = 0;
	virtual bool SetMaterial( CMaterial* pMaterial ) = 0;
	virtual void SelectPrevBrush() = 0;
	virtual void SelectNextBrush() = 0;
	virtual void UpdateSubMaterialComboBox() = 0;
};

class ICloneToolPanel : public IBaseToolPanel
{
public:
	virtual int GetNumOfClone() = 0;
	virtual BUtil::EPlacementType GetPlacementType() const = 0;
};

class IBrushSolidFlagsPanel : public IBaseToolPanel 
{
public:
	virtual void AddVariables() = 0;
	virtual void SetObject( CDesignerBrushObject* pObject ) = 0;
	virtual void ModifyFlag( int &nFlags,int flag,CSmartVariable<bool> &var,IVariable *pVar ) = 0;
	virtual void ModifyFlag( int &nFlags,int flag,int clearFlag,CSmartVariable<bool> &var,IVariable *pVar ) = 0;
	virtual void OnVarChange( IVariable *pVar ) = 0;
	virtual void SetMultiSelectFlag( bool bEnable ) = 0;
};

IBrushDesignerEditToolPanel* CreateDesignerEditToolPanel();
ICreateBoxToolPanel* CreateBoxPanel( CBrushDesignerCreateBoxTool* pBoxCreateTool, void* pData = NULL );
ICreateCylinderConeToolPanel* CreateCylinderPanel( CBrushDesignerCreateCylinderTool* pConeCreateTool, void* pData = NULL );
ICreateCylinderConeToolPanel* CreateConePanel( CBrushDesignerCreateConeTool* pCylinderCreateTool, void* pData = NULL );
ICreateRectangleToolPanel* CreateRectanglePanel( CBrushDesignerDrawRectangleTool* pRectangleCreateTool, void* pData = NULL );
ICreateSphereDiscCurveToolPanel* CreateSpherePanel( CBrushDesignerCreateSphereTool* pSphereCreateTool, void* pData = NULL );
ICreateSphereDiscCurveToolPanel* CreateDiscPanel( CBrushDesignerDrawDiscTool* pDiscCreateTool, void* pData = NULL );
ICreateSphereDiscCurveToolPanel* CreateCurvePanel( CBrushDesignerDrawCurveTool* pCurveCreateTool, void* pData = NULL );
ICreateStairToolPanel* CreateStairPanel( CBrushDesignerStairTool* pStairCreateTool, void* pData = NULL );
ICreateStairProfileToolPanel* CreateStairProfilePanel( CBrushDesignerStairProfileTool* pStairProfileCreateTool, void* pData = NULL );
IBaseToolPanel* CreateSubdivisionToolPanel( CBrushDesignerSubdivisionTool* pSubdivisionTool, void* pData = NULL );
IBaseToolPanel* CreateExportToolPanel( CBrushDesignerExportTool* pExportTool, void* pData = NULL );
IMirrorToolPanel* CreateMirrorToolPanel( CBrushDesignerMirrorTool* pMirroTool, void* pData = NULL );
IBaseToolPanel* CreatePivotToolPanel( CBrushDesignerPivotTool* pPivotTool, void* pData = NULL );
IRemoveDoubleToolPanel* CreateRemoveDoubleToolPanel( CBrushDesignerRemoveDoublesTool* pRemoveDoubleTool, void* pData = NULL );
IBaseToolPanel* CreateResetXFormToolPanel( CBrushDesignerResetXFormTool* pResetXFormTool, void* pData = NULL );
IBaseToolPanel* CreateSliceToolPanel( CBrushDesignerSliceTool* pSliceTool, void* pData = NULL );
ISmoothingGroupToolPanel* CreateSmoothingGroupToolPanel( CBrushDesignerSmoothingGroupTool* pSmoothingGroupTool, void* pData = NULL );
ITextureMappingToolPanel* CreateTextureMappingPanel( CBrushDesignerTextureMappingTool* pTextureMappingTool, void* pData = NULL );
ICubeEditorPanel* CreateCubeEditorPanel( CBrushDesignerCubeEditor* pCubeEditor, void* pData = NULL );
IBaseToolPanel* CreateBoolaenToolPanel( CBrushDesignerBooleanTool* pBooleanTool, void* pData = NULL );
ICloneToolPanel* CreateCloneToolPanel( CBrushDesignerCloneTool* pCloneTool, void* pData = NULL );
IBrushSolidFlagsPanel* CreateSolidFlagsPanel();
IDesignerRegionDebuggerDlg* CreateRegionDebuggerDlg();