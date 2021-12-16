//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File: AnimationManager.h
//  Implementation of Animation Manager.h
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _CRYTEK_GAHPMG_
#define _CRYTEK_GAHPMG_

#include "SDI.h"
#include "GlobalAnimationHeader.h"
#include <NameCRCHelper.h>

struct BSAnimationPMG
{
	string m_strAnimName;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_strAnimName);
	}
};


//////////////////////////////////////////////////////////////////////////
// this is the animation information on the module level (not on the per-model level)
// it doesn't know the name of the animation (which is model-specific), but does know the file name
// Implements some services for bone binding and ref counting
struct GlobalAnimationHeaderPMG : public GlobalAnimationHeader
{
	friend class CAnimationManager;
	friend class CAnimationSet;

	GlobalAnimationHeaderPMG ()
	{
		m_FilePathCRC32		=	0;
		m_nRef_by_Model		= 0;
		m_nTouchedCounter = 0;
	}

	virtual ~GlobalAnimationHeaderPMG()
	{
	};

	const char* GetFilePath() const {	return m_FilePath.c_str(); };
	int GetFilePathCRC32() { return m_FilePathCRC32; }
	void SetFilePath(const string& name) 
	{ 
		m_FilePath = name; 
#ifdef _USE_LOWERCASE
		assert (name.size() < MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];
		NameCRCHelper::MakeLowercase(pStringArray, name.c_str());
		m_FilePathCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(pStringArray); 
#else
		m_FilePathCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(m_Name.c_str()); 
#endif
	};

	void AddRef()
	{
		++m_nRef_by_Model;
	}

	void Release()
	{
		if (!--m_nRef_by_Model)
		{
			m_arrBSAnimationsPMG.clear();	// nobody uses the blend-spaces; clean them up.
		}
	}

	size_t SizeOfThis() const
	{
		size_t nSize = sizeof(GlobalAnimationHeaderPMG);
		nSize += m_AnimEventsPMG.get_alloc_size();
		nSize += m_arrBSAnimationsPMG.get_alloc_size();
		size_t s = m_strSpliceAnim.size();
		for(size_t i=0; i<s; ++i)
			nSize += (m_strSpliceAnim[i].length()+1);

		nSize += ( m_strSpliceAnim.capacity()-m_strSpliceAnim.size() ) * sizeof(string);

		nSize += m_paramSpecPMG.get_alloc_size();
		nSize += m_edgeList.get_alloc_size();
		nSize += m_subdivideEdgeList.get_alloc_size();
		nSize += m_virtualExmps.capacity();
		nSize += m_paramNamePMGMap.size() * (sizeof(uint32) + sizeof(string));

		s = m_userParam.size();
		for(size_t i=0; i< s; ++i)
			nSize += m_userParam[i].get_alloc_size();

		nSize += 2 * sizeof(f32)*MAX_LMG_ANIMS; // weightsSmooth and weightsSmoothRate

		return nSize;
	}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject( m_FilePath );

		pSizer->AddObject( m_AnimEventsPMG );
		pSizer->AddObject( m_arrBSAnimationsPMG );		
		pSizer->AddObject( m_strSpliceAnim );
		pSizer->AddObject( m_paramSpecPMG );		

		pSizer->AddObject( m_edgeList );		
		pSizer->AddObject( m_subdivideEdgeList );		
		pSizer->AddObject( m_virtualExmps );		
		pSizer->AddObject( m_paramNamePMGMap );	
		pSizer->AddObject( m_userParam );	

		for( int i = 0 ; i < m_userParam.size() ; ++i )
		{
			pSizer->AddObject( m_userParam[i] );				
		}
	}
	
public:

	struct SParamPMG
	{
		string name;
		string locatorJoint;
		f32 startKey;
		f32 endKey;
		f32 minValue;
		f32 maxValue;
		f32 scale;

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{	
			pSizer->AddObject(name);
			pSizer->AddObject(locatorJoint);
		}
	};

	struct SVirtualEdge
	{
		uint32 ind1;
		uint32 ind2;
		void GetMemoryUsage(ICrySizer *pSizer) const{}
	};

	struct SVirtualExamp
	{
		uint32 ind1;
		uint32 ind2;
		f32 alpha; // blended as ind1*alpha * ind2*(1-apha);
		void GetMemoryUsage(ICrySizer *pSizer) const{}
	};

	struct SUserParam // User specified PMG parameters
	{
		string name;
		f32 val;

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(name);
		}
	};

	struct SParamSDI
	{
		uint32 N; // number of examples
		uint32 D; // parameter dimension (homogenous)
		DynArray<f32> params; // N*D
		DynArray<f32> A; // D*N
		DynArray<f32> R; // N*N
		DynArray<f32> Scale; //N

		void WriteData(uint32 _N, uint32 _D, f32* _params, f32* _A, f32* _R, f32* _Scale)
		{
			assert(_params && _A && _R && _Scale);
			N = _N;
			D = _D;

			params.resize(N*D);
			memcpy( &params[0], _params, N*D*sizeof(f32) );

			A.resize(D*N);
			memcpy( &A[0], _A, D*N*sizeof(f32) );

			R.resize(N*N);
			memcpy( &R[0], _R, N*N*sizeof(f32) );

			Scale.resize(N);
			memcpy( &Scale[0], _Scale, N*sizeof(f32) );
		}

		void ReadData(uint32& _N, uint32& _D, f32* _params, f32* _A, f32* _R, f32* _Scale)
		{
			assert(_params && _A && _R && _Scale);
			_N = N;
			_D = D;

			memcpy( _params, &params[0], N*D*sizeof(f32) );
			memcpy( _A, &A[0], D*N*sizeof(f32) );
			memcpy( _R, &R[0], N*N*sizeof(f32) );
			memcpy( _Scale, &Scale[0], N*sizeof(f32) );
		}
	};

	string m_FilePath;								//path-name - unique per-model
	uint32 m_FilePathCRC32;						//hash value for searching animations
	int32  m_nRef_by_Model;
	uint32 m_nTouchedCounter;
	DynArray<string> m_strSpliceAnim;
	DynArray<AnimEvents> m_AnimEventsPMG;

	DynArray<SParamPMG> m_paramSpecPMG;
	std::map<string, uint32> m_paramNamePMGMap; // name - index of m_paramSpecPMG map
	DynArray<SVirtualEdge> m_edgeList;
	DynArray<SVirtualEdge> m_subdivideEdgeList;
	DynArray<SVirtualExamp> m_virtualExmps; // VirtualExamples
	DynArray< DynArray<SUserParam> > m_userParam;

	DynArray<BSAnimationPMG> m_arrBSAnimationsPMG;

	SParamSDI m_paramSDI; // Save parameters of SDI

} _ALIGN(16);




#endif
