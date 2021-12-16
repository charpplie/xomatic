/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2010.
-------------------------------------------------------------------------
Description: 
-------------------------------------------------------------------------
History:
- 08:06:2010   : Created by Benjamin Block

*************************************************************************/


#include "StdAfx.h"
#include "CharacterManager.h"
#include "ParamLoader.h"
#include "LoaderCHR.h"

#include "FacialAnimation/FacialModel.h"
#include "FacialAnimation/FaceEffectorLibrary.h"

// gcc only allows function attributes at the definition not the declaration
void Warning(const char* calFileName, EValidatorSeverity severity, const char *format,... ) PRINTF_PARAMS(3, 4);

void Warning(const char* calFileName, EValidatorSeverity severity, const char *format,... )
{
	if (!g_pISystem || !format)
		return;

	va_list	ArgList;
	va_start(ArgList, format);
	g_pISystem->WarningV(VALIDATOR_MODULE_ANIMATION, severity, VALIDATOR_FLAG_FILE, calFileName, format,ArgList);
	va_end(ArgList);
}

namespace
{
	struct FindByAliasName
	{
		uint32 fileCrc;

		bool operator () (const SAnimFile& animFile) const
		{
			return fileCrc == animFile.m_crcAnim;
		}

		bool operator () (const CAnimationSet::FacialAnimationEntry& facialAnimEntry) const
		{
			return fileCrc == facialAnimEntry.crc;
		}
	};
}

CParamLoader::CParamLoader()
{
	m_parsedLists.reserve(256);
}

CParamLoader::~CParamLoader(void)
{
	ClearLists();
}

// for a given list id (of an already loaded list) return all list i
bool CParamLoader::BuildDependencyList(int32 rootListID, DynArray<uint32>&listIDs)
{
	SAnimListInfo& animList = m_parsedLists[rootListID];

	int32 amountDep = animList.dependencies.size();
	for (int32 i = 0; i < amountDep; i++)
	{
		BuildDependencyList(animList.dependencies[i], listIDs);
	}
	listIDs.push_back(rootListID);
	return true;
}

int32 CParamLoader::ListProcessed(const char* paramFileName)
{
	// check if List has already been processed
	int32 nListIDs = m_parsedLists.size();
	for (int32 listID = 0; listID < nListIDs; listID++)
	{
		if (strcmp(m_parsedLists[listID].fileName,paramFileName) == 0)
		{
			return listID;
		}
	}
	return -1;
}

#define MAX_STRING_LENGTH (256)




// finds the first occurence of 
CAF_ID CParamLoader::MemFindFirst( char* strAnimName, const char* szFolder, uint32 crcFolder, int32 folderLenght, CAF_ID nCafID )
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	{
		const char* strFilePath=0;
		uint32 start=0;
		AnimSearchHelper::TIndexVector * vect = g_AnimationManager.m_animSearchHelper.GetAnimationsVector(crcFolder);

		if (vect && nCafID.m_nType==0)
		{
			for (size_t i = nCafID.m_nCafID, end = vect->size(); i < end; ++i) 
			{
				GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[vect->operator [](i)];
				for (uint32 c=folderLenght; c<MAX_STRING_LENGTH; c++)
				{
					if ( szFolder[c]==rCAF.m_FilePath[c] )
						continue;
					if ( szFolder[c]=='*' )
					{
						start=folderLenght;
						for ( ; c<MAX_STRING_LENGTH; c++)
						{
							if ( rCAF.m_FilePath[c]=='/' )
								break;
							if ( rCAF.m_FilePath[c]==0 )
							{
								strFilePath=rCAF.m_FilePath.c_str();
								break;
							}
						}
					}
					break;
				}

				if (strFilePath) 
				{
					strcpy(&strAnimName[0],&rCAF.m_FilePath[start]);
					return nCafID;
				}
			}
		}
	}

	//-------------------------------------------------------------------

	{
		if (nCafID.m_nType==0)
			nCafID.m_nCafID=0;

		const char* strFilePath=0;
		uint32 start=0;
		int32 numAIM = int32(g_AnimationManager.m_arrGlobalAIM.size());
		for ( ; nCafID.m_nCafID<numAIM; nCafID.m_nCafID++)
		{
			GlobalAnimationHeaderAIM& rAIM = g_AnimationManager.m_arrGlobalAIM[nCafID.m_nCafID];
			for (uint32 c=0; c<MAX_STRING_LENGTH; c++)
			{
				if ( szFolder[c]==rAIM.m_FilePath[c] )
					continue;
				if ( szFolder[c]=='*' )
				{
					start=folderLenght;
					for ( ; c<MAX_STRING_LENGTH; c++)
					{
						if ( rAIM.m_FilePath[c]=='/' )
							break;
						if ( rAIM.m_FilePath[c]==0 )
						{
							strFilePath=rAIM.m_FilePath.c_str();
							break;
						}
					}
				}
				break;
			}

			if (strFilePath) 
			{
				strcpy(&strAnimName[0],&rAIM.m_FilePath[start]);
				nCafID.m_nType=1;
				return nCafID;
			}
		}
	}

	return CAF_ID(-1,-1);
}


bool CParamLoader::ExpandWildcards(uint32 listID, const bool parseSubfolders = false)
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	CCharacterModel *pModel = m_pModel;
	SAnimListInfo& animList = m_parsedLists[listID];

	uint32 numAnims = animList.arrAnimFiles.size();
	if (numAnims==0)
		return 0;

	int nNullLen = strlen(NULL_ANIM_FILE);

	// insert all animations found in the DBA files
	uint32 numDBALoaders = g_AnimationManager.m_arrGlobalHeaderDBA.size();
	for ( uint32 j=0; j<numDBALoaders; j++ )
	{
		CGlobalHeaderDBA& rGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[j];
		if (rGlobalHeaderDBA.m_pDatabaseInfo==0)
		{
			const char* pName  = rGlobalHeaderDBA.m_strFilePathDBA;
			uint32 nDBACRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(pName); 
			uint32 numCAF = g_AnimationManager.m_arrGlobalCAF.size();
			uint32 nFoundAssetsInAIF=0;
			for (uint32 c=0; c<numCAF; c++)
			{
				if (g_AnimationManager.m_arrGlobalCAF[c].m_FilePathDBACRC32==nDBACRC32)
				{
					nFoundAssetsInAIF=1;
					break;
				}
			}
			if (nFoundAssetsInAIF==0)
			{
				rGlobalHeaderDBA.LoadDatabaseDBA(pModel->GetFilePath().c_str());
				if (rGlobalHeaderDBA.m_pDatabaseInfo==0)
				{
					continue;
				}
			}
		}
	}

	//----------------------------------------------------------------------------------------
	// expand wildcards
	uint32 numWildcard = animList.arrWildcardAnimFiles.size();
	for ( uint32 w=0; w<numWildcard; ++w )
	{
		const char* szFolder = animList.arrWildcardAnimFiles[w].m_FilePathQ;
		const char* szFile = PathUtil::GetFile(szFolder);
		const char* szExt = PathUtil::GetExt(szFolder);


		stack_string str = PathUtil::GetPath(szFolder);
		PathUtil::ToUnixPath(str);
		uint32 crcFolder = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(str);
		uint32 szFolderLen = str.length();

		uint32 IsLMG=strcmp(szExt,"lmg")==0;
		uint32 IsFSQ=strcmp(szExt,"fsq")==0;

		//	uint32 identical=strcmp(szFolder,"animations/human/male/lmg/stand/*.lmg")==0;
		//	if (identical)
		//	{
		//		uint32 ddd=0;
		//	}

		char szAnimName[MAX_STRING_LENGTH]; strcpy( szAnimName, animList.arrWildcardAnimFiles[w].m_AnimNameQ );

		const char* firstWildcard = strchr(szFolder,'*');
		if ( const char* qm = strchr(szFolder,'?') )
			if ( firstWildcard == NULL || firstWildcard > qm )
				firstWildcard = qm;
		int32 iFirstWildcard = (int32)(firstWildcard - szFolder);
		int32 iPathLength = (int32)(szFile - szFolder);

		// conversion to offset from beginning of name
		int32 offset = (int32)(firstWildcard - szFile);

		stack_string filepath = PathUtil::GetParentDirectoryStackString(stack_string(szFolder));
		char* starPos = strchr( szAnimName, '*' );
		if ( starPos )
			*starPos++ = 0;

		char strAnimName[MAX_STRING_LENGTH]; 
		uint32 img=g_pCharacterManager->IsInitializedByIMG();
		if (img&2  && IsLMG==0 && IsFSQ==0)
		{
			memset(strAnimName,0,sizeof(strAnimName));
			CAF_ID nCafID =  MemFindFirst(strAnimName,szFolder, crcFolder, szFolderLen, CAF_ID(0,0));
			if ( nCafID.m_nCafID != -1 )
			{
				do
				{
					uint32 d=0;
					uint32 e=0;
					char strMemBuffer[MAX_STRING_LENGTH];
					memset(strMemBuffer, 0, sizeof(strMemBuffer));

					//assert(offset==0);
					//stack_string strMemBuffer = (starPos == NULL) ? szAnimName : stack_string(szAnimName) + PathUtil::GetFileName(fd.name+offset) + starPos;
					//assert( strcmp(mname,m_StrMemBuffer)==0);
					//CryStringUtils::UnifyFilePath(name);

					if (starPos == NULL)
					{
						strcpy_s( strMemBuffer,szAnimName );
					}
					else
					{

						for (uint32 i=0; i<sizeof(strMemBuffer); i++)
						{
							if (szAnimName[i]==0) break;
							strMemBuffer[d++]=szAnimName[i];
						}

						char m_StrFile[MAX_STRING_LENGTH];
						memset( m_StrFile,0,sizeof(m_StrFile) );
						for (uint32 i=offset; i<sizeof(m_StrFile); i++)
						{
							if (strAnimName[i]=='.') break;
							m_StrFile[e++]=strAnimName[i];
						}
						char path_buffer[MAX_STRING_LENGTH];
						char fname[_MAX_FNAME];
						char ext[_MAX_EXT];
						_splitpath( m_StrFile,0,0,fname,ext );
						_makepath( path_buffer,0,0,fname,ext );
						for (uint32 i=0; i<sizeof(strMemBuffer)-1; i++)
						{
							if (path_buffer[i]==0) break;
							strMemBuffer[d++]=path_buffer[i];
						}

						for (uint32 i=0; i<sizeof(strMemBuffer)-1; i++)
						{
							if (starPos[i]==0) break;
							strMemBuffer[d++]=starPos[i];
						}
					}

					FindByAliasName pd; 
					pd.fileCrc =  gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(strMemBuffer);

					// Check whether the filename is a facial animation, by checking the extension.
					const char* szExtension = PathUtil::GetExt(strAnimName);
					// insert unique
					stack_string fpath = filepath + stack_string("/") + strAnimName, stack_string(strMemBuffer);
					AddIfNewAnimationAlias(animList, strMemBuffer, fpath);

					memset(strAnimName,0,sizeof(strAnimName));
					nCafID.m_nCafID++;
					nCafID = MemFindFirst(strAnimName,szFolder, crcFolder, szFolderLen, nCafID);
				} while (nCafID.m_nCafID>=0);
			}
		} 
		else 
		{ 
			//extend the files from disk
			_finddata_t fd;
			intptr_t handle =  g_pIPak->FindFirst( szFolder, &fd, ICryPak::FLAGS_NO_LOWCASE );
			if ( handle != -1 )
			{
				do
				{
					uint32 d=0;
					uint32 e=0;
					char strMemBuffer[MAX_STRING_LENGTH];
					memset(strMemBuffer, 0, sizeof(strMemBuffer));

					//assert(offset==0);
					//stack_string strMemBuffer = (starPos == NULL) ? szAnimName : stack_string(szAnimName) + PathUtil::GetFileName(fd.name+offset) + starPos;
					//assert( strcmp(mname,m_StrMemBuffer)==0);
					//CryStringUtils::UnifyFilePath(name);

					if (starPos == NULL)
					{
						strcpy_s( strMemBuffer,szAnimName );
					}
					else
					{
						for (uint32 i=0; i<sizeof(strMemBuffer); i++)
						{
							if (szAnimName[i]==0) break;
							strMemBuffer[d++]=szAnimName[i];
						}

						char m_StrFile[MAX_STRING_LENGTH];
						memset( m_StrFile,0,sizeof(m_StrFile) );
						for (uint32 i=offset; i<sizeof(m_StrFile)-1; i++)
						{
							if (fd.name[i]=='.') break;
							m_StrFile[e++]=fd.name[i];
						}

						char path_buffer[MAX_STRING_LENGTH];
						char fname[_MAX_FNAME];
						char ext[_MAX_EXT];
						_splitpath( m_StrFile,0,0,fname,ext );
						_makepath( path_buffer,0,0,fname,ext );
						for (uint32 i=0; i<sizeof(strMemBuffer)-1; i++)
						{
							if (path_buffer[i]==0) break;
							strMemBuffer[d++]=path_buffer[i];
						}

						for (uint32 i=0; i<sizeof(strMemBuffer); i++)
						{
							if (starPos[i]==0) break;
							strMemBuffer[d++]=starPos[i];
						}
					}

					FindByAliasName pd; 
					pd.fileCrc = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(strMemBuffer);

					// Check whether the filename is a facial animation, by checking the extension.
					const char* szExtension = PathUtil::GetExt(fd.name);
					if ( szExtension!=0 && stricmp("fsq", szExtension) == 0 )
					{
						// insert unique
						AddIfNewFacialAnimationAlias(animList, strMemBuffer, filepath+"/"+fd.name);
					}
					else
					{
						stack_string fpath = filepath + stack_string("/") + fd.name, stack_string(strMemBuffer);
						AddIfNewAnimationAlias(animList, strMemBuffer, fpath);
					}
				} while (g_pIPak->FindNext( handle, &fd )>=0);

				g_pIPak->FindClose( handle );
			}


		}




		//-----------------------------------------------------------------



		if (Console::GetInst().ca_UseIMG_CAF==0)
		{
			// insert all animations found in the DBA files
			for ( uint32 j=0; j<numDBALoaders; j++ )
			{

				CGlobalHeaderDBA& rGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[j];
				if (rGlobalHeaderDBA.m_pDatabaseInfo==0)
				{
					const char* pName  = rGlobalHeaderDBA.m_strFilePathDBA;
					uint32 nDBACRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(pName); 
					uint32 numCAF = g_AnimationManager.m_arrGlobalCAF.size();
					uint32 nFoundAssetsInAIF=0;
					for (uint32 c=0; c<numCAF; c++)
					{
						if (g_AnimationManager.m_arrGlobalCAF[c].m_FilePathDBACRC32==nDBACRC32)
						{
							nFoundAssetsInAIF++;

							const char* currentFile = g_AnimationManager.m_arrGlobalCAF[c].m_FilePath.c_str();
							const char* file = PathUtil::GetFile( currentFile );
							const char* ext  = PathUtil::GetExt( currentFile );

							bool match1 = strnicmp(szFolder, currentFile, iPathLength) == 0;
							bool match2 = PathUtil::MatchWildcard(file, szFile);
							if ( match1 && match2 )
							{
								stack_string folderPathCurrentFile = PathUtil::GetParentDirectoryStackString( stack_string(currentFile) );
								stack_string folderPathFileName    = PathUtil::GetParentDirectoryStackString( stack_string(szFolder)  );

								if( parseSubfolders || (!parseSubfolders && folderPathCurrentFile == folderPathFileName) )
								{
									stack_string name = "";
									if( starPos==NULL )
										name = szAnimName;
									else
									{
										stack_string sa = 	stack_string(szAnimName) + stack_string(currentFile+iFirstWildcard, ext-currentFile-1-iFirstWildcard) + starPos;
										name = sa;
									}

									AddIfNewAnimationAlias(animList, name.c_str(), currentFile);
								}
							}
						}
					}

					if (nFoundAssetsInAIF==0)
					{
						rGlobalHeaderDBA.LoadDatabaseDBA(pModel->GetModelFilePath());
						if (rGlobalHeaderDBA.m_pDatabaseInfo==0)
						{
							continue;
						}
					}
				}

				DynArray<string> arrDBAPathNames;
				if (rGlobalHeaderDBA.m_pDatabaseInfo)
				{
					uint32 numCAF=g_AnimationManager.m_arrGlobalCAF.size();
					for (uint32 c=0; c<numCAF; c++)
					{
						if (g_AnimationManager.m_arrGlobalCAF[c].m_FilePathDBACRC32==rGlobalHeaderDBA.m_FilePathDBACRC32)
							arrDBAPathNames.push_back(g_AnimationManager.m_arrGlobalCAF[c].GetFilePath());
					}
				}

				uint32 numCafFiles=arrDBAPathNames.size();
				for (uint32 f=0; f<numCafFiles; f++)
				{
					stack_string currentFile = arrDBAPathNames[f].c_str();
					CryStringUtils::UnifyFilePath(currentFile);

					const char* file = PathUtil::GetFile( currentFile.c_str() );
					const char* ext  = PathUtil::GetExt( currentFile.c_str() );
					if ( strnicmp(szFolder, currentFile, iPathLength) == 0 && PathUtil::MatchWildcard(file, szFile) )
					{
						stack_string folderPathCurrentFile = PathUtil::GetParentDirectory(currentFile);
						stack_string folderPathFileName = PathUtil::GetParentDirectory(szFolder);

						if( parseSubfolders || (!parseSubfolders && folderPathCurrentFile == folderPathFileName) )
						{
							stack_string name = "";
							if( starPos==NULL )
								name = szAnimName;
							else
							{
								stack_string sa = 	stack_string(szAnimName) + stack_string(currentFile.c_str()+iFirstWildcard, ext-currentFile-1-iFirstWildcard) + starPos;
								name = sa;
							}

							FindByAliasName pd;
							pd.fileCrc = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(name);

							AddIfNewAnimationAlias(animList, name.c_str(), currentFile);
						}
					}
				}

			}
		}
	}

	//	return 0;
	animList.PrepareHeaderList();
	return true;
}


// check if a list has already been parsed and is in memory, if not, parse it and load it into m_parsedLists
int32 CParamLoader::LoadAnimList(const XmlNodeRef calNode, const char* paramFileName, string strAnimDirName, bool& parseSubFolders)
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	if (!calNode)
		return -1;

	// check if List has already been processed
	int32 nListIDs = m_parsedLists.size();

	int32 newID = ListProcessed(paramFileName);
	if (newID != -1)
		return newID;

	SAnimListInfo animList(paramFileName);
	animList.arrAnimFiles.push_back( SAnimFile(stack_string(NULL_ANIM_FILE "/") + m_pModel->GetNameCStr(), "null"));


	parseSubFolders = false;

	const int BITE = 512;

	// [Alexey} - TODO! FIXME!!! FIXME!!!
	// We have 50 from 100 identical file parsing!!!
	//total++;

	//stl::hash_map<string, int, stl::hash_stricmp<string>>::iterator it = m_checkMap.find(string(calFileName));
	//if (it != m_checkMap.end())
	//{
	//	it->second = it->second + 1;
	//	int a = 0;
	//	identical++;
	//	Warning(calFileName, VALIDATOR_ERROR, "Identical cal readings %i from %i", identical, total);
	//} else
	//	m_checkMap[string(calFileName)] = 0;

	uint32 count = calNode->getChildCount();
	for (uint32 i=0; i<count; ++i)
	{

		XmlNodeRef assignmentNode = calNode->getChild(i);

		if (stricmp(assignmentNode->getTag(), "Animation"))
			continue;

		CryFixedStringT<BITE> line;

		CryFixedStringT<BITE> key;
		CryFixedStringT<BITE> value;	

		key.append(assignmentNode->getAttr("name"));
		line.append(assignmentNode->getAttr("path"));

		int32 pos = 0;
		value = line.Tokenize(" \t\n\r=", pos);

		// now only needed for aim poses
		char buffer[BITE] = "";
		strcpy(buffer, line.c_str());


		if (value.empty() || value.at(0) == '?')			
		{
			continue;
		}

		if (0 == stricmp(key, "#filepath"))
		{
			strAnimDirName = PathUtil::ToUnixPath(value.c_str());
			strAnimDirName.TrimRight('/'); // delete the trailing slashes			
			continue;
		}

		if(0 == stricmp(key, "#ParseSubFolders"))
		{
			parseSubFolders = stricmp (value, "true") == 0? true:false; // if it's false, stricmp return 0
		}

		// remove first '\' and replace '\' with '/'
		value.replace('\\', '/');
		value.TrimLeft('/');		


		// process the possible directives
		if (key.at(0) == '$')
		{
			if (!stricmp(key, "$AnimationDir") || !stricmp(key, "$AnimDir")	|| !stricmp(key, "$AnimationDirectory") ||!stricmp(key, "$AnimDirectory"))
			{
				Warning(paramFileName, VALIDATOR_ERROR, "Deprecated directive \"%s\"", key.c_str());
			}
			else if (!stricmp(key, "$AnimEventDatabase"))
			{
				if (animList.animEventDatabase.empty())
					animList.animEventDatabase = value.c_str();
				//				else
				//					Warning(calFileName, VALIDATOR_WARNING, "Failed to set animation event database \"%s\". Animation event database is already set to \"%s\"", value.c_str(), m_animEventDatabase.c_str());
			}
			else if (!stricmp(key, "$TracksDatabase"))
			{
				if (!AddIfNewModelTracksDatabase(animList, value.c_str()))
					Warning(paramFileName, VALIDATOR_WARNING, "Duplicate model tracks database declared \"%s\"", value.c_str());

				CryFixedStringT<BITE> flags;
				flags.append(assignmentNode->getAttr("flags"));

				// flag handling
				if (!flags.empty())
				{
					if (strstr(flags.c_str(), "persistent"))
					{
						animList.lockedDatabases.push_back(value.c_str());
					}
				}

			}
			else if (!stricmp(key, "$Include"))
			{
				int32 listID = ListProcessed(value.c_str());

				if (listID == -1)
				{


					bool dummy; // will always use the top-level options for "parseSubFolders"

					// load the new params file, but only parse the AnimationList section
					XmlNodeRef topRoot		= g_pISystem->LoadXmlFromFile(value.c_str());

					if (topRoot)
					{
						const char* nodeTag = topRoot->getTag();
						if (stricmp(nodeTag, "Params") == 0)
						{
							int32 numChildren = topRoot->getChildCount();
							for (int32 iChild=0; iChild < topRoot->getChildCount(); ++iChild)
							{
								XmlNodeRef node = topRoot->getChild(iChild);

								const char* newNodeTag = node->getTag();
								if (stricmp(newNodeTag, "AnimationList")==0)
								{
									listID = LoadAnimList(node, value, strAnimDirName, dummy);
								}
							}
						}
					}	
				}

				// we found a new dependency, add it if it is not already in
				if (listID >= 0)
				{
					if (std::find(animList.dependencies.begin(), animList.dependencies.end(), listID) == animList.dependencies.end())
						animList.dependencies.push_back(listID);										
				}

			}
			else if (!stricmp(key, "$FaceLib"))
			{
				if (animList.faceLibFile.empty())
				{
					animList.faceLibFile = value.c_str();
					animList.faceLibDir = strAnimDirName;
				}
				else
					Warning(paramFileName, VALIDATOR_WARNING, "Failed to set face lib \"%s\". Face lib is already set to \"%s\"", value.c_str(), animList.faceLibFile.c_str());
			}
			else
				Warning(paramFileName, VALIDATOR_ERROR, "Unknown directive in '%s'", key.c_str());
		}
		else
		{			
			// Check whether the filename is a facial animation, by checking the extension.
			const char* szExtension = PathUtil::GetExt(value);
			stack_string szFileName;
			szFileName.Format("%s/%s", strAnimDirName.c_str(), value.c_str());

			// is there any wildcard in the file name?
			if ( strchr(value,'*') != NULL || strchr(value,'?') != NULL )
			{
				animList.arrWildcardAnimFiles.push_back( SAnimFile(szFileName, key) );
			}
			else
			{
				const char* failedToCreateAlias = "Failed to create animation alias \"%s\" for file \"%s\". Such alias already exists.\"";
				if (szExtension!=0 && stricmp("fsq", szExtension) == 0)
				{
					bool added = AddIfNewFacialAnimationAlias(animList, key, szFileName.c_str());
					if (!added)
						if (Console::GetInst().ca_AnimWarningLevel > 2)
							Warning(paramFileName, VALIDATOR_WARNING, failedToCreateAlias, key.c_str(), szFileName.c_str());
				}
				else
				{
					bool added = AddIfNewAnimationAlias(animList, key, szFileName.c_str());
					if (!added)
						if (Console::GetInst().ca_AnimWarningLevel > 2)
							Warning(paramFileName, VALIDATOR_WARNING, failedToCreateAlias, key.c_str(), szFileName.c_str());
				}
			}

			//check if CAF or LMG has an Aim-Pose
			uint32 numAnims = animList.arrAnimFiles.size();
			if (szExtension!=0 && numAnims)
			{
				if ( (stricmp("lmg",szExtension)==0) || (stricmp("caf",szExtension)==0) )
				{
					//PREFAST_SUPPRESS_WARNING(6031) strtok (buffer, " \t\n\r=");
					const char *fname = strtok(buffer, " \t\n\r=(");
					while ( (fname = strtok(NULL, " \t\n\r,()")) != NULL && fname[0] != 0 )
					{
						if ( fname[0] == '/' )
							break;
						animList.arrAnimFiles.back().m_arrAimPoses.push_back( fname );
					}
				}
			}
		}
	} 

	//end of loop
	m_parsedLists.push_back(animList);
	return m_parsedLists.size()-1;
}

bool CParamLoader::AddIfNewAnimationAlias( SAnimListInfo& animList, const char* animName, const char* szFileName)
{
	if (NoAnimationAliasInDependencies(animList, animName))
	{
		animList.arrAnimFiles.push_back( SAnimFile( szFileName, animName ));
		return true;
	}
	return false;
}

bool CParamLoader::NoAnimationAliasInDependencies(SAnimListInfo& animList, const char* animName)
{
	FindByAliasName pd; 
	pd.fileCrc =  gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(animName);
	if (std::find_if(animList.arrAnimFiles.begin(), animList.arrAnimFiles.end(), pd) != animList.arrAnimFiles.end())
		return false;

	uint32 numDeps = animList.dependencies.size();
	for (uint32 i = 0; i < numDeps; i++)
	{
		SAnimListInfo& childList = m_parsedLists[animList.dependencies[i]];

		if(!NoAnimationAliasInDependencies(childList, animName))
			return false;
	}
	return true;
}


bool CParamLoader::AddIfNewFacialAnimationAlias( SAnimListInfo& animList,  const char* animName, const char* szFileName)
{	
	if (NoFacialAnimationAliasInDependencies(animList, animName))
	{
		animList.facialAnimations.push_back(CAnimationSet::FacialAnimationEntry(animName, szFileName));
		return true;
	}

	return false;
};

bool CParamLoader::NoModelTracksDatabaseInDependencies(SAnimListInfo&animList, const char* dataBase)
{
	DynArray<string>& mTB = animList.modelTracksDatabases;

	uint32 numDBA=mTB.size();
	if (std::find(mTB.begin(), mTB.end(), dataBase) != mTB.end())
	{
		return false;
	}

	uint32 numDeps = animList.dependencies.size();
	for (uint32 i = 0; i < numDeps; i++)
	{
		SAnimListInfo& childList = m_parsedLists[animList.dependencies[i]];
		if(!NoModelTracksDatabaseInDependencies(childList, dataBase))
			return false;
	}

	return true;	
}

bool CParamLoader::AddIfNewModelTracksDatabase(SAnimListInfo& animList, const char* dataBase)
{

	stack_string tmp=dataBase;
	CryStringUtils::UnifyFilePath(tmp);
	if (NoModelTracksDatabaseInDependencies(animList, tmp.c_str()))
	{
		animList.modelTracksDatabases.push_back(tmp);
		return true;
	}
	else
		return false;	
};

bool CParamLoader::NoFacialAnimationAliasInDependencies(SAnimListInfo& animList, const char* animName)
{
	for (uint32 i = 0; i < animList.facialAnimations.size(); ++i)
		if (0 == stricmp(animList.facialAnimations[i], animName))
		{
			return false;
		}

		uint32 numDeps = animList.dependencies.size();
		for (uint32 i = 0; i < numDeps; i++)
		{
			SAnimListInfo& childList = m_parsedLists[animList.dependencies[i]];
			if(!NoFacialAnimationAliasInDependencies(childList, animName))
				return false;
		}
		return true;
}

bool CParamLoader::LoadXML(CCharacterModel *pModel, string defaultAnimDir, const char* const paramFileName, uint32 isSkinAttachment, DynArray<uint32>& listIDs)
{
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	m_pModel = pModel;
	m_defaultAnimDir = defaultAnimDir;

	XmlNodeRef topRoot = g_pISystem->LoadXmlFromFile(paramFileName);

	if (!topRoot)
	{
		return false;
		const char* nodeTag = topRoot->getTag();
		if (!stricmp(nodeTag, "Params"))
			return false;
	}

	uint32 numChildren = topRoot->getChildCount();
	for (uint32 i=0; i<numChildren; ++i)
	{
		XmlNodeRef node = topRoot->getChild(i);

		const char* nodeTag = node->getTag();

		if (stricmp(nodeTag, "Lod")==0)
		{

			if(!LoadLod(node))
				return false;
		}

		if (stricmp(nodeTag, "IK_Definition")==0)
		{
			if(!ParseIKDef(node))
				return false;
		}

		if (stricmp(nodeTag, "AnimationList")==0)
		{
			if (isSkinAttachment==0)
			{
				bool bParseSubFolders = false;

				int32 rootNode = LoadAnimList(node, paramFileName, m_defaultAnimDir, bParseSubFolders);

				BuildDependencyList(rootNode, listIDs);
				if (rootNode < 0)
				{
					g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, paramFileName, "Failed to load animations: %s", paramFileName);
					return 0;
				}
			}

		}
	}
	return true;
}	


bool CParamLoader::LoadLod(const XmlNodeRef lodNode)
{
	if (!lodNode)	
		return false;

	XmlNodeRef lod = lodNode;

	uint32 count = lod->getChildCount();
	for (uint32 i=0; i<count; ++i)
	{

		XmlNodeRef jointList = lod->getChild(i);
		if (!jointList->isTag("JointList"))
			continue;

		uint32 level;
		if (!jointList->getAttr("level", level))
			continue;

		DynArray<uint32> jointMask;
		uint32 jointCount = jointList->getChildCount();
		for (uint32 j=0; j<jointCount; ++j)
		{
			XmlNodeRef joint = jointList->getChild(j);
			if (!joint->isTag("Joint"))
				continue;

			const char* name = joint->getAttr("name");
			if (!name)
				continue;

			uint32 nameCrc32 = g_pCrc32Gen->GetCRC32(name);
			jointMask.push_back(nameCrc32);
		}

		if (!jointMask.empty())
		{
			std::sort(jointMask.begin(), jointMask.end());
			m_pModel->m_arrAnimationLOD.push_back()->swap(jointMask);
		}
	}

	return true;
}



bool CParamLoader::ParseIKDef( const XmlNodeRef ikNode)
{
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;

	if (ikNode)
	{
		//use XML file
		const char* TopRootXMLTAG = ikNode->getTag();
		if (stricmp(TopRootXMLTAG,"IK_Definition")==0) 
		{

			uint32 numIKTypes = ikNode->getChildCount();
			for (uint32 iktype=0; iktype<numIKTypes; iktype++) 
			{
				XmlNodeRef IKNode = ikNode->getChild(iktype);

				const char* IKTypeTAG = IKNode->getTag();

				//-----------------------------------------------------------
				//check LimbIK-Setup  
				//-----------------------------------------------------------
				if (stricmp(IKTypeTAG,"LimbIK_Definition")==0) 
				{
					if(!LoadIKDefLimbIK(IKNode))
						return false;
				}

				//-----------------------------------------------------------
				// check IKTarget-Character Setup  
				//-----------------------------------------------------------
				if (stricmp(IKTypeTAG,"IKTarget_CharacterSetup")==0) 
				{
					if(!LoadIKTargetSetup(IKNode))
						return false;
				}

				//-----------------------------------------------------------
				//check Animation-Driven IK-Targets  
				//-----------------------------------------------------------
				if (stricmp(IKTypeTAG,"Animation_Driven_IK_Targets")==0) 
					LoadIKDefAnimDrivenIKTargets(IKNode);

				//-----------------------------------------------------------
				//check FeetLock pose-modifier  
				//-----------------------------------------------------------
				if (stricmp(IKTypeTAG,"FeetLock_Definition")==0) 
					LoadIKDefFeetLock(IKNode);

				//-----------------------------------------------------------
				//check Recoil-Setup  
				//-----------------------------------------------------------
				if (stricmp(IKTypeTAG,"Recoil_Definition")==0) 
					LoadIKDefRecoil(IKNode);

				//-----------------------------------------------------------
				//check LookIK-Setup  
				//-----------------------------------------------------------
				if (stricmp(IKTypeTAG,"LookIK_Definition")==0)
				{ 
					LoadIKDefLookIK(IKNode);
					uint32 numRot = pModelSkeleton->m_LookIK_Rot.size();	
					uint32 numPos = pModelSkeleton->m_LookIK_Pos.size();	
					for (uint32 r=0; r<numRot; r++)
					{
						for (uint32 p=0; p<numPos; p++)
						{
							const char* pRotName = pModelSkeleton->m_LookIK_Rot[r].m_strJointName;
							if (pRotName==0)
								continue;
							const char* pPosName = pModelSkeleton->m_LookIK_Pos[p].m_strJointName;
							if (pPosName==0)
								continue;
							uint32 SameJoint = strcmp(pRotName,pPosName)==0;
							if (SameJoint)
							{
								pModelSkeleton->m_LookIK_Rot[r].m_nPosIndex = p;
								break;
							}
						}
					}
				}
				//-----------------------------------------------------------
				//check AimIK-Setup  
				//-----------------------------------------------------------
				if (stricmp(IKTypeTAG,"AimIK_Definition")==0) 
				{
					LoadIKDefAimIK(IKNode);
					uint32 numRot = pModelSkeleton->m_AimIK_Rot.size();	
					uint32 numPos = pModelSkeleton->m_AimIK_Pos.size();	
					for (uint32 r=0; r<numRot; r++)
					{
						for (uint32 p=0; p<numPos; p++)
						{
							const char* pRotName = pModelSkeleton->m_AimIK_Rot[r].m_strJointName;
							if (pRotName==0)
								continue;
							const char* pPosName = pModelSkeleton->m_AimIK_Pos[p].m_strJointName;
							if (pPosName==0)
								continue;
							uint32 SameJoint = strcmp(pRotName,pPosName)==0;
							if (SameJoint)
							{
								pModelSkeleton->m_AimIK_Rot[r].m_nPosIndex = p;
								break;
							}
						}
					}
				}

			}
		}
	}



	return true;
}

void CParamLoader::ClearLists()
{
	int32 n = m_parsedLists.size();
	for (int32 i = 0; i < n; i++)
	{
		stl::free_container(m_parsedLists[i].arrWildcardAnimFiles);
		stl::free_container(m_parsedLists[i].arrAnimFiles);
		stl::free_container(m_parsedLists[i].modelTracksDatabases);
		stl::free_container(m_parsedLists[i].facialAnimations);
	}
	stl::free_container(m_parsedLists);
}


bool CParamLoader::LoadIKTargetSetup( XmlNodeRef IKNode )
{
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;

	uint32 num = IKNode->getChildCount();
	for (uint32 i=0; i<num; i++) 
	{
		//pModelSkeleton->m_charIKTargetData
		XmlNodeRef nodePos = IKNode->getChild(i);
		const char* PosTag = nodePos->getTag();

		if (stricmp(PosTag, "SpineSetup") == 0) 
		{
			if (nodePos->haveAttr("IKHandle"))
			{
				pModelSkeleton->m_charIKTargetData->m_spineHandleHash = *((uint64*)nodePos->getAttr("IKHandle"));
			}
			if (nodePos->haveAttr("spineLength"))
			{
				nodePos->getAttr("spineLength", pModelSkeleton->m_charIKTargetData->m_spineLength);
			}
		}

		if (stricmp(PosTag, "DefaultSetup") == 0) 
		{
			if (nodePos->haveAttr("armLength"))
			{
				nodePos->getAttr("armLength", pModelSkeleton->m_charIKTargetData->m_armLengthDefault);
			}
			if (nodePos->haveAttr("zEffPullback"))
			{
				nodePos->getAttr("zEffPullback", pModelSkeleton->m_charIKTargetData->m_endEffPullbackDefault.z);
			}
			if (nodePos->haveAttr("yEffPullback"))
			{
				nodePos->getAttr("yEffPullback", pModelSkeleton->m_charIKTargetData->m_endEffPullbackDefault.y);
			}
			if (nodePos->haveAttr("xEffPullback"))
			{
				nodePos->getAttr("xEffPullback", pModelSkeleton->m_charIKTargetData->m_endEffPullbackDefault.x);
			}
		}

		if (stricmp(PosTag, "LimbSetup") == 0) 
		{
			if (nodePos->haveAttr("IKHandle"))
			{
				uint64 ikHandle = *((uint64*)nodePos->getAttr("IKHandle"));
				Vec3 pullBack = pModelSkeleton->m_charIKTargetData->m_endEffPullbackDefault;
				float tempVal = 0;
				if (nodePos->haveAttr("armLength"))
				{
					nodePos->getAttr("armLength", tempVal);
					pModelSkeleton->m_charIKTargetData->m_armLengthValues[ikHandle] = tempVal;
				}
				if (nodePos->haveAttr("animLayer"))
				{
					int animLayer;
					nodePos->getAttr("animLayer", animLayer);
					pModelSkeleton->m_charIKTargetData->m_animLayers[ikHandle] = (int16)animLayer;
				}
				if (nodePos->haveAttr("zEffPullback"))
				{
					nodePos->getAttr("zEffPullback", tempVal);
					pullBack.z = tempVal;
				}
				if (nodePos->haveAttr("yEffPullback"))
				{
					nodePos->getAttr("yEffPullback", tempVal);
					pullBack.y = tempVal;
				}
				if (nodePos->haveAttr("xEffPullback"))
				{
					nodePos->getAttr("xEffPullback", tempVal);
					pullBack.x = tempVal;
				}
				pModelSkeleton->m_charIKTargetData->m_endEffPullback[ikHandle] = pullBack;
			}
		}
	}

	return true;
}


bool CParamLoader::LoadIKDefLimbIK( const XmlNodeRef limbNode)
{
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;

	uint32 num = limbNode->getChildCount();
	for (uint32 likdef=0; likdef<num; likdef++) 
	{
		IKLimbType IKLimb;
		XmlNodeRef nodePos = limbNode->getChild(likdef);
		const char* PosTag = nodePos->getTag();
		if (stricmp(PosTag,"IK")==0) 
		{
			const char* pHandle = nodePos->getAttr( "Handle" );
			if (pHandle==0)
				continue;
			IKLimb.m_nHandle = *(uint64*)pHandle;

			const char* pSolver = nodePos->getAttr( "Solver" );
			if (pSolver==0)
				continue;

			IKLimb.m_nSolver = *(uint32*)pSolver;

			//-----------------------------------------------------------------------------------

			if (IKLimb.m_nSolver==*(uint32*)"2BIK")  //check the Solver
			{
				const char* pRoot = nodePos->getAttr( "Root" );
				if (pRoot==0)
					continue;
				int32 nRootIdx = pModelSkeleton->GetJointIDByName(pRoot);
				if (nRootIdx<0)
					continue;

				const char* pEndEffector = nodePos->getAttr( "EndEffector" );
				if (pEndEffector==0)
					continue;
				int32 nEndEffectorIdx = pModelSkeleton->GetJointIDByName(pEndEffector);
				if (nEndEffectorIdx<0)
					continue;

				assert(nRootIdx<nEndEffectorIdx);
				if (nRootIdx>=nEndEffectorIdx)
					continue;


				int32 nBaseRootIdx = pModelSkeleton->GetJointParentIDByID(nRootIdx);
				if (nBaseRootIdx<0)
					continue;
				int32 nMiddleIdx = pModelSkeleton->GetJointParentIDByID(nEndEffectorIdx);
				if (nMiddleIdx<0)
					continue;
				//do a simple verification
				int32 idx  = pModelSkeleton->GetJointParentIDByID(nMiddleIdx);
				const char* strRoot  = pModelSkeleton->GetJointNameByID(idx);
				if (idx != nRootIdx)
					continue;

				IKLimb.m_arrJointChain.resize(4);
				IKLimb.m_arrJointChain[0].m_idxJoint = nBaseRootIdx;
				IKLimb.m_arrJointChain[0].m_strJoint = pModelSkeleton->GetJointNameByID(nBaseRootIdx);
				IKLimb.m_arrJointChain[1].m_idxJoint = nRootIdx;
				IKLimb.m_arrJointChain[1].m_strJoint = pModelSkeleton->GetJointNameByID(nRootIdx);
				IKLimb.m_arrJointChain[2].m_idxJoint = nMiddleIdx;
				IKLimb.m_arrJointChain[2].m_strJoint = pModelSkeleton->GetJointNameByID(nMiddleIdx);
				IKLimb.m_arrJointChain[3].m_idxJoint = nEndEffectorIdx;
				IKLimb.m_arrJointChain[3].m_strJoint = pModelSkeleton->GetJointNameByID(nEndEffectorIdx);

				uint32 numJoints = pModelSkeleton->GetNumJoints();
				int16 arrIndices[MAX_JOINT_AMOUNT];
				int32 icounter=0;
				int32 start=nRootIdx;
				for (uint32 i=start; i<numJoints; i++)
				{
					int32 c=i;
					while(c>0)
					{
						if (nRootIdx==c) {	arrIndices[icounter++]=i;	break; }
						else c = pModelSkeleton->m_arrModelJoints[c].m_idxParent;
					}
				}
				if (icounter)
				{
					IKLimb.m_arrLimbChildren.resize(icounter);
					for (int32 i=0; i<icounter; i++)
						IKLimb.m_arrLimbChildren[i]=arrIndices[i];
				}


				int32 arrEndEff2Root[MAX_JOINT_AMOUNT];
				arrEndEff2Root[0]=nEndEffectorIdx;

				int32 error=-1;	uint32 links;
				for (links=0; links<(MAX_JOINT_AMOUNT-1); links++)
				{
					arrEndEff2Root[links+1] = pModelSkeleton->GetJointParentIDByID(arrEndEff2Root[links]);
					error=arrEndEff2Root[links+1];
					if (error<0)
						break;
					if (arrEndEff2Root[links+1]==0)
						break;
				}
				if (error<0)
					continue;
				links+=2;
				IKLimb.m_arrRootToEndEffector.resize(links);
				int32 s=links-1;
				for (uint32 j=0; j<links; j++)
					IKLimb.m_arrRootToEndEffector[s--] = arrEndEff2Root[j];

				pModelSkeleton->m_IKLimbTypes.push_back( IKLimb );
			} //if "2BIK"

			//------------------------------------------------------------------------------------

			if (IKLimb.m_nSolver==*(uint32*)"3BIK")  //check the Solver
			{
				const char* pRoot = nodePos->getAttr( "Root" );
				if (pRoot==0)
					continue;
				int32 nRootIdx = pModelSkeleton->GetJointIDByName(pRoot);
				if (nRootIdx<0)
					continue;

				const char* pEndEffector = nodePos->getAttr( "EndEffector" );
				if (pEndEffector==0)
					continue;
				int32 nEndEffectorIdx = pModelSkeleton->GetJointIDByName(pEndEffector);
				if (nEndEffectorIdx<0)
					continue;

				assert(nRootIdx<nEndEffectorIdx);
				if (nRootIdx>=nEndEffectorIdx)
					continue;

				int32 nBaseRootIdx = pModelSkeleton->GetJointParentIDByID(nRootIdx);
				if (nBaseRootIdx<0)
					continue;
				int32 nLeg03Idx = pModelSkeleton->GetJointParentIDByID(nEndEffectorIdx);
				if (nLeg03Idx<0)
					continue;
				int32 nLeg02Idx = pModelSkeleton->GetJointParentIDByID(nLeg03Idx);
				if (nLeg02Idx<0)
					continue;
				//do a simple verification
				int32 idx  = pModelSkeleton->GetJointParentIDByID(nLeg02Idx);
				const char* strRoot  = pModelSkeleton->GetJointNameByID(idx);
				if (idx != nRootIdx)
					continue;


				IKLimb.m_arrJointChain.resize(5);
				IKLimb.m_arrJointChain[0].m_idxJoint = nBaseRootIdx;
				IKLimb.m_arrJointChain[0].m_strJoint = pModelSkeleton->GetJointNameByID(nBaseRootIdx);

				IKLimb.m_arrJointChain[1].m_idxJoint = nRootIdx;
				IKLimb.m_arrJointChain[1].m_strJoint = pModelSkeleton->GetJointNameByID(nRootIdx);

				IKLimb.m_arrJointChain[2].m_idxJoint = nLeg02Idx;
				IKLimb.m_arrJointChain[2].m_strJoint = pModelSkeleton->GetJointNameByID(nLeg02Idx);

				IKLimb.m_arrJointChain[3].m_idxJoint = nLeg03Idx;
				IKLimb.m_arrJointChain[3].m_strJoint = pModelSkeleton->GetJointNameByID(nLeg03Idx);

				IKLimb.m_arrJointChain[4].m_idxJoint = nEndEffectorIdx;
				IKLimb.m_arrJointChain[4].m_strJoint = pModelSkeleton->GetJointNameByID(nEndEffectorIdx);

				uint32 numJoints = pModelSkeleton->GetNumJoints();
				int16 arrIndices[MAX_JOINT_AMOUNT];
				int32 icounter=0;
				int32 start=nRootIdx;
				for (uint32 i=start; i<numJoints; i++)
				{
					int32 c=i;
					while(c>0)
					{
						if (nRootIdx==c) {	arrIndices[icounter++]=i;	break; }
						else c = pModelSkeleton->m_arrModelJoints[c].m_idxParent;
					}
				}
				if (icounter)
				{
					IKLimb.m_arrLimbChildren.resize(icounter);
					for (int32 i=0; i<icounter; i++)
						IKLimb.m_arrLimbChildren[i]=arrIndices[i];
				}


				int32 arrEndEff2Root[MAX_JOINT_AMOUNT];
				arrEndEff2Root[0]=nEndEffectorIdx;

				int32 error=-1;	uint32 links;
				for (links=0; links<(MAX_JOINT_AMOUNT-1); links++)
				{
					arrEndEff2Root[links+1] = pModelSkeleton->GetJointParentIDByID(arrEndEff2Root[links]);
					error=arrEndEff2Root[links+1];
					if (error<0)
						break;
					if (arrEndEff2Root[links+1]==0)
						break;
				}
				if (error<0)
					continue;
				links+=2;
				IKLimb.m_arrRootToEndEffector.resize(links);
				int32 s=links-1;
				for (uint32 j=0; j<links; j++)
					IKLimb.m_arrRootToEndEffector[s--] = arrEndEff2Root[j];

				pModelSkeleton->m_IKLimbTypes.push_back( IKLimb );
			} //if "3BIK"


			//--------------------------------------------------------------------------

			if (IKLimb.m_nSolver==*(uint32*)"CCDX")  //check the Solver
			{
				const char* pRoot = nodePos->getAttr( "Root" );
				if (pRoot==0)
					continue;
				int32 nRootIdx = pModelSkeleton->GetJointIDByName(pRoot);
				if (nRootIdx<0)
					continue;

				const char* pEndEffector = nodePos->getAttr( "EndEffector" );
				if (pEndEffector==0)
					continue;
				int32 nEndEffectorIdx = pModelSkeleton->GetJointIDByName(pEndEffector);
				if (nEndEffectorIdx<0)
					continue;

				assert(nRootIdx<nEndEffectorIdx);
				if (nRootIdx>=nEndEffectorIdx)
					continue;

				int32 nBaseRootIdx = pModelSkeleton->GetJointParentIDByID(nRootIdx);
				if (nBaseRootIdx<0)
					continue;

				if (!nodePos->getAttr( "fStepSize",IKLimb.m_fStepSize))
					continue;
				if (!nodePos->getAttr( "fThreshold",IKLimb.m_fThreshold))
					continue;
				if (!nodePos->getAttr( "nMaxInteration",IKLimb.m_nInterations))
					continue;

				int32 arrCCDChainIdx[MAX_JOINT_AMOUNT];
				const char* arrCCDChainStr[MAX_JOINT_AMOUNT];

				arrCCDChainIdx[0]=nEndEffectorIdx;
				arrCCDChainStr[0]=pModelSkeleton->GetJointNameByID(nEndEffectorIdx);

				int32 error=-1;	uint32 links;
				for (links=0; links<(MAX_JOINT_AMOUNT-1); links++)
				{
					arrCCDChainIdx[links+1] = pModelSkeleton->GetJointParentIDByID(arrCCDChainIdx[links]);
					error=arrCCDChainIdx[links+1];
					if (error<0)
						break;
					arrCCDChainStr[links+1] = pModelSkeleton->GetJointNameByID(arrCCDChainIdx[links+1]);
					if (arrCCDChainIdx[links+1]==nRootIdx)
						break;
				}
				if (error<0)
					continue;

				links++;
				arrCCDChainIdx[links+1] = nBaseRootIdx;
				arrCCDChainStr[links+1] = pModelSkeleton->GetJointNameByID(arrCCDChainIdx[links+1]);

				links+=2;
				IKLimb.m_arrJointChain.resize(links);
				int32 s=links-1;
				for (uint32 j=0; j<links; j++)
				{
					IKLimb.m_arrJointChain[s].m_idxJoint = arrCCDChainIdx[j];
					IKLimb.m_arrJointChain[s].m_strJoint = arrCCDChainStr[j];
					s--;
				}


				uint32 numJoints = pModelSkeleton->GetNumJoints();
				int16 arrIndices[MAX_JOINT_AMOUNT];
				int32 icounter=0;
				for (uint32 i=nRootIdx; i<numJoints; i++)
				{
					int32 c=i;
					while(c>0)
					{
						if (nRootIdx==c) {	arrIndices[icounter++]=i;	break; }
						else c = pModelSkeleton->m_arrModelJoints[c].m_idxParent;
					}
				}
				if (icounter)
				{
					IKLimb.m_arrLimbChildren.resize(icounter);
					for (int32 i=0; i<icounter; i++)
						IKLimb.m_arrLimbChildren[i]=arrIndices[i];
				}


				int32 arrEndEff2Root[MAX_JOINT_AMOUNT];
				arrEndEff2Root[0]=nEndEffectorIdx;
				error=-1;	
				for (links=0; links<(MAX_JOINT_AMOUNT-1); links++)
				{
					arrEndEff2Root[links+1] = pModelSkeleton->GetJointParentIDByID(arrEndEff2Root[links]);
					error=arrEndEff2Root[links+1];
					if (error<0)
						break;
					if (arrEndEff2Root[links+1]==0)
						break;
				}
				if (error<0)
					continue;
				links+=2;
				IKLimb.m_arrRootToEndEffector.resize(links);
				s=links-1;
				for (uint32 j=0; j<links; j++)
					IKLimb.m_arrRootToEndEffector[s--] = arrEndEff2Root[j];

				pModelSkeleton->m_IKLimbTypes.push_back( IKLimb );
			} //if "CCDX"


			//--------------------------------------------------------------------------

			if (IKLimb.m_nSolver==*(uint32*)"CCD5")  //check the Solver
			{
				int32 varify=3;
				IKLimb.m_arrJointChain.resize(5);

				const char* pJ0 = nodePos->getAttr( "J0" );
				if (pJ0) 
				{
					int32 nJ0Idx = pModelSkeleton->GetJointIDByName(pJ0);
					if (nJ0Idx>0)
					{
						IKLimb.m_arrJointChain[0].m_idxJoint = nJ0Idx;
						IKLimb.m_arrJointChain[0].m_strJoint = pModelSkeleton->GetJointNameByID(nJ0Idx);
						varify--;
					}
					ANIM_ASSET_CHECK_TRACE(nJ0Idx>0, ("For J0 %s", pJ0));
				}
				const char* pJ1 = nodePos->getAttr( "J1" );
				if (pJ1) 
				{
					int32 nJ1Idx = pModelSkeleton->GetJointIDByName(pJ1);
					if (nJ1Idx>0)
					{
						IKLimb.m_arrJointChain[1].m_idxJoint = nJ1Idx;
						IKLimb.m_arrJointChain[1].m_strJoint = pModelSkeleton->GetJointNameByID(nJ1Idx);
						varify--;
					}
					ANIM_ASSET_CHECK_TRACE(nJ1Idx>0, ("For J1 %s", pJ1));
				}

				const char* pJ2 = nodePos->getAttr( "J2" );
				if (pJ2) 
				{
					int32 nJ2Idx = pModelSkeleton->GetJointIDByName(pJ2);
					if (nJ2Idx>0)
					{
						IKLimb.m_arrJointChain[2].m_idxJoint = nJ2Idx;
						IKLimb.m_arrJointChain[2].m_strJoint = pModelSkeleton->GetJointNameByID(nJ2Idx);
						varify--;
					}
					ANIM_ASSET_CHECK_TRACE(nJ2Idx>0, ("For J2 %s", pJ2));
				}

				const char* pJ3 = nodePos->getAttr( "J3" );
				if (pJ3) 
				{
					int32 nJ3Idx = pModelSkeleton->GetJointIDByName(pJ3);
					if (nJ3Idx>0)
					{
						IKLimb.m_arrJointChain[3].m_idxJoint = nJ3Idx;
						IKLimb.m_arrJointChain[3].m_strJoint = pModelSkeleton->GetJointNameByID(nJ3Idx);
						varify--;
					}
					ANIM_ASSET_CHECK_TRACE(nJ3Idx>0, ("Failure for J3 %s", pJ3));
				}
				const char* pJ4 = nodePos->getAttr( "J4" );
				if (pJ4) 
				{
					int32 nJ4Idx = pModelSkeleton->GetJointIDByName(pJ4);
					if (nJ4Idx>0)
					{
						IKLimb.m_arrJointChain[4].m_idxJoint = nJ4Idx;
						IKLimb.m_arrJointChain[4].m_strJoint = pModelSkeleton->GetJointNameByID(nJ4Idx);
						varify--;
					}
					ANIM_ASSET_CHECK_TRACE(nJ4Idx>0, ("Failure for J4 %s", pJ4));
				}

				if (varify==-2)
					pModelSkeleton->m_IKLimbTypes.push_back( IKLimb );

			}
		}
	}
	return true;
}





bool CParamLoader::LoadIKDefAnimDrivenIKTargets( const XmlNodeRef adNode)
{
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;

	uint32 num = adNode->getChildCount();
	for (uint32 i=0; i<num; i++) 
	{
		ADIKTarget IKTarget;
		XmlNodeRef nodePos = adNode->getChild(i);
		const char* PosTag = nodePos->getTag();
		if (stricmp(PosTag,"ADIKTarget")==0) 
		{
			uint32 varify=3;
			const char* pHandle = nodePos->getAttr( "Handle" );
			if (pHandle)
			{
				IKTarget.m_nHandle = *(uint64*)pHandle;
				varify--;
			}
			const char* pTarget = nodePos->getAttr( "Target" );
			if (pTarget) 
			{
				int32 nTargetIdx = pModelSkeleton->GetJointIDByName(pTarget);
				if (nTargetIdx>0)
				{
					IKTarget.m_idxTarget  = nTargetIdx;
					IKTarget.m_strTarget  = pModelSkeleton->GetJointNameByID(nTargetIdx);
					varify--;
				}
				ANIM_ASSET_CHECK_TRACE(nTargetIdx>0, ("Failure for target %s", pTarget));
			}

			const char* pWeight = nodePos->getAttr( "Weight" );
			if (pWeight) 
			{
				int32 nWeightIdx = pModelSkeleton->GetJointIDByName(pWeight);
				if (nWeightIdx>0)
				{
					IKTarget.m_idxWeight  = nWeightIdx;
					IKTarget.m_strWeight  = pModelSkeleton->GetJointNameByID(nWeightIdx);
					varify--;
				}
				ANIM_ASSET_CHECK_TRACE(nWeightIdx>0, ("Failure for weight %s", pWeight));
			}
			if (varify==0)
				pModelSkeleton->m_ADIKTargets.push_back( IKTarget );
		}
	}
	return true;
}

bool CParamLoader::LoadIKDefFeetLock(XmlNodeRef aimNode)
{
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;

	uint32 numChilds = aimNode->getChildCount();
	for (uint32 c=0; c<numChilds; c++) 
	{
		XmlNodeRef node = aimNode->getChild(c);
		const char* strTag = node->getTag();

		if (stricmp(strTag,"RIKHandle")==0) 
		{
			const char* pJointName = node->getAttr( "Handle" );
			pModelSkeleton->m_strFeetLockRIKHandle=pJointName;
		}

		if (stricmp(strTag,"LIKHandle")==0) 
		{
			const char* pJointName = node->getAttr( "Handle" );
			pModelSkeleton->m_strFeetLockLIKHandle=pJointName;
		}
	}
	return true;
}



bool CParamLoader::LoadIKDefRecoil(XmlNodeRef aimNode)
{
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;

	uint32 numChilds = aimNode->getChildCount();
	for (uint32 c=0; c<numChilds; c++) 
	{
		XmlNodeRef node = aimNode->getChild(c);
		const char* strTag = node->getTag();

		if (stricmp(strTag,"RIKHandle")==0) 
		{
			const char* pJointName = node->getAttr( "Handle" );
			pModelSkeleton->m_strRecoilRIKHandle=pJointName;
		}

		if (stricmp(strTag,"LIKHandle")==0) 
		{
			const char* pJointName = node->getAttr( "Handle" );
			pModelSkeleton->m_strRecoilLIKHandle=pJointName;
		}


		if (stricmp(strTag,"RWeaponJoint")==0) 
		{
			const char* pJointName = node->getAttr( "JointName" );
			int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
			assert(jidx>0);
			if (jidx>0)
				pModelSkeleton->m_RecoilRWeaponBoneIdx = jidx;
		}

		//----------------------------------------------------------------------------------------

		if (stricmp(strTag,"LWeaponJoint")==0) 
		{
			const char* pJointName = node->getAttr( "JointName" );
			int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
			assert(jidx>0);
			if (jidx>0)
				pModelSkeleton->m_RecoilLWeaponBoneIdx = jidx;
		}

		//----------------------------------------------------------------------------
		if (stricmp(strTag,"ImpactJoints")==0) 
		{
			uint32 num = node->getChildCount();
			pModelSkeleton->m_RecoilJoints.reserve(num);
			for (uint32 i=0; i<num; i++) 
			{
				SRecoilJoints Recoil;
				XmlNodeRef nodeRot = node->getChild(i);
				const char* RotTag = nodeRot->getTag();
				if (stricmp(RotTag,"ImpactJoint")==0) 
				{
					const char* pJointName = nodeRot->getAttr( "JointName" );
					int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
					if (jidx>0)
					{
						Recoil.m_nIdx = jidx;
						Recoil.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
					}
					assert(jidx>0);
					nodeRot->getAttr( "Arm",    Recoil.m_nArm );
					nodeRot->getAttr( "Delay",  Recoil.m_fDelay );
					nodeRot->getAttr( "Weight", Recoil.m_fWeight );
					//	Recoil.m_nAdditive=0;
					pModelSkeleton->m_RecoilJoints.push_back( Recoil );
				}
			}
		}
	}
	return true;
}



bool CParamLoader::LoadIKDefLookIK(const XmlNodeRef lookNode)
{
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;

	pModelSkeleton->m_useLookIK=1;



	uint32 error=0;
	uint32 numChilds = lookNode->getChildCount();
	for (uint32 c=0; c<numChilds; c++) 
	{
		XmlNodeRef node = lookNode->getChild(c);
		const char* strTag = node->getTag();

		if (stricmp(strTag,"LEyeAttachment")==0) 
		{
			const char* pName = node->getAttr( "Name" );
			pModelSkeleton->m_strEyeAttachL=pName;				//left eyeball attachment
		}

		if (stricmp(strTag,"REyeAttachment")==0) 
		{
			const char* pName = node->getAttr( "Name" );
			pModelSkeleton->m_strEyeAttachR=pName;				//right eyeball attachment
		}

		if (stricmp(strTag,"FOV")==0) 
		{
			node->getAttr( "para",pModelSkeleton->m_fLookFieldOfView );
		}

		//----------------------------------------------------------------------------
		if (stricmp(strTag,"RotationList")==0) 
		{
			uint32 num = node->getChildCount();
			pModelSkeleton->m_LookIK_Rot.reserve(num);
			for (uint32 i=0; i<num; i++) 
			{
				SJointsAimIK_Rot LookIK_Rot;
				XmlNodeRef nodeRot = node->getChild(i);
				const char* RotTag = nodeRot->getTag();
				if (stricmp(RotTag,"Rotation")==0) 
				{
					const char* pJointName = nodeRot->getAttr( "JointName" );
					int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
					if (jidx>0)
					{
						LookIK_Rot.m_nJointIdx = jidx;
						LookIK_Rot.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
					}
					else
					{
						error++;
					}
					assert(jidx>0);
					nodeRot->getAttr( "Primary",    LookIK_Rot.m_nPreEvaluate );
					nodeRot->getAttr( "Additive",   LookIK_Rot.m_nAdditive );
					//	AimIK_Rot.m_nAdditive=0;
					pModelSkeleton->m_LookIK_Rot.push_back( LookIK_Rot );
				}
			}
		}

		//----------------------------------------------------------------------------------------

		if (stricmp(strTag,"PositionList")==0) 
		{
			uint32 num = node->getChildCount();
			pModelSkeleton->m_LookIK_Pos.reserve(num);
			for (uint32 i=0; i<num; i++) 
			{
				SJointsAimIK_Pos LookIK_Pos;
				XmlNodeRef nodePos = node->getChild(i);
				const char* PosTag = nodePos->getTag();
				if (stricmp(PosTag,"Position")==0) 
				{
					const char* pJointName = nodePos->getAttr( "JointName" );
					int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
					if (jidx>0)
					{
						LookIK_Pos.m_nJointIdx = jidx;
						LookIK_Pos.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
					}
					else
					{
						error++;
					}

					assert(jidx>0);
					nodePos->getAttr( "Additive",   LookIK_Pos.m_nAdditive );
					pModelSkeleton->m_LookIK_Pos.push_back( LookIK_Pos );
				}
			}
		}

		//----------------------------------------------------------------------------------------

		if (stricmp(strTag,"DirectionalBlends")==0) 
		{
			uint32 num = node->getChildCount();
			pModelSkeleton->m_LookDirBlends.reserve(num);
			for (uint32 i=0; i<num; i++) 
			{
				DirectionalBlends DirBlend;
				XmlNodeRef nodePos = node->getChild(i);
				const char* DirTag = nodePos->getTag();
				if (stricmp(DirTag,"Joint")==0) 
				{
					const char* pAnimToken = nodePos->getAttr( "AnimToken" );
					DirBlend.m_AnimToken = pAnimToken;
					DirBlend.m_AnimTokenCRC32 = g_pCrc32Gen->GetCRC32Lowercase(pAnimToken);

					const char* pJointName = nodePos->getAttr( "ParameterJoint" );
					int jidx1 = pModelSkeleton->GetJointIDByName(pJointName);
					if (jidx1>=0)
					{
						DirBlend.m_nParaJointIdx = jidx1;
						DirBlend.m_strParaJointName = pModelSkeleton->GetJointNameByID(jidx1);
					}
					else
					{
						error++;
					}
					assert(jidx1>=0);

					const char* pStartJointName = nodePos->getAttr( "StartJoint" );
					int jidx2 = pModelSkeleton->GetJointIDByName(pStartJointName);
					if (jidx2>=0)
					{
						DirBlend.m_nStartJointIdx	= jidx2;
						DirBlend.m_strStartJointName = pModelSkeleton->GetJointNameByID(jidx2);
					}
					else
					{
						error++;
					}
					assert(jidx2>=0);

					const char* pReferenceJointName = nodePos->getAttr( "ReferenceJoint" );
					int jidx3 = pModelSkeleton->GetJointIDByName(pReferenceJointName);
					if (jidx3>=0)
					{
						DirBlend.m_nReferenceJointIdx	= jidx3;
						DirBlend.m_strReferenceJointName = pModelSkeleton->GetJointNameByID(jidx3);
					}
					else
					{
						//	CryFatalError("ReferenceJoint");
						error++;
					}
					assert(jidx3>=0);

					pModelSkeleton->m_LookDirBlends.push_back( DirBlend );
				}
			}
		}

		if (stricmp(strTag,"DefaultLookPose") == 0)
		{
			if (const char* defaultPose = node->getAttr("name"))
			{
				m_pModel->SetDefaultLookPose(defaultPose);
			}
		}
	}

	//----------------------------------------------------------------------------------------
	if (error)
	{
		pModelSkeleton->m_LookIK_Rot.clear();
		pModelSkeleton->m_LookIK_Pos.clear();
		m_pModel->SetDefaultLookPose("default_lookpose");
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pModel->GetFilePath(), "Reference if not existing Joint-Name. Look-IK disabled");
		//	CryFatalError("CryAnimation: Bad Index");
	}
	return true;
}





bool CParamLoader::LoadIKDefAimIK(XmlNodeRef aimNode)
{
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;

	uint32 numChilds = aimNode->getChildCount();
	for (uint32 c=0; c<numChilds; c++) 
	{
		XmlNodeRef node = aimNode->getChild(c);
		const char* strTag = node->getTag();


		//---------------------------------------------------------------------
		uint32 error=0;
		if (stricmp(strTag,"DirectionalBlends")==0) 
		{
			uint32 num = node->getChildCount();
			pModelSkeleton->m_AimDirBlends.reserve(num);
			for (uint32 i=0; i<num; i++) 
			{
				DirectionalBlends DirBlend;
				XmlNodeRef nodePos = node->getChild(i);
				const char* DirTag = nodePos->getTag();
				if (stricmp(DirTag,"Joint")==0) 
				{
					const char* pAnimToken = nodePos->getAttr( "AnimToken" );
					DirBlend.m_AnimToken = pAnimToken;
					DirBlend.m_AnimTokenCRC32 = g_pCrc32Gen->GetCRC32Lowercase(pAnimToken);

					const char* pJointName = nodePos->getAttr( "ParameterJoint" );
					int jidx1 = pModelSkeleton->GetJointIDByName(pJointName);
					if (jidx1>=0)
					{
						DirBlend.m_nParaJointIdx = jidx1;
						DirBlend.m_strParaJointName = pModelSkeleton->GetJointNameByID(jidx1);
					}
					else
					{
						error++;
					}
					assert(jidx1>=0);

					const char* pStartJointName = nodePos->getAttr( "StartJoint" );
					int jidx2 = pModelSkeleton->GetJointIDByName(pStartJointName);
					if (jidx2>=0)
					{
						DirBlend.m_nStartJointIdx	= jidx2;
						DirBlend.m_strStartJointName = pModelSkeleton->GetJointNameByID(jidx2);
					}
					else
					{
						error++;
					}
					assert(jidx2>=0);

					const char* pReferenceJointName = nodePos->getAttr( "ReferenceJoint" );
					int jidx3 = pModelSkeleton->GetJointIDByName(pReferenceJointName);
					if (jidx3>=0)
					{
						DirBlend.m_nReferenceJointIdx	= jidx3;
						DirBlend.m_strReferenceJointName = pModelSkeleton->GetJointNameByID(jidx3);
					}
					else
					{
						//CryFatalError("ReferenceJoint");
						error++;
					}
					assert(jidx3>=0);

					pModelSkeleton->m_AimDirBlends.push_back( DirBlend );
				}
			}
		}

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
					else
					{
						error++;
					}
					assert(jidx>0);
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
					else
					{
						error++;
					}
					assert(jidx>0);
					nodePos->getAttr( "Additive",   AimIK_Pos.m_nAdditive );
					pModelSkeleton->m_AimIK_Pos.push_back( AimIK_Pos );
				}
			}
		}

		//----------------------------------------------------------------------------------------
		if (stricmp(strTag,"ProcAdjustments")==0) 
		{
			uint32 num = node->getChildCount();
			pModelSkeleton->m_RecoilJoints.reserve(num);
			for (uint32 i=0; i<num; i++) 
			{
				ProcAdjust Spine;
				XmlNodeRef nodeRot = node->getChild(i);
				const char* SpineTag = nodeRot->getTag();
				if (stricmp(SpineTag,"Spine")==0) 
				{
					const char* pJointName = nodeRot->getAttr( "JointName" );
					int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
					if (jidx>0)
					{
						Spine.m_nIdx = jidx;
						Spine.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
					}
					else
					{
						error++;
					}
					pModelSkeleton->m_AimProcAdjustments.push_back( Spine );
				}
			}
		}
		//----------------------------------------------------------------------------------------
		if (error)
		{
			pModelSkeleton->m_AimIK_Rot.clear();
			pModelSkeleton->m_AimIK_Pos.clear();
			pModelSkeleton->m_AimProcAdjustments.clear();
			g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pModel->GetFilePath(), "Reference if not existing Joint-Name. Aim-IK disabled");
			//	CryFatalError("CryAnimation: Bad Index");
		}
	}
	return true;
}


