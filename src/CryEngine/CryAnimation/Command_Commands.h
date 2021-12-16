#ifndef Command_Commands_h
#define Command_Commands_h

namespace Command {

class CState;

enum 
{
	eClearFull = 0,
	eClearSingle,

	eSampleAddAnimFull,
	eSampleAnimPart,
	eSampleFeatherAnimPart,

	eSamplePosePart,
	eSampleAddPoseFull,

	eCopyAddAnimFull,

	eScaleUniformFull,

	eNormalizeFull,

	eJointMask,

	ePoseModifier,

	eVerifyFull,
};

struct ClearFull
{
	enum { ID = eClearFull };

	uint8 m_nCommand;
	uint8 m_TargetBuffer;
	uint8 m_pad1;
	uint8 m_pad2;

	void Execute(const CState& state, void* buffers[]) const;
};

struct ClearSingle
{
	enum { ID = eClearSingle };

	uint8 m_nCommand;
	uint8 m_TargetBuffer;
	uint8 m_pad1;
	uint8 m_pad2;

	void Execute(const CState& state, void* buffers[]) const;
};

struct SampleAddAnimFull
{
	enum { ID = eSampleAddAnimFull };

	enum 
	{
		Flag_ADMotion     = 1,
		Flag_AnimEOC      = 2,
		Flag_IsPMG        = 4,
		Flag_TmpBuffer    = 8,
		Flag_free0        =16,
		Flag_free1        =32,
		Flag_AimCounter0  =64,
		Flag_AimCounter1  =128
	};

	uint8 m_nCommand;
	uint8 m_flags;
	int16 m_nEAnimID;
	f32 m_fETimeNew; //this is a percentage value between 0-1
	f32 m_fETimeOld; //this is a percentage value between 0-1
	f32 m_fWeight;

	void Execute(const CState& state, void* buffers[]) const;
};

struct SampleAnimPart
{
	enum { ID = eSampleAnimPart };

	uint8 m_nCommand;
	uint8 m_TargetBuffer;
	uint8 m_nFeatherBlend;	
	uint8 m_nMask; //temporary	

	int32 m_nEAnimID;

	f32 m_fAnimTime; //this is a percentage value between 0-1
	f32 m_fWeight;

	void Execute(const CState& state, void* buffers[]) const;
};

struct SampleFeatherAnimPart : public SampleAnimPart
{
	enum { ID = eSampleFeatherAnimPart };

	uint8 m_arrFeatherMask[MAX_JOINT_AMOUNT];
};

struct SamplePosePart
{
	enum { ID = eSamplePosePart };

	uint8 m_nCommand;
	uint8 m_TargetBuffer;
	uint8 m_nPAD1;	
	uint8 m_nPAD2; //temporary	

	int32 m_nEAnimID;

	f32 m_fAnimTime; //this is a percentage value between 0-1
	f32 m_fWeight;

	void Execute(const CState& state, void* buffers[]) const;
};

struct SampleAddPoseFull
{
	enum { ID = eSampleAddPoseFull };
	uint8 m_nCommand;
	uint8 m_flags;
	int16 m_nEAnimID;
	f32 m_fETimeNew; //this is a percentage value between 0-1
	f32 m_fWeight;

	void Execute(const CState& state, void* buffers[]) const;
};

struct CopyAddAnimFull
{
	enum { ID = eCopyAddAnimFull };

	uint8 m_nCommand;
	uint8 m_SourceBuffer;
	uint8 m_TargetBuffer;
	uint8 m_IsPMG; //temporary
	f32 m_fWeight;

	void Execute(const CState& state, void* buffers[]) const;
};

struct ScaleUniformFull
{
	enum { ID = eScaleUniformFull };

	uint8 m_nCommand;
	uint8 m_TargetBuffer;
	uint8 m_pad1;
	uint8 m_pad2;
	f32 m_fScale;

	void Execute(const CState& state, void* buffers[]) const;
};

struct NormalizeFull
{
	enum { ID = eNormalizeFull };

	uint8 m_nCommand;
	uint8 m_TargetBuffer;
	uint8 m_pad1;
	uint8 m_pad2;
	f32 m_fRootScale;

	void Execute(const CState& state, void* buffers[]) const;
};

struct PoseModifier
{
	enum { ID = ePoseModifier };

	uint8 m_nCommand;
	uint8 m_TargetBuffer;
	uint8 m_PAD0;
	uint8 m_PAD1;
	IAnimationPoseModifier* m_pPoseModifier;

	// TODO: This should be stored globally.
	QuatT m_PhysLocation;
	QuatT m_AnimLocation;

	void Execute(const CState& state, void* buffers[]) const;
};

struct JointMask
{
	enum { ID = eJointMask };

	uint8 m_nCommand;
	uint8 m_count;
	uint8 m_PAD0;
	uint8 m_PAD1;
	uint32* m_pMask;

	void Execute(const CState& state, void* buffers[]) const;
};

struct VerifyFull
{
	enum { ID = eVerifyFull };

	uint8 m_nCommand;
	uint8 m_TargetBuffer;
	uint8 m_pad1;
	uint8 m_pad2;

	void Execute(const CState& state, void* buffers[]) const;
};

} // namespace Command

#endif // Command_Commands_h
