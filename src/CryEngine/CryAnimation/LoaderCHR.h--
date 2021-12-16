////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	28/09/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  loading of model and animations
/////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _CRY_MODEL_LOADER_HDR_
#define _CRY_MODEL_LOADER_HDR_

#include "ModelAnimationSet.h"

class CCharacterModel;

#include "CalParser.h"

class CryCHRLoader
{
private:
	static const char* DEFAULT_BIPED_SETUP_FILENAME;
	static const char* DEFAULT_BIPED_IK_FILENAME;

public:
	CryCHRLoader()
	{
	}
	~CryCHRLoader() 
	{	
		clear(); 
	}

	CCharacterModel* LoadNewCHR (const string& strGeomFileName, CharacterManager* pManager, uint32 SurpressWarning,uint32 loadanimation);

	//loads animations for already loaded model
	bool LoadAnimations(const char* calFileName, CCharacterModel* pModel );

	
	// loads the animations from the array
	uint32 LoadAnimationArray( CCharacterModel* pModel, TAnimFilesVec& arrAnimFiles, CAnimationSet::FacialAnimationSet::container_type& arrFacialAnimations, const TAnimFilesVec& arrWildcardAnimFiles, const bool parseSubFolders);

	// loads the data from the animation event database file (.animevent) - this is usually
	// specified in the CAL file.
	bool loadAnimationEventDatabase( CCharacterModel* pModel, const char* pFileName );

	// cleans up the resources allocated during load
	void clear();
	
	// the controller manager for the new model; this remains the same during the whole lifecycle
	// the file without extension
	string m_strGeomFileNameNoExt;

	struct _finddata_t m_fileinfo;

private:
	const string GetDefaultAnimDir();
	void LoadFaceLib( const char* faceLibFile, const char* animDirName, CCharacterModel* pModel );

	bool IsModelBiped()
	{
		if (!m_pModel)
			return false;

		return m_pModel->m_ModelSkeleton.GetJointIDByName("Bip01 Pelvis") > 0;
	}

	XmlNodeRef LoadIKSetup( const char* AimIK_FileName, CCharacterModel* pModel );

	void LoadLod(XmlNodeRef root);
	void LoadSetup(const char* fileName);

private:
	CCharacterModel* m_pModel;
};

#endif
