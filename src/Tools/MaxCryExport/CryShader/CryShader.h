////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   cryshader.h
//  Version:     v1.00
//  Created:     6/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __cryshader_h__
#define __cryshader_h__
#pragma once

#include "shaders.h"

#define CrytekShaderClassID Class_ID(0x6c0b48e3, 0x2f836597)

// paramblock2 block and parameter IDs for the standard shaders
// NB these are duplicated in mtls/stdmtl2.h ...
enum { shdr_params, };

	// shdr_params param IDs
enum 
{ 
	shdr_ambient,
	shdr_diffuse,
	shdr_specular,
	shdr_ad_texlock,
	shdr_ad_lock,
	shdr_ds_lock, 
	shdr_self_illum_amnt,
	shdr_spec_lvl,
	shdr_glossiness,
	shdr_alpha_test,
	shdr_additive,
	shdr_shader_name,
	shdr_surface_name,
	shdr_physicalize,
	shdr_additive_decal,
	shdr_alpha_blend,
	shdr_no_shadow,
  shdr_use_glossiness,
	shdr_compute_sh,
	shdr_sh_opacity,
	shdr_sh_2sided,
	shdr_sh_ambient,
};

#define N_CRYSHADER_CHANNELS 12

// channels ids used by shader
enum 
{
	CH_DI = 0,// Diffuse Color.
	CH_SP,	// Specular Color.
	CH_GL,	// Glosiness.
	CH_OP,	// Opacity
	CH_DT,	// Detail.
	CH_BN,	// Bump Normal Map.
	CH_PR,	// Procedural Map.
	CH_DC,	// Decal Map.
	CH_SSS,	// Subsurface Scattering Map.
	CH_RESERVED2,	// Reserved Map2.
	CH_RESERVED3,	// Reserved Map3.
	CH_RESERVED4,	// Reserved Map4.

	CH_BU,	// Bump map.
	CH_RL,	// Reflection map.
	CH_RR,	// Reflection map.
	CH_DP,	// Displacment map.

	CH_AM = 20,	// Ambient.
	CH_SI,			// Self illumination.
	CH_SS				// Specular level
};

//////////////////////////////////////////////////////////////////////////
struct ShaderInfo
{
	//! Name of shader.
	CStr name;
	//! Description of shader.
	CStr desc;
	//! Shader parameters.
	// needed channels flags.
	int usedchannels;

	ShaderInfo() { usedchannels = 0; };
};

class CrytekShaderDlg;


class CrytekShader : public Shader
{
friend class CrytekShaderDlg;
public:
	IParamBlock2* pblock;   // ref 0
	Interval ivalid;
	TimeValue	curTime;

	CrytekShaderDlg* paramDlg;

	BOOL selfIllumClrOn;
	BOOL lockDS;
	BOOL lockAD;
	BOOL lockADTex;

	Color ambient;
	Color diffuse;
	Color specular;
	Color selfIllumClr;
	float selfIllum;	
	float softThresh;
	float glossiness;
	float specularLevel;
	float	shOpacity;

	//////////////////////////////////////////////////////////////////////////
	bool m_bAdditiveDecal;
  bool m_bUseGlossiness;
	bool m_bDiffuseAlpha; // diffuse have alpha.
	Mtl *pMtl;
	//////////////////////////////////////////////////////////////////////////


public:
	CrytekShader();

	Class_ID 	ClassID() { return CrytekShaderClassID; }
	TSTR GetName() { return _T("Crytek Shader"); }
	void Illum(ShadeContext &sc, IllumParams &ip);
	void AffectReflection(ShadeContext &sc, IllumParams &ip, Color &rcol)
	{ 
		rcol *= ip.channels[CH_SP];
	};
	float EvalHiliteCurve(float x)
	{
		double phExp = pow(2.0, glossiness * 10.0); 
		return specularLevel*(float)pow((double)cos(x*PI), phExp );  
	}
	RefTargetHandle Clone( RemapDir &remap )
	{
		CrytekShader* s = new CrytekShader(); 
		CrytekShader::Clone(remap, s); 
		BaseClone(this, s, remap);
		return s;	
	}
	ClassDesc2* GetCD();

	void ConvertParamBlk( ParamBlockDescID *descOld, int oldCount, IParamBlock *oldPB );
	void CopyStdParams( Shader* pFrom );

	ShaderParamDlg* GetParamDlg(int)
	{
		return (ShaderParamDlg*)paramDlg;
	}
	void SetParamDlg( ShaderParamDlg* newDlg, int)
	{
		paramDlg = (CrytekShaderDlg*)newDlg;
	}

	void CombineComponents( ShadeContext &sc, IllumParams& ip );

	virtual ULONG SupportStdParams();

	// texture maps
	virtual long  nTexChannelsSupported();
	virtual TSTR  GetTexChannelName( long nTex );
	virtual TSTR  GetTexChannelInternalName( long nTex );
	virtual long  ChannelType( long nTex );
	// map StdMat Channel ID's to the channel number
	virtual long StdIDToChannel( long stdID );

	BOOL KeyAtTime(int id,TimeValue t);
	void DeleteThis(){ delete this; }		
	ULONG GetRequirements( int subMtlNum );

	ShaderParamDlg* CreateParamDialog(HWND hOldRollup, HWND hwMtlEdit, IMtlParams *imp, StdMat2* theMtl, int rollupOpen, int );
	void Update(TimeValue t, Interval& valid);
	void Reset();
	void NotifyChanged();

	SClass_ID SuperClassID() { return SHADER_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetName(); }  

	int NumSubs() { return 1; }  
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	int SubNumToRefNum(int subNum) { return subNum;	}
 	// JBW: add direct ParamBlock access
	int	NumParamBlocks() { return 1; }
	IParamBlock2* GetParamBlock(int i) { return pblock; }
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; }

	// From ref
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i);
	void SetReference(int i, RefTargetHandle rtarg);

	RefTargetHandle Clone( RemapDir &remap=MAX_DEFAULT_REMAP(), CrytekShader* mnew=NULL );
	RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
		PartID& partID, RefMessage message );

	// IO
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// cache for mapping of params, mtl fills in ip
	void GetIllumParams( ShadeContext &sc, IllumParams& ip );

	// Shader specific section
	// these shd always be overridden

	void SetLockDS(BOOL lock){ lockDS = lock; pblock->SetValue( shdr_ds_lock, 0, lock); }
	BOOL GetLockDS(){ return lockDS; }
	void SetLockAD(BOOL lock){ lockAD = lock; pblock->SetValue( shdr_ad_lock, 0, lock); }
	BOOL GetLockAD(){ return lockAD; }
	void SetLockADTex(BOOL lock){ lockADTex = lock; pblock->SetValue( shdr_ad_texlock, 0, lock); }
	BOOL GetLockADTex(){ return lockADTex; }

	void SetSelfIllum(float v, TimeValue t);		
	void SetSelfIllumClrOn( BOOL on ) {};
	BOOL IsSelfIllumClrOn() { return FALSE; };
	void SetSelfIllumClr(Color c, TimeValue t);

	void SetAmbientClr(Color c, TimeValue t);		
	void SetDiffuseClr(Color c, TimeValue t);		
	void SetSpecularClr(Color c, TimeValue t);
	void SetGlossiness(float v, TimeValue t);		
	void SetSpecularLevel(float v, TimeValue t);		
	void SetSHOpacityLevel(float v, TimeValue t);		
	void SetSoftenLevel(float v, TimeValue t);

	BOOL IsSelfIllumClrOn(int mtlNum, BOOL backFace){ return selfIllumClrOn; };
	Color GetAmbientClr(int mtlNum, BOOL backFace){ return ambient;}		
	Color GetDiffuseClr(int mtlNum, BOOL backFace){ return diffuse;}		
	Color GetSpecularClr(int mtlNum, BOOL backFace){ return specular; };
	Color GetSelfIllumClr(int mtlNum, BOOL backFace){ return selfIllumClr; };
	float GetSelfIllum(int mtlNum, BOOL backFace){ return selfIllum; };
	float GetGlossiness(int mtlNum, BOOL backFace){ return glossiness; };	
	float GetSpecularLevel(int mtlNum, BOOL backFace){ return specularLevel; };
	float GetSHOpacityLevel(int mtlNum, BOOL backFace){ return shOpacity; };
	float GetSoftenLevel(int mtlNum, BOOL backFace){ return softThresh; };

	Color GetAmbientClr(TimeValue t);	
	Color GetDiffuseClr(TimeValue t);
	Color GetSpecularClr(TimeValue t);
	float GetGlossiness( TimeValue t);	
	float GetSpecularLevel(TimeValue t);
	float GetSoftenLevel(TimeValue t);
	float GetSelfIllum(TimeValue t);	
	Color GetSelfIllumClr(TimeValue t);	

	bool AssignMaterial( StdMat2 *mtl );
	//////////////////////////////////////////////////////////////////////////
	// ShaderInfo
	//////////////////////////////////////////////////////////////////////////
	//! Find shader info by name.
	//! If name is NULL shader info of current shader is found.
	ShaderInfo* FindShaderInfo( const char *shaderName = NULL );

	// New parameters.
	const TCHAR* GetShaderName();
	const TCHAR* GetSurfaceName();
	void SetShaderName( TCHAR* sName );
	void SetSurfaceName( TCHAR* sName );
	bool IsAdditive();
  bool IsUseGlossiness();
	bool IsPhysicalize();
	void SetPhysicalize(bool isPhys);
	bool IsSH();
	bool IsSH2Sided();
	void SetIsSH(bool val);
	void SetHasAmbientSH(bool val);
	void SetIsSH2Sided(bool val);

	bool HasAmbientSH();
	float GetSHOpacity(TimeValue t);
	bool IsAdditiveDecal();
	float GetAlphaTest();
	void SetAlphaTest( float fAlphaTest );
	bool IsAlphaBlend();
	bool IsNoShadow();

private:
	void EnumShaders();
};

#endif // __cryshader_h__