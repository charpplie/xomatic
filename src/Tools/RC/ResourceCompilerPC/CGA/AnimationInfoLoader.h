////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   AnimationInfoLoader.h
//  Version:     v1.00
//  Created:     22/6/2006 by Alexey Medvedev.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _ANIMATION_INFO_LOADER
#define _ANIMATION_INFO_LOADER

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ICryXML;

class IAnimationLoaderListener
{
public:
	enum MessageType
	{
		MESSAGE_INFO,
		MESSAGE_WARNING,
		MESSAGE_ERROR
	};
	virtual void OnAnimationLoaderMessage(MessageType type, const char* szMessage) = 0;
};


inline void FixString(string& str)
{
	std::transform(str.begin(),str.end(),str.begin(),tolower);	
	std::replace(str.begin(),str.end(), '\\','/' );	
}

// Compression parameters for individual bone
struct CompressionPreset
{
	string m_Name;

	struct BonePreset
	{
		float m_fRotError;
		float m_fPosError;
		uint8 m_RotCompressionFormat;
		uint8 m_PosCompressionFormat;
	};

	typedef std::map<uint32,BonePreset> TBoneInfoMap;
	TBoneInfoMap m_BoneInfoMap;

	const BonePreset * FindBonePreset(uint32  bone)
	{
		TBoneInfoMap::iterator it = m_BoneInfoMap.find(bone);
		if (it != m_BoneInfoMap.end())
		{
			return &(it->second);
		}

		return 0;
	}
};


// Processing & compression parametrs for single animation
struct SAnimationDesc
{
	int			m_CompressionQuality;
	float		m_fRootQuality;
	int			m_SkipSaveToDatabase;
	int			m_AdditiveAnimation;
	float		m_fPOS_EPSILON;  //if the change less then 2cm per meter, then we ignore this animation
	float		m_fROT_EPSILON;

	CompressionPreset m_Preset;

	int		m_FootPlant;
	float m_fFloorDist;
	float m_fSpeedXY;

	void SetAnimationName(string name)
	{
		m_AnimString = name;
		FixString(m_AnimString);
	}

	SAnimationDesc()  
	{
		m_CompressionQuality	=0; 
		m_fRootQuality				=0.0f; 
		m_AdditiveAnimation		=0; 
		m_SkipSaveToDatabase	=0;
		m_FootPlant						=0; 
		m_fFloorDist					=0.03f; 
		m_fSpeedXY						=0.005f;
		m_fPOS_EPSILON				=0.02f;  //if the change less then 2cm per meter, then we ignore this animation
		m_fROT_EPSILON				=0.005f;
	};

	const string& GetAnimString() const { return m_AnimString;};
private:
	string m_AnimString;


};


// Container of parameters for animations for reference model
struct SAnimationDefinition 
{
	string m_Model;
	string m_DBName;	
	

	SAnimationDesc m_MainDesc;

	std::vector<SAnimationDesc> m_OverrideAnimations;
	const SAnimationDesc& GetAnimationDesc(const string& name ) const;

	void SetAnimationPath(string& name)
	{
		m_AnimationPath = name;
		FixString(m_AnimationPath);
		m_AnimationPathWithoutSlash = m_AnimationPath;


		while (m_AnimationPathWithoutSlash[m_AnimationPathWithoutSlash.length() - 1] == '/')
		{
			m_AnimationPathWithoutSlash.erase(m_AnimationPathWithoutSlash.length() - 1, 1);
		}

	}

	bool FindIdentical(const string& name, bool checkLen = false );
	const string& GetAnimationPath() const { return m_AnimationPath; };
	const string& GetAnimationPathWithoutSlash() const { return m_AnimationPathWithoutSlash; };

private:

	string m_AnimationPath;
	string m_AnimationPathWithoutSlash;

};


// Helper class for loading & parsing parameters from .cba file
class CAnimationInfoLoader
{
public:
	CAnimationInfoLoader(ICryXML *pXML);
	~CAnimationInfoLoader(void);


	bool LoadDescription(const string& name, IAnimationLoaderListener * pListener, IPakSystem* pPakSystem);
	const SAnimationDefinition * GetAnimationDefinition(const string& name ) const;

	std::vector<SAnimationDefinition> m_ADefinitions;
private:
	ICryXML	* m_pXML;
};


void ReportWarning(IAnimationLoaderListener* pListener, const char* szFormat, ...);
void ReportInfo(IAnimationLoaderListener* pListener, const char* szFormat, ...);
void ReportError(IAnimationLoaderListener* pListener, const char* szFormat, ...);

#endif