#pragma once

#include "ModelViewport.h"
#include "ArcBall.h"
#include "RenderHelpers\\AxisHelper.h"
#include "Objects\\baseobject.h"
#include "CGFContent.h"

struct IAttachment;
class CCharacterPropertiesDlg;
class ICharacterChangeListener;
class CMotionAdaptorDlg;

struct IVec	{    Vec3 ntoe;Vec3 toe;   Vec3 nheel;Vec3 heel;   };
struct AimPoses	
{ 
	const char* n0; 
	const char* n1; 
	AimPoses(const char* a0,const char* a1 )
	{
		n0=a0; 
		n1=a1; 
	}
};
struct SPlayerControlMotionParams
{
	f32	 m_fCatchUpSpeed;
};

//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CModelViewportCE : public CModelViewport
{
	DECLARE_DYNCREATE(CModelViewportCE)

public:

	CModelViewportCE();

  virtual ~CModelViewportCE();


	void InitModelViewportCE();

	virtual void OnRender();
	void LoadObject( const CString &fileName, float scale = 1.f );
	void SetCharacter( ICharacterInstance *pInstance );
//	void SetCharacterUIInfo();

	void UpdateAnimationList();
	void UpdateBoneList();

	void Update();
	Vec3 SnapToGrid( Vec3 vec ) { return vec; }; // No snapping.

	f32 CharacterPicking( const Ray& mray ); 
	f32 AttachmentPicking( const Ray& mray, IAttachment* pIAttachment, const Matrix34& m34 );

	void SetCharacterChangeListener( ICharacterChangeListener* pListener );

	virtual void PlayAnimation(const char*szName);

protected:
	//////////////////////////////////////////////////////////////////////////
	// Message Handlers
	//////////////////////////////////////////////////////////////////////////
	DECLARE_MESSAGE_MAP()

	afx_msg void OnLButtonDown( UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(		UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown( UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(		UINT nFlags, CPoint point);

	afx_msg void OnMouseMove(		UINT nFlags, CPoint point); 
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	afx_msg void OnAnimPlay();
	//////////////////////////////////////////////////////////////////////////

	void SendOnCharacterChanged();

public:
	CMaterial* GetDefaultMaterial(){return m_pDefaultMaterial;}
	virtual void DrawCharacter( ICharacterInstance* pInstance, const SRendParams &rRP );
	virtual void DrawGrid( const Quat& tmRotation, const Vec3& MotionTranslation,const Vec3& FootSlide, const Matrix33& rGridRot);

	void DrawLocator( const QuatT& location, const Vec3& moveDirectionLocal );
	void DrawHeightField();
	
	Plane RayCast(const QuatT& rPhysLocation, f32 AnimHeight, const Vec3& MiddlePos, const Vec3& obbpos, const OBB& obb, Vec3& Intersection  );
	uint32 UseGroundAlignTest(ICharacterInstance* pInstance);
	
	//////////////////////////////////////////////////////////////////////////
	// UnitTest Player Control AnimPreview
	//////////////////////////////////////////////////////////////////////////
	void AnimPreview_UnitTest( ICharacterInstance* pInstance, IAnimationSet* pIAnimationSet, const SRendParams &rRP  );
	void PlayerControl_UnitTest( ICharacterInstance* pInstance, IAnimationSet* pIAnimationSet, const SRendParams &rRP  );
	void PathFollowing_UnitTest( ICharacterInstance* pInstance, IAnimationSet* pIAnimationSet, const SRendParams &rRP  );
	void Idle2Move_UnitTest( ICharacterInstance* pInstance, IAnimationSet* pIAnimationSet, const SRendParams &rRP  );
	void IdleStep_UnitTest( ICharacterInstance* pInstance, IAnimationSet* pIAnimationSet, const SRendParams &rRP );
	void AnimEventProcessing(ICharacterInstance* pInstance);
	void PreProcessCallback(ICharacterInstance* pInstance);
	uint32 UseHumanLimbIK(ICharacterInstance* pInstance, const char* strLimb);
	void PreProcessCallback_PlayControl(ICharacterInstance* pInstance);
	void PostProcessCallback_PlayControl(ICharacterInstance* pInstance);
	void LocomotionCallback(ICharacterInstance* pInstance);
	void PlayerControlIO();
	void PlayerControlHuman( const Vec2& vViewDir2D, f32 fGroundRadian, f32 fGroundRadianMoveDir );
	void PlayerControlCamera();
	uint32 UseFootIKNew(ICharacterInstance* pInstance);
	IVec CheckFootIntersection(const Vec3& Final_Heel,const Vec3& Final_ToeN);
	f32 Idle2Move( IAnimationSet* pIAnimationSet );
	f32 IdleStep( IAnimationSet* pIAnimationSet );
	SPlayerControlMotionParams EntityFollowing(uint32& ypos);
	void PathFollowing(const SPlayerControlMotionParams& mp, f32 fDesiredSpeed );
	void SetLocomotionValuesIvo(const SPlayerControlMotionParams& mp );
	f32 PathCreation( f32 fDesiredSpeed );
	void ExternalPostProcessing(ICharacterInstance* pInstance);
	//////////////////////////////////////////////////////////////////////////
	

	//////////////////////////////////////////////////////////////////////////
	// Members
	//////////////////////////////////////////////////////////////////////////
public:
	CRect m_WinRect;

	class CCharacterEditor* m_pCharacterEditor;
	class CAttachmentsDlg* m_pAttachmentsDlg;
	class CMorphingDlg* m_pMorphingDlg;
	class CAnimationControlDlg* m_pAnimationControlDlg;
	class CCharacterPartsDialog* m_pCharacterPartsDlg;
	class CMotionAdaptorDlg* m_pMotionAdaptorDlg;


	TFace* m_pFaces;
	uint32 m_numFaces;
	ExtSkinVertex* m_pSkinVertices;
	uint32 m_numSkinVertices;

	CArcBall3D	m_ArcBall;
	CAxisHelper	m_AxisHelper;
	HitContext m_HitContext;

	enum ViewModeCE
	{
		NothingMode = 0,
		SelectMode,
		MoveMode,
		RotateMode,
		ScrollZoomMode,
		ScrollMode,
		ZoomMode,
	};
	ViewModeCE m_opMode;

	int32		m_Button_MOVE;
	int32		m_Button_ROTATE;
	int32		m_MouseButtonL;
	int32		m_MouseButtonR;
	int32		m_SelectionUpdate;
	int32   m_MouseOnAttachment;
	int32		m_SelectedAttachment;
	//	Mouse		m_Mouse;
	uint8		m_bKey[256];
	uint64	m_keyrcr[256];
	uint8		m_keyflip[256];

	afx_msg void OnAnimForcePlay();

	CCharacterPropertiesDlg* m_pCharacterPropertiesDlg;

	struct ClosestPoint {
		uint32 basemodel;
		f32 distance;
		uint32 selection; 
		ClosestPoint() {
			basemodel = 0;
			distance	=(3.3E38f-200.0f);
			selection	=-1;
		}
	};
	ClosestPoint cp;

protected:
	ICharacterChangeListener* m_pCharacterChangeListener;
	CMaterial* m_pDefaultMaterial;
};


