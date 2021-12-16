////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   ModelViewport.h
//  Version:     v1.00
//  Created:     8/10/2001 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ModelViewport_h__
#define __ModelViewport_h__

#if _MSC_VER > 1000
#pragma once
#endif


#include "RenderViewport.h"
#include "Material\Material.h"
#include <ICryAnimation.h>
#include <IInput.h>
#include <ISound.h>
#include <ICharacterPartsManager.h>
#include "Util/Variable.h"

struct IPhysicalEntity;
struct CryCharAnimationParams;
struct ISkeletonAnim;
class CAnimationSet;

/////////////////////////////////////////////////////////////////////////////
// TODO: Merge this code with copied code from CryAction
class CAnimatedCharacterEffectManager
{
public:
	CAnimatedCharacterEffectManager();
	~CAnimatedCharacterEffectManager();

	void SetSkeleton(ISkeletonAnim* pSkeletonAnim,ISkeletonPose* pSkeletonPose);
	void Update(const QuatT& rPhysEntity );
	void SpawnEffect(int animID, const char* animName, const char* effectName, const char* boneName, const Vec3& offset, const Vec3& dir);
	void KillAllEffects();
	void Render(SRendParams& params);

private:
	struct EffectEntry
	{
		EffectEntry(_smart_ptr<IParticleEffect> pEffect, _smart_ptr<IParticleEmitter> pEmitter, int boneID, const Vec3& offset, const Vec3& dir, int animID);
		~EffectEntry();

		_smart_ptr<IParticleEffect> pEffect;
		_smart_ptr<IParticleEmitter> pEmitter;
		int boneID;
		Vec3 offset;
		Vec3 dir;
		int animID;
	};

	void GetEffectTM(Matrix34& tm, int boneID, const Vec3& offset, const Vec3& dir);
	bool IsPlayingAnimation(int animID);
	bool IsPlayingEffect(const char* effectName);

	ISkeletonAnim* m_pSkeleton2;
	ISkeletonPose* m_pSkeletonPose;
	DynArray<EffectEntry> m_effects;
};

/////////////////////////////////////////////////////////////////////////////
// CModelViewport window
class SANDBOX_API CModelViewport : public CRenderViewport, public IInputEventListener
{
	DECLARE_DYNCREATE(CModelViewport)

	// Construction
public:
	CModelViewport();
	virtual ~CModelViewport();

	virtual EViewportType GetType() const { return ET_ViewportModel; }
	virtual void SetType( EViewportType type ) { assert(type == ET_ViewportModel); };

	virtual void LoadObject( const CString &obj,float scale );

	virtual void OnActivate();
	virtual void OnDeactivate();

	virtual bool CanDrop( CPoint point,IDataBaseItem *pItem );
	virtual void Drop( CPoint point,IDataBaseItem *pItem );

	void AttachObjectToBone( const CString &model,const CString &bone );
	void AttachObjectToFace( const CString &model );

	void StopAnimationInLayer(int nLayer);

	// Callbacks.
	void OnShowShaders( IVariable *var );
	void OnShowNormals( IVariable *var );
	void OnShowTangents( IVariable *var );

	void OnShowPortals( IVariable *var );
	void OnShowShadowVolumes( IVariable *var );
	void OnShowTextureUsage( IVariable *var );
	void OnShowPhysics( IVariable *var );
	void OnNoCharPhysics( IVariable *var );
	void OnShowOcclusion( IVariable *var );
	void OnForceLOD( IVariable *var ); 
	void OnForceLODNum( IVariable *var );

	void OnLightColor( IVariable *var );
	void OnDisableVisibility( IVariable *var );

	void OnSubmeshSetChanged();
	ICharacterInstance* GetCharacterBase()
	{









			return m_pCharacterBase;
		
	} 

	ICharacterInstance* GetCharacterAnim(){ return m_pCharacterAnim; }
	void SetCharacterAnim(ICharacterInstance* ptr) {m_pCharacterAnim = ptr;}
	ICompoundCharacter* GetCompoundCharacter() { return m_pCompoundCharacter; }

	IStatObj* GetStaticObject(){ return m_object; }

	void GetOnDisableVisibility( IVariable *var );

	class CharPanel_Animation * GetModelPanelA() const { return m_pCharPanel_Animation; }
	void SetModelPanelA(class CharPanel_Animation * ptr) { m_pCharPanel_Animation=ptr; }

	class CharPanel_Preset * GetCharPanelPreset() const { return m_pCharPanel_Preset; }
	void SetCharPanelPreset(class CharPanel_Preset * ptr) { m_pCharPanel_Preset=ptr; }

	class CharPanel_BAttach* GetModelPanelW() const { return m_pCharPanel_BAttach; }
	void SetModelPanelW(class CharPanel_BAttach* ptr) { m_pCharPanel_BAttach=ptr; }


	class CAnimationBrowser* GetAnimBrowserPanel() const {return m_pAnimationBrowserPanel;}
	void SetAnimBrowserPanel(class CAnimationBrowser* ptr) {m_pAnimationBrowserPanel = ptr;}

	const CVarObject* GetVarObject() const { return &m_vars; }

	virtual void Update();


	void UseWeaponIK(bool val) { m_weaponIK = true; }

	// Set current material to render object.
	void SetCustomMaterial( CMaterial *pMaterial );
	// Get custom material that object is rendered with.
	CMaterial* GetCustomMaterial() { return m_pCurrentMaterial; };

	ICharacterManager* GetAnimationSystem() { return m_pAnimationSystem; };

	// Get material the object is actually rendered with.
	CMaterial* GetMaterial();

	void ReleaseObject();
	void RePhysicalize();

	Vec3 m_GridOrigin;
	_smart_ptr<ICompoundCharacter> m_pCompoundCharacter;
	_smart_ptr<ICharacterInstance> m_pCharacterBase;
	_smart_ptr<ICharacterInstance> m_pCharacterAnim;
	void UpdateAnimationList();

	class CharPanel_Animation* m_pCharPanel_Animation;
	class CharPanel_Preset* m_pCharPanel_Preset;
	class CAnimationBrowser* m_pAnimationBrowserPanel;

	void SetPaused(bool bPaused);
	bool GetPaused() {return m_bPaused;}

	virtual void PlayAnimation(const char*szName);

	const CString& GetLoadedFileName() const { return m_loadedFile; }

	void Physicalize();

protected:
	void LoadStaticObject( const CString &file );

	// Called to render stuff.
	virtual void OnRender();

	virtual void DrawGrid( const Quat& tmRotation, const Vec3& MotionTranslation,const Vec3& FootSlide, const Matrix33& rGridRot);
	void DrawCoordSystem( const QuatT& q, f32 length );

	void RestorePreviousUserDebugOptions();
	void DrawModel();
	void DrawSkyBox();
	virtual void DrawCharacter( ICharacterInstance* pInstance, const SRendParams &rp );

	virtual bool UseAnimationDrivenMotion() const;

	void SetConsoleVar( const char *var,int value );
	virtual void SetCharacterUIInfo();

	void OnEditorNotifyEvent( EEditorNotifyEvent event )
	{
		if ( event != eNotify_OnBeginGameMode )
			CRenderViewport::OnEditorNotifyEvent( event );
	}


	//virtual bool OnInputEvent( const SInputEvent &event ) = 0;
	bool OnInputEvent(const SInputEvent &rInputEvent);

	f32 m_RT;
	Vec2 m_LTHUMB;
	Vec2 m_RTHUMB;

	Vec2 m_arrLTHUMB[0x100];

	IStatObj* m_object;
	IStatObj *m_weaponModel;
	// this is the character to attach, instead of weaponModel
	_smart_ptr<ICharacterInstance> m_attachedCharacter;
	CString m_attachBone;

	AABB m_AABB;

	struct BBox	{ OBB obb; Vec3 pos; ColorB col;	};
	std::vector<BBox>  m_arrBBoxes;

	// Camera control.
	float m_camRadius;

	// True to show grid.
	bool m_bGrid;
	bool m_bBase;

	class CharPanel_BAttach* m_pCharPanel_BAttach;

	int m_rollupIndex;
	int m_rollupIndex2;

	bool m_weaponIK;

	ICharacterManager *m_pAnimationSystem;

	CString m_loadedFile;
	std::vector<CDLight> m_VPLights;

	f32 m_LightRotationRadiant;

	class CRESky* m_pRESky;
	struct ICVar* m_pSkyboxName;
	IShader *m_pSkyBoxShader;
	_smart_ptr<CMaterial> m_pCurrentMaterial;

	// Sound
	ListenerID				m_ListenerID;
	std::vector<tSoundID>	m_SoundIDs;

	//---------------------------------------------------
	//---    debug options                            ---
	//---------------------------------------------------
	CVariable<bool> mv_wireframe;
	CVariable<bool> mv_showGrid;
	CVariable<bool> mv_showBase;
	CVariable<bool> mv_showLocator;

	CVariable<bool> mv_showTangents;
	CVariable<bool> mv_showBinormals;
	CVariable<bool> mv_showNormals;

	CVariable<bool> mv_showSkeleton;
	CVariable<bool> mv_showJointNames;
	CVariable<bool> mv_showJointsValues;
	CVariable<bool> mv_showSuperimposed;
	CVariable<bool> mv_showRootUpdate;
	CVariable<bool> mv_showMotionCaps;
	CVariable<bool> mv_showStartLocation;
	CVariable<bool> mv_showFuturePath;
	CVariable<bool> mv_showBodyMoveDir;
	CVariable<bool> mv_printDebugText;
	CVariable<bool> mv_showMotionParam;

	CVariable<bool> mv_useKeyStrafe;
	CVariable<bool> mv_useNaturalSpeed;
	CVariable<bool> mv_AttachCamera;
	CVariable<float> mv_CameraSmoothRot;
	CVariable<float> mv_CameraSmoothPos;

	CVariable<bool> mv_showShaders;

	CVariable<bool> mv_lighting;
	CVariable<bool> mv_animateLights;

	CVariable<bool> mv_disableLod;

	CVariable<Vec3> mv_backgroundColor;
	CVariable<Vec3> mv_objectAmbientColor;

	CVariable<Vec3> mv_lightDiffuseColor0;
	CVariable<Vec3> mv_lightDiffuseColor1;
	CVariable<Vec3> mv_lightDiffuseColor2;

	CVariable<float> mv_fov;
	CVariable<bool> mv_showPhysics;
	CVariable<bool> mv_noCharPhysics;
	CVariable<bool> mv_showPhysicsTetriders;
	CVariable<int> mv_forceLODNum;
	CVariable<bool> mv_bForceLOD;

	CVariableArray mv_advancedTable;

	CVarObject m_vars;

public:
	IPhysicalEntity *m_pPhysicalEntity;
protected:
	CAnimatedCharacterEffectManager m_effectManager;

	bool m_bPaused;

	// Generated message map functions
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnAnimBack();
	afx_msg void OnAnimFastBack();
	afx_msg void OnAnimFastForward();
	afx_msg void OnAnimFront();
	afx_msg void OnAnimPlay();
	afx_msg void OnAnimForcePlay();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __ModelViewport_h__
