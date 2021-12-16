#include "StdAfx.h"

#include "AnimationCompiler.h"
#include "AnimationInfoLoader.h"
#include "CGF\CGFLoader.h"
#include "CGF\CGFSaver.h"
#include "FileUtil.h"
#include "SkeletonInfo.h"
#include "AnimationLoader.h"
#include "AnimationManager.h"
#include "IConfig.h"
#include "TrackStorage.h"

#include "IPakSystem.h"
#include "TempFilePakExtraction.h"
#include "ICryXML.h"

CAnimationCompiler * g_pAnimationCompiler;

static bool ScanDirectoryRecursive( const string &root,const string &path,const string &file,std::vector<string> &files, bool recursive , std::vector<uint32>& sizes);




class CAnimationLoaderListener : public IAnimationLoaderListener
{
public:
  CAnimationLoaderListener(ConvertContext &cc)
    :	cc(cc)
  {
  }

  virtual void OnAnimationLoaderMessage(MessageType type, const char* szMessage)
  {
    switch (type)
    {
    case MESSAGE_INFO:
      RCLog("%s", szMessage);
      break;
    case MESSAGE_WARNING:
      RCLogWarning("%s", szMessage);
      break;
    case MESSAGE_ERROR:
      RCLogError("%s", szMessage);
      break;
    }
  }

private:
  ConvertContext &cc;
};


CAnimationCompiler::CAnimationCompiler(ICryXML * pXML) : m_pXMLParser(pXML)
{
  m_TotalShared = 0;
  m_TotalMemoryShared = 0;
  m_TotalMemory = 0;
  m_TotalTracks = 0;
  m_pTrackStorage.reset(0);
  m_refCount = 1;
  m_iCount = 0;
  m_fAverage = 0;
  m_fTotal = 0;
  //	m_iThreads = 0;
}

CAnimationCompiler::~CAnimationCompiler(void)
{
}



//////////////////////////////////////////////////////////////////////////
XmlNodeRef CAnimationCompiler::LoadXml( const char *filename )
{
	// Get the xml serializer.
	IXMLSerializer* pSerializer = m_pXMLParser->GetXMLSerializer();

	// Read in the input file.
	XmlNodeRef root;
	{
		const bool bRemoveNonessentialSpacesFromContent = false;
		char szErrorBuffer[1024];
		root = pSerializer->Read(FileXmlBufferSource(filename), false, sizeof(szErrorBuffer), szErrorBuffer);
		if (!root)
		{
			//RCLogError("Cannot open XML file '%s': %s\n", filename, szErrorBuffer);
			return 0;
		}
	}
	return root;
}

uint32 CAnimationCompiler::LoadIKSetup( const char* pNameXML, CSkinningInfo* pModelSkeleton )
{
	XmlNodeRef TopRoot = LoadXml( pNameXML );
	if (TopRoot==0)
		return 0;

	const char* TopRootXMLTAG = TopRoot->getTag();
	if (stricmp(TopRootXMLTAG,"IK_Definition"))
		return 0;

	uint32 numIKTypes = TopRoot->getChildCount();
	for (uint32 iktype=0; iktype<numIKTypes; iktype++) 
	{
		XmlNodeRef IKNode = TopRoot->getChild(iktype);

		//-----------------------------------------------------------
		//check AimIK-Setup  
		//-----------------------------------------------------------
		const char* IKTypeTAG = IKNode->getTag();
		if (stricmp(IKTypeTAG,"AimIK_Definition"))
			continue;

		uint32 numChilds = IKNode->getChildCount();
		for (uint32 c=0; c<numChilds; c++) 
		{
			XmlNodeRef node = IKNode->getChild(c);
			const char* strTag = node->getTag();

			//----------------------------------------------------------------------------
			if (stricmp(strTag,"RotationList")==0) 
			{
				uint32 num = node->getChildCount();
				pModelSkeleton->m_AimIK_Rot.reserve(num);
				for (uint32 i=0; i<num; i++) 
				{
					SJointsAimIK_Rot AimIK_Rot;
					XmlNodeRef nodeRot = node->getChild(i);
					const char* RotTag = nodeRot->getTag();
					if (stricmp(RotTag,"Rotation")==0) 
					{
						const char* pJointName = nodeRot->getAttr( "JointName" );
						int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
						if (jidx>0)
						{
							AimIK_Rot.m_nJointIdx = jidx;
							AimIK_Rot.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
						}
				//		assert(jidx>0);
						nodeRot->getAttr( "Primary",    AimIK_Rot.m_nPreEvaluate );
						nodeRot->getAttr( "Additive",   AimIK_Rot.m_nAdditive );
						//	AimIK_Rot.m_nAdditive=0;
						pModelSkeleton->m_AimIK_Rot.push_back( AimIK_Rot );
					}
				}
			}

			//----------------------------------------------------------------------------------------

			if (stricmp(strTag,"PositionList")==0) 
			{
				uint32 num = node->getChildCount();
				pModelSkeleton->m_AimIK_Pos.reserve(num);
				for (uint32 i=0; i<num; i++) 
				{
					SJointsAimIK_Pos AimIK_Pos;
					XmlNodeRef nodePos = node->getChild(i);
					const char* PosTag = nodePos->getTag();
					if (stricmp(PosTag,"Position")==0) 
					{
						const char* pJointName = nodePos->getAttr( "JointName" );
						int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
						if (jidx>0)
						{
							AimIK_Pos.m_nJointIdx = jidx;
							AimIK_Pos.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
						}
					//	assert(jidx>0);
						pModelSkeleton->m_AimIK_Pos.push_back( AimIK_Pos );
					}
				}
			}

			//----------------------------------------------------------------------------------------

			if (stricmp(strTag,"DirectionalBlends")==0) 
			{
				uint32 num = node->getChildCount();
				pModelSkeleton->m_DirectionalBlends.reserve(num);
				for (uint32 i=0; i<num; i++) 
				{
					DirectionalBlends DirBlend;
					XmlNodeRef nodePos = node->getChild(i);
					const char* DirTag = nodePos->getTag();
					if (stricmp(DirTag,"Joint")==0) 
					{
						const char* pAnimToken = nodePos->getAttr( "AnimToken" );
						DirBlend.m_AnimToken = pAnimToken;
						DirBlend.m_AnimTokenCRC32 = g_crcGen.GetCRC32Lowercase(pAnimToken);

						const char* pJointName = nodePos->getAttr( "ParameterJoint" );
						int jidx1 = pModelSkeleton->GetJointIDByName(pJointName);
						if (jidx1>0)
						{
							DirBlend.m_nParaJointIdx = jidx1;
							DirBlend.m_strParaJointName = pModelSkeleton->GetJointNameByID(jidx1);
						}
						assert(jidx1>0);

						const char* pStartJointName = nodePos->getAttr( "StartJoint" );
						int jidx2 = pModelSkeleton->GetJointIDByName(pStartJointName);
						if (jidx2>0)
						{
							DirBlend.m_nStartJointIdx	= jidx2;
							DirBlend.m_strStartJointName = pModelSkeleton->GetJointNameByID(jidx2);
						}
						assert(jidx2>0);

						pModelSkeleton->m_DirectionalBlends.push_back( DirBlend );
					}
				}
			}


		}
	}
	return 1;
}


bool CAnimationCompiler::Process( ConvertContext &cc ) 
{
	string descfile;
	if (stricmp(cc.sourceFileFinalExtension,"cba") == 0)
	{
		descfile = cc.getSourcePath();
	}

	if (cc.platform == ePlatform_X360 || cc.platform == ePlatform_PS3)
	{
		m_Behavior.m_bNeedSwapEndian = true;
		RCLog("Endian conversion specified");
	}

	string singleFile;
	bool bSingleFile = cc.config->Get("file", singleFile);
	string fix(singleFile);
	FixString(fix);
	singleFile = fix.c_str();

	string destFolder;
	bool bDestFolder = cc.config->Get("dest", destFolder);

	int iReportMode(false);
	bool bReport = cc.config->Get("report", iReportMode);

	int iCheckloco(false);
	bool bCheckLoco = cc.config->Get("checkloco", iCheckloco);

	m_Behavior.m_bCheckLocomotion = bCheckLoco && iCheckloco > 0;


	{

		bool bCleanup(false);
		bool bF =	cc.config->Get("cleanupfast", bCleanup);

		bCleanup = bCleanup && bF;

		bool bOnlynew(false);
		bF =	cc.config->Get("cleanup", bOnlynew);

		bOnlynew = bOnlynew && bF;

		bool bNotUseDBA = cc.config->HasKey("SkipDba");


		m_Behavior.m_bUseDBA = !bNotUseDBA;

		if (bCleanup)
			m_Behavior.m_bNeedDeleteUncompressedData = true;

		if (bOnlynew)
			m_Behavior.m_bNeedDeleteAllData = true;

		if (!bCleanup)//bCleanup || iCompress)
			m_Behavior.m_bNeedCalculate = true;

		m_Behavior.m_bUseMultiThreading = !bSingleFile && cc.threads > 1;
		m_Behavior.m_iSupportedThreads = cc.threads;
	}

	if (m_Behavior.m_bUseDBA)
	{
		m_pTrackStorage.reset(new CTrackStorage);
	}

	class Listener : public ILoaderCGFListener
	{
	public:
		Listener(ConvertContext& cc): cc(cc) {}
		virtual void Warning( const char *format ) {RCLogWarning("%s", format);}
		virtual void Error( const char *format ) {RCLogError("%s", format);}

	private:
		ConvertContext& cc;
	};
	Listener listener(cc);


	if (bReport && iReportMode > 0)
	{
		return true;
	}

	PrepareThreads();

	if (bSingleFile)
	{
		m_pTrackStorage.reset(0);
	}

	CAnimationInfoLoader loader(m_pXMLParser);

	CAnimationLoaderListener listenerAnim(cc);

	IPakSystem* pPakSystem = (cc.pRC ? cc.pRC->GetPakSystem() : 0);
	if (!loader.LoadDescription(descfile, &listenerAnim, pPakSystem))
		return false;

	enum eErrorState 
	{
		eUnknown,
		eMissedModel
	};

	eErrorState errorState = eUnknown;
	string errorModel;

	//------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------
	RCLog("SourceFolder: %s",cc.sourceFolder);
	uint32 numDefinitions = loader.m_ADefinitions.size();
	for (uint32 i=0; i<numDefinitions; ++i)
	{
		const char* pPathDBA = loader.m_ADefinitions[i].m_DBName.c_str();
		RCLog("---------------------: No: %d   %s",i,pPathDBA);

		//size_t dbalength = loader.m_ADefinitions[i].m_DBName.size();
		//if (dbalength==0)
		//	continue;


		string filename(cc.sourceFolder);
		//		if (bDestFolder)
		//			filename =  destFolder;
		filename = PathHelpers::Join(filename,loader.m_ADefinitions[i].m_Model);


		//file may be in a pakfile, so extract it if required

		IPakSystem* pPakSystem = (cc.pRC ? cc.pRC->GetPakSystem() : 0);

		string strOriginalFileNameCHR=filename.c_str();
		TempFilePakExtraction fileProxyCHR( strOriginalFileNameCHR, pPakSystem );

		string strOriginalFileNameIK = strOriginalFileNameCHR;
		strOriginalFileNameIK.replace(".chr",".ik");
		TempFilePakExtraction fileProxyIK( strOriginalFileNameIK.c_str(), pPakSystem );


		CSkeletonInfo currentSkeleton;
		bool res = currentSkeleton.LoadCHRModel(fileProxyCHR.GetTempName(), &listener);
		if (res)
		{
			uint32 TopRoot = LoadIKSetup( fileProxyIK.GetTempName(), &currentSkeleton.m_SkinningInfo );

			//fill list with filenames
			std::vector<string> fileList;
			string dirPath(cc.sourceFolder);
			string destPath;
			dirPath = PathHelpers::AddSeparator(PathHelpers::Join(dirPath, loader.m_ADefinitions[i].GetAnimationPathWithoutSlash()));
			destPath = dirPath;
			std::vector<uint32> sizes;
			ScanDirectoryRecursive(dirPath.c_str(),string(""), string("*.caf"), fileList, true, sizes);

			if (bDestFolder)
			{
				destPath = destFolder;
				destPath = PathHelpers::AddSeparator(PathHelpers::Join(destPath, loader.m_ADefinitions[i].GetAnimationPathWithoutSlash()));
			}

			// clear trackstorage
			if (m_pTrackStorage.get())
			{
				m_pTrackStorage->Clear();
			}


			string dbFile;

			bool yep = true;
			uint32 numFiles = fileList.size();
			for (uint32 anim=0; anim<numFiles; anim++)
			{

				string animFile(dirPath);
				animFile = PathHelpers::Join(animFile, fileList[anim]);
				FixString(animFile);

				string strRelPath=animFile;
				FixString(strRelPath);
				std::size_t npos = strRelPath.find("animations");
				if (npos != string::npos)
					strRelPath.erase(0, npos);
				uint32 nFilePathCRC32 = g_crcGen.GetCRC32Lowercase(strRelPath); 

				uint32 numGAH = CAnimationManager::GetInst().m_arrGlobalAnimations2.size();
				uint32 nAlreadyProcessed=0;
				for (uint32 i=0; i<numGAH; i++)
				{
					GlobalAnimationHeaderCAF& rGAH = CAnimationManager::GetInst().m_arrGlobalAnimations2[i];
					nAlreadyProcessed=(rGAH.m_FilePathCRC32==nFilePathCRC32);
					if (nAlreadyProcessed)
						break;
				}
				if (nAlreadyProcessed)
					continue;

				string destFile(destPath);
				destFile = PathHelpers::Join(destFile, fileList[anim]);


				const SAnimationDefinition * pDefinition = loader.GetAnimationDefinition(animFile);
				if (!pDefinition)
				{
					//FIXME: Add message - definition not found!
					continue;
				}


				dbFile = (bDestFolder ? destFolder : cc.sourceFolder);
				dbFile = PathHelpers::Join(dbFile, pDefinition->m_DBName);

				if (m_pTrackStorage.get() && m_Behavior.m_bUseDBA && yep)
				{
					yep = false;
					uint32 nDBAExists = 0;//m_pTrackStorage->LoadDataBase(dbFile.c_str());
					if (nDBAExists)
						m_Behavior.m_bDBAExists = true;
					else
						m_Behavior.m_bDBAExists = false;
				}

				SAnimationDesc pDesc;



				if ( (!bSingleFile) || (bSingleFile && (strstr(/*fileList[anim]*/animFile.c_str(), singleFile) || strstr(singleFile, /*fileList[anim]*/animFile.c_str()))))
				{
					pDesc =  pDefinition->GetAnimationDesc(string(fileList[anim]));

					if (bSingleFile && strlen(singleFile) == 0)
						continue;
					if (bSingleFile && strlen(fileList[anim]) == 0)
						continue;

					ProcessSingleAnimation(fileProxyCHR.GetTempName(), &currentSkeleton, "animations/"+pDefinition->m_DBName, animFile, destFile, &listener, cc, pDesc, sizes[anim]);//, bCleanup, bOnlynew );

					if (bSingleFile)
					{
						WaitAllThreads();
						RCLog("Processed %d files. Average compress ratio %f%%", m_iCount, m_fAverage);
						return true;
					}
				}
			}

			WaitAllThreads();

			if (m_pTrackStorage.get())
			{
				if (m_Behavior.m_bNeedSwapEndian)
					m_pTrackStorage->SaveDataBase904(dbFile.c_str());
				else
					m_pTrackStorage->SaveDataBase903(dbFile.c_str());
			}
		} 
		else
		{
			if (bSingleFile && strstr(singleFile, loader.m_ADefinitions[i].GetAnimationPath().c_str())) {
				errorModel = "Probably you have missed character file " + filename;
				errorState = eMissedModel;
			}
		}
	} 

	//--------------------------------------------------------------------------------------------

	if (!m_bProcessed && bSingleFile)
	{
		//	// that mean we find no model or something
		//	// try to process with default param
		//	SAnimationDesc pDesc;
		//	int resSize = ProcessSingleAnimation(0, string(singleFile), string(singleFile), &listener, cc, pDesc, bCleanup, bOnlynew );
		switch (errorState)
		{
		case eMissedModel:
			RCLogError(errorModel.c_str());
			break;
		case eUnknown:
			RCLogError("The animation '%s' was exported to a path that does not have a corresponding animation build entry in the 'Animations.cba' file.\nTo export compressed animation data please either:\n\t1) provide an entry in the 'animations.cba' file that matches your intended export path.\n\t2) export the anim to a path that already has a corresponding entry in the 'Animations.cba' file.", singleFile);
		}

		DeleteFile(singleFile);
	}

	RCLog("Processed %d files. Average compress ratio %.1f%%.  ", m_iCount, m_fAverage  );

	if (m_pTrackStorage.get())
	{
		RCLog("All models shared %d tracks. Total tracks %d. Shared memory %d. Total Memory %d", m_TotalShared, m_TotalTracks, m_TotalMemoryShared , m_TotalMemory  );
	}

	//this is the place to create the ImageFile
/*	uint32 numGAH = CAnimationManager::GetInst().m_arrGlobalAnimations2.size();
	RCLog("numGAH %d",numGAH);
	for (uint32 i=0; i<numGAH; i++)
	{
		GlobalAnimationHeaderCAF& rGAH = CAnimationManager::GetInst().m_arrGlobalAnimations2[i];
		const char* pFP = rGAH.m_FilePath.c_str();
		uint32 crc32    = rGAH.m_FilePathDBACRC32;
		uint32 flags    = rGAH.m_nFlags;
		RCLog("no: %04d crc32: %08x %08x fname: %s",i,crc32,flags,pFP );
	}*/

	const FILETIME latest = FileUtil::GetLastWriteFileTime(destFolder);
	SaveAnimationImageFileCAF(destFolder+"\\Animations.img",latest);
	SaveAnimationImageFileAIM(destFolder+"\\DirectionalBlends.img",latest);
	return true;
}


struct CChunkData
{
	char *data;
	int size;

	CChunkData() { data = 0; size = 0; }
	~CChunkData() { free(data); }

	template <class T>
	void Add( const T& object )
	{
		AddData( &object,sizeof(object) );
	}
	void AddData( const void *pSrcData,int nSrcDataSize )
	{
		data = (char*)realloc(data,size+nSrcDataSize);
		memcpy( data+size,pSrcData,nSrcDataSize );
		size += nSrcDataSize;
	}
};


void CAnimationCompiler::ConstructAndSetOutputFile(ConvertContext &cc)
{
	cc.SetOutputFile(cc.sourceFileFinal);
}

void CAnimationCompiler::GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const
{
	if (filenameBuffer && (bufferSize > 0))
	{
		filenameBuffer[0] = 0;
	}
}


void CAnimationCompiler::Release()
{
  if (--m_refCount <= 0)
    delete this;
}


void RunThread(void * pData) 
{
	ThreadCompiler * pCompiler = (ThreadCompiler *)pData;
	pCompiler->Run();
}


void CAnimationCompiler::ProcessSingleAnimation( const char* referenceModel, CSkeletonInfo* currentSkeleton, const string& strFilePathDBA, const string& animFile, const string& destFile, ILoaderCGFListener* listener, ConvertContext& cc, SAnimationDesc& pDesc, int oldsize)
{
	ThreadCompiler * pCurrThread = AcquireThread();

	pCurrThread->m_referenceModel		= referenceModel;
	pCurrThread->m_currentSkeleton	= currentSkeleton;
	pCurrThread->m_FilePathDBA			= strFilePathDBA;
	pCurrThread->m_animFile					= animFile;
	pCurrThread->m_destFile					= destFile; 
	pCurrThread->m_listener					= listener;
	pCurrThread->m_cc = cc;
	pCurrThread->m_pDesc = pDesc;
	pCurrThread->m_oldsize = oldsize;
	pCurrThread->m_pCompiler = this;
	pCurrThread->m_resSize = -1;


	if (!m_Behavior.m_bUseMultiThreading)
	{
		//(pCurrThread->Run();
		RunThread((void *)pCurrThread);
	}
	else 
	{
		RCLog("Threads available %i->%i. Started %i %s", m_vThreads.size()+1, m_vThreads.size(), pCurrThread->m_id, pCurrThread->m_destFile.c_str() );
		_beginthread(RunThread, 0, (void *)pCurrThread);
	}
}

void ThreadCompiler::Run()
{
	const FILETIME latest = FileUtil::GetLastWriteFileTime(m_animFile.c_str());
	static volatile int g_lockMemMan;
	bool bFoundDesc = false;

	char animFile[512];
	memset(&animFile[0],0,512);
	uint32 len = m_animFile.length();
	memcpy(&animFile[0],&m_animFile[0],len);
	
	int32 i=0;
	for (i=len; i>-1; i--)
	{
		if (animFile[i] == '/')
			break;
	}
	memset(&animFile[0],0x20,i+1);
	
	bool IsAIM = 0; //	(CryStringUtils::stristr(pAnimName,"AimPoses") != 0);
	uint32 numDB = m_currentSkeleton->m_SkinningInfo.m_DirectionalBlends.size();
	for (uint32 d=0; d<numDB; d++)
	{
		const char* strAnimToken = m_currentSkeleton->m_SkinningInfo.m_DirectionalBlends[d].m_AnimToken;
		IsAIM = (CryStringUtils::stristr(animFile,strAnimToken) != 0);
		if (IsAIM)
			break;
	}
	if (IsAIM==0)
	{
		IsAIM = (CryStringUtils::stristr(animFile,"AimPoses") != 0);
	}

	if (IsAIM)
	{
		string RelPath(m_animFile);
		FixString(RelPath);
		std::size_t npos = RelPath.find("animations");
		if (npos != string::npos)
			RelPath.erase(0, npos);
		int p = RelPath.find("//");
		while (p != string::npos)
		{
			RelPath.erase(p,1 );
			p = RelPath.find("//");
		}

		RCLog("AIMPOSE fname: %s",RelPath);

		/*
		const char* pname = "animations/human/male/weapons/scar/3p/aimposes/stand_tac_aimposes_idle_scar_shoulder_3p_01.caf";
		if ( strcmp(pname,RelPath) )
		{
			m_pCompiler->ReleaseThread(this);
			return;
		} */

		uint32 nFilePathCRC32 = g_crcGen.GetCRC32Lowercase(RelPath); 
		uint32 numAIM = CAnimationManager::GetInst().m_arrGlobalAIM.size();
		uint32 nAlreadyProcessed=0;
		for (uint32 i=0; i<numAIM; i++)
		{
			GlobalAnimationHeaderAIM& rAIM = CAnimationManager::GetInst().m_arrGlobalAIM[i];
			nAlreadyProcessed=(rAIM.m_FilePathCRC32==nFilePathCRC32);
			if (nAlreadyProcessed)
			{
				m_pCompiler->ReleaseThread(this);
				return;
			}
		}


		GlobalAnimationHeaderAIM gaim;
		gaim.SetFilePath(RelPath);

		CAnimationCompressor CurrentAnimation(m_currentSkeleton);
		CurrentAnimation.LoadAnimationFromFileAIM(m_animFile.c_str(),gaim);

		CSkinningInfo* pModelSkeleton = &m_currentSkeleton->m_SkinningInfo;

		uint32 nAnimTokenCRC32 = 0;   //	(CryStringUtils::stristr(pAnimName,"AimPoses") != 0);
		int32 nDirectionIdx = -1;
		uint32 numDB = pModelSkeleton->m_DirectionalBlends.size();
		for (uint32 d=0; d<numDB; d++)
		{
			const char* strAnimToken = pModelSkeleton->m_DirectionalBlends[d].m_AnimToken;
			uint32 IsAIM = (CryStringUtils::stristr(m_animFile.c_str(),strAnimToken) != 0);
			if (IsAIM)
			{
				nAnimTokenCRC32 = pModelSkeleton->m_DirectionalBlends[d].m_AnimTokenCRC32;
				nDirectionIdx = pModelSkeleton->m_DirectionalBlends[d].m_nParaJointIdx;
				break;
			}
		}

		CurrentAnimation.ProcessAimPoses(pModelSkeleton,RelPath,gaim,nDirectionIdx,nAnimTokenCRC32);

		CAnimationManager::GetInst().m_arrGlobalAIM.push_back( gaim );

		m_pCompiler->m_iCount++;
		m_pCompiler->m_fAverage = m_pCompiler->m_fTotal / (float)m_pCompiler->m_iCount;

	//	const FILETIME latest = FileUtil::GetLastWriteFileTime(m_destFile);
	//	m_pCompiler->SaveAnimationImageFileAIM(m_destFile+"\\DirectionalBlends.img",latest);

	//	m_pCompiler->ReleaseThread(this);
	//	return;
	}

//	m_pCompiler->ReleaseThread(this);
//	return;



	//  RCLog("Started %s", destFile.c_str());
	std::auto_ptr<CAnimationCompressor> currentAnimation;
	currentAnimation.reset(new CAnimationCompressor(m_currentSkeleton));
	bool bNeedCalculate(m_pCompiler->m_Behavior.m_bNeedCalculate);
	uint32 success = currentAnimation->LoadAnimationFromFileCAF(m_animFile.c_str(), m_destFile.c_str(), m_listener, m_pDesc, m_pCompiler->m_Behavior.m_bNeedCalculate);
	assert(success);
	if (success)
	{

		if (m_pCompiler->m_Behavior.m_bNeedCalculate && !currentAnimation->HasOldFormat())
		{
			// we don't have an uncompressed data. File not processed
			//          RCLogError("File %s has only compressed data. File deleted", animFile.c_str());
			RCLog("File %s has only compressed data. Stored", m_animFile.c_str());
			// free chunk file
			bNeedCalculate = false;
			//          currentAnimation.reset(0);
			//          DeleteFile(animFile.c_str());
			//          return 0;
		}

		if (!m_pCompiler->m_Behavior.m_bNeedCalculate && !currentAnimation->HasNewFormat())
		{
			// we don't have a compressed data. need calculate this
			RCLog("File %s has only uncompressed data. Compiled", m_animFile.c_str());
			bNeedCalculate = true;
		}

		CAnimationCompressor::EDeleteMethod delFlag;

		bool bNeedProcessing(true);

		if (bNeedCalculate)
		{
			delFlag = CAnimationCompressor::eNew;

			if (m_pCompiler->m_Behavior.m_bNeedDeleteUncompressedData || m_pCompiler->m_Behavior.m_bNeedDeleteAllData)
				delFlag = CAnimationCompressor::eAll;
		}
		else
		{
			if (!m_pCompiler->m_Behavior.m_bNeedSwapEndian) 
			{
				delFlag = CAnimationCompressor::eOld;

				if (currentAnimation->HasNewFormat())
				{
					bNeedProcessing = false;
				}
			} 
			else 
			{
				delFlag = CAnimationCompressor::eAll;
			}
		}

		currentAnimation->DeleteOldChunk(delFlag, bNeedProcessing);

		if(currentAnimation->ProcessAnimation(m_cc,m_listener, m_pCompiler->m_pTrackStorage.get(), bNeedCalculate))
		{
			// Create the directory (including intermediate ones), if not already there.
			string windowPath = m_destFile; 
			windowPath.replace('/', '\\');
			MakeSureDirectoryPathExists(windowPath.c_str());

	//		string dbFile;
	//		dbFile = (bDestFolder ? destFolder : cc.sourceFolder);
	//		dbFile = PathHelpers::Join(dbFile, pDefinition->m_DBName);



			string RelPath(m_destFile);
			FixString(RelPath);
			std::size_t npos = RelPath.find("animations");
			if (npos != string::npos)
				RelPath.erase(0, npos);
			int p = RelPath.find("//");
			while (p != string::npos)
			{
				RelPath.erase(p,1 );
				p = RelPath.find("//");
			}
			currentAnimation->m_GlobalAnimationHeader.SetFilePathCAF(RelPath);
			m_resSize = currentAnimation->SaveAnimationToFile(m_destFile.c_str(), m_listener, m_cc, bNeedProcessing, latest);

			currentAnimation->m_dwTimestamp = FileUtil::FiletimeToUnixTime( FileUtil::GetLastWriteFileTime(m_animFile.c_str()) );

			if (m_pDesc.m_SkipSaveToDatabase==0)
			{
				currentAnimation->m_GlobalAnimationHeader.SetFilePathDBA(m_FilePathDBA);
				currentAnimation->SaveToDB(m_FilePathDBA, m_pCompiler->m_pTrackStorage.get(), m_pCompiler->m_Behavior.m_bDBAExists );
			}
			

//#ifdef PRINTOUT
			const char* pFP = currentAnimation->m_GlobalAnimationHeader.m_FilePath.c_str();
			uint32 crc32 = currentAnimation->m_GlobalAnimationHeader.m_FilePathDBACRC32;

			uint32 test1 = currentAnimation->m_GlobalAnimationHeader.m_arrController.size();

			uint32 m_nControllers4=0;
			for (uint32 c=0; c<test1; c++)
			{
				CController* pController = dynamic_cast<CController *>(currentAnimation->m_GlobalAnimationHeader.m_arrController[c].get());
				m_nControllers4 += (pController!=0);
			}
			CAnimationManager::GetInst().m_arrGlobalAnimations2.push_back(currentAnimation->m_GlobalAnimationHeader);
			RCLog("no: %04d crc32: %08x %02x fname: %s",m_pCompiler->m_iCount,crc32,m_nControllers4,pFP );

			//#endif

		}
		else
		{
			RCLogError("Error in processing %s (reference model is %s)", m_animFile.c_str(), m_referenceModel.c_str());
		}

		if (m_pCompiler->m_Behavior.m_bUseDBA && !m_pDesc.m_SkipSaveToDatabase)
		{
			currentAnimation.reset(0);
			DeleteFile(m_destFile.c_str());
		}
	}

	if (m_oldsize > 0)
	{
		float currentRatio = 100.0f - (float)m_resSize * 100.0f / (float)m_oldsize;
		m_pCompiler->m_fTotal += currentRatio;
#ifdef PRINTOUT
		RCLog( "Processed %s (reference model is %s). Old size %d. New size %d. Compress ratio %.1f%%",	m_destFile.c_str(), m_referenceModel.c_str(), m_oldsize, m_resSize, currentRatio );
#endif
	}

	WriteLock lock(g_lockMemMan);

	m_pCompiler->m_iCount++;
	m_pCompiler->m_fAverage = m_pCompiler->m_fTotal/ (float)m_pCompiler->m_iCount;
	m_pCompiler->ReleaseThread(this);
	return ;
}


void CAnimationCompiler::PrepareThreads()
{
	CryAutoLockRC lock(m_threadsLock);
	int count = 1;

	if (m_Behavior.m_bUseMultiThreading)
	{
		count = std::max(1, m_Behavior.m_iSupportedThreads);
	}

	for (int i = 0; i < count; ++i)
	{
		m_vThreads.push_back(new ThreadCompiler(i));
	}

	m_bProcessed = false;
}

ThreadCompiler* CAnimationCompiler::AcquireThread()
{
	if (m_Behavior.m_bUseMultiThreading)
	{
		while(true)
		{
			{
				CryAutoLockRC lock(m_threadsLock);
				if(!m_vThreads.empty())
				{
					ThreadCompiler * pCurrThread= m_vThreads.back();
					m_vThreads.pop_back();
					return pCurrThread;
				}
			}
			Sleep(0);
		}
	}
	ThreadCompiler * pCurrThread= m_vThreads.back();
	m_vThreads.pop_back();
	return pCurrThread;
}

void CAnimationCompiler::ReleaseThread(ThreadCompiler* thread)
{
	if (thread->m_resSize > 0)
	{
		m_bProcessed = true;
	}

	if (m_Behavior.m_bUseMultiThreading)
	{
		CryAutoLockRC lock(m_threadsLock);
		m_vThreads.push_back(thread);
	}
	else
	{
		m_vThreads.push_back(thread);	
	}
}

void CAnimationCompiler::WaitAllThreads()
{
	if (m_Behavior.m_bUseMultiThreading)
	{
		while(true)
		{
			{
				CryAutoLockRC lock(m_threadsLock);
				if(m_vThreads.size() == m_Behavior.m_iSupportedThreads)
				{
					return;
				}
			}
			Sleep(0);
		}
	}
}

//bool 



ICompiler* CAnimationCompiler::CreateCompiler()
{
  // Only ever return one compiler, since we don't support multithreading. Since
  // the compiler is just this object, we can tell whether we have already returned
  // a compiler by checking the ref count.
  if (m_refCount >= 2)
    return 0;

  // Until we support multithreading for this convertor, the compiler and the
  // convertor may as well just be the same object.
  ++m_refCount;
  return this;
}

bool CAnimationCompiler::SupportsMultithreading() const
{
  return false;
}

//////////////////////////////////////////////////////////////////////////
// the paths must have trailing slash
static bool ScanDirectoryRecursive( const string &root,const string &path,const string &file,std::vector<string> &files, bool recursive, std::vector<uint32>& sizes )
{
  __finddata64_t c_file;
  intptr_t hFile;

  bool anyFound = false;

  string fullPath = root + path + file;
  if ( (hFile = _findfirst64( fullPath.c_str(), &c_file )) != -1L )
  {
    // Find the rest of the files.
    do {
      if (!(c_file.attrib & _A_SUBDIR))
      {
        anyFound = true;
        string ext(c_file.name);
        int pos = int(ext.rfind('.'));
        if (pos != -1 && _stricmp(".caf", ext.Right(strlen(ext) -  pos) ) == 0)	 {
          files.push_back( path + c_file.name );
          sizes.push_back((unsigned int)c_file.size);
        }
      }
    }	while (_findnext64( hFile, &c_file ) == 0);
    _findclose( hFile );
  }

  if (recursive)
  {
    fullPath = root + path + "*.*";
    if( (hFile = _findfirst64( fullPath.c_str(), &c_file )) != -1L )
    {
      // Find directories.
      do {
        if (c_file.attrib & _A_SUBDIR)
        {
          // If recursive.
          if (c_file.name[0] != '.')
          {
            if (ScanDirectoryRecursive( root,path + c_file.name + "\\",file,files,recursive, sizes ))
              anyFound = true;
          }
        }
      }	while (_findnext64( hFile, &c_file ) == 0);
      _findclose( hFile );
    }
  }

  return anyFound;
}



//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
uint32 CAnimationCompiler::SaveAnimationImageFileCAF(const char* name, FILETIME timeStamp)
{
	CSaverCGF cgfSaver( name, m_ChunkFileCAF);

	uint32 numGAH = CAnimationManager::GetInst().m_arrGlobalAnimations2.size();
	for (uint32 i=0; i<numGAH; i++)
	{
		GlobalAnimationHeaderCAF& rGAH = CAnimationManager::GetInst().m_arrGlobalAnimations2[i];

		CHUNK_GAHCAF_INFO GAH_Info;

		memset(GAH_Info.m_FilePath,0,sizeof(GAH_Info.m_FilePath));
		uint32 num=rGAH.m_FilePath.size();
		for (int32 s=0; s<num; s++)
			GAH_Info.m_FilePath[s]=rGAH.m_FilePath[s];

		GAH_Info.m_FilePathCRC32		= rGAH.m_FilePathCRC32;
		SWAP	SwapEndianness(GAH_Info.m_FilePathCRC32);
		GAH_Info.m_FilePathDBACRC32 = rGAH.m_FilePathDBACRC32;
		SWAP	SwapEndianness(GAH_Info.m_FilePathDBACRC32);

		if (GAH_Info.m_FilePathDBACRC32==0)
			rGAH.OnAssetOnDemand(); 
		else 
			rGAH.OnAssetLoaded();
		//	rGAH.OnAssetCreated();
		rGAH.ClearAssetRequested();

		GAH_Info.m_Flags	=	rGAH.m_nFlags;
		SWAP	SwapEndianness(GAH_Info.m_Flags);

		GAH_Info.m_nControllers=0;
		uint32 numControllers =	rGAH.m_arrController.size();
		for (uint32 c=0; c<numControllers; c++)
		{
			CController* pController = dynamic_cast<CController *>(rGAH.m_arrController[c].get());
			GAH_Info.m_nControllers+= (pController!=0);
		}
		SWAP	SwapEndianness(GAH_Info.m_nControllers);

		GAH_Info.m_fSpeed						= rGAH.m_fSpeed;
		GAH_Info.m_fDistance				= rGAH.m_fDistance;
		GAH_Info.m_fSlope						= rGAH.m_fSlope;
		SWAP	SwapEndianness(GAH_Info.m_fSpeed);
		SWAP	SwapEndianness(GAH_Info.m_fDistance);
		SWAP	SwapEndianness(GAH_Info.m_fSlope);

		GAH_Info.m_StartLocation		= rGAH.m_StartLocation2;
		SWAP	SwapEndianness(GAH_Info.m_StartLocation.q.w);
		SWAP	SwapEndianness(GAH_Info.m_StartLocation.q.v.x);
		SWAP	SwapEndianness(GAH_Info.m_StartLocation.q.v.y);
		SWAP	SwapEndianness(GAH_Info.m_StartLocation.q.v.z);
		SWAP	SwapEndianness(GAH_Info.m_StartLocation.t.x);
		SWAP	SwapEndianness(GAH_Info.m_StartLocation.t.y);
		SWAP	SwapEndianness(GAH_Info.m_StartLocation.t.z);


		GAH_Info.m_fStartSec				=	rGAH.m_fStartSec;									//asset-feature: Start time in seconds.
		GAH_Info.m_fEndSec					=	rGAH.m_fEndSec;										//asset-feature: End time in seconds.
		GAH_Info.m_fTotalDuration		=	rGAH.m_fEndSec-rGAH.m_fStartSec;	//asset-feature: asset-feature: total duration in seconds.
		uint32 IsAdditive = GAH_Info.m_Flags&CA_ASSET_ADDITIVE;
		if (IsAdditive)
			GAH_Info.m_fStartSec += SECONDS_PER_TICK;
		SWAP	SwapEndianness(GAH_Info.m_fEndSec);
		SWAP	SwapEndianness(GAH_Info.m_fStartSec);
		SWAP	SwapEndianness(GAH_Info.m_fTotalDuration);

		GAH_Info.m_LHeelStart		= rGAH.m_FootPlantVectors.m_LHeelStart;
		SWAP	SwapEndianness(GAH_Info.m_LHeelStart);
		GAH_Info.m_LHeelEnd			= rGAH.m_FootPlantVectors.m_LHeelEnd;
		SWAP	SwapEndianness(GAH_Info.m_LHeelEnd);
		GAH_Info.m_LToe0Start		= rGAH.m_FootPlantVectors.m_LToe0Start;
		SWAP	SwapEndianness(GAH_Info.m_LToe0Start);
		GAH_Info.m_LToe0End			= rGAH.m_FootPlantVectors.m_LToe0End;
		SWAP	SwapEndianness(GAH_Info.m_LToe0End);
		GAH_Info.m_RHeelStart		= rGAH.m_FootPlantVectors.m_RHeelStart;
		SWAP	SwapEndianness(GAH_Info.m_RHeelStart);
		GAH_Info.m_RHeelEnd			= rGAH.m_FootPlantVectors.m_RHeelEnd;
		SWAP	SwapEndianness(GAH_Info.m_RHeelEnd);
		GAH_Info.m_RToe0Start		= rGAH.m_FootPlantVectors.m_RToe0Start;
		SWAP	SwapEndianness(GAH_Info.m_RToe0Start);
		GAH_Info.m_RToe0End			= rGAH.m_FootPlantVectors.m_RToe0End;
		SWAP	SwapEndianness(GAH_Info.m_RToe0End);

		const char* pFP = rGAH.m_FilePath.c_str();
		RCLog("no: %04d  crc32: %08x flags: %08x %02x fname: %s",i, GAH_Info.m_FilePathDBACRC32, GAH_Info.m_Flags, GAH_Info.m_nControllers, pFP );

		//		cgfSaver.SaveSpeedInfo2(&GAH_Info, sizeof(GAH_INFO) );

		ZeroStruct(GAH_Info.chdr);
		GAH_Info.chdr.ChunkType			= ChunkType_GlobalAnimationHeaderCAF;
		GAH_Info.chdr.ChunkVersion	= CHUNK_GAHCAF_INFO::VERSION;

		CChunkData chunkData;
		chunkData.Add( GAH_Info );
		m_ChunkFileCAF.AddChunk( GAH_Info.chdr, chunkData.data, chunkData.size );
	}


	SetFileAttributes( name,FILE_ATTRIBUTE_ARCHIVE );
	m_ChunkFileCAF.Write( name );

	FileUtil::SetFileTimes(name, timeStamp);
	const __int64 fileSize = FileUtil::GetFileSize(name);
	if (fileSize < 0)
	{
		RCLogError("Failed to get file size of '%s'", name);
		return ~0;
	}

	if (fileSize > 0xffFFffFFU)
	{
		RCLogError( "Unexpected huge file '%s' found", name);
		return ~0;
	}
	return 0;
}


//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
uint32 CAnimationCompiler::SaveAnimationImageFileAIM(const char* name, FILETIME timeStamp)
{
	CSaverCGF cgfSaver( name, m_ChunkFileAIM);

	uint32 numGAH = CAnimationManager::GetInst().m_arrGlobalAIM.size();
	for (uint32 i=0; i<numGAH; i++)
	{
		GlobalAnimationHeaderAIM& rGAH = CAnimationManager::GetInst().m_arrGlobalAIM[i];

		size_t nSizeOfPoses = rGAH.m_arrAimIKPosesAIM.get_alloc_size();
		uint32 numAimPoses  = rGAH.m_arrAimIKPosesAIM.size();
		for(size_t a=0; a<numAimPoses; ++a)
		{
			nSizeOfPoses += rGAH.m_arrAimIKPosesAIM[a].m_arrRotation.get_alloc_size();
			nSizeOfPoses += rGAH.m_arrAimIKPosesAIM[a].m_arrPosition.get_alloc_size();
		}
		nSizeOfPoses|=3;
		nSizeOfPoses+=1;

		uint32 nChunkSize=sizeof(CHUNK_GAHAIM_INFO)+nSizeOfPoses;
		CHUNK_GAHAIM_INFO* pGAH_Info = (CHUNK_GAHAIM_INFO*)malloc( nChunkSize ); 
		uint8* parrAimPoses = (uint8*)&pGAH_Info->m_numAimPoses;
		SWAP SwapEndianness(*((uint32*)parrAimPoses));   parrAimPoses+=sizeof(pGAH_Info->m_numAimPoses);
		memset( parrAimPoses, 0x20, nSizeOfPoses );

		memset(pGAH_Info->m_FilePath,0,sizeof(pGAH_Info->m_FilePath));
		uint32 num=rGAH.m_FilePath.size();
		for (int32 s=0; s<num; s++)
			pGAH_Info->m_FilePath[s]=rGAH.m_FilePath[s];
		pGAH_Info->m_FilePathCRC32		= rGAH.m_FilePathCRC32;
		SWAP SwapEndianness(pGAH_Info->m_FilePathCRC32);

		pGAH_Info->m_fStartSec				=	rGAH.m_fStartSec;									//asset-feature: Start time in seconds.
		uint32 IsAdditive = pGAH_Info->m_Flags&CA_ASSET_ADDITIVE;
		if (IsAdditive)
			pGAH_Info->m_fStartSec += SECONDS_PER_TICK;
		SWAP SwapEndianness(pGAH_Info->m_fStartSec);
		pGAH_Info->m_fEndSec					=	rGAH.m_fEndSec;										//asset-feature: End time in seconds.
		SWAP SwapEndianness(pGAH_Info->m_fEndSec);
		pGAH_Info->m_fTotalDuration		=	rGAH.m_fEndSec-rGAH.m_fStartSec;	//asset-feature: asset-feature: total duration in seconds.
		SWAP SwapEndianness(pGAH_Info->m_fTotalDuration);

		pGAH_Info->m_AnimTokenCRC32		=	rGAH.m_AnimTokenCRC32;
		SWAP SwapEndianness(pGAH_Info->m_AnimTokenCRC32);

		pGAH_Info->m_nExist		        =	rGAH.m_nExist;
		SWAP SwapEndianness(pGAH_Info->m_nExist);

		pGAH_Info->m_MiddleAimPoseRot =	rGAH.m_MiddleAimPoseRot;
		SWAP SwapEndianness(pGAH_Info->m_MiddleAimPoseRot.w);
		SWAP SwapEndianness(pGAH_Info->m_MiddleAimPoseRot.v.x);
		SWAP SwapEndianness(pGAH_Info->m_MiddleAimPoseRot.v.y);
		SWAP SwapEndianness(pGAH_Info->m_MiddleAimPoseRot.v.z);

		pGAH_Info->m_MiddleAimPose    =	rGAH.m_MiddleAimPose;
		SWAP SwapEndianness(pGAH_Info->m_MiddleAimPose.w);
		SWAP SwapEndianness(pGAH_Info->m_MiddleAimPose.v.x);
		SWAP SwapEndianness(pGAH_Info->m_MiddleAimPose.v.y);
		SWAP SwapEndianness(pGAH_Info->m_MiddleAimPose.v.z);

		for (uint32 v=0; v<(CHUNK_GAHAIM_INFO::XGRID*CHUNK_GAHAIM_INFO::YGRID); v++)
		{
			pGAH_Info->m_PolarGrid[v]=rGAH.m_PolarGrid[v];
			SWAP SwapEndianness(pGAH_Info->m_PolarGrid[v].v0);
			SWAP SwapEndianness(pGAH_Info->m_PolarGrid[v].v1);
			SWAP SwapEndianness(pGAH_Info->m_PolarGrid[v].v2);
			SWAP SwapEndianness(pGAH_Info->m_PolarGrid[v].v3);
		}

		pGAH_Info->m_numAimPoses=numAimPoses;
		SWAP SwapEndianness(pGAH_Info->m_numAimPoses);
		
		for(size_t a=0; a<numAimPoses; ++a)
		{
			uint32 numRot = rGAH.m_arrAimIKPosesAIM[a].m_arrRotation.size();
			*((uint32*)parrAimPoses)=numRot; 
			SWAP SwapEndianness(*((uint32*)parrAimPoses));		parrAimPoses+=sizeof(uint32);
			for (uint32 r=0; r<numRot; r++)
			{
				*((Quat*)parrAimPoses) = rGAH.m_arrAimIKPosesAIM[a].m_arrRotation[r]; 
				SWAP SwapEndianness(*((f32*)parrAimPoses));	parrAimPoses+=sizeof(f32);
				SWAP SwapEndianness(*((f32*)parrAimPoses));	parrAimPoses+=sizeof(f32);
				SWAP SwapEndianness(*((f32*)parrAimPoses));	parrAimPoses+=sizeof(f32);
				SWAP SwapEndianness(*((f32*)parrAimPoses));	parrAimPoses+=sizeof(f32);
			}

			uint32 numPos = rGAH.m_arrAimIKPosesAIM[a].m_arrPosition.size();
			*((uint32*)parrAimPoses)=numPos; 
			SWAP SwapEndianness(*((uint32*)parrAimPoses));		parrAimPoses+=sizeof(uint32);
			for (uint32 p=0; p<numPos; p++)
			{
				*((Vec3*)parrAimPoses) = rGAH.m_arrAimIKPosesAIM[a].m_arrPosition[p]; 
				SWAP SwapEndianness(*((f32*)parrAimPoses));	parrAimPoses+=sizeof(f32);
				SWAP SwapEndianness(*((f32*)parrAimPoses));	parrAimPoses+=sizeof(f32);
				SWAP SwapEndianness(*((f32*)parrAimPoses));	parrAimPoses+=sizeof(f32);
			}
		}


		const char* pFP = rGAH.m_FilePath.c_str();
		RCLog("no: %04d  fname: %s",i, pFP );

		ZeroStruct(pGAH_Info->chdr);
		pGAH_Info->chdr.ChunkType			= ChunkType_GlobalAnimationHeaderAIM;
		pGAH_Info->chdr.ChunkVersion	= CHUNK_GAHAIM_INFO::VERSION;

		CChunkData chunkData;
	//	chunkData.Add( pGAH_Info );
		chunkData.AddData(pGAH_Info,nChunkSize);
		m_ChunkFileAIM.AddChunk( pGAH_Info->chdr, chunkData.data, chunkData.size );

		free(pGAH_Info);
	}


	SetFileAttributes( name,FILE_ATTRIBUTE_ARCHIVE );
	m_ChunkFileAIM.Write( name );

	FileUtil::SetFileTimes(name, timeStamp);
	const __int64 fileSize = FileUtil::GetFileSize(name);
	if (fileSize < 0)
	{
		RCLogError("Failed to get file size of '%s'", name);
		return ~0;
	}

	if (fileSize > 0xffFFffFFU)
	{
		RCLogError( "Unexpected huge file '%s' found", name);
		return ~0;
	}
	return 0;
}

