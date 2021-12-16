////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ICryAnimation.h
//  Version:     v1.00
//  Created:     1/8/2004 by Ivo Herzeg
//  Compilers:   Visual Studio.NET
//  Description: CryAnimation interface
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include DEVIRTUALIZE_HEADER_FIX(ICryAnimation.h)

#ifndef ICRY_ANIMATION
#define ICRY_ANIMATION

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef CRYANIMATION_API
	#ifdef CRYANIMATION_EXPORTS
		#define CRYANIMATION_API DLL_EXPORT
	#else
		#define CRYANIMATION_API DLL_IMPORT
	#endif
#endif


//DOC-IGNORE-BEGIN
#include <Cry_Math.h>
#include <Cry_Geo.h>

#include <IRenderer.h>
#include <IPhysics.h>
#include <I3DEngine.h>
#include <IEntityRenderState.h>
#include <IRenderAuxGeom.h>
#include <IEntitySystem.h>
#include <IParticles.h>

#include "CryCharAnimationParams.h"
//////////////////////////////////////////////////////////////////////
// Handle which determines the object binding to a bone in the character.
//typedef ULONG_PTR ObjectBindingHandle;

// NOTE: This will eventually become deprecated.
enum EAnimationJointType
{
	eIM_BipIdx,
	eIM_PelvisIdx,
	eIM_Spine0Idx,
	eIM_Spine1Idx,
	eIM_Spine2Idx,
	eIM_Spine3Idx,
	eIM_NeckIdx,
	eIM_NeckIdx1,
	eIM_HeadIdx,

	eIM_LeftEyeIdx,
	eIM_RightEyeIdx,

	eIM_RWeaponBoneIdx,
	eIM_LWeaponBoneIdx,

	eIM_RClavicleIdx,
	eIM_RUpperArmIdx,
	eIM_RForeArmIdx,
	eIM_RHandIdx,

	eIM_LClavicleIdx,
	eIM_LUpperArmIdx,
	eIM_LForeArmIdx,
	eIM_LHandIdx,

	eIM_LThighIdx,
	eIM_LCalfIdx,
	eIM_LFootIdx,
	eIM_LHeelIdx,
	eIM_LToe0Idx,

	eIM_RThighIdx,
	eIM_RCalfIdx,
	eIM_RFootIdx,
	eIM_RHeelIdx,
	eIM_RToe0Idx,


	// Various non-human monsters
	eIM_HunterRootIdx,
	eIM_HunterFrontLegL01Idx,
	eIM_HunterFrontLegL12Idx,
	eIM_HunterFrontLegR01Idx,
	eIM_HunterFrontLegR12Idx,
	eIM_HunterBackLegL01Idx,
	eIM_HunterBackLegL13Idx,
	eIM_HunterBackLegR01Idx,
	eIM_HunterBackLegR13Idx,

	// insert new value id BEFORE last
	eIM_Last,

};

enum EAttachFlags	{
	FLAGS_ATTACH_ZOFFSET = 1, // Used to make sure a binding is more visible to the camera despite its distance.
	FLAGS_ATTACH_PHYSICALIZED_RAYS = 2,
	FLAGS_ATTACH_PHYSICALIZED_COLLISIONS = 4,
	FLAGS_ATTACH_PHYSICALIZED = FLAGS_ATTACH_PHYSICALIZED_RAYS|FLAGS_ATTACH_PHYSICALIZED_COLLISIONS,
};

//Flags used by game DO NOT REORDER
enum AttachmentTypes {
	CA_BONE,
	CA_FACE,
	CA_SKIN,
  CA_PART,// character part
};

//Flags used by game
enum ECharRenderFlags {
	CS_FLAG_DRAW_MODEL		  = 1 << 0,
	CS_FLAG_DRAW_NEAR			  = 1 << 1,
	CS_FLAG_UPDATE				  = 1 << 2,
	CS_FLAG_UPDATE_ALWAYS   = 1 << 3,
	CS_FLAG_UPDATE_ONRENDER = 1 << 4,
  CS_FLAG_COMPOUND_BASE   = 1 << 5
};

//Flags used human IK
enum IKFlags 
{
	CA_ARM_LEFT		= 0,
	CA_ARM_RIGHT	= 1,
	CA_LEG_LEFT		= 2,
	CA_LEG_RIGHT	= 3
};

//LMG animation check error codes
enum LmgAnimationStatus
{
	LMGAS_OK = 0,

	LMGAS_ANIMATION_NOT_IN_CAL_FILE,
	LMGAS_ASSET_DOESNT_EXIST,
	LMGAS_ANIMATION_HAS_NO_SPEED,
	LMGAS_INVALID_BLEND_CODE
};


#define CHR (0x11223344)
#define CGA (0x55aa55aa)

#define NULL_ANIM "null"
#define NULL_ANIM_FILE "null"


// Forward declarations
struct	IShader;
struct  SRendParams;
struct CryEngineDecalInfo;
struct ParticleParams;
struct CryCharMorphParams;
struct IMaterial;
struct IStatObj;
struct IRenderMesh;
class CDLight;

struct ICharacterManager;
struct ICharacterInstance;
struct ICharacterModel;
struct IAnimationSet;

struct ISkeletonAnim;
struct ISkeletonPose;
struct IMorphing;
struct IAttachmentManager;

struct IAttachment;
struct IAttachmentObject; // Entity, static object or character
struct IAnimEvents;
struct ExtSkinVertex;
struct TFace;
struct IFacialInstance;
struct IFacialAnimation;

class ICrySizer;
struct CryCharAnimationParams;

struct IAnimationLayer;
typedef int (*CallBackFuncType)(ICharacterInstance*,void*);

// Description:
//     This class is the main access point for any character anymation 
//     required for a program which uses CryEngine.
// See Also:
//     CreateCharManager
UNIQUE_IFACE struct ICharacterManager
{

	// Description:
	//     Model keep in memory hints.
	//     The model of a character can be locked in memory. This means, that even if the 
	//     number of characters using this model drops to zero, it'll be kept in memory.
	//     Such model is called Persistent.
	//     A model that's deleted as soon as there are no characters referencing it is called Transient.
	//     The default (when there's no hint) is defined by ca_KeepModels console variable.
	//     If there are both hints, it's up to the character animation system to decide whether to keep it or not.
	// Summary:
	//     Flags to unload a model from memory when it's no longer being used
	enum EModelPersistence
	{
		// Let the Animation System releases the model data when it's no longer used
		nHintModelTransient  = 1, 
		// Force the model data to stay in memory even if no character instance uses it anymore
		nHintModelPersistent = 2, 
	};

	// Description:
	//     Contains statistics about CryCharManager.
	struct Statistics
	{
		// Number of character instances
		unsigned numCharacters;
		// Number of character models (CGF)
		unsigned numCharModels;
		// Number of animobjects
		unsigned numAnimObjects;
		// Number of animobject models
		unsigned numAnimObjectModels;
	};

	// Description:
	//     Will fill the Statistics parameters with statistic on the instance 
	//     of the Animation System.
	//     It isn't recommanded to call this function often.
	// Arguments:
	//     rStats - Structure which hold the statistics
	// Summary:
	//     Get statistics on the Animation System
	virtual void GetStatistics(Statistics& rStats) = 0;

	// Description:
	//     Gather the memory currently used by the animation. The information
	//     returned is classified according to the flags set in the sizer 
	//     argument.
	// Arguments:
	//     pSizer - Sizer class which will store the memory usage
	// Summary:
	//     Track memory usage
	virtual void GetMemoryUsage( ICrySizer* pSizer) const = 0;

	// Description:
	//     Create a new instance for a model Load the model file along with any animation file that might be
	//     available.
	// See Also:
	//     RemoveCharacter
	// Arguments:
	//     szFilename - Filename of the model to be loaded
	//     nFlags     - Set how the model will be kept in memory after being 
	//                  used. Uses flags defined with EModelPersistence.
	// Return Value:
	//     A pointer to a ICharacterInstance class if the model could be loaded 
	//     properly.
	//     NULL if the model couldn't be loaded.
	// Summary:
	//     Create a new instance of a model
	//	virtual ICharacterInstance * MakeCharacter(const char * szFilename, unsigned nFlags = 0)=0;
	virtual ICharacterInstance* CreateInstance(const char * szFilename, uint32 IsSkinAtt=0, IAttachment* pIMasterAttachment=0 )=0;
  virtual ICharacterInstance* CreateCHRInstance( const char* strPath, uint32 nFlags=0, IAttachment* pMasterAttachment=0 ) = 0;
	// Description:
	//     Cleans up all resources. Currently deletes all bodies and characters even if there are references on them.
	// Arguments:
	//     bForceCleanup - When set to true will force all models to be deleted, even if references to them still left.
	// Summary:
	//     Cleans up all resources 
	virtual void ClearResources( bool bForceCleanup ) = 0;

	// Description:
	//     Update the Animation System. It's important to call this function at every frame. This should perform very fast.
	// Summary:
	//     Update the Animation System
	virtual void Update() = 0;

	// Description:
	//     Increment the frame counter.
	// Summary:
	//     Useful to prevent log spam: "several updates per frame..."
	virtual void DummyUpdate() = 0;

	// Description:
	//     Releases any resource allocated by the Animation System and shut it down properly.
	// Summary:
	//     Release the Animation System
	virtual void Release()=0;

	//! Locks all models in memory

	// Description:
	//     Lock all the models to stay loaded in memory.
	// See Also:
	//     UnlockResources
	// Summary:
	//     Lock all the models to stay loaded in memory.
	virtual void LockResources() = 0;

	// Description:
	//     Unlock all the models allow them to be unloaded from memory.
	// See Also:
	//     LockResources
	// Summary:
	//     Unlock all the models allow them to be unloaded from memory.
	virtual void UnlockResources() = 0;

	virtual uint32 SaveCharacterDefinition(ICharacterInstance* ptr, const char* pathname) = 0;

	// Description:
	//     Retrieve all loaded models.
	virtual void GetLoadedModels( ICharacterModel** pCharacterModels,int &nCount ) = 0;

  // Description
  //     Reloads loaded model
  virtual void ReloadModel(ICharacterModel* pModel) = 0;
  virtual void ReloadAllModels() = 0;

	//////////////////////////////////////////////////////////////////////////
	// Retrieve facial animation interface.
	virtual IFacialAnimation* GetIFacialAnimation() = 0;
	virtual IAnimEvents* GetIAnimEvents() = 0;

	virtual void SetScalingLimits( const Vec2& limits ) = 0;


	// Description:
	//		Use to synchronize all animation computations like forwardkinematics and calls
	//		all function which must occur after these like SkeletonPostProcess
	//		Should be called only once per frame and as late as possible to prevent waiting
	//		for functions which run asynchronly
	virtual void SyncAllAnimations() =0;
	
	// Description:
	//		Adds an animation to the synchronize queue from SyncAllAnimations
	virtual void AddAnimationToSyncQueue(ICharacterInstance *pCharInstance) =0;

	// Description:
	//		Removes an animation to the synchronize queue from SyncAllAnimations
	virtual void RemoveAnimationToSyncQueue( ICharacterInstance *pCharInstance ) =0;
};

struct CloseInfo {
	uint32 FaceNr;
	Vec3 tv0;
	Vec3 tv1;
	Vec3 tv2;
	Vec3 middle;
};

struct SJointProperty {
	SJointProperty(const char *pname, float val) { name=pname; type=0; fval=val; }
	SJointProperty(const char *pname, bool val) { name=pname; type=1; bval=val; }
	SJointProperty(const char *pname, const char *val) { name=pname; type=2; strval=val; }
	const char *name;
	int type; 
	union {
		float fval;
		bool bval;
		const char *strval;
	};
};



UNIQUE_IFACE struct ICharacterModelSkeleton
{
	virtual uint32 GetJointCount() const = 0;
	virtual int32 GetJointIDByName(const char* name) const = 0;
	virtual const char* GetJointNameByID(int32 id) const = 0;
	virtual int32 GetJointParentIDByID(int32 id) const = 0;

	// NOTE: Will become deprecated.
	virtual int32 GetJointIDByType(EAnimationJointType eType) const = 0;

//	virtual const IKLimbType* GetLimbDefinition(int32 index) const = 0;
//	virtual int32 GetLimbDefinitionIdx(uint64 handle) const = 0;
	// END: Will become deprecated.
};

//////////////////////////////////////////////////////////////////////////
UNIQUE_IFACE struct ICharacterModel
{
  // Returns type name given to the model on creation time.
  virtual const char* GetName() const = 0;
	// Return number of instances for that model.
	virtual void AddRef() = 0;
	virtual void Release() = 0;
	virtual uint32 GetNumInstances() = 0;
	virtual uint32 GetNumLods() = 0;
	virtual const char* GetModelFilePath() const =0;

  // joints
  virtual phys_geometry* GetJointPhysGeom(uint32 jointIndex,int nLod) = 0;
	virtual DynArray<SJointProperty> GetJointPhysProperties(uint32 jointIndex, int nLod) = 0;
	virtual bool SetJointPhysProperties(uint32 jointIndex, int nLod, const DynArray<SJointProperty> &props) = 0;

//	virtual const char * GetFile() = 0;
	// Retrieve render mesh for specified lod.
	virtual IRenderMesh* GetRenderMesh( int nLod ) = 0;

  virtual CloseInfo FindClosestPointOnMesh( const Vec3& RMWPosition,bool bUseCenterPointDist=true ) = 0; 

	virtual uint32 GetTextureMemoryUsage( ICrySizer *pSizer=0 ) = 0;
	virtual uint32 GetMeshMemoryUsage(ICrySizer *pSizer=0) = 0;
	virtual ICharacterInstance * GetInstance(uint32 num) = 0;

	virtual ICharacterModelSkeleton* GetICharacterModelSkeleton() = 0;

	// NOTE: Will become deprecated.
	// Use GetICharacterModelSkeleton() instead!
	virtual uint32 GetNumJoints() = 0;
	// END: Will become deprecated.
};


//DOC-IGNORE-BEGIN
//! TODO:
//! Split this interface up into a few logical interfaces, starting with the ICryCharModel
//DOC-IGNORE-END

struct SCharUpdateFeedback
{
	SCharUpdateFeedback() { flags=0; pPhysHost=0; mtxDelta.SetIdentity(); }
	int flags;									// |1 if pPhysHost is valid, |2 is mtxDelta is valid
	IPhysicalEntity *pPhysHost;	// tells the caller to restore this host as the main phys entity
	Matrix34 mtxDelta;					// tells the caller to instantly post-multiply its matrix with this one
};

// Description:
//     This interface contains methods for manipulating and querying an animated character
//     Instance. The methods allow modify the animated instance to the certain way,
//     animate it, render, retrieve BBox/etc, control physics, particles and skinning, transform.
// Summary:
//     Interface to character animation
struct ICharacterInstance
{
	// Increase reference count of the interface
	virtual void AddRef() = 0;
	// Decrease reference count of the interface
	virtual void Release() = 0;

	//////////////////////////////////////////////////////////////////////
	// Description:
	//     Return a pointer of the instance of a ISkeletonAnim derived class applicable for the model.
	// Return Value:
	//     A pointer to a ISkeletonAnim derived class
	// Summary:
	//     Get the skeleton for this instance
	virtual ISkeletonAnim* GetISkeletonAnim() = 0;


	//////////////////////////////////////////////////////////////////////
	// Description:
	//     Return a pointer of the instance of a ISkeletonPose derived class applicable for the model.
	// Return Value:
	//     A pointer to a ISkeletonPose derived class
	// Summary:
	//     Get the skeleton for this instance
	virtual ISkeletonPose* GetISkeletonPose() = 0;


	//////////////////////////////////////////////////////////////////////
	// Description:
	//     Return a pointer of the instance of a IMorphing derived class.
	// Return Value:
	//     A pointer to a IMorphing derived class
	// Summary:
	//     Get the low-level morphing interface for this instance
	virtual IMorphing* GetIMorphing() = 0;


	//////////////////////////////////////////////////////////////////////
	// Description:
	//     Return a pointer of the instance of a IAttachmentManager derived class applicable for the model.
	// Return Value:
	//     A pointer to a IAttachmentManager derived class
	// Summary:
	//     Get the attachment manager for this instance
	virtual IAttachmentManager* GetIAttachmentManager() = 0;




	//////////////////////////////////////////////////////////////////////
	// Description:
	//     Return shared character model used by this instance.
	virtual ICharacterModel* GetICharacterModel() = 0;

	// Description:
	//     Return a pointer of the instance of a ICryAnimationSet derived class applicable for the model.
	// Return Value:
	//     A pointer to a ICryAnimationSet derived class
	// Summary:
	//     Get the Animation Set defined for the model
	virtual IAnimationSet* GetIAnimationSet() = 0;

	// Description:
	//		Get the name of the file that stores the animation event definitions for this model. This
	//		is usually stored in the CAL file
	// Return Value:
	//		A pointer to a null terminated char string which contains the filename of the database
	// Summary:
	//		Get the animation event file
	virtual const char* GetModelAnimEventDatabase()=0;

	virtual void Serialize(TSerialize ser)=0;

  virtual void ReplaceModel(ICharacterModel* pNewModel) = 0;

	//enables/disables StartAnimation* calls; puts warning into the log if StartAnimation* is called while disabled
	virtual void EnableStartAnimation (bool bEnable)=0;

	//! apply animation to the base-character. 
	virtual void SkeletonPreProcess (const QuatT &rPhysLocationCurr,const QuatTS &rAnimLocationCurr, const CCamera& rCamera, uint32 OnRender ) =0;
	//! Updates the bones and the bounding box. 
	virtual void SkeletonPostProcess(const QuatT &rPhysLocationNext,const QuatTS &rAnimLocationNext, IAttachment* pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )=0;

	virtual void UpdateAttachedObjectsFast(const QuatT& rPhysLocationNext, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )=0;

	//! disable rendering of this render this instance
	virtual void HideMaster(uint32 h)=0;

	virtual void DrawWireframeStatic( const Matrix34& m34, int nLOD, uint32 color)=0;

	virtual uint32 GetResetMode()=0;
	virtual void SetResetMode(uint32 rm)=0;


	//! Return dynamic bbox of object
	// Description:
	// Arguments:
	// Summary:
	//     Get the bounding box
	virtual AABB GetAABB()=0;

	// Summary:
	//     Return the extent (length, volume, or area) of the object
	// Arguments:
	//     eType, eForm - See RandomPos
	virtual f32 ComputeExtent(GeomQuery& geo, EGeomForm eForm)=0;

	// Summary:
	//     Generate a random point in object.
	// Arguments:
	//     eType - Object geometry to use (physics or render geometry)
	//     eForm - Object aspect to generate on (surface, volume, etc)
	virtual void GetRandomPos(RandomPos& ran, GeomQuery& geo, EGeomForm eForm)=0;


	// Description:
	//     Draw the character using specified rendering parameters.
	// Arguments:
	//     RendParams - Rendering parameters
	// Summary:
	//     Draw the character
	virtual void Render(const SRendParams& RendParams, const QuatTS& Offset, Matrix34 *pFinalPhysLocation=0, bool* updated = 0)=0;	

	// Description:
	//     Set rendering flags defined in ECharRenderFlags for this character instance
	// Arguments:
	//     Pass the rendering flags
	// Summary:
	//     Set rendering flags
	virtual void SetFlags(int nFlags)=0;

	// Description:
	//     Get the rendering flags enabled. The valid flags are the ones declared in ECharRenderFlags.
	// Return Value:
	//     Return an integer value which hold the different rendering flags
	// Summary:
	//     Set rendering flags
	virtual int  GetFlags()=0;

	// Description:
	//     Get the object type contained inside the character instance. It will return either the CHR or CGA constant.
	// Return Value:
	//     An object type constant
	// Summary:
	//     Get the object type
	virtual int GetOjectType() = 0;

	// Description:
	//     Set the color to be used by the character instance
	// Arguments:
	//     fR - Red value 
	//     fG - Green value
	//     fB - Blue value
	//     fA - Alpha value
	// Summary:
	//     Set the color
	virtual void SetColor(f32 fR, f32 fG, f32 fB, f32 fA) = 0;


	// Description:
	//     Get the RenderMesh materials in this character as they are used in the renderer
	// Return Value:
	//     List of material info
	// Summary:
	//     Return the RenderMesh materials
	virtual const PodArray<struct CRenderChunk>*getRenderMeshMaterials() {return NULL;}

	// Description:
	//     Get a property attached to the model during exporting process.
	// Return Value:
	//     A pointer to a null terminated char string which contain the filename of the model.
	// Summary:
	//     Get the filename of the model
	virtual const char* GetFilePath()=0;

	//! Returns true if this character was created from the file the path refers to.
	//! If this is true, then there's no need to reload the character if you need to change its model to this one.
	virtual bool IsModelFileEqual (const char* szFileName) = 0;


	//! is called when the character is datached (if it was an attachment)
	virtual void OnDetach() = 0;



	//! enable and disable decals for this instance by default its always disabled
	virtual void EnableDecals(uint32 d)=0;
	//! Spawn a decal on animated characters
	//! The decal hit direction and source point are in the local coordinates of the character.
	virtual void CreateDecal(CryEngineDecalInfo& DecalLCS)=0;


	//! Pushes the underlying tree of objects into the given Sizer object for statistics gathering
	virtual void GetMemoryUsage(class ICrySizer* pSizer) const = 0;
	//! notifies the renderer that the character will soon be rendered
	virtual void PreloadResources ( f32 fDistance, f32 fTime, int nFlags) {}

	virtual void ReleaseTemporaryResources() = 0;







	// Description:
	//     Set custom instance material for this character.
	// Arguments:
	//     pMaterial - A valid pointer to the material.
	virtual f32 GetAverageFrameTime() = 0;

	// Description:
	//     Set custom instance material for this character.
	// Arguments:
	//     pMaterial - A valid pointer to the material.
	virtual void SetMaterial( IMaterial *pMaterial ) = 0;

	// Description:
	//     Returns material used to render this character, can be either custom or model material.
	// Arguments:
	//     nType - Pass 0 to get the physic geometry or pass 1 to get the obstruct geometry
	// Return Value:
	//     A pointer to a phys_geometry class. 
	virtual IMaterial* GetMaterial() = 0;

	// Description:
	//     Returns the instance-specific material - if this is 0, then the default model material is in use.
	// Return Value:
	//     A pointer to the material, or 0.
	virtual IMaterial* GetMaterialOverride() = 0;


	//! Renderer calls this function to get the current skeleton-pose to deform the mesh right before the rendering
	virtual uint32 GetSkeletonPose(int nLod, int nList, const Matrix34& RenderMat34, QuatTS*& pBoneQuatsL, QuatTS*& pBoneQuatsS, QuatTS*& pMBBoneQuatsL, QuatTS*& pMBBoneQuatsS, Vec4 shapeDeformationData[], uint32 &DoWeNeedMorphtargets, uint8*& pRemapTable) = 0;


	virtual IRenderMesh* GetRenderMesh () = 0;

	//////////////////////////////////////////////////////////////////////////
	// Facial interface.
	//////////////////////////////////////////////////////////////////////////
	virtual IFacialInstance* GetFacialInstance() = 0;
	virtual void EnableFacialAnimation( bool bEnable ) = 0;
	virtual void EnableProceduralFacialAnimation( bool bEnable ) = 0;

	virtual int FindAffectedVertices(const Vec3 &vPos,float fRadius,std::vector<int> &lstIDs)=0;

	//////////////////////////////////////////////////////////////////////////
	// Lip sync character with the played sound.
	//////////////////////////////////////////////////////////////////////////
	virtual void LipSyncWithSound( uint32 nSoundId, bool bStop=false ) = 0;


	//! Set animations speed scale
	//! This is the scale factor that affects the animation speed of the character.
	//! All the animations are played with the constant real-time speed multiplied by this factor.
	//! So, 0 means still animations (stuck at some frame), 1 - normal, 2 - twice as fast, 0.5 - twice slower than normal.
	virtual void SetAnimationSpeed(f32 fSpeed) = 0;
	virtual f32 GetAnimationSpeed() = 0;
	virtual uint32 IsCharacterVisible() = 0;
	// Return true if animation graph for this character is still valid.
	virtual bool IsAnimationGraphValid() = 0;
	virtual void SetFPWeapon(f32 fp) = 0;
	virtual void ProcessSkinAttachment(const QuatT &rPhysLocationNext,const QuatTS &rAnimLocationNext, IAttachment* pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )=0;
	virtual f32* GetShapeDeformArray()=0;

	// Skeleton effects interface.
	virtual void SpawnSkeletonEffect(int animID, const char* animName, const char* effectName, const char* boneName, const Vec3& offset, const Vec3& dir, const Matrix34& entityTM) = 0;
	virtual void KillAllSkeletonEffects() = 0;

  virtual void SetUniformScale(const float& fUniformScale) = 0;
  virtual float GetUniformScale() const = 0;

  virtual bool GetIgnoreScaleForJoint(uint32 jointIndex) const = 0;
  virtual bool GetIsCustomScaleForJoint(uint32 jointIndex) const = 0;
  virtual float GetCustomScaleForJoint(uint32 jointIndex) const = 0;
  virtual void SetCustomScaleForJoint(uint32 jointIndex,bool bValue,float fValue) = 0;
	virtual void SetForceLOD(const bool bSet) = 0;
	virtual void SetForceLODNum(const int32 level) = 0;


	// functions for asyncronous execution of ProcessAnimationUpdate and ForwardKinematics

	// Makes sure all functions which must be called after forwardkinematics are executed and also syncronizes the execution
	// Is called during GetISkeletonAnim and GetISkeletonPose if the bNeedSync flag is set to true(the default) to ensure all computations have finished when
	// needed
	virtual void FinishAnimationComputations() = 0;

	// Set parameters with which SkeletonPostProcess should be called
	// NOTE: PostProcess is only called in case the parameters are set, if not the client is responsible to call PostProcess when needed after synchronizing the animation with FinishAnimationComputations()
	virtual void SetPostProcessParameter( const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext, IAttachment*	pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 nOnRender ) = 0;
};



//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

UNIQUE_IFACE struct ISkeletonAnim
{

	// Description:
	//     Enable special debug text for this skeleton
	virtual void SetDebugging( uint32 flags ) = 0;

	//motion initialization
	virtual void SetMirrorAnimation(uint32 ts)	=0;
	virtual void SetAnimationDrivenMotion(uint32 ts)	=0;
	virtual uint32 GetAnimationDrivenMotion() = 0;
	virtual void SetTrackViewExclusive(uint32 i) =0;
	virtual uint32 GetTrackViewStatus() =0;
	virtual void SetCharEditMode( uint32 m )=0; 


	//motion playback and blending
	virtual bool StartAnimation(const char* szAnimName0, const CryCharAnimationParams& Params)=0;
	virtual bool StartAnimationById(int32 id, const CryCharAnimationParams& Params)=0;
	virtual bool StopAnimationInLayer(int32 nLayer, f32 BlendOutTime)=0;
	virtual bool StopAnimationsAllLayers()=0;

	// PMG control parameter interface
	virtual void SetControlParam(const f32 param, const char* name, const int32 i) = 0;
	virtual f32 GetControlParam(const char* name) = 0;
	virtual void ResetControlParam() = 0 ;

	virtual size_t GetAnimationSize(const bool isCAF, const bool isLMG, const bool isPMG, const uint32 nGlobalAnimID) = 0;
	virtual uint32 GetToltalPosKeys(const bool isCAF, const uint32 nGlobalAnimID) const = 0;
	virtual uint32 GetToltalRotKeys(const bool isCAF, const uint32 nGlobalAnimID) const = 0;
	
	// Description
	//   Find an animation with a given user token.
	// Arguments:
	//   nUserToken - User token that identify an animation to search for.
	//   nLayer - Layer of FIFO where to search for animation, if -1 all layers are searched.
	virtual CAnimation* FindAnimInFIFO( uint32 nUserToken,int nLayer=1, bool bIncludeDeferredAnimation=false ) = 0;

	virtual bool RemoveAnimFromFIFO(uint32 nLayer, uint32 num) = 0;
	virtual int  GetNumAnimsInFIFO(uint32 nLayer)=0;
  virtual void ClearFIFOLayer(uint32 nLayer)=0;
	virtual CAnimation& GetAnimFromFIFO(uint32 nLayer, uint32 num )=0;
	// If manual update is set for anim, then set anim time and handle anim events - advance is used to tell whether we have moved forward or backward.
	virtual void ManualSeekAnimationInFIFO(uint32 nLayer, uint32 num, float time, bool advance) = 0;

	// [*DavidR | 22/Mar/2010] BEGIN: 
	// TEMPORARY METHODS UNTIL m_arrDeferredQueueUpdates GETS REMOVED
	virtual int  GetNumAnimsInFIFOAndDeferredQueue(uint32 nLayer)=0;
	virtual CAnimation& GetAnimFromFIFOAndDeferredQueue(uint32 nLayer, uint32 num)=0;
	// [*DavidR | 22/Mar/2010] END 

	virtual void SetLayerBlendMultiplier(int32 nLayer, f32 fMult) = 0;

	virtual void SetDesiredLocalLocation(const QuatT& desiredLocalLocation, float deltaTime, float frameTime, float turnSpeedMultiplier) =0;
	virtual void SetDesiredMotionParam(EMotionParamID id, f32 value, f32 frametime, bool initOnly = false) = 0; // Updates the given parameter (will perform clamping and clearing as needed)
	virtual float GetDesiredMotionParam(EMotionParamID id) const = 0; // What is the current value of a given param.
	virtual void SetBlendSpaceOverride(EMotionParamID id, float value, bool enable)=0; // Enable/Disable direct override of blendspace weights, used from CharacterEditor.


		//! This is the same as SetAnimationSpeed, but it sets the speed for layers
	//! NOTE: It does NOT override the overall animation speed, but it's multiplies it
	virtual void SetLayerUpdateMultiplier(int32 nLayer, f32 fSpeed)=0;
	virtual f32 GetLayerUpdateMultiplier(uint32 nLayer)=0;
	virtual void SetAdditiveWeight(int32 nLayer, f32 fWeight)=0;

	//! Set the current time of the given layer, in seconds
	virtual void SetLayerTime (uint32 nLayer, f32 fTimeSeconds) = 0;
	//! Return the current time of the given layer, in seconds
	virtual f32 GetLayerTime (uint32 nLayer) const = 0;

	//return values
	virtual Vec3 GetCurrentVelocity() =0; 
	virtual Vec3 GetCurrentAimDirection()  =0; 
	virtual Vec3 GetCurrentLookDirection() =0; 
	virtual f32  GetCurrentSlope() = 0;


	virtual void SetEventCallback(CallBackFuncType func, void *pdata) = 0;
	virtual AnimEventInstance GetLastAnimEvent() = 0;

	virtual void SetPreProcessCallback(CallBackFuncType func, void *pdata) = 0;

	virtual const QuatT& GetRelMovement() =0;

	virtual f32  GetUserData( int i ) = 0;

	// functions for asyncronous execution of ProcessAnimationUpdate and ForwardKinematics

	// Makes sure all functions which must be called after forwardkinematics are executed and also syncronizes the execution
	// Is called during GetISkeletonAnim and GetISkeletonPose if the bNeedSync flag is set to true(the default) to ensure all computations have finished when
	// needed
	virtual void FinishAnimationComputations() = 0;

	// Set parameters with which SkeletonPostProcess should be called
	// NOTE: PostProcess is only called in case the parameters are set, if not the client is responsible to call PostProcess when needed after synchronizing the animation with FinishAnimationComputations()
	virtual void SetPostProcessParameter( const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext, IAttachment*	pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 nOnRender ) = 0;

	virtual void PushLayer(IAnimationLayer* pLayer) = 0;
	virtual bool SetJointMask(const char* szBoneName, uint32 nLayerNo, uint8 nVal)=0; 
	virtual bool SetLayerMask(uint32 nLayerNo, uint8 nVal)=0;
};

struct IAnimationPoseBlenderAim;

UNIQUE_IFACE struct ISkeletonPose
{
	virtual void SetDefaultPose()=0;

	// Description:
	//     Return the number of joints included in the model. All joint-ids start from 0. The first joint, which has the id 0, is the root joint.
	// Return Value:
	//     An int value which hold the number of joints.
	// Summary:
	//     Get the number of joints
	virtual uint32 GetJointCount() const = 0;

	//! Returns the index of the joint in the set, -1 if there's no such joint
	// Summary:
	//   Searches for the index of an joint using its name.
	// Arguments:
	//   szJointName - Null terminated string holding the name of the joint.
	// Return Value:
	//   An integer representing the index of the joint. In case the joint-name couldn't be found, -1 will be returned.
	virtual int16 GetJointIDByName (const char* szJointName) const =0;

	//this should be in one single joint-structure
	virtual int16 GetParentIDByID (int32 ChildID) const =0;
	virtual uint32 GetJointCRC32 (int32 nJointID) const =0;
	virtual const char* GetJointNameByID(int32 nJointID) const =0;
	virtual const QuatT& GetAbsJointByID(int32 nJointID) =0;
	virtual void SetAbsJointByID(int32 nJointID, const QuatT& pose) =0;
	virtual const QuatT& GetRelJointByID(int32 nJointID) =0;
	virtual const QuatT& GetDefaultRelJointByID(int32 nJointID) =0;
	virtual const QuatT& GetDefaultAbsJointByID(int32 nJointID) =0;

  virtual void SetDefaultAbsInvJointByID(int32 nJointID, const QuatT& pose) =0;

	//motion post-process
	virtual void SetPostProcessCallback(int (*func)(ICharacterInstance*,void*), void *pdata) = 0;
	virtual void SetPostPhysicsCallback(int (*func)(ICharacterInstance*,void*), void *pdata) = 0;
	virtual void SetFootAnchoring(uint32 ts)	=0;

	virtual void SetLookIK(uint32 ik, f32 FOR, const Vec3& LookAtTarget,const f32 *customBlends=0, bool allowAdditionalTransforms=true, bool ignoreAimBlend=false) = 0;
	virtual uint8 IsLookIKEnabled() = 0;
	virtual uint8 GetLookIKInterpolatedTarget( Vec3& vTarget ) = 0;  // Returns false if look IK is disabled

	virtual void SetAimIK(bool bUseAimIK, const Vec3& AimAtTarget, uint32 arms=3, float blendTime = -1.0f ) = 0; 

	virtual void SetAimIKLayer(uint32 Layer) = 0;
	virtual void SetAimIKFadeOut(uint32 a) = 0;
	virtual void SetAimIKFadeOutSpeed(f32 time) = 0;
	virtual void SetAimIKFadeInSpeed(f32 time) = 0;
	virtual void SetAimIKTargetSmoothTime(f32 fSmoothTime)=0;
//	virtual void SetAimIKRefPose(const AimPoseDesc & poseDesc) = 0;
	virtual void SetAimIKPolarCoordinates(const Vec2& pc) = 0;
	virtual uint32 GetAimIKStatus() = 0;
	virtual f32 GetAimIKBlend() = 0;

	virtual void SetPoseBlenderAimState(bool bEnable) = 0;
	virtual void SetPoseBlenderAimLayer(uint32 layer) = 0;
	virtual IAnimationPoseBlenderAim* GetPoseBlenderAim() = 0;

	virtual void ApplyRecoilAnimation(f32 fDuration, f32 fKinematicImpact, uint32 arms=3 )=0; 
	virtual void SetWeaponRaisedPose(EWeaponRaisedPose pose) = 0;

	//functions to handle arm-IK for humans with different skeleton-hierarchies
	virtual uint32 SetCustomArmIK(const Vec3& wgoal,int32 idx0,int32 idx1,int32 idx2)=0;
	virtual uint32 SetHumanLimbIK(const Vec3& wgoal, const char* limb) = 0;

	virtual void EnableFootGroundAlignment(bool enable) = 0;

	virtual void CCDInitIKBuffer(QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK)=0;;
	virtual int32* CCDInitIKChain(int32 sCCDJoint,int32 eCCDJoint)=0;
	virtual void CCDRotationSolver( const Vec3& pos, f32 fThreshold,f32 StepSize, uint32 iTry,const Vec3& normal,QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK,Quat* pQuats=0)=0;
	virtual void CCDTranslationSolver(const Vec3& EndPos,QuatT* g_RelativeQuatIK,QuatT* g_AbsoluteQuatIK)=0;
	virtual void CCDUpdateSkeleton(QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK)=0;

	virtual void SetPostProcessQuat(int32 idx, const QuatT& qt )=0; 






	virtual void SetSuperimposed(uint32 i) =0;
	virtual void SetForceSkeletonUpdate(int32 i) =0;
	virtual IStatObj* GetStatObjOnJoint(int32 nId) = 0;
	virtual void SetStatObjOnJoint(int32 nId, IStatObj* pStatObj) = 0;
	virtual IPhysicalEntity *GetPhysEntOnJoint(int32 nId) = 0;
	virtual void SetPhysEntOnJoint(int32 nId, IPhysicalEntity *pPhysEnt) = 0;
	virtual int GetPhysIdOnJoint(int32 nId) = 0;
	virtual void SetMaterialOnJoint(int32 nId, IMaterial* pMaterial) = 0;
	virtual IMaterial* GetMaterialOnJoint(int32 nId) = 0;


	//! Used by physics engine)
	virtual void BuildPhysicalEntity(IPhysicalEntity *pent,f32 mass,int surface_idx,f32 stiffness_scale=1.0f,int nLod=0,int partid0=0, const Matrix34& mtxloc=Matrix34(IDENTITY)) = 0;
	virtual IPhysicalEntity* CreateCharacterPhysics(IPhysicalEntity *pHost, f32 mass,int surface_idx,f32 stiffness_scale, int nLod=0,const Matrix34& mtxloc=Matrix34(IDENTITY)) = 0;
	virtual int CreateAuxilaryPhysics(IPhysicalEntity *pHost, const Matrix34 &mtx, int nLod=0) = 0;
	virtual IPhysicalEntity *GetCharacterPhysics() const = 0;
	virtual IPhysicalEntity *GetCharacterPhysics(const char *pRootBoneName) = 0;
	virtual IPhysicalEntity *GetCharacterPhysics(int iAuxPhys) = 0;
	virtual void SetCharacterPhysics(IPhysicalEntity *pent) = 0;
	virtual void SynchronizeWithPhysicalEntity(IPhysicalEntity *pent, const Vec3& posMaster=Vec3(ZERO),const Quat& qMaster=Quat(1,0,0,0)) = 0;
	virtual IPhysicalEntity* RelinquishCharacterPhysics(const Matrix34 &mtx, f32 stiffness=0.0f, bool bCopyJointVelocities=false) = 0;
	virtual void DestroyCharacterPhysics(int iMode=0) = 0;
	virtual bool AddImpact(int partid, Vec3 point,Vec3 impact) = 0;
	virtual int TranslatePartIdToDeadBody(int partid) = 0;
	virtual int GetAuxPhysicsBoneId(int iAuxPhys, int iBone=0) = 0;

	virtual void Fall() = 0;
	virtual void GoLimp() = 0;
	virtual void StandUp(const Matrix34 &mtx, bool b3DOF, IPhysicalEntity *&pNewPhysicalEntity, Matrix34 &mtxDelta) = 0;
	virtual bool SetFnPAnimGroup(const char *name) = 0;
	virtual bool SetFnPAnimGroup(int idx) = 0;

	virtual f32 Falling() const = 0;
	virtual f32 Lying() const = 0;
	virtual f32 StandingUp() const = 0;
	virtual int GetFallingDir() /*const*/ = 0;

	//! Sets the current pose as the one dead body will move non-physicalized parts to
	virtual void SetRagdollDefaultPose() = 0;

	//need to attachmentmanager
	virtual int getBonePhysParentOrSelfIndex (int nBoneIndex, int nLod=0) = 0;
	
	virtual int GetBoneSurfaceTypeId(int nBoneIndex, int nLod=0) = 0;
	virtual void DrawSkeleton( const Matrix34& rRenderMat34, uint32 shift=0 ) = 0;
	virtual void SetGroundAlignmentData(const bool bAlignSkeletonVertical, const f32 rootHeight, const Plane& planeLeft, const Plane& planeRight) = 0;
};


//-----------------------------------------------------------------
//-----------------------------------------------------------------
//-----------------------------------------------------------------

UNIQUE_IFACE struct IMorphing
{
  //! returns morphs count
  virtual size_t GetNumMorphs() const = 0;
  //! returns morph name by index
  virtual const char* GetMorphName(size_t index) const = 0;
	//! Start the specified by parameters morph target
	virtual void StartMorph (const char* szMorphTarget, const CryCharMorphParams& params)=0;
	//! Set morph speed scale
	//! Finds the morph target with the given id, sets its morphing speed and returns true;
	//! if there's no such morph target currently playing, returns false
	virtual bool SetMorphSpeed(const char* szMorphTarget, f32 fSpeed) =0;
	//! Stops morph by target id
	virtual bool StopMorph(const char* szMorphTarget)=0;
	//! Stops all morphs
	virtual void StopAllMorphs()=0;
	//! freezes all currently playing morphs at the point they're at
	virtual void FreezeAllMorphs()=0;
	//! play all morphs in a sequence
	virtual void SetLinearMorphSequence(f32 i) =0;
  //! instant morph applying (by default is false)
  virtual void SetInstantMorphsApply(bool value) =0;
};


//DOC-IGNORE-BEGIN
//! This interface is free from concepts specific for CryEngine
//DOC-IGNORE-END

// Description:
//     This interface hold a set of animation in which each animation is described as properties.
// Summary:
//     Hold description of a set of animation
struct IAnimationSet
{

	//! Returns the index of the morph target in the set, -1 if there's no such morph target
	// Summary:
	//   Searches for morph target using a specified name.
	// Arguments:
	//   szMorphTarget - Name of the morph target to find.
	// Return Value:
	//   An integer representing the index of the morph target. The value -1 will 
	//   be returned in case that an appropriate morph target haven't been found.
	virtual int FindMorphTarget (const char* szMorphTarget) {return -1;}

	//! Returns the name of the morph target
	// Summary:
	//   Gets the name of a morph target.
	// Arguments:
	//   nMorphTargetId - Id of the morph target.
	// Return Value:
	//   A null terminated string holding the name of the morph target. In case 
	//   the specified id is out of range, the string 
	//   "!MORPH TARGET ID OUT OF RANGE!" is returned.
	virtual const char* GetNameMorphTarget (int nMorphTargetId) =0;

	//! Returns the number of morph targets in the set
	// Summary:
	//   Retrieves the amount of morph target.
	// Return Value: 
	//     An integer holding the amount of morth target
	virtual uint32 numMorphTargets() const =0;



	// Summary:
	//   Retrieves the amount of animation.
	// Return Value: 
	//     An integer holding the amount of animation
	virtual uint32 GetAnimationCount() const = 0;

	//! Returns the index of the animation in the set, -1 if there's no such animation
	// Summary:
	//   Searches for the index of an animation using its name.
	// Arguments:
	//   szAnimationName - Null terminated string holding the name of the animation.
	// Return Value:
	//   An integer representing the index of the animation. In case the animation 
	//   couldn't be found, -1 will be returned.
	virtual int GetAnimIDByName (const char* szAnimationName) = 0;

	//! Returns the given animation name
	// Summary:
	//   Gets the name of the specified animation.
	// Arguments:
	//   nAnimationId - Id of an animation.
	// Return Value:
	//   A null terminated string holding the name of the animation. In case the 
	//   animation wasn't found, the string "!NEGATIVE ANIMATION ID!" will be 
	//   returned.
	virtual const char* GetNameByAnimID(int nAnimationId) = 0;

	//get file-path either by ID or by animation-name
	virtual const char* GetFilePathByName (const char* szAnimationName) = 0;
	virtual const char* GetFilePathByID(int nAnimationId) = 0;

	virtual int32 GetGlobalIDByName(const char* szAnimationName) = 0;
	virtual int32 GetGlobalIDByAnimID(int nAnimationId) = 0;

	//! Returns the given animation's start, in seconds; 0 if the id is invalid
	virtual f32 GetStart (int nAnimationId) {return 0;} // default implementation
	f32 GetStart (const char* szAnimationName) {	return GetStart (GetAnimIDByName(szAnimationName));}

	virtual f32 GetSpeed(int nAnimationId) = 0;
	virtual f32 GetSlope(int nAnimationId) = 0;
	virtual Vec2 GetMinMaxSpeedAsset_msec(int32 animID )=0;
	virtual LMGCapabilities GetLMGPropertiesByName( const char* animname, Vec2& vStrafeDirection, f32 fDesiredTurn, f32 fSlope ) =0;
	virtual f32  GetIWeightForSpeed(int nAnimationId, f32 Speed) = 0;

	virtual f32 GetDuration_sec(int nAnimationId) = 0;
	virtual uint32 GetAnimationFlags(int nAnimationId) = 0;
	virtual uint32 GetBlendSpaceCode(int nAnimationId) = 0;
	virtual CryAnimationPath GetAnimationPath(const char* szAnimationName) = 0;
	virtual const QuatT& GetAnimationStartLocation(const char* szAnimationName) = 0;
	virtual const SAnimationSelectionProperties* GetAnimationSelectionProperties(const char* szAnimationName) = 0;

	virtual const QuatT& GetJointStartLocation(const char* szAnimationName, int32 jointCRC32) = 0;

	virtual f32 GetClosestQuatInChannel(const char* szAnimationName,int32 JointID, const Quat& q) = 0;

	virtual int ReloadCAF(const char * szFileName, bool bFullPath) = 0;
	virtual int RenewCAF(const char * szFileName, bool bFullPath) = 0;

	// Facial animation information
	virtual const char* GetFacialAnimationPathByName(const char* szName) = 0; // Returns 0 if name not found.
	virtual int GetNumFacialAnimations() = 0;
	virtual const char* GetFacialAnimationName(int index) = 0; // Returns 0 on invalid index.
	virtual const char* GetFacialAnimationPath(int index) = 0; // Returns 0 on invalid index.

	virtual const char *GetFnPAnimGroupName(int idx) = 0;

	virtual bool IsInPMGParamNameMap(const uint32 globalAnimID, const char* name) const = 0;
	virtual int LoadLMG(const char * szFileName, const char * szAnimName) = 0;
	virtual int LoadLMGFromMemory(const char* resourceName, const char* szAnimName, const char* xmlData) = 0;
	virtual LmgAnimationStatus IsAnimationValidForLMG(const char* lmgBlendCode, const char* szAnimationName) = 0;

	virtual void GetPMGParamLimits(const uint32 globalAnimID, const uint32 paramInd, f32& minV, f32& maxV) const = 0;
	virtual uint32 GetPMGParamDim(const uint32 globalAnimID) const = 0;
	virtual string GetPMGParamName(const uint32 globalAnimID, const uint32 paramInd) const = 0;
	virtual void SetDummyPMGAnimName(const uint32 globalAnimID, const char* animNameA, const char* animNameB) = 0;
};


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

struct SAnimationStatistics {
	const char * name;
	long count;
};

UNIQUE_IFACE struct IAnimEvents
{
	//! Returns the number of installed anim-events for this asset
	virtual int32 GetAnimEventsCount_CAF(int nGlobalID ) const =0;
	virtual int32 GetAnimEventsCount_CAF(const char* pFilePath ) =0;
	virtual void AddAnimEvent_CAF(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, f32 fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel) = 0;
	virtual void AddAnimEvent_LMG(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, f32 fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel) = 0;
	virtual void AddAnimEvent_PMG(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, f32 fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel) = 0;

	virtual int GetGlobalAnimID_CAF(const char* pFilePath) = 0;
	virtual int GetGlobalAnimID_LMG(const char* pFilePath) = 0;
	virtual int GetGlobalAnimID_PMG(const char* pFilePath) = 0;
	virtual void DeleteAllEventsForCAF(int nGlobalID) = 0;
	virtual void DeleteAllEventsForLMG(int nGlobalID) = 0;
	virtual void DeleteAllEventsForPMG(int nGlobalID) = 0;

	virtual size_t GetGlobalAnimCount() = 0;

};


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
enum EVisibilityMask
{
  VISIBLE_THIRD_PERSON = 0x01,              // Visible in third person view on the character.
  VISIBLE_FIRST_PERSON = 0x02,              // Visible in first person view on the character.
  VISIBLE_HIGHQUALITY_THIRD_PERSON = 0x04,  // Visible when first person view of high quality third person parts is enabled.
  VISIBLE_SHADOW_GENERATION = 0x08,         // Visible in shadow.

  // NOTE: add string compare to CCharacterPartsManager::LoadPart() if you add something here
};
//---------------------------------------------------------------------------
UNIQUE_IFACE struct IAttachmentManager
{
	virtual uint32 LoadAttachmentList(const char* pathname ) = 0;
	virtual uint32 SaveAttachmentList(const char* pathname ) = 0;

	virtual IAttachment* CreateAttachment( const char* szName, uint32 type, const char* szBoneName=0,bool bCallProject=true,bool bAllowDuplicates=false, bool bNoDefaultPose = false ) =0;
	virtual int32 RemoveAttachmentByInterface( const IAttachment* ptr  ) =0;
	virtual int32 RemoveAttachmentByName( const char* szName ) =0;

	virtual IAttachment* GetInterfaceByName( const char* szName ) const =0;
	virtual IAttachment* GetInterfaceByIndex( uint32 c) const =0;

	virtual int32 GetAttachmentCount() const =0;
	virtual int32 GetIndexByName( const char* szName ) const =0;

	virtual uint32 ProjectAllAttachment()=0;

	virtual void PhysicalizeAttachment( int idx, IPhysicalEntity *pent=0, int nLod=0 )=0;
	virtual void DephysicalizeAttachment( int idx, IPhysicalEntity *pent=0 )=0;

  // See EVisibilityMask for possible mask bits
  virtual void ApplyVisibilityMask(int mask) = 0;
  virtual void BakeMaterials() = 0;

  virtual ICharacterInstance* GetSkinInstance() const = 0; 
  virtual ICharacterInstance* GetSkelInstance() const = 0;
};


UNIQUE_IFACE struct IAttachment
{
	virtual const char* GetName() const=0;   
	virtual uint32 ReName( const char* szBoneName ) =0;   

	virtual uint32 GetType() const =0; 
	virtual uint32 SetType(uint32 type, const char* szBoneName=0)=0;

	virtual uint32 GetFlags()=0;
	virtual void SetFlags(uint32 flags)=0;

	//attachment location in default model-pose
	virtual const QuatT& GetAttAbsoluteDefault() const =0; 
	virtual void SetAttAbsoluteDefault(const QuatT& rot)=0; 

	//attachment location relative to the attachment point (bone,face). Similar to an additional rotation 
	//its is the location in the default-pose 
	virtual void SetAttRelativeDefault(const QuatT& mat)=0; 
	virtual const QuatT& GetAttRelativeDefault() const =0; 

	//its is the location of the attachment in the animated-pose is model-space 
	virtual const QuatT& GetAttModelRelative() const=0; 
	//its is the location of the attachment in the animated-pose in world-space 
	virtual const QuatT& GetAttWorldAbsolute() const=0; 


	virtual uint32 ProjectAttachment()=0;

	virtual void HideAttachment( uint32 x )=0;
	virtual uint32 IsAttachmentHidden()=0;
	virtual void HideInRecursion( uint32 x )=0;
	virtual uint32 IsAttachmentHiddenInRecursion()=0;
	virtual void HideInShadow( uint32 x )=0;
	virtual uint32 IsAttachmentHiddenInShadow()=0;

	virtual void AlignBoneAttachment( uint32 x )=0;

	virtual uint32 GetBoneID() const =0; 

	virtual uint32 AddBinding( IAttachmentObject* pModel) =0;   
	virtual IAttachmentObject* GetIAttachmentObject() const=0; 
	virtual void ClearBinding() =0;   

	virtual void GetHingeParams(int &idx,f32 &limit,f32 &damping) = 0;
	virtual void SetHingeParams(int idx=-1,f32 limit=120.0f,f32 damping=2.0f) = 0;

  virtual void SetFaceNr(uint32 faceNr) = 0;
};



// Description:
//     This interface define a way to allow an object to be bound to a character.
struct IAttachmentObject
{
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const {/*REPLACE LATER*/}

	enum EType
	{
		eAttachment_Unknown,
		eAttachment_StatObj,
		eAttachment_Character,
		eAttachment_Entity,
		eAttachment_Light,
		eAttachment_Effect,
	};
	virtual EType GetAttachmentType() = 0;

	virtual void UpdateAttachment( IAttachment* pIAttachment, const QuatT& mtx, float fZoomAdjustedDistanceFromCamera, uint32 OnRender ) =0;
	virtual void RenderAttachment( SRendParams& rParams, IAttachment *pAttachment) {};
  // returns handled state
  virtual bool PhysicalizeAttachment( IAttachmentManager* pManager, int idx, int nLod, IPhysicalEntity *pent, const Vec3 &offset ) { return false; }

  virtual bool UpdatePhysicalizedAttachment( IAttachmentManager* pManager, int idx, IPhysicalEntity *pent, const QuatT &offset) { return false; }

	virtual bool StartAnimation (const char* szAnimName, const struct CryCharAnimationParams& Params) { return 0; }
	virtual bool StopAnimationInLayer (int nLayer) { return 0; }

  virtual AABB GetAABB()=0;

	virtual IStatObj* GetIStatObj() { return 0; }
	SPU_INDIRECT(CommandBufferExecute(M))
	virtual ICharacterInstance* GetICharacterInstance() const { return 0; }

	virtual IMaterial *GetMaterial() = 0;
	virtual void SetMaterial(IMaterial *pMaterial) = 0;
	virtual IMaterial* GetMaterialOverride() = 0;

	virtual void Release() = 0;

  virtual void OnRemoveAttachment(IAttachmentManager* pManager, int idx){}

};




struct CCGFAttachment : public IAttachmentObject
{
	virtual EType GetAttachmentType() { return eAttachment_StatObj; };
	void UpdateAttachment(IAttachment *pIAttachment, const QuatT &m, float fZoomAdjustedDistanceFromCamera, uint32 OnRender) {}
	void RenderAttachment(SRendParams &rParams, IAttachment *pAttachment )
  {
    m_lastRenderMatrix = *rParams.pMatrix;
		IMaterial *pPrev = rParams.pMaterial;
		if (pMaterial)
			rParams.pMaterial = pMaterial;

		pObj->Render(rParams);
		rParams.pMaterial = pPrev;
	};

	AABB GetAABB() { return pObj->GetAABB(); };
	IStatObj* GetIStatObj() {	return pObj;	}
	void Release() { delete this;	}
	IMaterial *GetMaterial() { return pMaterial? (IMaterial*)pMaterial : pObj->GetMaterial();};
	void SetMaterial(IMaterial *pMaterial) { this->pMaterial = pMaterial; };
	IMaterial* GetMaterialOverride() { return pMaterial; }
	_smart_ptr<IStatObj> pObj;
	_smart_ptr<IMaterial> pMaterial;
  Matrix34 m_lastRenderMatrix;
};


struct CCHRAttachment : public IAttachmentObject
{
  CCHRAttachment()
  {
  }

  virtual EType GetAttachmentType() { return eAttachment_Character; };
  void UpdateAttachment( IAttachment *pAttachment, const QuatT& rWorldLocation, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )	
  {
    uint32 type = pAttachment->GetType();	
		// here PostProcess must be called directly because it invokes the ProcessAnimations function itself, if SetPostProcessParameters
		// would be used, the PostProcess function wouldn be called since ProcessAnimation was never called which leads to missing
		// animations for attachments
		if (type==CA_FACE)
			m_pCharInstance->SkeletonPostProcess(rWorldLocation,rWorldLocation,pAttachment, fZoomAdjustedDistanceFromCamera, OnRender);
    else if (type==CA_BONE)
			m_pCharInstance->SkeletonPostProcess(rWorldLocation,rWorldLocation,pAttachment, fZoomAdjustedDistanceFromCamera, OnRender);
    else if (type==CA_SKIN || type==CA_PART)
      m_pCharInstance->ProcessSkinAttachment(rWorldLocation,rWorldLocation,pAttachment, fZoomAdjustedDistanceFromCamera, OnRender );
  };
  void RenderAttachment( SRendParams &rParams, IAttachment *pAttachment  )	
  {
    IMaterial *pPrev = rParams.pMaterial;
    if (m_pMaterial)
      rParams.pMaterial = m_pMaterial;
    m_pCharInstance->Render(rParams,QuatTS(IDENTITY));
    rParams.pMaterial = pPrev;
  };

	bool StartAnimation (const char* szAnimName, const struct CryCharAnimationParams& Params)	
	{
		return m_pCharInstance->GetISkeletonAnim()->StartAnimation(szAnimName,Params);
	}
	bool StopAnimationInLayer (int nLayer)	
	{ 
		return m_pCharInstance->GetISkeletonAnim()->StopAnimationInLayer(nLayer,0.0f);
	}

  AABB GetAABB() {	return m_pCharInstance ? m_pCharInstance->GetAABB() : AABB(AABB::RESET);	};
	SPU_INDIRECT(CommandBufferExecute(M))
  ICharacterInstance* GetICharacterInstance() const { return m_pCharInstance; }

  void Release() 
  { 
    if (m_pCharInstance) 
      m_pCharInstance->OnDetach(); 
    delete this;	
  }

	IMaterial *GetMaterial() { return m_pMaterial ? (IMaterial*)m_pMaterial : (m_pCharInstance?m_pCharInstance->GetMaterial():NULL); }
  void SetMaterial(IMaterial *pMaterial) { m_pMaterial = pMaterial; }
  IMaterial* GetMaterialOverride() { return m_pMaterial; }

  _smart_ptr<ICharacterInstance> m_pCharInstance;
  _smart_ptr<IMaterial> m_pMaterial;
  //
};

//-------------------------------------------------------------------------
UNIQUE_IFACE struct ICharacterPartAttachment : public CCHRAttachment
{
  // Visibility mask
  virtual uint32 GetVisibilityMask() const = 0;
  virtual void SetVisibilityMask(uint32 newVisMask) = 0;
  virtual void AddFaceAttachment(const char* partName,const char* name, QuatT attRelativeDefault, QuatT attAbsoluteDefault, uint32 faceNr) = 0;
};
struct CEntityAttachment :	public IAttachmentObject
{
public:
	CEntityAttachment():m_scale(1.0f,1.0f,1.0f),m_id(0){}
	
	virtual EType GetAttachmentType() { return eAttachment_Entity; };
	void SetEntityId(EntityId id) { m_id = id; };
	EntityId GetEntityId() { return m_id; }

	void UpdateAttachment(IAttachment *pIAttachment,const QuatT &m, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )
	{
		const QuatT& quatT = pIAttachment->GetAttWorldAbsolute();
		
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_id);

		if (pEntity)
		{
    //	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
		//	gEnv->pRenderer->Draw2dLabel(500.0f, 560.0f, 1.6f, white, false, "UpdateAttach transform: %.12f, %.12f, %.12f\n  rot: %.9f, %.9f, %.9f, %.9f", quatT.t.x, quatT.t.y, quatT.t.z, quatT.q.v.x, quatT.q.v.y, quatT.q.v.z, quatT.q.w);

			pEntity->SetPosRotScale( quatT.t, quatT.q, m_scale, ENTITY_XFORM_NO_PROPOGATE);
		}
	}

	AABB GetAABB()
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_id);
		AABB aabb(Vec3(0,0,0), Vec3(0,0,0));

		if (pEntity) pEntity->GetLocalBounds(aabb);
		return aabb;
	};

	void Release() { delete this;	};

	IMaterial *GetMaterial() { return 0; };
	void SetMaterial(IMaterial *pMaterial) {};
	IMaterial* GetMaterialOverride() { return 0; }

  void SetScale(const Vec3& scale)
  {
    m_scale = scale;
  }

private:
	EntityId	m_id;
  Vec3 m_scale;
};


struct CLightAttachment :	public IAttachmentObject
{
public:
	CLightAttachment(): m_pLightSource(0)	{};
	virtual ~CLightAttachment()
	{
		if (m_pLightSource)
		{
			gEnv->p3DEngine->UnRegisterEntity(m_pLightSource);
			m_pLightSource->ReleaseNode();
		}
	};

	virtual EType GetAttachmentType() { return eAttachment_Light; };

	void LoadLight(const CDLight &light)
	{
		m_pLightSource = gEnv->p3DEngine->CreateLightSource();
    if (m_pLightSource)
			m_pLightSource->SetLightProperties(light);
	}

  ILightSource* GetLightSource() { return m_pLightSource; }

	void UpdateAttachment(IAttachment *pAttachment, const QuatT &m, float fZoomAdjustedDistanceFromCamera, uint32 OnRender)	
	{
		if (m_pLightSource)
		{
			CDLight &light = m_pLightSource->GetLightProperties();

			Matrix34 worldMatrix = Matrix34(pAttachment->GetAttWorldAbsolute());
			Vec3 origin = worldMatrix.GetTranslation();
			//light.SetPosition( origin ); //Diesel cut
			light.SetMatrix(worldMatrix);
			light.m_sName = pAttachment->GetName();
			m_pLightSource->SetMatrix(worldMatrix);
			f32 r = light.m_fRadius;
			m_pLightSource->SetBBox(AABB(Vec3(origin.x-r, origin.y-r, origin.z-r), Vec3(origin.x+r,origin.y+r,origin.z+r)));
			gEnv->p3DEngine->RegisterEntity(m_pLightSource);
		}
	}

	AABB GetAABB()
	{
		f32 r = m_pLightSource->GetLightProperties().m_fRadius;
		return AABB(Vec3(-r, -r, -r), Vec3(+r, +r, +r));
	};

	void Release() { delete this; };

	IMaterial *GetMaterial() { return 0; };
	void SetMaterial(IMaterial *pMaterial) {};
	IMaterial* GetMaterialOverride() { return 0; }

private:
	ILightSource *m_pLightSource;
};



struct CEffectAttachment :	public IAttachmentObject
{
public:

	virtual EType GetAttachmentType() { return eAttachment_Effect; };

	CEffectAttachment(const char *effectName, const Vec3 &offset, const Vec3 &dir, f32 scale)
		:	m_offset(offset),
		m_dir(dir),
		m_scale(scale),
		m_pEmitter(0)
	{
		m_pEffect = gEnv->pParticleManager->FindEffect(effectName,"Character Attachment",true);
	}

	virtual ~CEffectAttachment()
	{
		if (m_pEmitter)
			m_pEmitter->Activate(false);
		m_pEmitter = 0;
		m_pEffect = 0;
	}

	void CreateEffect(const Matrix34A& a_baseMatrix)
	{
		if (!m_pEmitter && m_pEffect != 0)
		{
			if (m_dir.len2()>0)
				m_loc = Matrix34(Matrix33::CreateRotationVDir(m_dir));
			else
				m_loc.SetIdentity();
			m_loc.AddTranslation(m_offset);
			m_loc.Scale(Vec3(m_scale));

			const Matrix34 spawnMatrix = a_baseMatrix * m_loc;
			m_pEmitter = m_pEffect->Spawn(false, spawnMatrix);
		}
	}

	IParticleEmitter *GetEmitter()
	{
		return m_pEmitter;
	}

	void UpdateAttachment(IAttachment *pIAttachment,const QuatT &m, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )
	{
		if (!pIAttachment->IsAttachmentHidden())
		{
			if (!m_pEmitter)
			{
				CreateEffect(Matrix34A(pIAttachment->GetAttWorldAbsolute()));
			}

			if (m_pEmitter)
				m_pEmitter->SetMatrix( Matrix34A(pIAttachment->GetAttWorldAbsolute()) * m_loc);
		}
		else
		{
			m_pEmitter = 0;
		}
	}

	AABB GetAABB()
	{
		if (m_pEmitter)
		{
			AABB bb;
			m_pEmitter->GetLocalBounds(bb);
			return bb;
		}
		else
		{
			return AABB(Vec3(-0.1f), Vec3(0.1f));
		}
	};

	void Release() { delete this; };

	IMaterial *GetMaterial() { return 0; };
	void SetMaterial(IMaterial *pMaterial) {};
	IMaterial* GetMaterialOverride() { return 0; }

	void SetSpawnParams( const SpawnParams& params, GeomRef geom=GeomRef() )
	{  
		if (m_pEmitter)
			m_pEmitter->SetSpawnParams(params, geom);
	}

private:
	_smart_ptr<IParticleEmitter>	m_pEmitter;
	_smart_ptr<IParticleEffect>		m_pEffect;
	Vec3							m_offset;
	Vec3							m_dir;
	f32							m_scale;
	Matrix34					m_loc;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

	// Description:
	//		Experimental way to track interface version.
	//		This value will be compared with value passed from system module
	const char gAnimInterfaceVersion[64] = __TIMESTAMP__;

	// Description:
	//		CreateCryAnimation function type definition.
	typedef ICharacterManager * (*PFNCREATECRYANIMATION)(ISystem	* pSystem,const char * szInterfaceVersion);

	// Description:
	//     Create an instance of the Animation System. It should usually be called 
	//     by ISystem::InitAnimationSystem().
	// See Also:
	//     ICharacterManager, ICharacterManager::Release
	// Arguments:
	//     ISystem            - Pointer to the current ISystem instance
	//     szInterfaceVersion - String version of with the build date
	// Summary:
	//     Create an instance of the Animation System
	CRYANIMATION_API ICharacterManager * CreateCharManager(ISystem* pSystem, const char * szInterfaceVersion=gAnimInterfaceVersion);

#ifdef __cplusplus
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ICryAnimation2.h"

#endif // ICRY_ANIMATION
