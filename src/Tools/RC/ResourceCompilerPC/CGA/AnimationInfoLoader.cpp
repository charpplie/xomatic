#include "StdAfx.h"

#include "AnimationInfoLoader.h"
#include "ICryXML.h"
#include "IXMLSerializer.h"

#include "PakXmlFileBufferSource.h"

CAnimationInfoLoader::CAnimationInfoLoader(ICryXML * pXML) : m_pXML(pXML)
{
}

CAnimationInfoLoader::~CAnimationInfoLoader(void)
{
}


int GetIntFromStringBOOL(const string& val)
{
	if (!val.c_str())
		return -1;

	int ret = 0;
	if ((stricmp(val.c_str(), "1") == 0) || (stricmp(val.c_str(), "yes") == 0))
		ret = 1;

	return ret;

}


void SkipSpaces(string& str)
{
	std::size_t i =str.find(' ') ;
	while ( i!= str.npos)
	{
		str.erase(i);
		i =str.find(' ') ;
	}
}



int GetNodeIDFromName(const char * name, const XmlNodeRef& node)
{
	uint32 numChildren= node->getChildCount();

	for (uint32 chield = 0; chield < numChildren; ++ chield )
	{
		if (stricmp(name, node->getChild(chield)->getTag()) == 0)
		{
			return chield;
		}
	}

	return -1;
}




bool CAnimationInfoLoader::LoadDescription(const string& name, IAnimationLoaderListener * pListener, IPakSystem* pPakSystem)//, IAnimationLoaderListener * pListener)
{
	// fill structures
	char error[4096];

	//Move to use pak file system or file: XmlNodeRef root =  m_pXML->GetXMLSerializer()->Read(FileXmlBufferSource(name.c_str()), sizeof(error), error);
	const bool bRemoveNonessentialSpacesFromContent = true;
	XmlNodeRef root =  m_pXML->GetXMLSerializer()->Read(PakXmlFileBufferSource(pPakSystem, name.c_str()), bRemoveNonessentialSpacesFromContent, sizeof(error), error);
	if (!root)
	{
		ReportError(pListener, "Cannot read file \"%s\": %s\n", name.c_str(), error);
		return false;
	}


	// temporary storage for the presets
	struct  PresetFinder
	{
		static bool FillPreset(std::vector<CompressionPreset>& presets, const char * name, CompressionPreset& newpreset)
		{
			uint32 end = presets.size();
			for (uint32 i = 0; i < end; ++i)
			{
				if (stricmp(presets[i].m_Name.c_str(), name) == 0)
				{
					newpreset = presets[i];
					return true;
				}
			}

			return false;
		}
	};

	std::vector<CompressionPreset> presets;

	uint32 numDefinitions = root->getChildCount();

	for (uint32 nDefinitionNode=0; nDefinitionNode<numDefinitions; ++nDefinitionNode)
	{
		XmlNodeRef pRoot = root->getChild(nDefinitionNode);
		if(!pRoot)
		{
			ReportError(pListener, "Node not found");
			continue;
		}

		if (stricmp(pRoot->getTag(), "preset") == 0)
		{

			// this is preset for compression
			CompressionPreset preset;
			uint32 numBonesInfo = pRoot->getChildCount();
			string presetName = pRoot->getAttr("Name");
			preset.m_Name = presetName;


			for (uint32 nBone = 0; nBone < numBonesInfo; ++nBone )
			{
				CompressionPreset::BonePreset bone;

				XmlNodeRef pBone = pRoot->getChild(nBone);
				//<Bone id="12345" error="0.00001" rotformat="auto" posformat="auto"/>
				uint32 boneId;

				pBone->getAttr("id", boneId);
				pBone->getAttr("poserror", bone.m_fPosError);
				pBone->getAttr("roterror", bone.m_fRotError);
				pBone->getAttr("rotformat", bone.m_RotCompressionFormat);
				pBone->getAttr("posformat", bone.m_PosCompressionFormat);

				preset.m_BoneInfoMap[boneId] = bone;
			}

			presets.push_back(preset);
		}
	}


	for (uint32 nDefinitionNode=0; nDefinitionNode<numDefinitions; ++nDefinitionNode)
	{
		XmlNodeRef pRoot = root->getChild(nDefinitionNode);
		if(!pRoot)
		{
			ReportError(pListener, "Node not found");
			continue;
		}

		if (stricmp(pRoot->getTag(), "preset") == 0)
		{
			continue;
		}

		SAnimationDefinition def;

		uint32 numAttribs = pRoot->getChildCount();

		{
			uint32 model = GetNodeIDFromName("model", pRoot);
			if (model == -1)
			{
				ReportInfo(pListener, "Model name not found in \"%s\" node", pRoot->getTag());
			}
			else
			{
				def.m_Model = pRoot->getChild(model)->getAttr("File");
				if (def.m_Model.empty())
				{
					ReportError(pListener, "Model name not found in \"%s\" node", pRoot->getTag());
				}
			}

		}


		{
			uint32 path = GetNodeIDFromName("animation", pRoot);
			if (path == -1)
			{
				ReportInfo(pListener, "Path not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
			}
			else
			{
				const char * pChar = pRoot->getChild(path)->getAttr("Path");
				if (!pChar)
				{
					ReportError(pListener, "Path not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				}

				def.SetAnimationPath(string(pChar));
			}
		}

		{
			uint32 path = GetNodeIDFromName("database", pRoot);
			if (path == -1)
			{
				ReportInfo(pListener, "Database not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
			}
			else
			{
				const char * pChar = pRoot->getChild(path)->getAttr("Path");
				if (!pChar)
				{
					ReportError(pListener, "Path not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				}

				def.m_DBName = string(pChar);
			}
		}





		{
			uint32 floordist = GetNodeIDFromName("FloorDist", pRoot);
			if (floordist == -1)
			{
				//ReportInfo(pListener, "FloorDist value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
			}
			else
			{

				if (!pRoot->getChild(floordist)->getAttr("value", def.m_MainDesc.m_fFloorDist))
				{
					ReportInfo(pListener, "FloorDist value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				};
			}
		}

		{
			uint32 SpeedXY = GetNodeIDFromName("SpeedXY", pRoot);
			if (SpeedXY == -1)
			{
			}
			else
			{

				if (!pRoot->getChild(SpeedXY)->getAttr("value", def.m_MainDesc.m_fSpeedXY))
				{
					ReportInfo(pListener, "SpeedXY value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				};
			}
		}




	

		{
			uint32 preset = GetNodeIDFromName("preset", pRoot);
			if (preset == -1)
			{
				//ReportInfo(pListener, "Preset not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
			}
			else
			{
				const char * pChar = pRoot->getChild(preset)->getAttr("name");
				if (!pChar)
				{
					ReportInfo(pListener, "Preset not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				}
				PresetFinder::FillPreset(presets, pChar, def.m_MainDesc.m_Preset);
			}
		}

		//int m_RootToHeels;
		{
			uint32 skipSaveToDB = GetNodeIDFromName("SkipSaveToDatabase", pRoot);
			if (skipSaveToDB == -1)
			{
				//ReportInfo(pListener, "SkipSaveToDatabase value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
			}
			else
			{
				string val = pRoot->getChild(skipSaveToDB)->getAttr("value");
				if (val.empty())
				{
					ReportInfo(pListener, "SaveToDatabase value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				}
				def.m_MainDesc.m_SkipSaveToDatabase = GetIntFromStringBOOL(val);
			}
		}


		{
			int32 compression = GetNodeIDFromName("compression", pRoot);
			if (compression == -1)
			{
				ReportInfo(pListener, "Compression value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
			}
			else
			{

				if (!pRoot->getChild(compression)->getAttr("value", def.m_MainDesc.m_CompressionQuality))
				{
					ReportInfo(pListener, "Compression value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				};
			}
		}

		{
			uint32 ROTEpsilon = GetNodeIDFromName("RotEpsilon", pRoot);
			if (ROTEpsilon == -1)
			{
			}
			else
			{
				uint32 status = pRoot->getChild(ROTEpsilon)->getAttr("value", def.m_MainDesc.m_fROT_EPSILON);
				if (status==0)
				{
					ReportInfo(pListener, "ROT_EPSILON value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				};
			}
		}
		{
			uint32 POSEpsilon = GetNodeIDFromName("PosEpsilon", pRoot);
			if (POSEpsilon == -1)
			{
			}
			else
			{
				uint32 status = pRoot->getChild(POSEpsilon)->getAttr("value", def.m_MainDesc.m_fPOS_EPSILON);
				if (status==0)
				{
					ReportInfo(pListener, "ROT_EPSILON value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				};
			}
		}




		{
			uint32 footplant = GetNodeIDFromName("footplants", pRoot);
			if (footplant == -1)
			{
				ReportInfo(pListener, "Footplants value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
			}
			else
			{
				string val = pRoot->getChild(footplant)->getAttr("value");
				if (val.empty())
				{
					ReportInfo(pListener, "Footplants value not found in \"%s\" node \"%s\"", pRoot->getTag(), def.m_Model.c_str());
				}

				def.m_MainDesc.m_FootPlant = GetIntFromStringBOOL(val);
			}
		}



		//-----------------------------------------------------------------------


		XmlNodeRef pAttrib;

		uint32 details = GetNodeIDFromName("SpecialAnimsList", pRoot);
		if (details != -1)
		{
			pAttrib = pRoot->getChild(details);
			uint32 numAnims = pAttrib->getChildCount();
			for (uint32 nAnims = 0; nAnims < numAnims; ++nAnims )
			{
				SAnimationDesc desc(def.m_MainDesc);
				XmlNodeRef pAnim = pAttrib->getChild(nAnims);
				if(!pAnim)
				{
					ReportInfo(pListener, "Node not found");
				}
				else
				{
					const char *pPath = pAnim->getAttr("APath");

					if (!pPath)
					{
						ReportInfo(pListener, "Animation name not found in \"%s\" node \"subnode\" \"%s\"", pRoot->getTag(), pAnim->getTag(), def.m_Model.c_str());
						continue;
					}
					desc.SetAnimationName(string(pPath));

					string val;
					val = pAnim->getAttr("SkipSaveToDatabase");
					if (!val.empty())
					{
						desc.m_SkipSaveToDatabase = GetIntFromStringBOOL(val);
					}

					val = pAnim->getAttr("compression");
					if (!val.empty())
					{
						pAnim->getAttr("compression", desc.m_CompressionQuality);
					}
					val = pAnim->getAttr("RotEpsilon");
					if (!val.empty())
					{
						pAnim->getAttr("RotEpsilon", desc.m_fROT_EPSILON);
					}
					val = pAnim->getAttr("PosEpsilon");
					if (!val.empty())
					{
						pAnim->getAttr("PosEpsilon", desc.m_fPOS_EPSILON);
					}


					val = pAnim->getAttr("Additive_Animation");
					if (!val.empty())
					{
						desc.m_AdditiveAnimation = GetIntFromStringBOOL(val);
					}


					val = pAnim->getAttr("FloorDist");
					if (!val.empty())
					{
						pAnim->getAttr("FloorDist", desc.m_fFloorDist);
					}

					val = pAnim->getAttr("SpeedXY");
					if (!val.empty())
					{
						pAnim->getAttr("SpeedXY", desc.m_fSpeedXY);
					}

					val = pAnim->getAttr("footplants");
					if (!val.empty())
					{
						desc.m_FootPlant = GetIntFromStringBOOL(val);
					}




					val = pAnim->getAttr("preset");
					if (!val.empty())
					{
						PresetFinder::FillPreset(presets, val.c_str(), desc.m_Preset);
					}

					size_t num = def.m_OverrideAnimations.size();


					if (def.FindIdentical(string(pPath), true))
					{
						ReportInfo(pListener, "Duplicate  \"%s\" \"%s\"", pPath,  def.m_Model.c_str());
					}
					else
					{
						def.m_OverrideAnimations.push_back(desc);
					}
				}
			}
		}

		size_t num = m_ADefinitions.size();
		m_ADefinitions.push_back(def);
	}

	return true;
}



const SAnimationDesc& SAnimationDefinition::GetAnimationDesc(const string& name ) const
{
	string searchname(name);
	FixString(searchname);

	uint32 numOverAnims = m_OverrideAnimations.size();
	for (uint32 i=0; i<numOverAnims; ++i)
	{
		string mapname(m_OverrideAnimations[i].GetAnimString());
		if ((strstr(mapname.c_str(), searchname.c_str()) != 0) || (strstr(searchname.c_str(), mapname.c_str()) != 0))
		{
			return m_OverrideAnimations[i];
		}
	}

	return m_MainDesc;
}

bool SAnimationDefinition::FindIdentical(const string& name, bool checkLen )
{
	string searchname(name);
	FixString(searchname);

	uint32 numOverAnims = m_OverrideAnimations.size();
	for (uint32 i=0; i<numOverAnims; ++i)
	{
		string mapname(m_OverrideAnimations[i].GetAnimString());
		if ((strstr(mapname.c_str(), searchname.c_str()) != 0) || (strstr(searchname.c_str(), mapname.c_str()) != 0))
		{
			if (checkLen)
			{
				if (mapname.length() == searchname.length())
					return true;
			}
			else
				return true;
		}
	}

	return false;
}

const SAnimationDefinition * CAnimationInfoLoader::GetAnimationDefinition(const string& name ) const
{
	string searchname(name);
	FixString(searchname);

	for (uint32 i = 0; i < m_ADefinitions.size(); ++i )
	{
		string mapname(m_ADefinitions[i].GetAnimationPath());

		if ((strstr(mapname.c_str(), searchname.c_str()) != 0) || (strstr(searchname.c_str(), mapname.c_str()) != 0))
		{
			return &m_ADefinitions[i];
		}
	}

	return 0;
}


void ReportWarning(IAnimationLoaderListener* pListener, const char* szFormat, ...)
{

	va_list args;
	va_start(args, szFormat);
	char szBuffer[1024];
	vsprintf(szBuffer, szFormat, args);
	pListener->OnAnimationLoaderMessage(IAnimationLoaderListener::MESSAGE_WARNING, szBuffer);
	va_end(args);
}

void ReportInfo(IAnimationLoaderListener* pListener, const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	char szBuffer[1024];
	vsprintf(szBuffer, szFormat, args);
	pListener->OnAnimationLoaderMessage(IAnimationLoaderListener::MESSAGE_INFO, szBuffer);
	va_end(args);
}

void ReportError(IAnimationLoaderListener* pListener, const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	char szBuffer[1024];
	vsprintf(szBuffer, szFormat, args);
	pListener->OnAnimationLoaderMessage(IAnimationLoaderListener::MESSAGE_ERROR, szBuffer);
	va_end(args);
}

