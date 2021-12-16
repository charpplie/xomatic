////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   ColladaLoader.cpp
//  Version:     v1.00
//  Created:     3/4/2006 by Michael Smith
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IXml.h"
#include "IXMLSerializer.h"
#include "Xml/xml.h"
#include "MeshCompiler\MeshCompiler.h"
#include "IPakSystem.h"
#include <cstdarg>
#include <vector>
#include <map>
#include <cctype>
#include <Cry_Vector3.h>
#include "ColladaLoader.h"
#include "PropertyHelpers.h"
#include "StringUtils.h"
#include "StringHelpers.h"
#include <crc32.h>

#include "cga\Controller.h"
#include "cga\ControllerPQ.h"
#include "cga\ControllerPQLog.h"
#include "cgf\LoaderCAF.h"

#include "PakXmlFileBufferSource.h"

#include "physinterface.h"

#include "AdjustRotLog.h"
#include "Decompose.h"

extern Crc32Gen g_crcGen;

namespace
{
	void DecomposeTransform(Vec3& translation, CryQuat& rotation, Vec3& scale, const Matrix34& transform)
	{
		translation = transform.GetTranslation();
		Matrix33 orientation(transform);
		scale.x = Vec3(orientation.m00,orientation.m10,orientation.m20).GetLength();
		scale.y = Vec3(orientation.m01,orientation.m11,orientation.m21).GetLength();
		scale.z = Vec3(orientation.m02,orientation.m12,orientation.m22).GetLength();
		orientation.OrthonormalizeFast();
		rotation = !CryQuat(orientation);
	}

	string GetSafeBoneName(const string& boneName)
	{
		// Check for a name override property
		string marker = "--PRprops";
		size_t propMarker = boneName.find(marker);
		size_t propStart = (propMarker != string::npos ? propMarker + marker.size() : propMarker);
		size_t propEnd = boneName.find("__", propStart);

		for (size_t propPos = propStart; propPos < propEnd;)
		{
			for (; boneName[propPos] == '_'; ++propPos);
			size_t keyStart = propPos;
			for (; propPos < propEnd && boneName[propPos] != '='; ++propPos);
			size_t equalPos = propPos;
			if (boneName[propPos] == '=')
				++propPos;
			for (; std::isspace(boneName[propPos]); ++propPos);
			size_t valueStart = propPos;
			size_t valueEnd = boneName.find('=', propPos);
			if (valueEnd == string::npos)
			{
				valueEnd = propEnd;
			}
			else
			{
				for (; valueEnd > valueStart && boneName[valueEnd] != '_'; --valueEnd);
				for (; valueEnd > valueStart && boneName[valueEnd] == '_'; --valueEnd);
				++valueEnd;
			}
			propPos = valueEnd;

			string key = StringHelpers::Trim(StringHelpers::Replace(StringHelpers::MakeLowerCase(boneName.substr(keyStart, equalPos - keyStart)), '*', ' '));
			string value = boneName.substr(valueStart, valueEnd - valueStart);
			if (stricmp(key.c_str(), "name") == 0)
			{
				value.replace('*', ' '); // We can't use spaces in names in Collada since the joints are separated by whitespace.
				return StringHelpers::Trim(value); // Found an override property, use the value as the name.
			}
		}

		size_t propsStartPos = boneName.find("--pr");
		string nodeNameWithoutProps = boneName.substr(0, propsStartPos);

		// No override, so find the bone name.
		char *name = new char[strlen(nodeNameWithoutProps.c_str())+1];
		strcpy(name,nodeNameWithoutProps.c_str());
		int len = strlen(name);
		char *pos = strstr(name, "%");
		if (pos)
		{
			pos[0] = 0;
			len = pos-name+1;
		}
		//for (int j = 0; j < len; j++)
		//{
		//	if (name[j] == '_')
		//		name[j] = ' ';
		//}
		string result(name);
		delete name;

		size_t propsPos = result.find("--pr");
		return result.substr(0, propsPos);
	}

	void ParseBoneProperties(const string& boneName, PhysBone& bone)
	{
		string marker = "--prprops";
		size_t propMarker = boneName.find(marker);
		size_t propStart = (propMarker != string::npos ? propMarker + marker.size() : propMarker);
		size_t propEnd = boneName.find("__", propStart);

		// Loop through all the properties.
		for (size_t propPos = propStart; propPos != propEnd;)
		{
			for (; boneName[propPos] == '_'; ++propPos);
			size_t propNext = boneName.find('_', propPos);
			string text = boneName.substr(propPos, (propNext != string::npos ? propNext - propPos : string::npos));
			propPos = propNext;

			// Check whether this property is an axis limit.
			size_t equalPos = text.find('=');
			string key = text.substr(0, equalPos);
			string value = (equalPos != string::npos ? text.substr(equalPos + 1) : "");
			static const char* keys[] = {"xmin", "xmax", "ymin", "ymax", "zmin", "zmax"};
			static const PhysBone::AxisLimit axisLimits[] = {
				PhysBone::AxisLimit(PhysBone::AxisX, PhysBone::LimitMin),
				PhysBone::AxisLimit(PhysBone::AxisX, PhysBone::LimitMax),
				PhysBone::AxisLimit(PhysBone::AxisY, PhysBone::LimitMin),
				PhysBone::AxisLimit(PhysBone::AxisY, PhysBone::LimitMax),
				PhysBone::AxisLimit(PhysBone::AxisZ, PhysBone::LimitMin),
				PhysBone::AxisLimit(PhysBone::AxisZ, PhysBone::LimitMax)};
			bool foundKey = false;
			PhysBone::AxisLimit axisLimit;
			for (int i = 0; i < 6; ++i)
			{
				if (stricmp(key.c_str(), keys[i]) == 0)
				{
					foundKey = true;
					axisLimit = axisLimits[i];
				}
			}
			if (foundKey)
			{
				float limit = (float)atof(value.c_str());
				bone.limits.insert(std::make_pair(axisLimit, limit));
			}

			// Check whether this property is a different IK property.
			int axisChar = key[0];
			if (axisChar >= 'x' && axisChar <= 'z')
			{
				PhysBone::Axis axis = PhysBone::Axis(axisChar - 'x');
				string paramName = key.substr(1);
				static const char* keys[] = {"damping", "springangle", "springtension"};
				typedef std::map<PhysBone::Axis, float> PhysBone::*PhysBoneAxisMapPtr;
				static const PhysBoneAxisMapPtr maps[] = {&PhysBone::dampings, &PhysBone::springAngles, &PhysBone::springTensions};
				PhysBoneAxisMapPtr map = 0;
				for (int i = 0; i < 3; ++i)
				{
					if (stricmp(paramName.c_str(), keys[i]) == 0)
						map = maps[i];
				}
				if (map)
				{
					float v = (float)atof(value.c_str());
					(bone.*map).insert(std::make_pair(axis, v));
				}
			}
		}
	}

	void FindRootBones(std::vector<ColladaNode*>& rootBoneNodes, const NameNodeMap& nodes, ColladaController* controller)
	{
		std::set<string> boneSet;
		for (int boneIndex = 0, boneCount = (controller ? controller->GetNumJoints() : 0); boneIndex < boneCount; ++boneIndex)
		{
			CryBoneDescData boneDesc;
			string boneID = (controller ? controller->GetJointID(boneIndex) : "");

			// Check whether the node referred to in the controller actually exists.
			NameNodeMap::const_iterator jointNodePos = nodes.find(boneID);
			if (jointNodePos != nodes.end())
				boneSet.insert(boneID);
		}

		for (int boneIndex = 0, boneCount = (controller ? controller->GetNumJoints() : 0); boneIndex < boneCount; ++boneIndex)
		{
			// Check whether this node is a root node.
			NameNodeMap::const_iterator nameNodePos = (controller ? nodes.find(controller->GetJointID(boneIndex)) : nodes.end());
			ColladaNode* jointNode = (nameNodePos != nodes.end() ? (*nameNodePos).second : 0);

			bool foundAncestorNode = false;
			for (ColladaNode* ancestorJointNode = (jointNode ? jointNode->GetParent() : 0); ancestorJointNode; ancestorJointNode = ancestorJointNode->GetParent())
			{
				string parentJointName = (ancestorJointNode ? ancestorJointNode->GetID() : "");
				std::set<string>::const_iterator boneSetPos = (!parentJointName.empty() ? boneSet.find(parentJointName) : boneSet.end());
				foundAncestorNode = foundAncestorNode || (boneSetPos != boneSet.end());
			}

			bool isRootNode = (jointNode && !foundAncestorNode);

			if (isRootNode)
				rootBoneNodes.push_back(jointNode);
		}
	}
}

// float reader (same as std::strtod() but faster)
float read_float(const char*& buf, char** endptr)
{
	const char* p;
	int count_num = 0;
	int count_dot = 0;
	int count_sign = 0;
	int count_dirty = 0;
	int pos_sign = -1;
	int pos_dot = -1;
	for (p=buf ; *p && !isspace(*p) ; p++)
	{
		if (*p >= '0' && *p <= '9')
		{
			count_num++;
		}
		else if (*p == '.')
		{
			count_dot++;
			pos_dot = p - buf;
		}
		else if (*p == '+')
		{
			count_sign++;
			pos_sign = p - buf;
		}
		else if (*p == '-')
		{
			count_sign++;
			pos_sign = p - buf;
		}
		else
		{
			count_dirty++;
		}
	}
	int bufsize = p - buf;
	int frac_digits = bufsize - pos_dot - 1;

	// if the buffer contains an special character (over the + - . 0123456789 characters)
	// or if it has any trouble then call the std::strtod() function
	if (bufsize >= 256 || count_num == 0 || count_dirty || count_dot > 1 || count_sign > 1 || (count_sign && pos_sign != 0) || (count_dot && frac_digits > 18))
		return (float)std::strtod(buf,endptr);

	// fast conversion of an simple float string
	char buffer[256];
	const char* buf_int;			// integer part of string
	const char* buf_frac;			// fraction part of string
	
	strncpy(buffer,buf,bufsize);
	buffer[bufsize] = 0;

	// separate the integer and the fraction buffer
	if (pos_dot != -1)
		buffer[pos_dot] = 0;

	p = buffer;
	buf_frac = p + pos_dot + 1;

	int sign = 1;
	if (*p == '-')
	{
		sign = -1;
		p++;
	}
	else if (*p == '+')
	{
		sign = 1;
		p++;
	}

	buf_int = p;

	// convert the integer part
	int val_int = 0;
	if (*buf_int)
	{
		char* end;
		val_int = std::strtol(buf_int,&end,10);
		if (end == buf_int)
		{
			*endptr = (char*)buf;
			return 0.0f;
		}
	}

	// convert the fraction part
	int val_frac = 0;
	if (*buf_frac && pos_dot != -1)
	{
		char* end;
		val_frac = std::strtol(buf_frac,&end,10);
		if (end == buf_frac)
		{
			*endptr = (char*)buf;
			return 0.0f;
		}
	}
	
	float value = (float)val_int;

	if (val_frac)
	{
		__int64 divisor = 1;
		assert(frac_digits <= 18);
		for (int i=0;i<frac_digits;i++)
			divisor *= 10;
		value += (float)val_frac / (float)divisor;
	}

	value *= float(sign);

	*endptr = (char*)buf + bufsize;

	return value;
}

bool StringToInt(const string& str, int& value)
{
	const char* cstr = str.c_str();
	char* end;
	value = strtol(cstr, &end, 10);
	if (end == cstr)
		return false;

	return true;
}

bool StringToFloat(const string& str, float& value)
{
	const char* cstr = str.c_str();
	char* end;
	value = read_float(cstr, &end);
	if (end == cstr)
		return false;

	return true;
}

void ReportWarning(IColladaLoaderListener* pListener, const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	char szBuffer[1024];
	vsprintf(szBuffer, szFormat, args);
	pListener->OnColladaLoaderMessage(IColladaLoaderListener::MESSAGE_WARNING, szBuffer);
	va_end(args);
}

void ReportInfo(IColladaLoaderListener* pListener, const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	char szBuffer[1024];
	vsprintf(szBuffer, szFormat, args);
	pListener->OnColladaLoaderMessage(IColladaLoaderListener::MESSAGE_INFO, szBuffer);
	va_end(args);
}

void ReportError(IColladaLoaderListener* pListener, const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	char szBuffer[1024];
	vsprintf(szBuffer, szFormat, args);
	pListener->OnColladaLoaderMessage(IColladaLoaderListener::MESSAGE_ERROR, szBuffer);
	va_end(args);
}

void SplitString(const string& inputString, const char sepChar, std::vector<string>& parts)
{
	string str = inputString;

	while (str.length())
	{
		// remove unnecessary separators
		while (str.length() && str[0] == sepChar)
			str = str.erase(0,1);

		if (!str.length())
			break;

		size_t pos = str.find(sepChar);

		if (pos == string::npos || pos > 0)
		{
			string slice;
			slice = str.substr(0,pos);
			if (pos == string::npos)
				str.clear();
			else
				str = str.substr(pos+1, string::npos);
			parts.push_back(slice);
		}
	}
}

void SplitString(const string& inputString, const string& sepString, std::vector<string>& parts)
{
	string str = inputString;
	const size_t sepLength = sepString.length();

	for (;;)
	{
		// remove unnecessary separators
		while ((str.length() >= sepLength) && (str.substr(0,sepLength) == sepString))
		{
			str = str.erase(0,sepLength);
		}

		if (str.empty())
		{
			break;
		}

		const size_t pos = str.find(sepString);

		parts.push_back( str.substr(0,pos) );

		if (pos == string::npos)
			str.clear();
		else
			str = str.substr(pos+sepLength, string::npos);
	}
}

IntStream::IntStream(const char* text)
:	position(text)
{
}

int IntStream::Read()
{
	char* end;
	int value = int(std::strtol(this->position, &end, 0));
	if (end == this->position)
		return -1;
	else
		this->position = end;
	return value;
}

IColladaArray* CreateColladaArray(ColladaArrayType type)
{
	switch (type)
	{
		case TYPE_FLOAT:
			return new ColladaFloatArray();
		case TYPE_INT:
			return new ColladaIntArray();
		case TYPE_NAME:
			return new ColladaNameArray();
		case TYPE_BOOL:
			return new ColladaBoolArray();
		case TYPE_ID:
			return new ColladaIDArray();

		default:
			return 0;
	}
}

bool ColladaLoader::Load(std::vector<ExportFile>& exportFiles, std::vector<sMaterialLibrary>& materialLibraryList, const char* szFileName, ICryXML* pCryXML, IPakSystem* pPakSystem, IColladaLoaderListener* pListener)
{
	pCryXML->AddRef();
	bool success = ColladaLoaderImpl().Load(exportFiles,materialLibraryList,szFileName, pCryXML, pPakSystem, pListener);
	pCryXML->Release();
	return success;
}

string TidyCAFName(string filename)
{
	for (string::iterator it = filename.begin(), end = filename.end(); it != end; ++it)
	{
		char c = *it;
		if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9'))
			*it = '_';
	}
	return filename;
}

bool ColladaLoaderImpl::Load(std::vector<ExportFile>& exportFiles, std::vector<sMaterialLibrary>& materialLibraryList, const char* szFileName, ICryXML* pCryXML, IPakSystem* pPakSystem, IColladaLoaderListener* pListener)
{
	XmlNodeRef root = this->LoadXML(szFileName, pPakSystem, pCryXML, pListener);
	if (root == 0)
		return false;

	std::map<string, XmlNodeRef> idMap;
	std::multimap<string, XmlNodeRef> tagMap;
	if (!this->ParseElementIDs(root, idMap, tagMap, pListener))
		return false;


	ColladaAssetMetaData assetMetaData;
	if (tagMap.find("asset") != tagMap.end())
	{
		this->ParseMetaData((*tagMap.find("asset")).second, &assetMetaData);
	}

	ImageMap images;
	if (!this->ParseImages(images, tagMap, pListener))
		return false;

	EffectMap effects;
	if (!this->ParseEffects(effects, images, tagMap, pListener))
		return false;

	MaterialMap materials;
	if (!this->ParseMaterials(materials, effects, tagMap, pListener))
		return false;

	ArrayMap arrays;
	if (!this->ParseArrays(arrays, tagMap, pListener))
		return false;

	DataSourceMap sources;
	if (!this->ParseDataSources(sources, arrays, tagMap, pListener))
		return false;

	VertexStreamMap vertexStreams;
	if (!this->ParseVertexStreams(vertexStreams, sources, tagMap, pListener))
		return false;

	GeometryMap geometries;
	if (!this->ParseGeometries(geometries, materials, vertexStreams, sources, tagMap, pListener))
		return false;

	ControllerMap morphControllers;
	if (!this->ParseMorphControllers(morphControllers, geometries, sources, tagMap, pListener))
		return false;

	ControllerMap skinControllers;
	if (!this->ParseSkinControllers(skinControllers, morphControllers, geometries, sources, tagMap, assetMetaData.GetUnitSizeInMeters(), pListener))
		return false;

	SceneMap scenes;
	if (!this->ParseSceneDefinitions(scenes, geometries, skinControllers, materials, tagMap, assetMetaData.GetUnitSizeInMeters(), pListener))
		return false;

	AnimationMap animations;
	if (!this->ParseAnimations(animations, scenes, sources, tagMap, assetMetaData.GetUnitSizeInMeters(), pListener))
		return false;

	AnimClipMap animclips;
	if (!this->ParseAnimClips(animclips, animations, scenes, tagMap, pListener))
		return false;

	ColladaAuthorTool const authorTool = assetMetaData.GetAuthorTool();

	for (SceneMap::iterator itscene = scenes.begin(); itscene != scenes.end(); itscene++)
	{
		ColladaScene* scene = (*itscene).second;

		int contentFlags = 0;
		if (scene->GetExportType() == EXPORT_CAF || scene->GetExportType() == EXPORT_CHR_CAF)
			contentFlags |= ContentFlags_Animation;
		if (scene->GetExportType() != EXPORT_CAF)
			contentFlags |= ContentFlags_Geometry;

		CGFMaterialMap cgfMaterials;
		NodeMaterialMap nodeMaterialMap;
		CContentCGF* cgf = scene->CreateContentCGF(
			szFileName, *scene->GetNodesNameMap(), nodeMaterialMap, assetMetaData, cgfMaterials, pListener, contentFlags);
		if (cgf == 0)
		{
			continue;
		}

		// set skinning info for character
		if (scene->GetExportType() == EXPORT_CHR || scene->GetExportType() == EXPORT_CHR_CAF)
		{
			if (!scene->SetSkinningInfoCGF(cgf, assetMetaData, pListener,*scene->GetNodesNameMap()))
				continue;

			if (!scene->SetBoneParentFrames(cgf, pListener))
				continue;

			if (!scene->SetIKParams(cgf, pListener))
				continue;

			if (!scene->AddCollisionMeshesCGF(cgf, assetMetaData, *scene->GetNodesNameMap(), cgfMaterials, nodeMaterialMap, pListener))
				continue;

			if (!scene->AddMorphTargetsCGF(cgf, assetMetaData, pListener))
				continue;
		}

		switch (scene->GetExportType())
		{
		case EXPORT_CGA:
		case EXPORT_CGA_ANM:
			EnsureRootHasIdentityTransform(cgf, pListener);
			break;
		default:
			break;
		}

		// fill out ExportFile
		switch (scene->GetExportType())
		{
			case EXPORT_CGF:
			case EXPORT_CHR:
			case EXPORT_CGA:
				{
					ExportFile exportfile;
					exportfile.authorTool = authorTool;
					exportfile.name = scene->GetExportNodeName();
					exportfile.type = scene->GetExportType();
					exportfile.pCGF = cgf;
					std::vector<ColladaAnimClip*> animClipList;
					scene->GetAnimClipList(animclips,animClipList,pListener);
					// If it is a CGA file and has some animations, add a default animation info.
					if (scene->GetExportType() == EXPORT_CGA && animClipList.size() > 0)
						exportfile.pCtrlSkinningInfo = scene->CreateControllerTCBSkinningInfo(cgf, animClipList[0], assetMetaData, pListener);
					else
						exportfile.pCtrlSkinningInfo = NULL;
					exportFiles.push_back(exportfile);
				}
				break;

			case EXPORT_ANM:
				{
					std::vector<ColladaAnimClip*> animClipList;
					scene->GetAnimClipList(animclips,animClipList,pListener);
					for (int i=0; i<animClipList.size(); i++)
					{
						ExportFile exportfile;
						exportfile.authorTool = authorTool;
						exportfile.name = scene->GetExportNodeName()+"_"+animClipList[i]->GetAnimClipID();
						exportfile.type = scene->GetExportType();
						exportfile.pCGF = cgf;
						exportfile.pCtrlSkinningInfo = scene->CreateControllerTCBSkinningInfo(cgf, animClipList[i], assetMetaData, pListener);

						if (exportfile.pCtrlSkinningInfo->m_TrackVec3.size() != 0 || exportfile.pCtrlSkinningInfo->m_TrackQuat.size() != 0)
						{
							exportFiles.push_back(exportfile);
						}
					}
				}
				break;

			case EXPORT_CAF:
				{
					std::vector<ColladaAnimClip*> animClipList;
					scene->GetAnimClipList(animclips,animClipList,pListener);
					for (int i=0; i<animClipList.size(); i++)
					{
						ExportFile exportfile;
						exportfile.authorTool = authorTool;
						exportfile.name = TidyCAFName(animClipList[i]->GetAnimClipID());
						exportfile.type = scene->GetExportType();
						exportfile.pCGF = cgf;
						exportfile.pCtrlSkinningInfo = scene->CreateControllerSkinningInfo(cgf, animClipList[i], *scene->GetNodesNameMap(), assetMetaData, pListener);
						exportFiles.push_back(exportfile);
					}
				}
				break;

			case EXPORT_CGA_ANM:
				{
					// CGA
					ExportFile exportfile;
					exportfile.authorTool = authorTool;
					exportfile.name = scene->GetExportNodeName();
					exportfile.type = EXPORT_CGA;
					exportfile.pCGF = cgf;
					exportfile.pCtrlSkinningInfo = NULL;
					exportFiles.push_back(exportfile);

					// ANM
					std::vector<ColladaAnimClip*> animClipList;
					scene->GetAnimClipList(animclips,animClipList,pListener);
					for (int i=0; i<animClipList.size(); i++)
					{
						ExportFile exportfile;
						exportfile.authorTool = authorTool;
						exportfile.name = scene->GetExportNodeName()+"_"+animClipList[i]->GetAnimClipID();
						exportfile.type = EXPORT_ANM;
						exportfile.pCGF = cgf;
						exportfile.pCtrlSkinningInfo = scene->CreateControllerTCBSkinningInfo(cgf, animClipList[i], assetMetaData, pListener);

						if (exportfile.pCtrlSkinningInfo->m_TrackVec3.size() != 0 || exportfile.pCtrlSkinningInfo->m_TrackQuat.size() != 0)
						{
							exportFiles.push_back(exportfile);
						}
					}
				}
				break;

			case EXPORT_CHR_CAF:
				{
					// CHR
					ExportFile exportfile;
					exportfile.authorTool = authorTool;
					exportfile.name = scene->GetExportNodeName();
					exportfile.type = EXPORT_CHR;
					exportfile.pCGF = cgf;
					exportfile.pCtrlSkinningInfo = NULL;
					exportFiles.push_back(exportfile);

					// CAF
					std::vector<ColladaAnimClip*> animClipList;
					scene->GetAnimClipList(animclips,animClipList,pListener);
					for (int i=0; i<animClipList.size(); i++)
					{
						ExportFile exportfile;
						exportfile.authorTool = authorTool;

						// Remove non-alphanumeric characters from name.
						exportfile.name = TidyCAFName(animClipList[i]->GetAnimClipID());
						exportfile.type = EXPORT_CAF;
						exportfile.pCGF = cgf;
						exportfile.pCtrlSkinningInfo = scene->CreateControllerSkinningInfo(cgf, animClipList[i], *scene->GetNodesNameMap(), assetMetaData, pListener);
						exportFiles.push_back(exportfile);
					}
				}
				break;
		}
	}

	CreateMaterialLibrariesContent(materials, materialLibraryList);

	ReportInfo(pListener, "\nFinished loading '%s'\n", szFileName);

	return true;
}

void ColladaLoaderImpl::CreateMaterialLibrariesContent(MaterialMap& materials, std::vector<sMaterialLibrary>& materialLibraryList)
{
	std::set<string> librarySet;
	for (MaterialMap::iterator itMaterial = materials.begin(); itMaterial != materials.end(); itMaterial++)
	{
		ColladaMaterial* colladaMaterial = (*itMaterial).second;
		string libraryName = colladaMaterial->GetLibrary();
		assert(!libraryName.empty());		// must be filled already

		librarySet.insert(libraryName);
	}

	for (std::set<string>::iterator itLib = librarySet.begin(); itLib != librarySet.end(); itLib++)
	{
		string libraryName = (*itLib);

		sMaterialLibrary materialLibrary;
		materialLibrary.flag = 256;
		materialLibrary.library = libraryName;

		for (MaterialMap::iterator itMaterial = materials.begin(); itMaterial != materials.end(); itMaterial++)
		{
			ColladaMaterial* colladaMaterial = (*itMaterial).second;
			if (colladaMaterial->GetLibrary() != libraryName)
				continue;

			ColladaEffect* colladaEffect = colladaMaterial->GetEffect();
			if (colladaEffect)
			{
				sMaterial material;

				material.name = colladaMaterial->GetMatName();
				material.flag = 128;
				material.diffuse = ColladaColor(0.3f,0.3f,0.3f,1.0f);		//colladaEffect->GetDiffuse();
				material.emissive = ColladaColor(0.0f,0.0f,0.0f,1.0f);		//colladaEffect->GetEmission();
				material.specular = ColladaColor(0.0f,0.0f,0.0f,1.0f);		//colladaEffect->GetSpecular();
				material.shininess = 0.0f;									//colladaEffect->GetShininess();
				material.opacity = 1.0f;									//colladaEffect->GetTransparency();
				material.surfacetype = "";
				material.shader = colladaEffect->GetShader();

				if (colladaEffect->GetDiffuseImage())
				{
					sTexture texture;
					texture.type = "Diffuse";
					texture.filename = colladaEffect->GetDiffuseImage()->GetRelativeFileName();
					material.textures.push_back(texture);
				}

				if (colladaEffect->GetSpecularImage())
				{
					sTexture texture;
					texture.type = "Specular";
					texture.filename = colladaEffect->GetSpecularImage()->GetRelativeFileName();
					material.textures.push_back(texture);
				}

				if (colladaEffect->GetNormalImage())
				{
					sTexture texture;
					texture.type = "NormalMap";
					texture.filename = colladaEffect->GetNormalImage()->GetRelativeFileName();
					material.textures.push_back(texture);
				}

				materialLibrary.materials.push_back(material);
			}
		}

		materialLibraryList.push_back(materialLibrary);
	}
}

void ColladaLoaderImpl::EnsureRootHasIdentityTransform(CContentCGF* cgf, IColladaLoaderListener* pListener)
{
	// This function is performed only for CGAs. This is because the runtime vehicle system (which
	// is based on CGAs) doesn't properly handle the case where nodes representing components (such as
	// doors or sparewheels or turrents, etc) have local transforms different from their global transforms.
	// Therefore we push the root transform down to its children and then reset the root transform. This
	// should have no effect on the geometry, as the transforms should concatenate to the same thing.

	// TODO: Is it sufficient to push this down to the children of the root? Probably it should really
	// go down to the actual mesh nodes, but some meshes have children of their own, so this kind of
	// can't work. Best solution, fix the runtime (but we need to support old builds of CryEngine, so
	// we should do our best here).

	// Loop through all non-roots.
	for (int nodeIndex = 0, nodeCount = cgf->GetNodeCount(); nodeIndex < nodeCount; ++nodeIndex)
	{
		CNodeCGF* node = cgf->GetNode(nodeIndex);
		if (node->pParent)
		{
			CNodeCGF* root;
			for (root = node->pParent; root->pParent; root = root->pParent);

			if (root->pMesh == 0)
			{
				Matrix34 tm = root->localTM * node->localTM;
				node->localTM.SetTranslation(tm.GetTranslation());
				DecomposeTransform(node->pos, node->rot, node->scl, node->localTM);
				node->worldTM = node->localTM;
				tm.SetTranslation(Vec3(ZERO));

				if (node->pMesh != NULL)
				{
					node->pMesh->m_bbox.SetTransformedAABB(tm,node->pMesh->m_bbox);

					for (int j = 0; j < node->pMesh->GetVertexCount(); j++)
					{
						node->pMesh->m_pPositions[j] = tm.TransformPoint( node->pMesh->m_pPositions[j] );
						node->pMesh->m_pNorms[j] = tm.TransformVector( node->pMesh->m_pNorms[j] ).GetNormalized();
					}
				}
			}
		}

	}

	// Loop through all the nodes, looking for root nodes.
	for (int nodeIndex = 0, nodeCount = cgf->GetNodeCount(); nodeIndex < nodeCount; ++nodeIndex)
	{
		CNodeCGF* root = cgf->GetNode(nodeIndex);
		if (root->pParent == 0 && root->pMesh == 0)
		{
			root->localTM.SetIdentity();
			root->worldTM.SetIdentity();
			root->pos = Vec3(ZERO);
			root->rot = CryQuat(IDENTITY);
			root->scl = Vec3(ZERO);
		}
	}
}

XmlNodeRef ColladaLoaderImpl::LoadXML(const char* szFileName, IPakSystem* pPakSystem, ICryXML* pCryXML, IColladaLoaderListener* pListener)
{
	ReportInfo(pListener, "Loading XML...");

	// Get the xml serializer.
	IXMLSerializer* pSerializer = pCryXML->GetXMLSerializer();

	// Read in the input file.
	XmlNodeRef root;
	{
		const bool bRemoveNonessentialSpacesFromContent = false;
		char szErrorBuffer[1024];
		szErrorBuffer[0] = 0;
		root = pSerializer->Read(PakXmlFileBufferSource(pPakSystem, szFileName), bRemoveNonessentialSpacesFromContent, sizeof(szErrorBuffer), szErrorBuffer);
		if (!root)
		{
			const char* const pErrorStr = 
				(szErrorBuffer[0])
				? &szErrorBuffer[0] 
				: "Probably this file has bad XML syntax or it's not XML file at all.";
			ReportError(pListener, "Cannot read file '%s': %s.\n", szFileName, pErrorStr);
			return 0;
		}
	}

	ReportInfo(pListener, "Finished loading XML.\n");

	return root;
}

bool ColladaLoaderImpl::ParseMetaData(XmlNodeRef node, ColladaAssetMetaData* const pMetadata)
{
	XmlNodeRef contributorNode = node->findChild("contributor");
	XmlNodeRef authorNode = contributorNode->findChild("authoring_tool");
	string authorContent = authorNode->getContent();

	ColladaAuthorTool authorTool = TOOL_UNKNOWN;
	if (authorContent.find("XSI") != string::npos)
		authorTool = TOOL_SOFTIMAGE_XSI;
	else if (authorContent.find("Motion") != string::npos)
		authorTool = TOOL_MOTIONBUILDER;
	else if (authorContent.find("Maya") != string::npos)
		authorTool = TOOL_MAYA;
	else if ((authorContent.find("Max") != string::npos) || (authorContent.find("MAX") != string::npos))
		authorTool = TOOL_AUTODESK_MAX;

	ColladaAssetMetaData::EUpAxis upAxis = ColladaAssetMetaData::eUA_Y;
	{
		XmlNodeRef upAxisNode = node->findChild("up_axis");
		if (upAxisNode)
		{
			for (const char* pChar = upAxisNode->getContent(); *pChar; ++pChar)
			{
				switch (*pChar)
				{
				case 'x': case 'X': upAxis = ColladaAssetMetaData::eUA_X; break;
				case 'y': case 'Y': upAxis = ColladaAssetMetaData::eUA_Y; break;
				case 'z': case 'Z': upAxis = ColladaAssetMetaData::eUA_Z; break;
				}
			}
		}
	}

	float unitSize = 0.01f; // Default to centimeters.
	{
		XmlNodeRef unitNode = node->findChild("unit");
		if (unitNode)
		{
			if (unitNode->haveAttr("meter"))
			{
				unitNode->getAttr("meter", unitSize);
			}
		}
	}

	if (pMetadata)
	{
		pMetadata->set(upAxis, unitSize, authorTool);
	}

	return true;
}

bool ColladaLoaderImpl::ParseElementIDs(XmlNodeRef node, std::map<string, XmlNodeRef>& idMap, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	// Check whether the node has an ID attribute.
	if (node->haveAttr("id"))
		idMap.insert(std::make_pair(node->getAttr("id"), node));

	// Add the node to the tag map.
	string tag = node->getTag();
	std::transform(tag.begin(), tag.end(), tag.begin(), std::tolower);
	tagMap.insert(std::make_pair(tag, node));

	// Recurse to the children.
	for (int childIndex = 0; childIndex < node->getChildCount(); ++childIndex)
	{
		XmlNodeRef child = node->getChild(childIndex);
		if (!this->ParseElementIDs(child, idMap, tagMap, pListener))
			return false;
	}

	return true;
}


bool ColladaLoaderImpl::ParseArrays(ArrayMap& arrays, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	// Create a list of node tags that indicate arrays.
	std::pair<string, ColladaArrayType> typeList[] = {
		std::make_pair("float_array", TYPE_FLOAT),
		std::make_pair("int_array", TYPE_INT),
		std::make_pair("name_array", TYPE_NAME),
		std::make_pair("bool_array", TYPE_BOOL),
		std::make_pair("idref_array", TYPE_ID)};

	// Loop through all the types of array.
	for (int typeIndex = 0; typeIndex < sizeof(typeList) / sizeof(typeList[0]); ++typeIndex)
	{
		const string& tag = typeList[typeIndex].first;
		ColladaArrayType type = typeList[typeIndex].second;

		// Loop through all the elements with this tag.
		typedef std::multimap<string, XmlNodeRef> TagMap;
		std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range(tag);
		for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
		{
			XmlNodeRef node = (*itNodeEntry).second;
			if (node->haveAttr("id"))
			{
				IColladaArray* pArray = this->ParseArray(node, type, pListener);
				if (pArray)
					arrays.insert(std::make_pair(node->getAttr("id"), pArray));
				else
					ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as array.", node->getLine(), node->getTag());
			}
			else
			{
				ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
			}
		}
	}

	return true;
}

IColladaArray* ColladaLoaderImpl::ParseArray(XmlNodeRef node, ColladaArrayType type, IColladaLoaderListener* pListener)
{
	IColladaArray* pArray = CreateColladaArray(type);
	if (!pArray->Parse(node->getContent(), pListener))
	{
		delete pArray;
		ReportError(pListener, LINE_LOG" '%s' contains invalid data.", node->getLine(), node->getTag());
		return 0;
	}
	return pArray;
}

bool ColladaLoaderImpl::ParseColladaVector(XmlNodeRef node, Vec2& param, IColladaLoaderListener* pListener)
{
	std::vector<float> values;
	if (!ParseColladaFloatList(node,2,values,pListener))
		return false;

	for (int i=0;i<2;i++)
		param[i] = values[i];

	return true;
}

bool ColladaLoaderImpl::ParseColladaVector(XmlNodeRef node, Vec3& param, IColladaLoaderListener* pListener)
{
	std::vector<float> values;
	if (!ParseColladaFloatList(node,3,values,pListener))
		return false;

	for (int i=0;i<3;i++)
		param[i] = values[i];

	return true;
}

bool ColladaLoaderImpl::ParseColladaVector(XmlNodeRef node, Vec4& param, IColladaLoaderListener* pListener)
{
	std::vector<float> values;
	if (!ParseColladaFloatList(node,4,values,pListener))
		return false;

	for (int i=0;i<4;i++)
		param[i] = values[i];

	return true;
}

// Parse 4x4 float, and convert it to 3x4 matrix. The last column must be [0,0,0,1]
bool ColladaLoaderImpl::ParseColladaMatrix(XmlNodeRef node, Matrix34& param, IColladaLoaderListener* pListener)
{
	std::vector<float> values;
	if (!ParseColladaFloatList(node,16,values,pListener))		// read the 4x4 float values
		return false;

	// Convert to 3x4 matrix
	int v = 0;
	for (int i=0;i<3;i++)
		for (int j=0;j<4;j++)
			param(i,j) = values[v++];

	// The last column must be [0,0,0,1]
	if (values[v++] != 0.0f)
		return false;
	if (values[v++] != 0.0f)
		return false;
	if (values[v++] != 0.0f)
		return false;
	if (values[v++] != 1.0f)
		return false;

	return true;
}

bool ColladaLoaderImpl::ParseColladaFloatList(XmlNodeRef node, int listSize, std::vector<float>& values, IColladaLoaderListener* pListener)
{
	bool parseError = false;

	IColladaArray* pArray = this->ParseArray(node, TYPE_FLOAT, pListener);
	if (!pArray || !pArray->ReadAsFloat(values,1,0,pListener))
		parseError = true;

	if (values.size() != listSize)
		parseError = true;

	return !parseError;
}

bool ColladaLoaderImpl::ParseColladaColor(XmlNodeRef node, const string& parentTag, ColladaColor& color, IColladaLoaderListener* pListener)
{
	bool parseError = false;

	string nodeSid;
	if (node->haveAttr("sid"))
		nodeSid = node->getAttr("sid");

	std::vector<float> values;
	IColladaArray* pArray = this->ParseArray(node, TYPE_FLOAT, pListener);
	if (!pArray || !pArray->ReadAsFloat(values,1,0,pListener))
		parseError = true;

	if (values.size() == 4)
	{
		color.r = values[0];
		color.g = values[1];
		color.b = values[2];
		color.a = values[3];
	}
	else
	{
		parseError = true;
	}

	if (nodeSid != parentTag)
		ReportWarning(pListener, LINE_LOG" Color SID ('%s') and the parent tag ('%s') does not match.", node->getLine(), nodeSid.c_str(), parentTag.c_str());

	if (parseError)
		ReportWarning(pListener, LINE_LOG" Cannot parse color '%s'.", node->getLine(), parentTag.c_str());
	
	return !parseError;
}

bool ColladaLoaderImpl::ParseColladaFloatParam(XmlNodeRef node, const string& parentTag, float& param, IColladaLoaderListener* pListener)
{
	bool parseError = false;

	string nodeSid;
	if (node->haveAttr("sid"))
		nodeSid = node->getAttr("sid");

	std::vector<float> values;
	IColladaArray* pArray = this->ParseArray(node, TYPE_FLOAT, pListener);
	if (!pArray || !pArray->ReadAsFloat(values,1,0,pListener))
		parseError = true;

	if (values.size() == 1)
		param = values[0];
	else
		parseError = true;

	if (nodeSid != parentTag)
		ReportWarning(pListener, LINE_LOG" float SID ('%s') and the parent tag ('%s') does not match.", node->getLine(), nodeSid.c_str(), parentTag.c_str());

	if (parseError)
		ReportWarning(pListener, LINE_LOG" Cannot parse float parameter '%s'.", node->getLine(), parentTag.c_str());

	return !parseError;
}

bool ColladaLoaderImpl::ParseDataSources(DataSourceMap& sources, ArrayMap& arrays, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("source");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			ColladaDataSource* pDataSource = this->ParseDataSource(node, arrays, pListener);
			if (pDataSource)
				sources.insert(std::make_pair(node->getAttr("id"), pDataSource));
			else
				ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as data source.", node->getLine(), node->getTag());
		}
		else
		{
			// the effect sources has some simple <source> string </source> format (it has no id attribute)
			//ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

ColladaArrayType AttrToColladaArrayType(string attribute)
{
	std::transform(attribute.begin(), attribute.end(), attribute.begin(), tolower);
	if (attribute == "float")
		return TYPE_FLOAT;
	if (attribute == "float4x4")
		return TYPE_MATRIX;
	if (attribute == "int")
		return TYPE_INT;
	if (attribute == "name")
		return TYPE_NAME;
	if (attribute == "bool")
		return TYPE_BOOL;
	if (attribute == "idref")
		return TYPE_ID;
	return ColladaArrayType(-1);
}


ColladaDataSource* ColladaLoaderImpl::ParseDataSource(XmlNodeRef node, ArrayMap& arrays, IColladaLoaderListener* pListener)
{
	ColladaDataAccessor* pAccessor = 0;

	// Look for the common technique tag - either a tag called technique with an
	// attribute of profile="COMMON" or technique_common tag (Collada v1.3/1.4).
	XmlNodeRef techniqueCommonNode = 0;
	for (int childIndex = 0; childIndex < node->getChildCount(); ++childIndex)
	{
		XmlNodeRef child = node->getChild(childIndex);
		string childTag = child->getTag();
		std::transform(childTag.begin(), childTag.end(), childTag.begin(), std::tolower);

		if (childTag == "technique_common")
		{
			techniqueCommonNode = child;
			break;
		}
		else if (childTag == "technique")
		{
			if (child->haveAttr("profile") && string("COMMON") == child->getAttr("profile"))
				techniqueCommonNode = child;
		}
	}

	if (techniqueCommonNode)
	{
		// Look inside the technique node for an accessor node.
		XmlNodeRef accessorNode = techniqueCommonNode->findChild("accessor");
		if (accessorNode)
		{
			if (accessorNode->haveAttr("source") && accessorNode->haveAttr("stride"))
			{
				int stride;
				accessorNode->getAttr("stride", stride);
				int size = 0;
				if (accessorNode->haveAttr("count"))
					accessorNode->getAttr("count", size);
				int offset = 0;
				if (accessorNode->haveAttr("offset"))
					accessorNode->getAttr("offset", offset);

				bool foundSource = false;
				string sourceID = accessorNode->getAttr("source");

				// Try to find the source reference id.
				size_t hashPos = sourceID.rfind('#');
				if (hashPos != string::npos && hashPos > 0)
				{
					ReportWarning(pListener, "'accessor' node uses non-local data URI - cannot load data source ('%s').", sourceID.c_str());
				}
				else if (hashPos != string::npos)
				{
					sourceID = sourceID.substr(hashPos + 1, string::npos);
					foundSource = true;
				}
				else
				{
					foundSource = true;
				}

				if (foundSource)
				{
					// Look up the data source.
					ArrayMap::iterator itArrayEntry = arrays.find(sourceID);
					if (itArrayEntry != arrays.end())
					{
						IColladaArray* array = (*itArrayEntry).second;
						pAccessor = new ColladaDataAccessor(array, stride, offset, size);

						// Look for parameter entries.
						int offset = 0;
						for (int childIndex = 0; childIndex < accessorNode->getChildCount(); ++childIndex)
						{
							XmlNodeRef child = accessorNode->getChild(childIndex);
							string childTag = child->getTag();
							std::transform(childTag.begin(), childTag.end(), childTag.begin(), tolower);
							if (childTag == "param")
							{
								if (child->haveAttr("type"))
								{
									// Read the type attribute and look up the type.
									ColladaArrayType type = AttrToColladaArrayType(child->getAttr("type"));
									if (type >= 0)
									{
										string name;
										if (child->haveAttr("name"))
										{
											name = child->getAttr("name");
										}
										else
										{
											char id[16];
											ltoa(offset,id,10);
											name = "param_offs_";
											name.append(id);
										}

										std::transform(name.begin(), name.end(), name.begin(), tolower);
										pAccessor->AddParameter(name, type, offset);
									}
									else
									{
										ReportWarning(pListener, "'param' node has unknown 'type' attribute ('%s' - skipping param.", child->getAttr("type"));
									}
								}
								else
								{
									ReportWarning(pListener, "'param' node needs to specify a 'type' attribute - skipping param.");
								}
								++offset;
							}
						}
					}
					else
					{
						ReportWarning(pListener, "'accessor' node refers to non-existent data array ('%s') - cannot load data source.", sourceID.c_str());
						return 0;
					}
				}
				else
				{
					return 0;
				}
			}
			else
			{
				ReportWarning(pListener, "'accessor' node requires 'source' and 'stride' attributes.");
				return 0;
			}
		}
		else
		{
			ReportWarning(pListener, "Data source contains no accessor (cannot understand format without it).");
			return 0;
		}
	}
	else
	{
		ReportWarning(pListener, "Data source contains no common technique (cannot understand format without it).");
		return 0;
	}

	if (pAccessor != 0)
		return new ColladaDataSource(pAccessor);
	return 0;
}

bool ColladaLoaderImpl::ParseMaterials(MaterialMap& materials, EffectMap& effects, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("material");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			ColladaMaterial* pMaterial = this->ParseMaterial(node, effects, pListener);
			if (pMaterial)
				materials.insert(std::make_pair(node->getAttr("id"), pMaterial));
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains no 'id' attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

ColladaMaterial* ColladaLoaderImpl::ParseMaterial(XmlNodeRef node, EffectMap& effects, IColladaLoaderListener* pListener)
{
	string name = node->getAttr("id");

	ColladaMaterial* material = new ColladaMaterial(name);

	XmlNodeRef effectNode = node->findChild("instance_effect");
	if (!effectNode)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node contains no 'instance_effect' child.", node->getLine(), node->getTag());
		return material;
	}

	if (!effectNode->haveAttr("url"))
	{
		ReportWarning(pListener, LINE_LOG" '%s' node contains no 'url' attribute.", effectNode->getLine(), effectNode->getTag());
		return material;
	}

	string effectURL = effectNode->getAttr("url");
	if (effectURL[0] != '#')
	{
		ReportWarning(pListener, LINE_LOG" '%s' node refers to external effect data (%s) - this is not supported.", effectNode->getLine(), effectNode->getTag(), effectURL.c_str());
		return NULL;
	}

	effectURL = effectURL.substr(1, string::npos);

	EffectMap::iterator itEffectEntry = effects.find(effectURL);
	if (itEffectEntry != effects.end())
	{
		ColladaEffect* effect = (*itEffectEntry).second;
		material->SetEffect(effect);
	}

	return material;
}

bool ColladaLoaderImpl::ParseImages(ImageMap& images, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("image");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			ColladaImage* pImage = this->ParseImage(node, pListener);
			if (pImage)
				images.insert(std::make_pair(node->getAttr("id"), pImage));
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

ColladaImage* ColladaLoaderImpl::ParseImage(XmlNodeRef node, IColladaLoaderListener* pListener)
{
	string name,filename;
	
	name = node->getAttr("id");
	XmlNodeRef fileNode = node->findChild("init_from");
	if (fileNode)
	{
		filename = fileNode->getContent();
		if (filename.substr(0,8) == "file:///")
			filename = filename.substr(8,string::npos);
	}

	ColladaImage* image = new ColladaImage(name);
	image->SetFileName(filename);
	
	return image;
}

bool ColladaLoaderImpl::ParseEffects(EffectMap& effects, ImageMap& images, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("effect");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			ColladaEffect* pEffect = this->ParseEffect(node, images, pListener);
			if (pEffect)
				effects.insert(std::make_pair(node->getAttr("id"), pEffect));
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

ColladaImage* ColladaLoaderImpl::GetImageFromTextureParam(XmlNodeRef profileNode, XmlNodeRef textureNode, ImageMap& images, IColladaLoaderListener* pListener)
{
	if (!textureNode->haveAttr("texture"))
	{
		ReportWarning(pListener, LINE_LOG" '%s' node has no 'texture' attribute.", textureNode->getLine(), textureNode->getTag());
		return NULL;
	}

	string texture = textureNode->getAttr("texture");

	ColladaImage* image = NULL;
	bool foundimage = false;
	for (int i=0;i<profileNode->getChildCount();i++)
	{
		XmlNodeRef paramNode = profileNode->getChild(i);
		string paramTag = paramNode->getTag();
		if (paramTag != "newparam")
			continue;
		
		if (!paramNode->haveAttr("sid"))
			continue;
		
		string sampler = paramNode->getAttr("sid");
		if (sampler != texture)
			continue;

		XmlNodeRef sampler2DNode = paramNode->findChild("sampler2D");
		if (!sampler2DNode)
			continue;

		XmlNodeRef sourceNode = sampler2DNode->findChild("source");
		if (!sourceNode)
			continue;

		string source = sourceNode->getContent();

		for (int j=0;j<profileNode->getChildCount();j++)
		{
			if (i == j)
				continue;

			XmlNodeRef paramNode2 = profileNode->getChild(j);
			string paramTag2 = paramNode2->getTag();
			if (paramTag2 != "newparam")
				continue;

			if (!paramNode2->haveAttr("sid"))
				continue;

			string surface = paramNode2->getAttr("sid");
			if (surface != source)
				continue;

			XmlNodeRef surfaceNode2 = paramNode2->findChild("surface");
			if (!surfaceNode2)
				continue;

			if (!surfaceNode2->haveAttr("type"))
				continue;

			string surfaceattr = surfaceNode2->getAttr("type");
			if (surfaceattr != "2D")
				continue;

			XmlNodeRef imageNode2 = surfaceNode2->findChild("init_from");
			if (!imageNode2)
				continue;

			string imageId = imageNode2->getContent();

			ImageMap::iterator itImageEntry = images.find(imageId);
			if (itImageEntry == images.end())
				continue;

			// found image :-)
			image = (*itImageEntry).second;
			foundimage = true;
			break;
		}

		if (foundimage)
			break;
	}

	return image;
}

ColladaEffect* ColladaLoaderImpl::ParseEffect(XmlNodeRef node, ImageMap& images, IColladaLoaderListener* pListener)
{
	string name = node->getAttr("id");
	
	XmlNodeRef profileNode = node->findChild("profile_COMMON");
	if (!profileNode)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node has no 'profile_COMMON' child.", node->getLine(), node->getTag());
		return NULL;
	}

	XmlNodeRef techniqueNode = profileNode->findChild("technique");
	if (!techniqueNode)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node has no 'technique' child.", profileNode->getLine(), profileNode->getTag());
		return NULL;
	}

	ColladaEffect* effect = new ColladaEffect(name);

	for (int shIndex=0;shIndex<techniqueNode->getChildCount();shIndex++)
	{
		XmlNodeRef shaderNode = techniqueNode->getChild(shIndex);
		
		for (int paramIndex=0;paramIndex<shaderNode->getChildCount();paramIndex++)
		{
			XmlNodeRef paramNode = shaderNode->getChild(paramIndex);

			string shaderTag = shaderNode->getTag();
			string paramTag = paramNode->getTag();

			effect->SetShader(shaderTag);

			if (paramTag == "ambient")
			{
				XmlNodeRef colorNode = paramNode->findChild("color");
				if (colorNode)
				{
					ColladaColor color;
					if (ParseColladaColor(colorNode,paramTag,color,pListener))
						effect->SetAmbient(color);
				}
			}
			else if (paramTag == "emission")
			{
				XmlNodeRef colorNode = paramNode->findChild("color");
				if (colorNode)
				{
					ColladaColor color;
					if (ParseColladaColor(colorNode,paramTag,color,pListener))
						effect->SetEmission(color);
				}
			}
			else if (paramTag == "reflective")
			{
				XmlNodeRef colorNode = paramNode->findChild("color");
				if (colorNode)
				{
					ColladaColor color;
					if (ParseColladaColor(colorNode,paramTag,color,pListener))
						effect->SetReflective(color);
				}
			}
			else if (paramTag == "transparent")
			{
				XmlNodeRef colorNode = paramNode->findChild("color");
				if (colorNode)
				{
					ColladaColor color;
					if (ParseColladaColor(colorNode,paramTag,color,pListener))
						effect->SetTransparent(color);
				}
			}
			else if (paramTag == "shininess")
			{
				XmlNodeRef floatNode = paramNode->findChild("float");
				if (floatNode)
				{
					float param;
					if (ParseColladaFloatParam(floatNode,paramTag,param,pListener))
						effect->SetShininess(param);
				}
			}
			else if (paramTag == "reflectivity")
			{
				XmlNodeRef floatNode = paramNode->findChild("float");
				if (floatNode)
				{
					float param;
					if (ParseColladaFloatParam(floatNode,paramTag,param,pListener))
						effect->SetReflectivity(param);
				}
			}
			else if (paramTag == "transparency")
			{
				XmlNodeRef floatNode = paramNode->findChild("float");
				if (floatNode)
				{
					float param;
					if (ParseColladaFloatParam(floatNode,paramTag,param,pListener))
						effect->SetTransparency(param);
				}
			}
			else if (paramTag == "diffuse")
			{
				XmlNodeRef textureNode = paramNode->findChild("texture");
				XmlNodeRef colorNode = paramNode->findChild("color");

				// parse diffuse texture
				if (textureNode)
					effect->SetDiffuseImage(GetImageFromTextureParam(profileNode,textureNode,images,pListener));

				// parse diffuse color
				if (colorNode)
				{
					ColladaColor color;
					if (ParseColladaColor(colorNode,paramTag,color,pListener))
						effect->SetDiffuse(color);
				}
			}
			else if (paramTag == "specular")
			{
				XmlNodeRef textureNode = paramNode->findChild("texture");
				XmlNodeRef colorNode = paramNode->findChild("color");

				// parse specular texture
				if (textureNode)
					effect->SetSpecularImage(GetImageFromTextureParam(profileNode,textureNode,images,pListener));

				// parse specular color
				if (colorNode)
				{
					ColladaColor color;
					if (ParseColladaColor(colorNode,paramTag,color,pListener))
						effect->SetSpecular(color);
				}
			}
			else if (paramTag == "normal")			// ?
			{
				XmlNodeRef textureNode = paramNode->findChild("texture");
				if (textureNode)
					effect->SetNormalImage(GetImageFromTextureParam(profileNode,textureNode,images,pListener));
			}
		}
	}

	return effect;
}

bool ColladaLoaderImpl::ParseVertexStreams(VertexStreamMap& vertexStreams, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("vertices");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			ColladaVertexStream* pVertexStream = this->ParseVertexStream(node, dataSources, pListener);
			if (pVertexStream)
				vertexStreams.insert(std::make_pair(node->getAttr("id"), pVertexStream));
			else
				ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as vertex stream.", node->getLine(), node->getTag());
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

ColladaVertexStream* ColladaLoaderImpl::ParseVertexStream(XmlNodeRef node, DataSourceMap& dataSources, IColladaLoaderListener* pListener)
{
	ColladaVertexStream* pVertexStream = 0;
	ColladaDataSource* pPositionDataSource = 0;

	// Loop through the children, looking for input elements.
	for (int childIndex = 0; childIndex < node->getChildCount(); ++childIndex)
	{
		XmlNodeRef inputNode = node->getChild(childIndex);
		string inputTag = inputNode->getTag();
		std::transform(inputTag.begin(), inputTag.end(), inputTag.begin(), tolower);
		if (inputTag == "input")
		{
			// Check whether this input specifies a semantic.
			if (inputNode->haveAttr("semantic"))
			{
				string semantic = inputNode->getAttr("semantic");
				std::transform(semantic.begin(), semantic.end(), semantic.begin(), tolower);
				if (semantic == "position")
				{
					if (inputNode->haveAttr("source"))
					{
						string sourceID = inputNode->getAttr("source");
						if (sourceID[0] == '#')
						{
							sourceID = sourceID.substr(1, string::npos);

							// Look up the data source.
							DataSourceMap::iterator itDataSourceEntry = dataSources.find(sourceID);
							if (itDataSourceEntry != dataSources.end())
							{
								pPositionDataSource = (*itDataSourceEntry).second;
							}
							else
							{
								ReportWarning(pListener, "'input' node refers to non-existent data array ('%s') - skipping input.", sourceID.c_str());
								return 0;
							}
						}
						else
						{
							ReportWarning(pListener, "'input' node uses non-local data URI - skipping input ('%s').", sourceID.c_str());
						}
					}
					else
					{
						ReportWarning(pListener, "'input' node does not specify data source - skipping input.");
					}
				}
			}
			else
			{
				ReportWarning(pListener, "'input' node does not specify semantic - skipping input.");
			}
		}
	}

	// If we found a source for position data, then create the vertex stream.
	if (pPositionDataSource != 0)
		pVertexStream = new ColladaVertexStream(pPositionDataSource);
	return pVertexStream;
}

bool ColladaLoaderImpl::ParseGeometries(GeometryMap& geometries, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("geometry");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			ColladaGeometry* pGeometry = this->ParseGeometry(node, materials, vertexStreams, dataSources, pListener);
			if (pGeometry)
				geometries.insert(std::make_pair(node->getAttr("id"), pGeometry));
			else
				ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as geometry.", node->getLine(), node->getTag());
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

ColladaGeometry* ColladaLoaderImpl::ParseGeometry(XmlNodeRef node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener)
{
	ColladaMesh* pMesh = 0;

	// Look for a mesh element.
	XmlNodeRef meshNode = node->findChild("mesh");
	if (meshNode)
	{
		// Look for the vertex stream - this has already been parsed and added to vertexStreams, so we should
		// just find the node, read its ID and look it up rather than re-creating it.
		XmlNodeRef vertexNode = (meshNode ? meshNode->findChild("vertices") : 0);
		const char* vertexStreamIDStr = (vertexNode ? vertexNode->getAttr("id") : 0);
		VertexStreamMap::iterator vertexStreamPos = (vertexStreamIDStr ? vertexStreams.find(vertexStreamIDStr) : vertexStreams.end());
		ColladaVertexStream* vertexStream = (vertexStreamPos != vertexStreams.end() ? (*vertexStreamPos).second : 0);

		// Look for a polygon element.
		enum PolygonStreamType
		{
			POLY_SOURCE_LINES,
			POLY_SOURCE_LINESTRIPS,
			POLY_SOURCE_POLYGONS,
			POLY_SOURCE_POLYLIST,
			POLY_SOURCE_TRIANGLES,
			POLY_SOURCE_TRIFANS,
			POLY_SOURCE_TRISTRIPS
		};

		typedef std::map<string, PolygonStreamType> StreamTypeMap;
		StreamTypeMap streamTypeMap;
		streamTypeMap.insert(std::make_pair("lines", POLY_SOURCE_LINES));
		streamTypeMap.insert(std::make_pair("linestrips", POLY_SOURCE_LINESTRIPS));
		streamTypeMap.insert(std::make_pair("polygons", POLY_SOURCE_POLYGONS));
		streamTypeMap.insert(std::make_pair("polylist", POLY_SOURCE_POLYLIST));
		streamTypeMap.insert(std::make_pair("triangles", POLY_SOURCE_TRIANGLES));
		streamTypeMap.insert(std::make_pair("trifans", POLY_SOURCE_TRIFANS));
		streamTypeMap.insert(std::make_pair("tristrips", POLY_SOURCE_TRISTRIPS));

		// Loop through the children, looking for polygon sources.
		XmlNodeRef sourceNode = 0;
		std::vector<IPrimitiveStream*> streams;
		for (int i = 0; i < meshNode->getChildCount(); ++i)
		{
			sourceNode = meshNode->getChild(i);
			string sourceTag = sourceNode->getTag();
			std::transform(sourceTag.begin(), sourceTag.end(), sourceTag.begin(), tolower);
			StreamTypeMap::iterator itStreamTypeEntry = streamTypeMap.find(sourceTag);
			IPrimitiveStream* stream = 0;
			if (itStreamTypeEntry != streamTypeMap.end())
			{
				PolygonStreamType sourceType = (*itStreamTypeEntry).second;
				switch (sourceType)
				{
					// Unsupported source types.
					case POLY_SOURCE_LINES:
					case POLY_SOURCE_LINESTRIPS:
					case POLY_SOURCE_POLYGONS:		// todo: this is exist, but not tested
					case POLY_SOURCE_TRIFANS:		// todo: this is exist, but not tested
					case POLY_SOURCE_TRISTRIPS:		// todo: this is exist, but not tested
						ReportWarning(pListener, LINE_LOG" '%s' node contains unsupported primitive source '%s'.", sourceNode->getLine(), meshNode->getTag(), sourceNode->getTag());
						break;

					// Supported source types.
					case POLY_SOURCE_POLYLIST:
						stream = new PolyStripPrimitiveStream();
						break;

					case POLY_SOURCE_TRIANGLES:
						stream = new TrianglesPrimitiveStream();
						break;

					/*
					case POLY_SOURCE_POLYGONS:
						stream = new PolygonsPrimitiveStream();
						break;

					case POLY_SOURCE_TRIFANS:
						stream = new TrifansPrimitiveStream();
						break;

					case POLY_SOURCE_TRISTRIPS:
						stream = new TriStripsPrimitiveStream();
						break;
						*/

					default:
						assert(0);
						break;
				}
			}

			// Check whether we found a primitive stream.
			if (stream != 0)
			{
				// Initialise the source object with data from the node.
				if (!stream->ParseNode(sourceNode, materials, vertexStreams, dataSources, pListener))
				{
					delete stream;
					stream = 0;
				}
				else
				{
					streams.push_back(stream);
				}
			}
		}

		if (!vertexStream)
			ReportWarning(pListener, LINE_LOG" \"%s\" node contains no \"vertices\" element.", meshNode->getLine(), meshNode->getTag());

		if (vertexStream)
		{
			pMesh = new ColladaMesh(streams, vertexStream);
		}
	
	}
	else
	{
		ReportWarning(pListener, LINE_LOG" 'geometry' node contains no 'mesh' node - only mesh geometry is supported.", node->getLine());
	}

	if (pMesh)
	{
		const char* nameStr = (node ? node->getAttr("name") : 0);
		string name = (nameStr ? nameStr : "");
		return new ColladaGeometry(name, pMesh);
	}
	return 0;
}

bool ColladaLoaderImpl::ParseMorphControllers(ControllerMap& morphControllers, GeometryMap& geometries, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("controller");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->findChild("morph"))
		{
			if (node->haveAttr("id"))
			{
				ColladaController* pController = this->ParseMorphController(node, geometries, dataSources, pListener);
				if (pController)
					morphControllers.insert(std::make_pair(node->getAttr("id"), pController));
				else
					ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as morph controller.", node->getLine(), node->getTag());
			}
			else
			{
				ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
			}
		}
	}

	return true;
}

ColladaController* ColladaLoaderImpl::ParseMorphController(XmlNodeRef node, GeometryMap& geometries, DataSourceMap& dataSources, IColladaLoaderListener* pListener)
{
	XmlNodeRef morphNode = node->findChild("morph");
	if (!morphNode)
		ReportWarning(pListener, LINE_LOG" '%s' node contains no morph element.", node->getLine(), node->getTag());

	string baseGeometryRef = (morphNode ? morphNode->getAttr("source") : "");
	if (baseGeometryRef.empty())
		ReportWarning(pListener, LINE_LOG" '%s' node contains no source attribute.", morphNode->getLine(), morphNode->getTag());
	if (!baseGeometryRef.empty() && baseGeometryRef[0] != '#')
		ReportWarning(pListener, LINE_LOG" '%s' node references external geometry '%s'.", morphNode->getLine(), morphNode->getTag(), baseGeometryRef.c_str());

	string baseGeometryID = (!baseGeometryRef.empty() && baseGeometryRef[0] == '#' ? baseGeometryRef.substr(1) : "");

	GeometryMap::iterator itGeometry = (!baseGeometryID.empty() ? geometries.find(baseGeometryID) : geometries.end());
	if (!baseGeometryID.empty() && itGeometry == geometries.end())
		ReportWarning(pListener, LINE_LOG" '%s' node references non-existent geometry '%s'.", morphNode->getLine(), morphNode->getTag(), baseGeometryID.c_str());

	ColladaGeometry* pBaseGeometry = (itGeometry != geometries.end() ? (*itGeometry).second : 0);
	ColladaController* pController = (pBaseGeometry ? new ColladaController(pBaseGeometry) : 0);

	// Look for the targets in the morph element.
	XmlNodeRef targetsNode = (morphNode ? morphNode->findChild("targets") : 0);
	for (int childIndex = 0, childCount = (targetsNode ? targetsNode->getChildCount() : 0); childIndex < childCount; ++childIndex)
	{
		XmlNodeRef targetsChildNode = (targetsNode ? targetsNode->getChild(childIndex) : 0);
		XmlNodeRef inputNode = ((stricmp(targetsChildNode->getTag(), "input") == 0) ? targetsChildNode : 0);
		const char* semanticStr = (inputNode ? inputNode->getAttr("semantic") : 0);
		bool isMorphTargetNode = (stricmp((semanticStr ? semanticStr : ""), "MORPH_TARGET") == 0);
		const char* morphSourceIDStr = (inputNode && isMorphTargetNode ? inputNode->getAttr("source") : 0);

		if (morphSourceIDStr && morphSourceIDStr[0] && morphSourceIDStr[0] != '#')
			ReportWarning(pListener, LINE_LOG"Morph target element \"%s\" references external data source \"%s\".", inputNode->getLine(), inputNode->getTag(), morphSourceIDStr);

		string morphSourceID = (morphSourceIDStr ? morphSourceIDStr + 1 : "");
		DataSourceMap::iterator dataSourcePosition = (!morphSourceID.empty() ? dataSources.find(morphSourceID) : dataSources.end());
		if (!morphSourceID.empty() && dataSourcePosition == dataSources.end())
			ReportWarning(pListener, LINE_LOG"Morph target element \"%s\" references non-existent data source \"%s\".", inputNode->getLine(), inputNode->getTag(), morphSourceIDStr);

		ColladaDataSource* dataSource = (dataSourcePosition != dataSources.end() ? (*dataSourcePosition).second : 0);
		ColladaDataAccessor* dataAccessor = (dataSource ? dataSource->GetDataAccessor() : 0);
		std::vector<string> morphTargetGeomIDs;
		if (dataAccessor)
			dataAccessor->ReadAsID("", morphTargetGeomIDs, pListener);

		// Loop through all the morph target IDs we found.
		for (int idIndex = 0, idCount = int(morphTargetGeomIDs.size()); idIndex < idCount; ++idIndex)
		{
			GeometryMap::iterator geometryPos = geometries.find(morphTargetGeomIDs[idIndex]);
			if (geometryPos == geometries.end())
				ReportWarning(pListener, LINE_LOG"Morph target references non-existent geometry \"%s\".", inputNode->getLine(), morphTargetGeomIDs[idIndex].c_str());
			ColladaGeometry* morphGeometry = (geometryPos != geometries.end() ? (*geometryPos).second : 0);
			if (morphGeometry)
				pController->AddMorphTarget(morphGeometry->GetName(), morphGeometry);
		}
	}

	return pController;
}

bool ColladaLoaderImpl::ParseSkinControllers(ControllerMap& skinControllers, ControllerMap& morphControllers, GeometryMap& geometries, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, float const scale, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("controller");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->findChild("skin"))
		{
			if (node->haveAttr("id"))
			{
				ColladaController* pController = this->ParseSkinController(node, morphControllers, geometries, dataSources, scale, pListener);
				if (pController)
					skinControllers.insert(std::make_pair(node->getAttr("id"), pController));
				else
					ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as controller.", node->getLine(), node->getTag());
			}
			else
			{
				ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
			}
		}
	}

	return true;
}

ColladaController* ColladaLoaderImpl::ParseSkinController(XmlNodeRef node, ControllerMap& morphControllers, GeometryMap& geometries, DataSourceMap& dataSources, float const scale, IColladaLoaderListener* pListener)
{
	// Look for a skin element.
	XmlNodeRef skinNode = node->findChild("skin");
	if (!skinNode)
	{
		ReportWarning(pListener, LINE_LOG" 'controller' node contains no 'skin' node.", node->getLine());
		return NULL;
	}

	if (!skinNode->haveAttr("source"))
	{
		ReportWarning(pListener, LINE_LOG" 'skin' node has no 'source' attribute.", skinNode->getLine());
		return NULL;
	}

	string geometryURL = skinNode->getAttr("source");
	if (geometryURL[0] != '#')
	{
		ReportWarning(pListener, LINE_LOG" 'skin' node refers to external geometry data (%s) - this is not supported.", skinNode->getLine(), geometryURL.c_str());
		return NULL;
	}

	geometryURL = geometryURL.substr(1, string::npos);

	// Skinning geometry can either come from a geometry element or a morph object (morphs contain
	// both the base mesh and all targets).
	ColladaController* pController = 0;

	// Look for the geometry in the geometry map.
	{
		// Look up the geometry in the geometry map.
		GeometryMap::iterator itGeometryEntry = geometries.find(geometryURL);
		if (itGeometryEntry != geometries.end())
		{
			ColladaGeometry* pGeometry = (*itGeometryEntry).second;
			pController = new ColladaController(pGeometry);
		}
	}
	// If there was no geometry, look for it in the controllers (it might be a morph element).
	if (!pController)
	{
		ControllerMap::iterator itMorphControllerEntry = morphControllers.find(geometryURL);
		if (itMorphControllerEntry != morphControllers.end())
		{
			ColladaController* pMorphController = (*itMorphControllerEntry).second;
			pController = new ColladaController(pMorphController->GetGeometry());

			// Copy the morph targets.
			for (int i = 0, count = pMorphController->GetMorphTargetCount(); i < count; ++i)
				pController->AddMorphTarget(pMorphController->GetMorphTargetName(i), pMorphController->GetMorphTargetGeometry(i));
		}
	}

	if (!pController)
	{
		ReportWarning(pListener, LINE_LOG" 'skin' node refers to non-existent geometry (%s).", skinNode->getLine(), geometryURL.c_str());
		return NULL;
	}

	// Loop through the children, looking for vertex weights.
	bool weightNodeParsed = false;
	bool parseSuccess = true;
	for (int i=0; i<skinNode->getChildCount(); i++)
	{
		XmlNodeRef childNode = skinNode->getChild(i);
		string childTag = childNode->getTag();
		std::transform(childTag.begin(), childTag.end(), childTag.begin(), tolower);
		
		if (childTag == "vertex_weights")
		{
			if (weightNodeParsed)
			{
				ReportWarning(pListener, LINE_LOG" 'skin' node has more 'vertex_weights' node - the first node will use.", skinNode->getLine());
				break;
			}
			
			weightNodeParsed = true;
			if (!pController->ParseVertexWeights(childNode, dataSources, pListener))
			{
				parseSuccess = false;
				continue;
			}

			if (!pController->ParseBoneMap(childNode, dataSources, pListener))
			{
				parseSuccess = false;
				continue;
			}
		}
		
		if (childTag == "bind_shape_matrix")
		{
			Matrix34& m = pController->GetShapeMatrix();
			if (!ParseColladaMatrix(childNode,m,pListener))
				ReportWarning(pListener, LINE_LOG" Cannot parse 'bind_shape_matrix'", childNode->getLine());
			m.m03 *= scale;
			m.m13 *= scale;
			m.m23 *= scale;
		}

		if (childTag == "joints")
		{
			if (!pController->ParseBoneMatrices(childNode, dataSources, scale, pListener))
			{
				parseSuccess = false;
				continue;
			}
		}
	}

	pController->ReduceBoneMap(node, pListener);

	//pController->ReIndexBoneIndices();		// moved to Parse ParseSceneDefinition()

	if (!parseSuccess)
		return NULL;

	return pController;
}

bool ColladaLoaderImpl::ParseAnimations(AnimationMap& animations, SceneMap & scenes, DataSourceMap& dataSources, std::multimap<string, XmlNodeRef>& tagMap, float scale, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("animation");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			ColladaAnimation* pAnimation = this->ParseAnimation(node, scenes, dataSources, scale, pListener);
			if (pAnimation)
				animations.insert(std::make_pair(node->getAttr("id"), pAnimation));
			else
				ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as animation.", node->getLine(), node->getTag());
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

ColladaAnimation* ColladaLoaderImpl::ParseAnimation(XmlNodeRef node, SceneMap& scenes, DataSourceMap& dataSources, float scale, IColladaLoaderListener* pListener)
{
	// Look for the channel element.
	XmlNodeRef channelNode = node->findChild("channel");
	if (!channelNode)
	{
		ReportWarning(pListener, LINE_LOG" 'animation' node contains no 'channel' node.", node->getLine());
		return NULL;
	}

	if (!channelNode->haveAttr("source"))
	{
		ReportWarning(pListener, LINE_LOG" 'channel' node has no 'source' attribute.", channelNode->getLine());
		return NULL;
	}

	if (!channelNode->haveAttr("target"))
	{
		ReportWarning(pListener, LINE_LOG" 'channel' node has no 'target' attribute.", channelNode->getLine());
		return NULL;
	}

	// Look for a sampler array
	string samplerID = channelNode->getAttr("source");
	if (samplerID[0] != '#')
	{
		ReportWarning(pListener, LINE_LOG" 'channel' node specifies external data source ('%s') - skipping animation.", channelNode->getLine(), samplerID.c_str());
		return NULL;
	}

	samplerID = samplerID.substr(1, string::npos);

	XmlNodeRef samplerNode = NULL;
	for (int childIndex=0; childIndex < node->getChildCount(); childIndex++)
	{
		samplerNode = node->getChild(childIndex);
		if (!samplerNode->haveAttr("id"))
			continue;
		
		string samplerTag = samplerNode->getTag();
		std::transform(samplerTag.begin(), samplerTag.end(), samplerTag.begin(), tolower);

		if (samplerTag == "sampler" && samplerNode->getAttr("id") == samplerID)
			break;
	}

	if (!samplerNode)
	{
		ReportWarning(pListener, LINE_LOG" source of 'channel' node does not found", channelNode->getLine());
		return NULL;
	}

	ColladaAnimation* pAnimation = new ColladaAnimation();

	if (!pAnimation->ParseChannel(channelNode,samplerNode,scenes,dataSources,scale,pListener))
		return NULL;

	const char* name = node->getAttr("id");
	pAnimation->ParseName(name ? name : "");

	return pAnimation;
}

bool ColladaLoaderImpl::ParseAnimClips(AnimClipMap& animclips, AnimationMap& animations, SceneMap& scenes, std::multimap<string, XmlNodeRef>& tagMap, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("animation_clip");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			ColladaAnimClip* pAnimClip = this->ParseAnimClip(node, animations, scenes, pListener);
			if (pAnimClip)
				animclips.insert(std::make_pair(node->getAttr("id"), pAnimClip));
			else
				ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as animclip.", node->getLine(), node->getTag());
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

ColladaAnimClip* ColladaLoaderImpl::ParseAnimClip(XmlNodeRef node, AnimationMap& animations, SceneMap& scenes, IColladaLoaderListener* pListener)
{
	float startTime,endTime;

	if (!node->haveAttr("start") || !StringToFloat(node->getAttr("start"),startTime))
	{
		ReportError(pListener, LINE_LOG" 'start' attribute of animation_clip is missing or invalid.", node->getLine());
		return NULL;
	}

	if (!node->haveAttr("end") || !StringToFloat(node->getAttr("end"),endTime))
	{
		ReportError(pListener, LINE_LOG" 'end' attribute of animation_clip is missing or invalid.", node->getLine());
		return NULL;
	}

	if (!node->haveAttr("id"))
	{
		ReportError(pListener, LINE_LOG" 'id' attribute of animation_clip is missing.", node->getLine());
		return NULL;
	}

	string id_attr = node->getAttr("id");
	//std::transform(id_attr.begin(), id_attr.end(), id_attr.begin(), tolower); // Removed this, animators want CAF files named in mixed case.
	std::vector<string> parts;
	SplitString(id_attr,'-',parts);
	if (parts.size() != 2)
	{
		ReportError(pListener, LINE_LOG" syntax of 'id' attribute of animation_clip is wrong.", node->getLine());
		return NULL;
	}

	string animClipID = parts[0];
	ColladaScene* targetScene = NULL;
	for (SceneMap::iterator itScene = scenes.begin(); itScene != scenes.end(); itScene++)
	{
		ColladaScene* scene = (*itScene).second;
		if (_stricmp(scene->GetExportNodeName().c_str(), parts[1].c_str()) == 0)
			targetScene = scene;
	}
	if (!targetScene)
	{
		ReportError(pListener, LINE_LOG" target scene not found in 'id' attribute of animation_clip.", node->getLine());
		return NULL;
	}

	ColladaAnimClip* pAnimClip = new ColladaAnimClip(startTime,endTime,animClipID,targetScene);

	for (int childIndex=0; childIndex < node->getChildCount(); childIndex++)
	{
		XmlNodeRef animNode = node->getChild(childIndex);
		if (!animNode->haveAttr("url"))
			continue;
		
		string animURL = animNode->getAttr("url");
		if (animURL[0] != '#')
		{
			ReportWarning(pListener, LINE_LOG" instance_animation URL refers to external animation data (%s) - this is not supported.", animNode->getLine(), animURL.c_str());
			continue;
		}

		animURL = animURL.substr(1, string::npos);

		// Look up the animation in the animation map.
		AnimationMap::iterator itAnimationEntry = animations.find(animURL);
		if (itAnimationEntry == animations.end())
		{
			ReportWarning(pListener, LINE_LOG" instance_animation URL refers to non-existent animation (%s).", animNode->getLine(), animURL.c_str());
			continue;
		}

		ColladaAnimation* anim = (*itAnimationEntry).second;

		if (anim->GetFlags() & ColladaAnimation::Flags_NoExport)
		{
			continue;
		}

		pAnimClip->AddAnim(anim);
	}

	return pAnimClip;
}

void ColladaLoaderImpl::ReadXmlNodeTransform(ColladaNode* colladaNode, XmlNodeRef node, float const scale, IColladaLoaderListener* pListener)
{
	// Transforms are described in COLLADA as a series of elementary operations, like OpenGL. It
	// is assumed that vectors are stored in columns, and that transform matrices are post-multiplied.
	// This effectively means that transforms are given in the reverse order to which they are applied
	// to the node.

	// Loop through all the children looking for transformations. Keep post-multiplying to make sure
	// they get applied in the correct order (i.e. reverse order).
	Matrix44 transform(IDENTITY);
	for (int childIndex = 0; childIndex < node->getChildCount(); ++childIndex)
	{
		XmlNodeRef child = node->getChild(childIndex);
		string childTag = child->getTag();
		std::transform(childTag.begin(), childTag.end(), childTag.begin(), tolower);

		// Check whether this child represents a translation.
		if (childTag == "translate")
		{
			Vec3 translation;
			if (!ParseColladaVector(child,translation,pListener))
			{
				ReportWarning(pListener, LINE_LOG" Cannot parse values of 'translate' node.", child->getLine());
			}
			else
			{
				translation = translation.scale(scale);
				Matrix44 translationTM(IDENTITY);
				translationTM.SetTranslation(translation);
				transform = transform * translationTM;
			}
		}
		// Check whether this child represents a rotation.
		else if (childTag == "rotate")
		{
			Vec4 rotation;
			if (!ParseColladaVector(child,rotation,pListener))
			{
				ReportWarning(pListener, LINE_LOG" Cannot parse values of 'rotate' node.", child->getLine());
			}
			else
			{
				Matrix44 rotationTM(Matrix33::CreateRotationAA(DEG2RAD(rotation[3]), Vec3(rotation[0], rotation[1], rotation[2])));
				transform = transform * rotationTM;
			}
		}
		// Check whether this child represents a scale.
		else if (childTag == "scale")
		{
			Vec3 scale;
			if (!ParseColladaVector(child,scale,pListener))
			{
				ReportWarning(pListener, LINE_LOG" Cannot parse values of 'scale' node.", child->getLine());
			}
			else
			{
				Matrix44 scaleTM = Matrix33::CreateScale(scale);
				transform = transform * scaleTM;
			}
		}
		// Check whether this child represents a skew.
		else if (childTag == "skew")
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains 'skew' transform, which is unsupported - ignoring skew.", child->getLine(), node->getAttr("id"));
		}
	}

	colladaNode->SetTransform(Matrix34(transform));

	// Decompose the final matrix into affine transformations.
	decomp::AffineParts parts;
	struct MatrixContainer {decomp::HMatrix m;};
	decomp::decomp_affine(((MatrixContainer*)&transform)->m, &parts);

	// Set translation.
	Vec3 translation(parts.t.x, parts.t.y, parts.t.z);
	colladaNode->SetTranslate(translation);

	// Set rotations - this whole decomposition thing is silly, I'm sure we can remove it.
	Quat rotation = *(Quat*)&parts.q;
	Ang3 angles(rotation);

	colladaNode->SetRotationX(Vec4(1.0f, 0.0f, 0.0f, RAD2DEG(angles.x)));
	colladaNode->SetRotationY(Vec4(0.0f, 1.0f, 0.0f, RAD2DEG(angles.y)));
	colladaNode->SetRotationZ(Vec4(0.0f, 0.0f, 1.0f, RAD2DEG(angles.z)));

	// Set scale.
	Vec3 vScale(parts.k.x, parts.k.y, parts.k.z);
	colladaNode->SetScale(vScale);

	// Check for off-axis scaling.
	Quat scaleRotation = *(Quat*)&parts.q;
	AngleAxis angleAxis(scaleRotation);
	float totalScale = fabs(vScale.x - 1.0f) + fabs(vScale.y - 1.0f) + fabs(vScale.z - 1.0f);
	if (totalScale > 0.001f && fabs(angleAxis.angle) > 0.001f)
		ReportWarning(pListener, LINE_LOG" '%s' node contains off-axis scaling, which is unsupported. Object may appear different in engine.", node->getLine(), node->getAttr("id"));
}

string ColladaLoaderImpl::ReadXmlNodeUserProperties(XmlNodeRef node, IColladaLoaderListener* pListener)
{
	XmlNodeRef node_extra = node->findChild("extra");
	if (node_extra)
	{
		XmlNodeRef node_technique = node_extra->findChild("technique");
		const char* str_profile = 0;
		if (node_technique && node_technique->haveAttr("profile") && (str_profile = node_technique->getAttr("profile")) && (strcmp(str_profile,"CryEngine")==0))
		{
			XmlNodeRef node_properties = node_technique->findChild("properties");
			if (node_properties)
			{
				const char* properties = node_properties->getContent();
				return string(properties);
			}
		}
	}
	return string();
}

SHelperData ColladaLoaderImpl::ReadXmlNodeHelperData(XmlNodeRef node, float const scale, IColladaLoaderListener* pListener)
{
	SHelperData helperData;
	helperData.m_eHelperType = SHelperData::eHelperType_UNKNOWN;

	XmlNodeRef node_extra = node->findChild("extra");
	if (node_extra)
	{
		XmlNodeRef node_technique = node_extra->findChild("technique");
		const char* str_profile = 0;
		if (node_technique && node_technique->haveAttr("profile") && (str_profile = node_technique->getAttr("profile")) && (strcmp(str_profile,"CryEngine")==0))
		{
			XmlNodeRef node_helper = node_technique->findChild("helper");
			const char* str_type = 0;
			if (node_helper && node_helper->haveAttr("type") && (str_type = node_helper->getAttr("type")))
			{
				if (stricmp(str_type,"point") == 0)
				{
					helperData.m_eHelperType = SHelperData::eHelperType_Point;
				}
				else if (stricmp(str_type,"dummy") == 0)
				{
					helperData.m_eHelperType = SHelperData::eHelperType_Dummy;
					XmlNodeRef node_bound_box_min = node_helper->findChild("bound_box_min");
					XmlNodeRef node_bound_box_max = node_helper->findChild("bound_box_max");
					Vec3 bbMin, bbMax;
					if ( (!node_bound_box_min) || 
						(!node_bound_box_max) || 
						(!ParseColladaVector(node_bound_box_min,bbMin,pListener)) ||
						(!ParseColladaVector(node_bound_box_max,bbMax,pListener)) )
					{
						ReportWarning(pListener, LINE_LOG" Cannot parse values of bound_box_* element.", node_bound_box_min->getLine());
						helperData.m_eHelperType = SHelperData::eHelperType_UNKNOWN;
					}
					else
					{
						helperData.m_boundBoxMin[0] = bbMin.x * scale;
						helperData.m_boundBoxMin[1] = bbMin.y * scale;
						helperData.m_boundBoxMin[2] = bbMin.z * scale;

						helperData.m_boundBoxMax[0] = bbMax.x * scale;
						helperData.m_boundBoxMax[1] = bbMax.y * scale;
						helperData.m_boundBoxMax[2] = bbMax.z * scale;
					}
				}
			}
		}
	}

	return helperData;
}

bool ColladaLoaderImpl::ParseSceneDefinitions(SceneMap& scenes, GeometryMap& geometries, ControllerMap& skinControllers, MaterialMap& materials, std::multimap<string, XmlNodeRef>& tagMap, float const scale, IColladaLoaderListener* pListener)
{
	typedef std::multimap<string, XmlNodeRef> TagMap;
	std::pair<TagMap::iterator, TagMap::iterator> range = tagMap.equal_range("visual_scene");
	for (TagMap::iterator itNodeEntry = range.first; itNodeEntry != range.second; ++itNodeEntry)
	{
		XmlNodeRef node = (*itNodeEntry).second;
		if (node->haveAttr("id"))
		{
			this->ParseSceneDefinition(scenes, node, geometries, skinControllers, materials, scale, pListener);
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node contains no id attribute.", node->getLine(), node->getTag());
		}
	}

	return true;
}

static bool ExtractCryExportNodeParams(const string& nodeID, const string& properties, CryExportNodeParams& nodeParams)
{
	if (StringHelpers::MakeLowerCase(nodeID.substr(0,CRYEXPORTNODE_LENGTH)) != CRYEXPORTNODE_LOWERCASE)
	{
		return false; 
	}

	nodeParams.fullName = nodeID;
	nodeParams.nodeName = nodeID.substr(CRYEXPORTNODE_LENGTH, string::npos);

	{
		string exportType;
		PropertyHelpers::GetPropertyValue(properties, "fileType", exportType);
		exportType = StringHelpers::MakeLowerCase(exportType);

		if (exportType == "cgf")
			nodeParams.exportType = EXPORT_CGF;
		else if (exportType == "cga")
			nodeParams.exportType = EXPORT_CGA;
		else if (exportType == "chr")
			nodeParams.exportType = EXPORT_CHR;
		else if (exportType == "anm")
			nodeParams.exportType = EXPORT_ANM;
		else if (exportType == "caf")
			nodeParams.exportType = EXPORT_CAF;
		else if (exportType == "cgaanm")
			nodeParams.exportType = EXPORT_CGA_ANM;
		else if (exportType == "chrcaf")
			nodeParams.exportType = EXPORT_CHR_CAF;
		else
			nodeParams.exportType = EXPORT_CGF;
	}

	nodeParams.mergeAllNodes = !PropertyHelpers::HasProperty(properties, "DoNotMerge");;

	return true;
}

void ColladaLoaderImpl::ParseSceneDefinition(SceneMap& scenes, XmlNodeRef sceneNode, GeometryMap& geometries, ControllerMap& skinControllers, MaterialMap& materials, float const scale, IColladaLoaderListener* pListener)
{
	ReportInfo(pListener, "\n");

	std::vector<NodeStackEntry> nodeStack;
	std::vector<ColladaScene*> cryScenes;			// collection of CryExportNode scenes

	// Phys bones are just nodes that exist as flags - if a node called '<somename> phys' exists, then we should
	// export the bone '<somename>' as a bone collision mesh. Therefore we keep track of what phys bones we
	// encounter during our pass.
	PhysBoneMap physBones;

	// Phys parent frames are nodes that specify the parent frames for ragdoll bones.
	std::map<string, Matrix33> physParentFrames;

	// Add the root node to the scene.
	nodeStack.push_back(NodeStackEntry(sceneNode));

	bool cryExportNodeProcess = false;				// true if CryExportNode processing

	int nodeCounter = 0;

	// Look for nodes in the scene.
	while (!nodeStack.empty())
	{
		NodeStackEntry& stackTop = nodeStack.back();
		int nodeIndex = stackTop.childIndex++;
		XmlNodeRef parentNode = stackTop.parentXmlNode;

		if (nodeIndex >= parentNode->getChildCount())
		{
			// If this scene in CryExportNode hierarchy then store it
			if (nodeStack.back().cryExportNode)
			{
				cryExportNodeProcess = false;

				if (cryScenes.back())
				{
					scenes.insert(std::make_pair(parentNode->getAttr("id"), cryScenes.back()));
					ColladaScene * pScene = cryScenes.back();
					if ( pScene )
					{
						pScene->SetPhysBones(physBones);
						pScene->SetPhysParentFrames(physParentFrames);
						physBones.clear();
						physParentFrames.clear();
					}
				}
				else
				{
					ReportWarning(pListener, LINE_LOG" Failed to load '%s' node as scene.", sceneNode->getLine(), sceneNode->getTag());
				}
			}

			// All the children of this node have been processed - pop it off the stack.
			nodeStack.pop_back();
		}
		else
		{
			XmlNodeRef nodeNode = parentNode->getChild(nodeIndex);
			string nodeTag = nodeNode->getTag();
			std::transform(nodeTag.begin(), nodeTag.end(), nodeTag.begin(), tolower);
			if (nodeTag == "node")
			{
				// Push this node onto the stack.
				nodeStack.push_back(NodeStackEntry(nodeNode));

				string id_attr,id_attr_lower;

				if (nodeNode->haveAttr("id"))
				{
					id_attr = nodeNode->getAttr("id");
				}
				else
				{
					ReportError(pListener, LINE_LOG" node contains no id attribute. Scene skipped.", nodeNode->getLine());
					return;
				}

				string const properties = ReadXmlNodeUserProperties(nodeNode, pListener);
				SHelperData const helperData = ReadXmlNodeHelperData(nodeNode, scale, pListener);

				id_attr_lower.resize(id_attr.length());
				std::transform(id_attr.begin(), id_attr.end(), id_attr_lower.begin(), tolower);

				string safeBoneName = GetSafeBoneName(id_attr);
				string safeBoneNameLower = safeBoneName;
				std::transform(safeBoneNameLower.begin(), safeBoneNameLower.end(), safeBoneNameLower.begin(), tolower);

				// Check whether this is a phys bone.
				if (safeBoneNameLower.substr(safeBoneNameLower.size() - CRYPHYSBONESUFFIX_LENGTH, CRYPHYSBONESUFFIX_LENGTH) == CRYPHYSBONESUFFIX)
				{
					string boneName = safeBoneName.substr(0, safeBoneName.size() - CRYPHYSBONESUFFIX_LENGTH - 1); // Subtract an extra 1 to skip the character between the name and 'phys'.
					PhysBoneMap::iterator physBonePos = physBones.insert(std::make_pair(boneName, PhysBone(boneName))).first;
					ParseBoneProperties(id_attr_lower, (*physBonePos).second);
				}

				// Check whether this is a phys parent frame.
				if (safeBoneNameLower.substr(safeBoneNameLower.size() - CRYPHYSPARENTFRAMESUFFIX_LENGTH, CRYPHYSPARENTFRAMESUFFIX_LENGTH) == CRYPHYSPARENTFRAMESUFFIX)
				{
					string boneName = safeBoneName.substr(0, safeBoneName.size() - CRYPHYSPARENTFRAMESUFFIX_LENGTH - 1);
					ColladaNode physParentFrame("", "", 0, -1); // Just need a node to get the transform.
					ReadXmlNodeTransform(&physParentFrame, nodeNode, scale, pListener);
					Matrix33 frame(physParentFrame.GetTransform());
					physParentFrames.insert(std::make_pair(boneName, frame));
				}

				// CryExportNode check
				if (id_attr_lower.substr(0,CRYEXPORTNODE_LENGTH) == CRYEXPORTNODE_LOWERCASE)
				{
					if (cryExportNodeProcess)
					{
						ReportError(pListener, LINE_LOG" Cannot parse '%s'. It is in another CryExportNode. Scene skipped.", nodeNode->getLine(), id_attr.c_str());
						return;
					}

					CryExportNodeParams nodeParams;
					if (!ExtractCryExportNodeParams(id_attr_lower,properties,nodeParams))
					{
						ReportError(pListener, LINE_LOG" Cannot parse '%s'. Invalid CryExportNode 'id' format. Scene skipped.", nodeNode->getLine(), id_attr.c_str());
						return;
					}
					
					cryExportNodeProcess = true;
					nodeStack.back().cryExportNode = true;

					ColladaScene* scene = new ColladaScene(nodeParams);

					cryScenes.push_back(scene);

					ReportInfo(pListener, "Processing '%s'", id_attr.c_str());

					// Don't export the CryExportNode.
					continue;
				}

				ColladaNodeType nodeType;
				XmlNodeRef instanceNode;
				if ((instanceNode = nodeNode->findChild("instance_camera")) != NULL)
					nodeType = NODE_CAMERA;
				else if ((instanceNode = nodeNode->findChild("instance_controller")) != NULL)
					nodeType = NODE_CONTROLLER;
				else if ((instanceNode = nodeNode->findChild("instance_geometry")) != NULL)
					nodeType = NODE_GEOMETRY;
				else if ((instanceNode = nodeNode->findChild("instance_light")) != NULL)
					nodeType = NODE_LIGHT;
				else if ((instanceNode = nodeNode->findChild("instance_node")) != NULL)
					nodeType = NODE_NODE;
				else
					nodeType = NODE_NULL;

				string nodeId = nodeNode->getAttr("id");
				string nodeName = nodeId;
				if (nodeNode->haveAttr("name"))
					nodeName = nodeNode->getAttr("name");

				ColladaNode* parentNode = nodeStack[nodeStack.size()-2].parentColladaNode;

				ColladaNode* colladaNode = new ColladaNode(nodeId, nodeName,parentNode,nodeCounter++);
				nodeStack.back().parentColladaNode = colladaNode;
				
				colladaNode->SetProperty(properties);
				colladaNode->SetHelperData(helperData);

				// Set the type of the node
				colladaNode->SetType(nodeType);

				// Set the transform
				ReadXmlNodeTransform(colladaNode, nodeNode, scale, pListener);

				switch (nodeType)
				{
					case NODE_NULL:
						ParseInstanceNull(colladaNode,instanceNode,nodeNode,pListener);
						break;
					case NODE_CAMERA:
						ParseInstanceCamera(colladaNode,instanceNode,nodeNode,pListener);
						break;
					case NODE_CONTROLLER:
						ParseInstanceController(colladaNode,instanceNode,nodeNode,skinControllers,materials,pListener);
						break;
					case NODE_GEOMETRY:
						ParseInstanceGeometry(colladaNode,instanceNode,nodeNode,geometries,materials,pListener);
						break;
					case NODE_LIGHT:
						ParseInstanceLight(colladaNode,instanceNode,nodeNode,pListener);
						break;
					case NODE_NODE:
						ParseInstanceNode(colladaNode,instanceNode,nodeNode,pListener);
						break;

					default:
						assert(false);
				}

				// If this node begins with a minus sign, skip it.
				if (!id_attr_lower.empty() && id_attr_lower[0] == '-')
					continue;

				// The node should be added to the current scene, if there is a current scene.
				if (!cryScenes.empty() && cryExportNodeProcess)
				{
					cryScenes.back()->AddNode(colladaNode);

					NameNodeMap * nodeNameMap = cryScenes.back()->GetNodesNameMap();
					if ( nodeNameMap->find(colladaNode->GetID()) != nodeNameMap->end() )
					{
						ReportError(pListener, LINE_LOG" There is more than one bone in the collada dae file with the name '%s', check the scene in the DCC package and ensure all your bone names are unique\n", nodeNode->getLine(), colladaNode->GetID().c_str());
						return;
					}					
					nodeNameMap->insert(std::make_pair(colladaNode->GetID(), colladaNode));
				}
			}
		}
	}

	if (skinControllers.size() == 0)
		return;

	ControllerMap::iterator itcontroller = skinControllers.begin();
	SceneMap::iterator itscene = scenes.begin();

	for (; itscene != scenes.end(); ++itscene, ++itcontroller)
	{
		ColladaScene* scene = (*itscene).second;
		ColladaController* controller = (itcontroller != skinControllers.end()) ? (*itcontroller).second : NULL;

		// Make sure the root bone is bone 0. Note that this is one of few parts of the code
		// that assumes there is only one root bone. If there is more than one root bone then
		// we cannot be expected to make them all root 0.
		// Create a mapping between the bone names and their indices.
		std::map<string, int> boneNameIndexMap;
		int numJoints = (controller ? controller->GetNumJoints() : 0);
		for (int i = 0; i < numJoints; ++i)
		{
			string boneID = (controller ? controller->GetJointID(i) : "");
			boneNameIndexMap.insert(std::make_pair(boneID, i));
		}

		// Find all the root nodes for this skeleton and then add that hierarchy.
		std::vector<ColladaNode*> rootBoneNodes;
		FindRootBones(rootBoneNodes, *scene->GetNodesNameMap(), controller);
		for (int rootBoneIndex = 0, rootBoneCount = int(rootBoneNodes.size()); rootBoneIndex < rootBoneCount; ++rootBoneIndex)
			controller->ReIndexBoneIndices(rootBoneNodes[rootBoneIndex]->GetID());			// moved from ParseController()
	}

	ReportInfo(pListener, "\n");
}

bool ColladaLoaderImpl::ParseInstanceNull(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, IColladaLoaderListener* pListener)
{
	return false;
}

bool ColladaLoaderImpl::ParseInstanceCamera(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, IColladaLoaderListener* pListener)
{
	return false;
}

bool ColladaLoaderImpl::ParseInstanceController(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, ControllerMap& skinControllers, MaterialMap& materials, IColladaLoaderListener* pListener)
{
	if (!instanceNode->haveAttr("url"))
	{
		ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node has no 'url' attribute.", instanceNode->getLine(), instanceNode->getTag(), nodeNode->getTag());
		return false;
	}
	
	string controllerURL = instanceNode->getAttr("url");
	if (controllerURL[0] != '#')
	{
		ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node refers to external controller data (%s) - this is not supported.", instanceNode->getLine(), instanceNode->getTag(), nodeNode->getTag(), controllerURL.c_str());
		return false;
	}
	
	controllerURL = controllerURL.substr(1, string::npos);

	// Look up the controller in the controller map.
	ControllerMap::iterator itControllerEntry = skinControllers.find(controllerURL);
	if (itControllerEntry == skinControllers.end())
	{
		ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node refers to non-existent controller (%s).", instanceNode->getLine(), instanceNode->getTag(), nodeNode->getTag(), controllerURL.c_str());
		return false;
	}

	ColladaController* controller = (*itControllerEntry).second;
	colladaNode->SetController(controller);
	colladaNode->SetGeometry(controller->GetGeometry());		// set the node geometry pointer from controller's geometry

	// Parse materials of geometry controller
	XmlNodeRef materialNodes = NULL;
	XmlNodeRef bind = instanceNode->findChild("bind_material");
	if (bind)
		materialNodes = bind->findChild("technique_common");
	if (!materialNodes)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node has no any materials.", instanceNode->getLine(), nodeNode->getTag());
		return false;
	}

	for (int i=0;i<materialNodes->getChildCount();i++)
	{
		XmlNodeRef matNode = materialNodes->getChild(i);
		string matTag = matNode->getTag();
		if (matTag != "instance_material")
		{
			ReportWarning(pListener, LINE_LOG" 'bind_material/technique_common' in '%s' node has an unknown node ('%s').", matNode->getLine(), nodeNode->getTag(), matTag.c_str());
			continue;
		}

		string symbol,target;
		if (matNode->haveAttr("symbol"))
			symbol = matNode->getAttr("symbol");
		if (matNode->haveAttr("target"))
			target = matNode->getAttr("target");

		if (target[0] != '#')
		{
			ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node refers to external material (%s) - this is not supported.", matNode->getLine(), matNode->getTag(), nodeNode->getTag(), target.c_str());
			return false;
		}

		target = target.substr(1, string::npos);

		MaterialMap::iterator itMaterialEntry = materials.find(target);
		if (itMaterialEntry == materials.end())
		{
			ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node refers to non-existent material (%s).", matNode->getLine(), matNode->getTag(), nodeNode->getTag(), target.c_str());
		}
		else
		{
			ColladaMaterial* material = (*itMaterialEntry).second;
			colladaNode->AddSubMaterial(material);
		}
	}

	return true;
}

bool ColladaLoaderImpl::ParseInstanceGeometry(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, GeometryMap& geometries, MaterialMap& materials, IColladaLoaderListener* pListener)
{
	if (!instanceNode->haveAttr("url"))
	{
		ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node has no 'url' attribute.", instanceNode->getLine(), instanceNode->getTag(), nodeNode->getTag());
		return false;
	}
	
	string geometryURL = instanceNode->getAttr("url");
	if (geometryURL[0] != '#')
	{
		ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node refers to external geometry data (%s) - this is not supported.", instanceNode->getLine(), instanceNode->getTag(), nodeNode->getTag(), geometryURL.c_str());
		return false;
	}
	
	geometryURL = geometryURL.substr(1, string::npos);

	// Look up the geometry in the geometry map.
	GeometryMap::iterator itGeometryEntry = geometries.find(geometryURL);
	if (itGeometryEntry == geometries.end())
	{
		ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node refers to non-existent geometry (%s).", instanceNode->getLine(), instanceNode->getTag(), nodeNode->getTag(), geometryURL.c_str());
		return false;
	}

	ColladaGeometry* geometry = (*itGeometryEntry).second;
	colladaNode->SetGeometry(geometry);

	// Parse materials of geometry
	XmlNodeRef materialNodes = NULL;
	XmlNodeRef bind = instanceNode->findChild("bind_material");
	if (bind)
		materialNodes = bind->findChild("technique_common");
	if (!materialNodes)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node has no any materials.", instanceNode->getLine(), nodeNode->getTag());
		return false;
	}

	for (int i=0;i<materialNodes->getChildCount();i++)
	{
		XmlNodeRef matNode = materialNodes->getChild(i);
		string matTag = matNode->getTag();
		if (matTag != "instance_material")
		{
			ReportWarning(pListener, LINE_LOG" 'bind_material/technique_common' in '%s' node has an unknown node ('%s').", matNode->getLine(), nodeNode->getTag(), matTag.c_str());
			continue;
		}

		string symbol,target;
		if (matNode->haveAttr("symbol"))
			symbol = matNode->getAttr("symbol");
		if (matNode->haveAttr("target"))
			target = matNode->getAttr("target");

		if (target[0] != '#')
		{
			ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node refers to external material (%s) - this is not supported.", matNode->getLine(), matNode->getTag(), nodeNode->getTag(), target.c_str());
			return false;
		}

		target = target.substr(1, string::npos);

		MaterialMap::iterator itMaterialEntry = materials.find(target);
		if (itMaterialEntry == materials.end())
		{
			ReportWarning(pListener, LINE_LOG" '%s' node in '%s' node refers to non-existent material (%s).", matNode->getLine(), matNode->getTag(), nodeNode->getTag(), target.c_str());
		}
		else
		{
			ColladaMaterial* material = (*itMaterialEntry).second;
			colladaNode->AddSubMaterial(material);
		}
	}

	return true;
}

bool ColladaLoaderImpl::ParseInstanceLight(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, IColladaLoaderListener* pListener)
{
	return false;
}

bool ColladaLoaderImpl::ParseInstanceNode(ColladaNode* colladaNode, XmlNodeRef& instanceNode, XmlNodeRef& nodeNode, IColladaLoaderListener* pListener)
{
	return false;
}

class CCustomMaterialSettings
{
public:
	string physicalizeName;
/*
	bool sh;
	bool shTwoSided;
	bool shAmbient;
	int shOpacity;
*/
};

/*
bool ReadCustomMaterialSettingsFromMaterialName(const string& name, CCustomMaterialSettings& settings)
{
	bool settingsReadSuccessfully = true;

	// Keep track of what values we have read.
	bool readPhysicalizeType = false;
	int physicalizeType = 0;
	bool readSHSides = false;
	int shSides = 2;
	bool readSHAmbient = false;
	bool shAmbient = true;
	bool readSHOpacity = false;
	float shOpacity = 1.0f;

	// Loop through all the characters in the string.
	const char* position = name.c_str();
	string token;
	token.reserve(name.size());
	while (*position != 0)
	{
		// Skip any underscores.
		for (; *position == '_'; ++position);

		// Read until the next non-alpha character.
		token.clear();
		for (; ((*position) >= 'A' && (*position) <= 'Z') || ((*position) >= 'a' && (*position) <= 'z'); ++position)
			token.push_back(char(tolower(*position)));

		// Read the integer argument.
		char* end;
		int argument = strtol(position, &end, 10);
		bool argumentReadSuccessfully = true;
		if (position == end)
		{
			settingsReadSuccessfully = false;
			break;
		}
		
		position = end;

		// Check what token this is.
		if (token == "phys")
		{
			readPhysicalizeType = true;
			physicalizeType = argument;
		}
		else if (token == "shsides")
		{
			readSHSides = true;
			shSides = argument;
		}
		else if (token == "shambient")
		{
			readSHAmbient = true;
			shAmbient = (argument != 0);
		}
		else if (token == "shopacity")
		{
			readSHOpacity = true;
			shOpacity = float(argument) / 100.0f;
		}
		else
		{
			settingsReadSuccessfully = false;
			break;
		}
	}

	// Apply the values we found.
	settings.physicalizeType = physicalizeType;
	bool sh = readSHSides || readSHAmbient || readSHOpacity;
	if (sh)
	{
		settings.sh = true;
		settings.shSides = shSides;
		settings.shAmbient = shAmbient;
		settings.shOpacity = shOpacity;
	}
	else
	{
		settings.sh = false;
		settings.shSides = 1;
		settings.shAmbient = false;
		settings.shOpacity = 0.0f;
	}

	return settingsReadSuccessfully;
}
*/

void ReadMaterialSettings(std::vector<string>& parts, CCustomMaterialSettings& settings)
{
	string physicalizeName = "None";
    /*
	bool sh = false;
	bool shTwoSided = true;
	bool shAmbient = true;
	int shOpacity = 100;
    */

	for (int p=3; p<parts.size(); ++p)
	{
		string part = parts[p];

		if (part.compare(0, 4, "phys", 4) == 0)
		{
			physicalizeName = part.substr(4);
		}
		else
		{
			// Every parameter expected to be in form <Name><IntegerValue>

			int pos = 0;
			while (pos < part.length() && (part[pos] < '0' || part[pos] > '9'))
			{
				++pos;
			}

			if (pos <= 0 || pos >= part.length())
			{
				continue;
			}

			const string token = part.substr(0,pos);
			const string arg = part.substr(pos,string::npos);

			int intValue;
			if (!StringToInt(arg,intValue))
			{
				continue;
			}

            /*
			if (token == "SH")
			{
				sh = (intValue != 0);
			}
			else if (token == "SHTwoSided")
			{
				shTwoSided = (intValue != 0);
			}
			else if (token == "SHAmbient")
			{
				shAmbient = (intValue != 0);
			}
			else if (token == "SHOpacity")
			{
				shOpacity = intValue;
			}
            */
		}
	}

	// Apply the values we found.
	settings.physicalizeName = physicalizeName;
	/*
	if (sh)
	{
		settings.sh = true;
		settings.shTwoSided = shTwoSided;
		settings.shAmbient = shAmbient;
		settings.shOpacity = shOpacity;
	}
	else
	{
		settings.sh = false;
		settings.shTwoSided = false;
		settings.shAmbient = false;
		settings.shOpacity = 0;
	}
	*/
}

bool SplitMaterialName(const string& name, int& id, string& actualName, CCustomMaterialSettings& settings)
{
	// Syntax:
	//     MaterialLibraryName__MaterialID__MaterialName[__parameter[__parameter...]]
	// Or, if you prefer 3D Studio MAX naming convention:
	//     MultimaterialName__SubmaterialID__SubmaterialName[__parameter[__parameter...]]

	actualName = name;

	std::vector<string> parts;
	SplitString(name,"__",parts);

	// parts[3]..parts[n] : material settings
	ReadMaterialSettings(parts,settings);

	if (parts.size() < 3)
		return false;

	// parts[0] : material library name

	// parts[1] : material ID
	if (!StringToInt(parts[1],id))
		return false;

	if (id <= 0 || id > MAX_SUB_MATERIALS)
		return false;

	// parts[2] : material name
	actualName = parts[2];

	return true;
	
	
	
	/*
	// Name defaults to the whole string.
	actualName = name;

	// Ignore everything up to and including the last '.' in the string, if there is one.
	const char* position = name.c_str();
	const char* lastPeriod = strrchr(position, '.');
	if (lastPeriod)
		position = lastPeriod + 1;

	// Check whether the string starts with a number. There may be underscores in front of the number.
	// If so, we want to skip the underscores, but not if there is no number.
	bool startsWithNumber = true;
	int numberPosition;
	for (numberPosition = 0; position[numberPosition] == '_'; ++numberPosition);
	if (position[numberPosition] == 0)
		startsWithNumber = false;
	char* end;
	id = strtol(position + numberPosition, &end, 10);
	if (end == position + numberPosition)
		startsWithNumber = false;
	else
		position = end;

	// Check whether there is an _ between the number and the name, and if so skip it.
	if (startsWithNumber && *position == '_')
		++position;

	// Check whether there is a name after the id. There may also be material
	// directives, such as physicalisation flags.
	string nameBuffer;
	nameBuffer.reserve(200);
	while (*position != 0)
	{
		// Check whether the rest of the string specifies custom settings.
		if (ReadCustomMaterialSettingsFromMaterialName(position, settings))
			break;

		// Add all the underscores to the name.
		for (; *position == '_'; ++position)
			nameBuffer.push_back(*position);

		// Add the string up until the next _ to the name.
		for (; *position != '_' && *position != 0; ++position)
			nameBuffer.push_back(*position);
	}

	// Set the actual name if appropriate.
	bool hasName = !nameBuffer.empty();
	if (hasName)
		actualName = nameBuffer;

	// Return a value indicating how we went.
	if (startsWithNumber && hasName)
		return MATERIAL_NAME_VALID;
	else if (startsWithNumber)
		return MATERIAL_NAME_NUMBERONLY;
	else
		return MATERIAL_NAME_INVALID;
		*/
}

ColladaDataAccessor::ColladaDataAccessor(IColladaArray* array, int stride, int offset, int size)
:	array(array),
stride(stride),
offset(offset),
size(size)
{
}

void ColladaDataAccessor::AddParameter(const string& name, ColladaArrayType type, int offset)
{
	Parameter param;
	param.name = name;
	param.offset = offset;
	param.type = type;
	this->parameters.insert(std::make_pair(name, param));
}

int ColladaDataAccessor::GetSize(const string& param)
{
	// Calculate the size based on the size of the array.
	int size = this->array->GetSize() / this->stride;

	// Return the smaller of that and the claimed size.
	if (size > this->size && this->size > 0)
		size = this->size;
	return size;
}

bool ColladaDataAccessor::ReadAsFloat(const string& param, std::vector<float>& values, IColladaLoaderListener* pListener)
{
	int offset = this->offset;

	// If the array has only one param and the input param is empty then read the array with offset 0.
	if (param != "" || this->parameters.size() != 1 || this->stride != 1)
	{
		// Look up the parameter.
		ParameterMap::iterator itParameter = this->parameters.find(param);
		if (itParameter == this->parameters.end())
			return false;
		const Parameter& paramDef = (*itParameter).second;

		// Compute the offset based on the accessor offset and the parameter offset.
		offset += paramDef.offset;
	}

	// Read the data from the array.
	return this->array->ReadAsFloat(values, this->stride, offset, pListener);
}

bool ColladaDataAccessor::ReadAsMatrix(const string& param, std::vector<Matrix34>& values, IColladaLoaderListener* pListener)
{
	if (this->parameters.size() != 1 || this->stride != 16)
		return false;

	// Look up the parameter.
	ParameterMap::iterator itParameter = this->parameters.begin();
	if (itParameter == this->parameters.end())
		return false;
	const Parameter& paramDef = (*itParameter).second;
	
	if (paramDef.offset != 0 || paramDef.type != TYPE_MATRIX)
		return false;

	std::vector<float> float_values;
	if (!this->array->ReadAsFloat(float_values, 1, 0, pListener))
		return false;

	if (float_values.size() == 0 || (float_values.size() % this->stride) != 0)
		return false;

	values.resize(float_values.size() / this->stride);

	for (int m=0, f=0; m<values.size(); m++)
	{
		for (int i=0;i<3;i++)
		{
			for (int j=0;j<4;j++)
			{
				values[m](i,j) = float_values[f++];
			}
		}

		if (float_values[f++] != 0.0f)
			return false;
		if (float_values[f++] != 0.0f)
			return false;
		if (float_values[f++] != 0.0f)
			return false;
		if (float_values[f++] != 1.0f)
			return false;
	}
	
	return true;
}

bool ColladaDataAccessor::ReadAsInt(const string& param, std::vector<int>& values, IColladaLoaderListener* pListener)
{
	int offset = this->offset;

	// If the array has only one param and the input param is empty then read the array with offset 0.
	if (param != "" || this->parameters.size() != 1 || this->stride != 1)
	{
		// Look up the parameter.
		ParameterMap::iterator itParameter = this->parameters.find(param);
		if (itParameter == this->parameters.end())
			return false;
		const Parameter& paramDef = (*itParameter).second;

		// Compute the offset based on the accessor offset and the parameter offset.
		offset += paramDef.offset;
	}

	// Read the data from the array.
	return this->array->ReadAsInt(values, this->stride, offset, pListener);
}

bool ColladaDataAccessor::ReadAsName(const string& param, std::vector<string>& values, IColladaLoaderListener* pListener)
{
	int offset = this->offset;

	// If the array has only one param and the input param is empty then read the array with offset 0.
	if (param != "" || this->parameters.size() != 1 || this->stride != 1)
	{
		// Look up the parameter.
		ParameterMap::iterator itParameter = this->parameters.find(param);
		if (itParameter == this->parameters.end())
			return false;
		const Parameter& paramDef = (*itParameter).second;

		// Compute the offset based on the accessor offset and the parameter offset.
		offset += paramDef.offset;
	}

	// Read the data from the array.
	return this->array->ReadAsName(values, this->stride, offset, pListener);
}

bool ColladaDataAccessor::ReadAsBool(const string& param, std::vector<bool>& values, IColladaLoaderListener* pListener)
{
	int offset = this->offset;

	// If the array has only one param and the input param is empty then read the array with offset 0.
	if (param != "" || this->parameters.size() != 1 || this->stride != 1)
	{
		// Look up the parameter.
		ParameterMap::iterator itParameter = this->parameters.find(param);
		if (itParameter == this->parameters.end())
			return false;
		const Parameter& paramDef = (*itParameter).second;

		// Compute the offset based on the accessor offset and the parameter offset.
		offset += paramDef.offset;
	}

	// Read the data from the array.
	return this->array->ReadAsBool(values, this->stride, offset, pListener);
}

bool ColladaDataAccessor::ReadAsID(const string& param, std::vector<string>& values, IColladaLoaderListener* pListener)
{
	int offset = this->offset;

	// If the array has only one param and the input param is empty then read the array with offset 0.
	if (param != "" || this->parameters.size() != 1 || this->stride != 1)
	{
		// Look up the parameter.
		ParameterMap::iterator itParameter = this->parameters.find(param);
		if (itParameter == this->parameters.end())
			return false;
		const Parameter& paramDef = (*itParameter).second;

		// Compute the offset based on the accessor offset and the parameter offset.
		offset += paramDef.offset;
	}

	// Read the data from the array.
	return this->array->ReadAsID(values, this->stride, offset, pListener);
}

ColladaDataSource::ColladaDataSource(ColladaDataAccessor* pDataAccessor)
:	pDataAccessor(pDataAccessor)
{
}

ColladaDataSource::~ColladaDataSource()
{
	delete pDataAccessor;
}

ColladaVertexStream::ColladaVertexStream(ColladaDataSource* pPositionDataSource)
:	pPositionDataSource(pPositionDataSource)
{
}


bool ColladaVertexStream::ReadVertexPositions(Vec3* const positions, int const size, float const scale, IColladaLoaderListener* const pListener, const ColladaAssetMetaData::EUpAxis upAxis)
{
	std::vector<float> xs;
	std::vector<float> ys;
	std::vector<float> zs;
	bool hasCorrectParams = true;
	hasCorrectParams = hasCorrectParams && this->pPositionDataSource->GetDataAccessor()->ReadAsFloat("x", xs, pListener);
	hasCorrectParams = hasCorrectParams && this->pPositionDataSource->GetDataAccessor()->ReadAsFloat("y", ys, pListener);
	hasCorrectParams = hasCorrectParams && this->pPositionDataSource->GetDataAccessor()->ReadAsFloat("z", zs, pListener);
	if (!hasCorrectParams)
	{
		ReportWarning(pListener, "Vertex stream requires XYZ parameters.");
		return false;
	}
	for (int i = 0; i < size; ++i)
	{
		if( upAxis == ColladaAssetMetaData::eUA_Y )
		{
			positions[i].x = (i < int(xs.size())) ?  xs[i] * scale : 0;
			positions[i].y = (i < int(zs.size())) ? -zs[i] * scale : 0;
			positions[i].z = (i < int(ys.size())) ?  ys[i] * scale : 0;
		}
		else
		{
			positions[i].x = (i < int(xs.size())) ? xs[i] * scale : 0;
			positions[i].y = (i < int(ys.size())) ? ys[i] * scale : 0;
			positions[i].z = (i < int(zs.size())) ? zs[i] * scale : 0;
		}
	}

	return true;
}

int ColladaVertexStream::GetSize()
{
	return this->pPositionDataSource->GetDataAccessor()->GetSize("x");
}

ColladaMesh::ColladaMesh(std::vector<IPrimitiveStream*>& streams, ColladaVertexStream* vertexStream)
:	streams(streams), vertexStream(vertexStream)
{
	pCMesh = NULL;
}

ColladaMesh::~ColladaMesh()
{
	for (std::vector<IPrimitiveStream*>::iterator itStream = this->streams.begin(); itStream != this->streams.end(); ++itStream)
		delete *itStream;
}

namespace
{
	template <typename T> void compare(const T& l, const T& r, int& result)
	{
		if (result != 0)
			return;
		if (l < r)
			result = -1;
		if (l > r)
			result = 1;
	}

	struct VertexLess : public std::less<int>
	{
		VertexLess(const CMesh32& mesh): mesh(mesh) {}

		bool operator()(int l, int r)
		{
			SMeshColor black = {0, 0, 0, 0};
			const Vec3& lpos = mesh.m_pPositions[l];
			const Vec3& lnorm = mesh.m_pNorms[l];
			const SMeshColor& lcol = (l < int(mesh.m_pColor0.size()) ? mesh.m_pColor0[l] : black);
			const Vec3& rpos = mesh.m_pPositions[r];
			const Vec3& rnorm = mesh.m_pNorms[r];
			const SMeshColor& rcol = (r < int(mesh.m_pColor0.size()) ? mesh.m_pColor0[r] : black);

			int result = 0;
			compare(lpos.x, rpos.x, result);
			compare(lpos.y, rpos.y, result);
			compare(lpos.z, rpos.z, result);
			compare(lnorm.x, rnorm.x, result);
			compare(lnorm.y, rnorm.y, result);
			compare(lnorm.z, rnorm.z, result);
			compare(lcol.r, rcol.r, result);
			compare(lcol.g, rcol.g, result);
			compare(lcol.b, rcol.b, result);
			compare(lcol.a, rcol.a, result);
			return result < 0;
		}

		const CMesh32& mesh;
	};

	struct UVLess : public std::less<int>
	{
		UVLess(const CMesh32& mesh): mesh(mesh) {}

		bool operator()(int l, int r)
		{
			const SMeshTexCoord& luv = mesh.m_pTexCoord[l];
			const SMeshTexCoord& ruv = mesh.m_pTexCoord[r];

			int result = 0;
			compare(luv.s, ruv.s, result);
			compare(luv.t, ruv.t, result);
			return result < 0;
		}

		const CMesh32& mesh;
	};

	static const float weldEpsilon = 0.002f;
	static const float weldTextureEpsilon = 0.002f;

	struct VertexCell
	{
		static const float size;

		VertexCell(int x, int y, int z) {p[0] = x; p[1] = y; p[2] = z;}
		int p[3];
	};
	const float VertexCell::size = weldEpsilon;

	bool operator<(const VertexCell& l, const VertexCell& r)
	{
		int result = 0;
		compare(l.p[0], r.p[0], result);
		compare(l.p[1], r.p[1], result);
		compare(l.p[2], r.p[2], result);
		return result < 0;
	}
}

static void debugWriteMesh32(const CMesh32& mesh, const char* meshFilename, int meshIdx)
{
	char filename[MAX_PATH];
	sprintf(filename, meshFilename, meshIdx);

	FILE* f;
	f = fopen(filename, "wb");
	if (!f)
	{
		return;
	}

	fprintf(f, "[Pos]\n");
	for (size_t i = 0; i < mesh.m_pPositions.size(); ++i)
	{
		fprintf(f, "\t%g %g %g\n", mesh.m_pPositions[i].x, mesh.m_pPositions[i].y, mesh.m_pPositions[i].z);
	}

	fprintf(f, "[Color]\n");
	for (size_t i = 0; i < mesh.m_pColor0.size(); ++i)
	{
		fprintf(f, "\t%u %u %u %u\n", mesh.m_pColor0[i].r, mesh.m_pColor0[i].g, mesh.m_pColor0[i].b, mesh.m_pColor0[i].a);
	}

	fprintf(f, "[Indices]\n");
	fprintf(f, "\tPos Color\n");
	for (size_t i = 0; i < mesh.m_pFaces.size(); ++i)
	{
		for (size_t j = 0; j < 3; ++j)
		{
			fprintf(f, "\t%d %d\n", mesh.m_pFaces[i].v[j], mesh.m_pFaces[i].v[j]);
		}
		fprintf(f, "\n");
	}

	fclose(f);
}

static bool s_bDebugWriteMesh32 = false;

CMesh* ColladaMesh::CreateMesh(CGFSubMaterialMap& cgfMaterials, float const scale, IColladaLoaderListener* pListener, std::vector<int>& meshToVertexListIndexMapping, const string& nodeName, const std::vector<SMeshBoneMapping>& boneMapping, const ColladaAssetMetaData::EUpAxis upAxis)
{
	// Loop through all the primitive streams, reading in meshes for them.
	std::vector<CMesh32> streamMeshes(this->streams.size());
	std::vector<std::vector<int> > streamNewToOldPositionMappings(this->streams.size());
	for (size_t streamIndex = 0; streamIndex < this->streams.size(); ++streamIndex)
	{
		IPrimitiveStream* const stream = this->streams[streamIndex];

		// Create a mesh for this stream.
		if (!stream->CreateMesh(&streamMeshes[streamIndex], scale, pListener, streamNewToOldPositionMappings[streamIndex], upAxis))
		{
			ReportWarning(pListener, "Failed to add stream to mesh of node '%s'", nodeName.c_str());
			return NULL;
		}
	}

	if (s_bDebugWriteMesh32)
	{
		for (size_t streamIndex = 0; streamIndex < this->streams.size(); ++streamIndex)
		{
			debugWriteMesh32(streamMeshes[streamIndex], "CMesh32_%02i.mesh", streamIndex);
		}
	}

	// Create all the mesh subsets.
	// Build a mapping table from COLLADA material pointers to the material id that it will correspond to.
	std::map<const ColladaMaterial*, int> materialSubsetMap;
	std::vector<int> subsetMaterialIDs;
	for (CGFSubMaterialMap::iterator itMaterialEntry = cgfMaterials.begin(); itMaterialEntry != cgfMaterials.end(); ++itMaterialEntry)
	{
		// Add the subset.
		int subsetIndex = int(subsetMaterialIDs.size());
		subsetMaterialIDs.push_back((*itMaterialEntry).second.id);

		// Add the reference to the subset to the mapping table.
		materialSubsetMap.insert(std::make_pair((*itMaterialEntry).first, subsetIndex));
	}

	// Assign each stream to a mesh subset index, based on the material it uses.
	std::vector<int> streamSubsetMap(this->streams.size());
	for (size_t streamIndex = 0; streamIndex < this->streams.size(); ++streamIndex)
	{
		IPrimitiveStream* const stream = this->streams[streamIndex];

		streamSubsetMap[streamIndex] = -1;

		if (stream->GetMaterial() == 0)
		{
			ReportWarning(pListener, "Polygon stream has no material assigned.");
		}
		else
		{
			// Look up the subset index for the material associated with this primitive stream.
			std::map<const ColladaMaterial*, int>::iterator materialSubsetMapPos = materialSubsetMap.find(stream->GetMaterial());
			const bool fail = materialSubsetMap.empty() || (materialSubsetMapPos == materialSubsetMap.end());
			if (fail)
			{
				ReportWarning(pListener, "Polygon stream specifies material '%s' with no ID.", stream->GetMaterial()->GetFullName().c_str());
			}
			else
			{
				streamSubsetMap[streamIndex] = (*materialSubsetMapPos).second;
			}
		}
	}

	// Check whether any stream has UVs.
	bool hasUVs = false;
	for (size_t streamIndex = 0; streamIndex < this->streams.size(); ++streamIndex)
	{
		hasUVs = hasUVs || (streamMeshes[streamIndex].GetTexCoordsCount() > 0);
	}

	// Check whether any stream has vertex colors.
	bool hasColors = false;
	for (size_t streamIndex = 0; streamIndex < this->streams.size(); ++streamIndex)
	{
		hasColors = hasColors || (streamMeshes[streamIndex].GetColor0Count() > 0);
	}

	// Allocate space for the vertices of each stream in the overall mesh.
	std::vector<int> streamVertexPositions(this->streams.size());
	int expandedVertexCount = 0;
	for (size_t streamIndex = 0; streamIndex < this->streams.size(); ++streamIndex)
	{
		streamVertexPositions[streamIndex] = expandedVertexCount;
		expandedVertexCount += streamMeshes[streamIndex].GetVertexCount();
	}

	// Allocate space for the faces of each stream in the overall mesh.
	std::vector<int> streamFacePositions(this->streams.size());
	int expandedFaceCount = 0;
	for (size_t streamIndex = 0; streamIndex < this->streams.size(); ++streamIndex)
	{
		streamFacePositions[streamIndex] = expandedFaceCount;
		expandedFaceCount += streamMeshes[streamIndex].GetFacesCount();
	}

	// Allocate the buffers in the combined mesh of the correct size.
	CMesh32* mesh32 = new CMesh32;
	mesh32->Clear();
	mesh32->SetVertexCount(expandedVertexCount);
	mesh32->SetTexCoordsCount(hasUVs ? expandedVertexCount : 0);
	mesh32->SetColor0Count(hasColors ? expandedVertexCount : 0);
	mesh32->SetColor1Count(0);
	mesh32->SetFacesCount(expandedFaceCount);

	// Write each mesh's data into the combined mesh.
	std::vector<int> expandedMeshToVertexListIndexMapping(expandedVertexCount);
	std::fill(expandedMeshToVertexListIndexMapping.begin(), expandedMeshToVertexListIndexMapping.end(), -1);
	for (size_t streamIndex = 0; streamIndex < this->streams.size(); ++streamIndex)
	{
		const CMesh32& streamMesh = streamMeshes[streamIndex];

		// Copy the vertices, and update the remapping table.

		int const streamVertexStart = streamVertexPositions[streamIndex];

		std::copy(streamMesh.m_pPositions.begin(), streamMesh.m_pPositions.end(), mesh32->m_pPositions.begin() + streamVertexStart);
		std::copy(streamMesh.m_pNorms.begin(), streamMesh.m_pNorms.end(), mesh32->m_pNorms.begin() + streamVertexStart);

		if (streamMesh.m_pPositions.size() != streamMesh.m_pNorms.size())
		{
			ReportWarning(pListener, "Unexpected size mismatch: stream %d: #pos(%u) != #norm(%u)", streamIndex, streamMesh.m_pPositions.size(), streamMesh.m_pNorms.size());
		}

		if (hasUVs)
		{
			if (streamMesh.m_pPositions.size() != streamMesh.m_pTexCoord.size())
			{
				ReportWarning(pListener, "Unexpected size mismatch: stream %d: #pos(%u) != #texcoord(%u)", streamIndex, streamMesh.m_pPositions.size(), streamMesh.m_pTexCoord.size());
			}
			std::copy(streamMesh.m_pTexCoord.begin(), streamMesh.m_pTexCoord.end(), mesh32->m_pTexCoord.begin() + streamVertexStart);
		}

		if (hasColors)
		{
			if (streamMesh.m_pPositions.size() != streamMesh.m_pTexCoord.size())
			{
				ReportWarning(pListener, "Unexpected size mismatch: stream %d: #pos(%u) != #color0(%u)", streamIndex, streamMesh.m_pPositions.size(), streamMesh.m_pColor0.size());
			}
			std::copy(streamMesh.m_pColor0.begin(), streamMesh.m_pColor0.end(), mesh32->m_pColor0.begin() + streamVertexStart);
		}

		for (int vertexIndex = 0, vertexCount = streamMesh.GetVertexCount(); vertexIndex < vertexCount; ++vertexIndex)
		{
			expandedMeshToVertexListIndexMapping[streamVertexStart + vertexIndex] = streamNewToOldPositionMappings[streamIndex][vertexIndex];
		}

		// Copy the faces.
		int const streamFaceStart = streamFacePositions[streamIndex];
		for (int faceIndex = 0, streamFaceCount = int(streamMesh.m_pFaces.size()); faceIndex < streamFaceCount; ++faceIndex)
		{
			for (int i = 0; i < 3; ++i)
			{
				mesh32->m_pFaces[streamFaceStart + faceIndex].v[i] = streamVertexStart + streamMesh.m_pFaces[faceIndex].v[i];
				mesh32->m_pFaces[streamFaceStart + faceIndex].t[i] = streamVertexStart + streamMesh.m_pFaces[faceIndex].t[i];
			}
			mesh32->m_pFaces[streamFaceStart + faceIndex].nSubset = streamSubsetMap[streamIndex];
			mesh32->m_pFaces[streamFaceStart + faceIndex].dwFlags = streamMesh.m_pFaces[faceIndex].dwFlags;
		}
	}

	// Weld vertices that are close together. Here we don't actually delete them, we just move them to the
	// same position so that they will be merged later on.
	{
		typedef std::map<VertexCell, std::vector<int> > CellIndexMap;
		CellIndexMap cellIndexMap;
		for (int vertexIndex = 0, vertexCount = mesh32->GetVertexCount(); vertexIndex < vertexCount; ++vertexIndex)
		{
			// Assign the vertex to a cell.
			Vec3 pos = mesh32->m_pPositions[vertexIndex];
			Vec3 norm = mesh32->m_pNorms[vertexIndex];
			SMeshTexCoord dummyuv = {0.0f, 0.0f};
			SMeshTexCoord uv = (vertexIndex < int(mesh32->m_pTexCoord.size()) ? mesh32->m_pTexCoord[vertexIndex] : dummyuv);
			VertexCell cell = VertexCell(int(pos.x / VertexCell::size), int(pos.y / VertexCell::size), int(pos.z / VertexCell::size));

			// Check whether an almost identical vertex exists in this cell, or in one of the neighbours.
			int weldIndex = -1;
			for (int dx = -1; dx <= 1; ++dx)
			{
				for (int dy = -1; dy <= 1; ++dy)
				{
					for (int dz = -1; dz <= 1; ++dz)
					{
						VertexCell neighbour(cell.p[0] + dx, cell.p[1] + dy, cell.p[2] + dz);
						CellIndexMap::iterator cellPos = cellIndexMap.find(neighbour);
						if (cellPos != cellIndexMap.end())
						{
							std::vector<int>& indices = (*cellPos).second;
							for (int indexIndex = 0, indexCount = int(indices.size()); indexIndex < indexCount; ++indexIndex)
							{
								int candidateIndex = indices[indexIndex];

								// Compare the vertices to see whether they should be welded.
								Vec3 cpos = mesh32->m_pPositions[candidateIndex];
								Vec3 cnorm = mesh32->m_pNorms[candidateIndex];
								SMeshTexCoord cuv = (candidateIndex < int(mesh32->m_pTexCoord.size()) ? mesh32->m_pTexCoord[candidateIndex] : dummyuv);
								if (fabs(cpos.x - pos.x) < weldEpsilon &&
									fabs(cpos.y - pos.y) < weldEpsilon && 
									fabs(cpos.z - pos.z) < weldEpsilon &&
									fabs(cnorm.x - norm.x) < weldEpsilon &&
									fabs(cnorm.y - norm.y) < weldEpsilon &&
									fabs(cnorm.z - norm.z) < weldEpsilon &&
									fabs(cuv.s - uv.s) < weldTextureEpsilon &&
									fabs(cuv.t - uv.t) < weldTextureEpsilon)
								{
									weldIndex = candidateIndex;
								}
							}
						}
					}
				}
			}

			if (weldIndex >= 0)
			{
				// Copy the other vertex over this one. The vertex will then be merged out later on.
				mesh32->m_pPositions[vertexIndex] = mesh32->m_pPositions[weldIndex];
				mesh32->m_pNorms[vertexIndex] = mesh32->m_pNorms[weldIndex];
				if (vertexIndex < int(mesh32->m_pTexCoord.size()))
					mesh32->m_pTexCoord[vertexIndex] = (weldIndex < int(mesh32->m_pTexCoord.size()) ? mesh32->m_pTexCoord[weldIndex] : dummyuv);
				if (mesh32->GetColor0Count() > vertexIndex && mesh32->GetColor0Count() > weldIndex)
					mesh32->m_pColor0[vertexIndex] = mesh32->m_pColor0[weldIndex];
			}
			else
			{
				cellIndexMap[cell].push_back(vertexIndex);
			}
		}
	}

	// Create the mesh subsets.
	for (size_t subsetIndex = 0; subsetIndex < subsetMaterialIDs.size(); ++subsetIndex)
	{
		SMeshSubset meshSubset;
		meshSubset.nMatID = subsetMaterialIDs[subsetIndex];
		meshSubset.nPhysicalizeType = PHYS_GEOM_TYPE_NONE;
		mesh32->m_subsets.push_back(meshSubset);
	}

	// Compact vertices (ie positions, normals and colours).
	std::vector<int> compactedVertexToExpandedMap(mesh32->GetVertexCount());
	{
		for (int i = 0, count = mesh32->GetVertexCount(); i < count; ++i)
			compactedVertexToExpandedMap[i] = i;
		VertexLess less(*mesh32);
		std::sort(compactedVertexToExpandedMap.begin(), compactedVertexToExpandedMap.end(), less);
		int writePos = 0;
		std::vector<int> expandedToCompactedMap(mesh32->GetVertexCount());
		for (int readPos = 0, count = int(compactedVertexToExpandedMap.size()); readPos < count; ++readPos)
		{
			if (writePos == 0 || less(compactedVertexToExpandedMap[writePos - 1], compactedVertexToExpandedMap[readPos]))
				compactedVertexToExpandedMap[writePos++] = compactedVertexToExpandedMap[readPos];
			expandedToCompactedMap[compactedVertexToExpandedMap[readPos]] = writePos - 1;
		}
		compactedVertexToExpandedMap.resize(writePos);
		std::vector<Vec3> newPos(compactedVertexToExpandedMap.size());
		std::vector<Vec3> newNorms(compactedVertexToExpandedMap.size());
		for (int i = 0, count = int(compactedVertexToExpandedMap.size()); i < count; ++i)
		{
			newPos[i] = mesh32->m_pPositions[compactedVertexToExpandedMap[i]];
			newNorms[i] = mesh32->m_pNorms[compactedVertexToExpandedMap[i]];
		}
		mesh32->m_pPositions.swap(newPos);
		mesh32->m_pNorms.swap(newNorms);
		if (mesh32->GetColor0Count() > 0)
		{
			std::vector<SMeshColor> newCols(compactedVertexToExpandedMap.size());
			for (int i = 0, count = int(compactedVertexToExpandedMap.size()); i < count; ++i)
			{
				newCols[i] = mesh32->m_pColor0[compactedVertexToExpandedMap[i]];
			}
			mesh32->m_pColor0.swap(newCols);
		}
		for (int i = 0, count = mesh32->GetFacesCount(); i < count; ++i)
		{
			for (int v = 0; v < 3; ++v)
			{
				assert(expandedToCompactedMap[mesh32->m_pFaces[i].v[v]] < mesh32->GetVertexCount());
				mesh32->m_pFaces[i].v[v] = expandedToCompactedMap[mesh32->m_pFaces[i].v[v]];
			}
		}
	}

	// Remove degenerate faces.
	{
		int writePos = 0;
		for (int readPos = 0, count = mesh32->GetFacesCount(); readPos < count; ++readPos)
		{
			const SMeshFace32& face = mesh32->m_pFaces[readPos];
			if (face.v[0] != face.v[1] && face.v[1] != face.v[2] && face.v[0] != face.v[2])
				mesh32->m_pFaces[writePos++] = mesh32->m_pFaces[readPos];
		}
		mesh32->SetFacesCount(writePos);
	}

	// Report the mapping from the final mesh topology to the original vertex indices.
	meshToVertexListIndexMapping.resize(compactedVertexToExpandedMap.size());
	for (int i = 0, count = meshToVertexListIndexMapping.size(); i < count; ++i)
		meshToVertexListIndexMapping[i] = expandedMeshToVertexListIndexMapping[compactedVertexToExpandedMap[i]];

	// Compact uvs.
	{
		std::vector<int> compactedUVToExpandedMap(mesh32->GetTexCoordsCount());
		std::vector<int> expandedToCompactedMap(mesh32->GetTexCoordsCount());
		for (int i = 0, count = mesh32->GetTexCoordsCount(); i < count; ++i)
			compactedUVToExpandedMap[i] = i;
		UVLess less(*mesh32);
		std::sort(compactedUVToExpandedMap.begin(), compactedUVToExpandedMap.end(), less);
		int writePos = 0;
		for (int readPos = 0, count = int(compactedUVToExpandedMap.size()); readPos < count; ++readPos)
		{
			if (writePos == 0 || less(compactedUVToExpandedMap[writePos - 1], compactedUVToExpandedMap[readPos]))
				compactedUVToExpandedMap[writePos++] = compactedUVToExpandedMap[readPos];
			expandedToCompactedMap[compactedUVToExpandedMap[readPos]] = writePos - 1;
		}
		compactedUVToExpandedMap.resize(writePos);
		std::vector<SMeshTexCoord> newUV(compactedUVToExpandedMap.size());
		for (int i = 0, count = int(compactedUVToExpandedMap.size()); i < count; ++i)
			newUV[i] = mesh32->m_pTexCoord[compactedUVToExpandedMap[i]];
		mesh32->m_pTexCoord.swap(newUV);
		for (int i = 0, count = mesh32->GetFacesCount(); i < count; ++i)
		{
			for (int v = 0; v < 3; ++v)
			{
				if (mesh32->m_pFaces[i].t[v] < expandedToCompactedMap.size())
					mesh32->m_pFaces[i].t[v] = expandedToCompactedMap[mesh32->m_pFaces[i].t[v]];
			}
		}
	}

	// Calculate the AABB of the mesh.
	for (int i = 0; i < mesh32->GetVertexCount(); ++i)
		mesh32->m_bbox.Add(mesh32->m_pPositions[i]);

	// Copy mesh32 to mesh
	CMesh* mesh = CreateMeshFromMesh32(mesh32, nodeName, pListener);
	delete mesh32;
	if (!mesh)
		return NULL;

	// If there are bone mappings, sort them so they match up with the new vertex list. boneMapping
	// stores the mappings for each vertex in the original vertex list, so we have to find the
	// original vertex index for each vertex in the new mesh using meshToVertexListIndexMapping.
	std::vector<SMeshBoneMapping> reindexedBoneMappings;
	if (!boneMapping.empty())
	{
		reindexedBoneMappings.resize(meshToVertexListIndexMapping.size());
		for (size_t vertexIndex = 0, vertexCount = meshToVertexListIndexMapping.size(); vertexIndex < vertexCount; ++vertexIndex)
			reindexedBoneMappings[vertexIndex] = boneMapping[meshToVertexListIndexMapping[vertexIndex]];
	}

	// Copy the bone mapping to the mesh.
	if (!reindexedBoneMappings.empty())
	{
		mesh->ReallocStream(CMesh::BONEMAPPING, int(meshToVertexListIndexMapping.size()));
		for (size_t i = 0, vertexCount = meshToVertexListIndexMapping.size(); i < vertexCount; ++i)
			mesh->m_pBoneMapping[i] = reindexedBoneMappings[i];
	}

	return mesh;
}

CMesh* ColladaMesh::CreateMeshFromMesh32(CMesh32* mesh32, const string& nodeName, IColladaLoaderListener* pListener)
{
	CMesh* mesh = new CMesh();

	int numFaces = mesh32->GetFacesCount();
	int numVertices = mesh32->GetVertexCount();
	int numTexCoords = mesh32->GetTexCoordsCount();
	int numColor0 = mesh32->GetColor0Count();
	int numColor1 = mesh32->GetColor1Count();
	
	if (numVertices > 65535)
	{
		ReportError(pListener, "There is too many vertices after merge (%d) in mesh of node '%s'", numVertices, nodeName.c_str());
		return NULL;
	}

	int numSubsets = mesh32->m_subsets.size();
	mesh->m_subsets.resize(numSubsets);

	mesh->SetFacesCount(numFaces);
	mesh->SetVertexCount(numVertices);
	mesh->SetTexCoordsCount(numTexCoords);

	if (numColor0)
		mesh->ReallocStream(CMesh::COLORS_0, numVertices);
	if (numColor1)
		mesh->ReallocStream(CMesh::COLORS_1, numVertices);

	for (int i=0; i<numFaces; i++)
	{
		assert(mesh32->m_pFaces[i].v[0] < 65536);
		assert(mesh32->m_pFaces[i].v[1] < 65536);
		assert(mesh32->m_pFaces[i].v[2] < 65536);
		assert(mesh32->m_pFaces[i].t[0] < 65536);
		assert(mesh32->m_pFaces[i].t[1] < 65536);
		assert(mesh32->m_pFaces[i].t[2] < 65536);

		mesh->m_pFaces[i].v[0] = mesh32->m_pFaces[i].v[0];
		mesh->m_pFaces[i].v[1] = mesh32->m_pFaces[i].v[1];
		mesh->m_pFaces[i].v[2] = mesh32->m_pFaces[i].v[2];
		mesh->m_pFaces[i].t[0] = mesh32->m_pFaces[i].t[0];
		mesh->m_pFaces[i].t[1] = mesh32->m_pFaces[i].t[1];
		mesh->m_pFaces[i].t[2] = mesh32->m_pFaces[i].t[2];
		mesh->m_pFaces[i].nSubset = mesh32->m_pFaces[i].nSubset;
		mesh->m_pFaces[i].dwFlags = mesh32->m_pFaces[i].dwFlags;
	}

	for (int i=0; i<numVertices; i++)
	{
		mesh->m_pPositions[i] = mesh32->m_pPositions[i];
		mesh->m_pNorms[i] = mesh32->m_pNorms[i];
	}

	for (int i=0; i<numTexCoords; i++)
		mesh->m_pTexCoord[i] = mesh32->m_pTexCoord[i];
	for (int i=0; i<numColor0; i++)
		mesh->m_pColor0[i] = mesh32->m_pColor0[i];
	for (int i=0; i<numColor1; i++)
		mesh->m_pColor1[i] = mesh32->m_pColor1[i];

	for (int i=0; i<numSubsets; i++)
		mesh->m_subsets[i] = mesh32->m_subsets[i];

	mesh->m_bbox = mesh32->m_bbox;

	return mesh;
}

bool ColladaMesh::GetVertexPositions(std::vector<Vec3>& positions, float const scale, IColladaLoaderListener* const pListener, const ColladaAssetMetaData::EUpAxis upAxis)
{
	positions.resize(this->vertexStream->GetSize());
	return this->vertexStream->ReadVertexPositions(&positions[0], this->vertexStream->GetSize(), scale, pListener, upAxis);
}

ColladaGeometry::ColladaGeometry(const string& name, ColladaMesh* pMesh)
:	name(name), pMesh(pMesh)
{
}

CMesh* ColladaGeometry::CreateMesh(CGFSubMaterialMap& cgfMaterials, float const scale, IColladaLoaderListener* pListener, std::vector<int>& meshToVertexListIndexMapping, const string& nodeName, const std::vector<SMeshBoneMapping>& boneMapping, const ColladaAssetMetaData::EUpAxis upAxis)
{
	return this->pMesh->CreateMesh(cgfMaterials, scale, pListener, meshToVertexListIndexMapping, nodeName, boneMapping, upAxis);
}

bool ColladaGeometry::GetVertexPositions(std::vector<Vec3>& positions, float const scale, IColladaLoaderListener* pListener, const ColladaAssetMetaData::EUpAxis upAxis)
{
	return this->pMesh->GetVertexPositions(positions, scale, pListener, upAxis);
}

BasePrimitiveStream::BasePrimitiveStream()
	: indexGroupSize(-1)
	, vertexStream(0)
	, normalSource(0)
	, colourSource(0)
	, texCoordSource(0)
	, positionIndexOffset(-1)
	, normalIndexOffset(-1)
	, colourIndexOffset(-1)
	, texCoordIndexOffset(-1)
	, material(0)
{
}

bool BasePrimitiveStream::ParseNode(XmlNodeRef& node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener)
{
	// Check whether the node specifies a material.
	if (node->haveAttr("material"))
	{
		// Look for the material the object calls for.
		bool foundMaterial = true;
		string materialID = node->getAttr("material");
		size_t hashPos = materialID.rfind('#');
		if (hashPos != string::npos && hashPos == 0)
			materialID = materialID.substr(1, string::npos);
		else if (hashPos != string::npos)
			foundMaterial = false;

		if (foundMaterial)
		{
			// Look up the data source.
			MaterialMap::iterator itMaterialEntry = materials.find(materialID);
			if (itMaterialEntry != materials.end())
			{
				this->material = (*itMaterialEntry).second;
			}
			else
			{
				ReportWarning(pListener, LINE_LOG" '%s' node refers to non-existent material ('%s').", node->getLine(), node->getTag(), materialID.c_str());
				return 0;
			}
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" '%s' node uses non-local material URI ('%s').", node->getLine(), node->getTag(), materialID.c_str());
		}
	}
	else
	{
		ReportInfo(pListener, LINE_LOG" '%s' node does not specify a material.", node->getLine(), node->getTag());
	}

	// Create a list of semantic types that map to data sources. Note that this does not involve vertex semantics, since
	// they must be handled specially, as they point to a <vertex> tag rather than a <source> tag.
	enum DataStreamType
	{
		DATA_STREAM_NORMAL,
		DATA_STREAM_COLOUR,
		DATA_STREAM_TEXCOORD
	};
	std::map<string, DataStreamType> semanticMap;
	semanticMap["normal"] = DATA_STREAM_NORMAL;
	semanticMap["color"] = DATA_STREAM_COLOUR;
	semanticMap["texcoord"] = DATA_STREAM_TEXCOORD;

	// Look through the child nodes for input nodes.
	int offset = 0;
	int numIndices = 0;
	for (int childIndex = 0; childIndex < node->getChildCount(); ++childIndex)
	{
		XmlNodeRef inputNode = node->getChild(childIndex);
		string inputTag = inputNode->getTag();
		std::transform(inputTag.begin(), inputTag.end(), inputTag.begin(), tolower);
		if (inputTag == "input")
		{
			// Check whether this input overrides the offset.
			const char* offsetAttr = 0;
			if (inputNode->haveAttr("idx"))
				offsetAttr = "idx";
			if (inputNode->haveAttr("offset"))
				offsetAttr = "offset";
			if (offsetAttr)
			{
				char* end;
				const char* buf = inputNode->getAttr(offsetAttr);
				int newOffset = strtol(buf, &end, 0);
				if (buf != end)
					offset = newOffset;
			}

			// Check whether tag has both a semantic and a source.
			if (inputNode->haveAttr("semantic") && inputNode->haveAttr("source"))
			{
				// Get the semantic of the input.
				string semantic = inputNode->getAttr("semantic");
				std::transform(semantic.begin(), semantic.end(), semantic.begin(), tolower);
				if (semantic == "vertex")
				{
					// Look for a vertex stream of this id.
					string vertexStreamID = inputNode->getAttr("source");
					if (vertexStreamID[0] == '#')
					{
						vertexStreamID = vertexStreamID.substr(1, string::npos);
						VertexStreamMap::iterator itVertexStreamEntry = vertexStreams.find(vertexStreamID);
						if (itVertexStreamEntry != vertexStreams.end())
						{
							ColladaVertexStream* vertexStream = (*itVertexStreamEntry).second;
							if (!this->vertexStream)
							{
								this->SetVertexStream(vertexStream);
								this->positionIndexOffset = offset;
							}
							else
							{
								ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"));
							}
						}
						else
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag specifies non-existent vertex stream ('%s') - skipping input.", inputNode->getLine(), vertexStreamID.c_str());
						}
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag specifies external vertex stream ('%s') - skipping input.", inputNode->getLine(), vertexStreamID.c_str());
					}
				}
				// Check whether the input refers to another data stream.
				else if (semanticMap.find(semantic) != semanticMap.end())
				{
					DataStreamType streamType = semanticMap[semantic];

					// Look for a data source with this id.
					string dataSourceID = inputNode->getAttr("source");
					if (dataSourceID[0] == '#')
					{
						dataSourceID = dataSourceID.substr(1, string::npos);
						DataSourceMap::iterator itDataSourceEntry = dataSources.find(dataSourceID);
						if (itDataSourceEntry != dataSources.end())
						{
							ColladaDataSource* dataSource = (*itDataSourceEntry).second;
							switch (streamType)
							{
							case DATA_STREAM_NORMAL:
								if (!this->normalSource)
								{
									this->SetNormalSource(dataSource);
									this->normalIndexOffset = offset;
								}
								else
								{
									ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"));
								}
								break;
							case DATA_STREAM_COLOUR:
								if (!this->colourSource)
								{
									this->SetColourSource(dataSource);
									this->colourIndexOffset = offset;
								}
								else
								{
									ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"));
								}
								break;
							case DATA_STREAM_TEXCOORD:
								if (!this->texCoordSource)
								{
									this->SetTexCoordSource(dataSource);
									this->texCoordIndexOffset = offset;
								}
								else
								{
									ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"));
								}
								break;
							}
						}
						else
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies non-existent data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						}
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies external data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
				}
			}
			else
			{
				ReportWarning(pListener, LINE_LOG" 'input' tag requires both 'semantic' and 'source' attributes.", inputNode->getLine());
			}
			++offset;
			if (offset > numIndices)
				numIndices = offset;
		}
	}

	// The offset at this point is the total number of indices per polygon.
	this->indexGroupSize = numIndices;
	if (this->indexGroupSize == 0)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node contains no input tags.", node->getLine(), node->getTag());
		return false;
	}

	return true;
}

ColladaMaterial* BasePrimitiveStream::GetMaterial()
{
	return this->material;
}

bool BasePrimitiveStream::CreateMesh(CMesh32* mesh, float const scale, IColladaLoaderListener* pListener, std::vector<int>& newToOldPositionMapping, const ColladaAssetMetaData::EUpAxis upAxis)
{
	mesh->Clear();

	// -------------------- Read the positions --------------------------------------------
	std::vector<Vec3> positions;
	if (!this->vertexStream)
	{
		ReportError(pListener, "Mesh has no vertex stream.");
		return false;
	}
	else
	{
		int const count = this->vertexStream->GetSize();
		positions.resize(count);
		if (!this->vertexStream->ReadVertexPositions(&positions[0], count, scale, pListener, upAxis))
		{
			ReportError(pListener, "Failed to load positions for mesh.");
			return false;
		}
	}

	// -------------------- Read the normal vectors --------------------------------------------
	std::vector<Vec3> normals;
	if (!this->normalSource)
	{
		ReportError(pListener, "Mesh has no normals. TODO: generate proper normals.");
		return false;
	}
	else
	{
		static const char* const streamName[] = { "x", "y", "z" };
		static const int streamCount = sizeof(streamName) / sizeof(streamName[0]);
		std::vector<float> streamData[streamCount];
		size_t streamSize = 0;
		bool bDifferentSizes = false;

		for (int i = 0; i < streamCount; ++i)
		{
			bool const ok = this->normalSource->GetDataAccessor()->ReadAsFloat(streamName[i], streamData[i], pListener);
			if (!ok)
			{
				streamData[i].clear();
			}

			// All streams should have same size
			if ((i > 0) && (streamData[i].size() != streamSize))
			{
				bDifferentSizes = true;
				break;
			}
			streamSize = streamData[i].size();
		}

		if (bDifferentSizes)
		{
			ReportError(pListener, "Normals streams (XYZ) have different sizes.");
			return false;
		}
		else if (streamSize == 0)
		{
			ReportError(pListener, "Normals streams (XYZ) are empty.");
			return false;
		}
		else
		{
			normals.resize(streamSize);
			for (size_t i = 0; i < streamSize; ++i)
			{
				if( upAxis == ColladaAssetMetaData::eUA_Y )
				{
					normals[i].x =  streamData[0][i];
					normals[i].y = -streamData[2][i];
					normals[i].z =  streamData[1][i];
				}
				else
				{
					normals[i].x = streamData[0][i];
					normals[i].y = streamData[1][i];
					normals[i].z = streamData[2][i];
				}
			}
		}
	}

	if (positionIndexOffset == -1 || normalIndexOffset == -1)
	{
		ReportError(pListener, "Index offset error of input node.");
		return false;
	}

	// --------------------- Read in the texture coordinates ------------------------------------
	bool readTextureCoordinatesSuccessfully = false;
	std::vector<SMeshTexCoord> uvs;
	if (this->texCoordSource)
	{
		static const char* const streamName[] = { "s", "t" };
		static const int streamCount = sizeof(streamName) / sizeof(streamName[0]);
		std::vector<float> streamData[streamCount];

		size_t streamSize = 0;
		bool bDifferentSizes = false;

		for (int i = 0; i < streamCount; ++i)
		{
			bool const ok = this->texCoordSource->GetDataAccessor()->ReadAsFloat(streamName[i], streamData[i], pListener);
			if (!ok)
			{
				streamData[i].clear();
			}

			// Non-empty streams should have same size
			if ((!streamData[i].empty()) && (streamData[i].size() != streamSize))
			{
				if (streamSize != 0)
				{
					bDifferentSizes = true;
					break;
				}
				streamSize = streamData[i].size();
			}
		}

		if (bDifferentSizes)
		{
			ReportWarning(pListener, "Texture coordinate streams have different sizes - ignoring texture coordinates.");
		}
		else if (streamSize == 0)
		{
			ReportWarning(pListener, "Texture coordinate streams are empty - ignoring texture coordinates.");
		}
		else
		{
			uvs.resize(streamSize);
			for (size_t i = 0; i < streamSize; ++i)
			{
				uvs[i].s = streamData[0].empty() ? 0 : streamData[0][i];
				uvs[i].t = streamData[1].empty() ? 0 : streamData[1][i];
			}
			readTextureCoordinatesSuccessfully = true;
		}
	}

	// ---------------------- Read in the vertex colors -------------------------------------------
	bool readVertexColoursSuccessfully = false;
	std::vector<SMeshColor> vertexColours;
	if (this->colourSource)
	{
		static const char* const streamName[] = { "r", "g", "b", "a" };
		static const int streamCount = sizeof(streamName) / sizeof(streamName[0]);
		std::vector<float> streamData[streamCount];

		size_t streamSize = 0;
		bool bDifferentSizes = false;

		for (int i = 0; i < streamCount; ++i)
		{
			bool const ok = this->colourSource->GetDataAccessor()->ReadAsFloat(streamName[i], streamData[i], pListener);
			if (!ok)
			{
				streamData[i].clear();
			}

			// Non-empty streams should have same size
			if ((!streamData[i].empty()) && (streamData[i].size() != streamSize))
			{
				if (streamSize != 0)
				{
					bDifferentSizes = true;
					break;
				}
				streamSize = streamData[i].size();
			}
		}

		if (bDifferentSizes)
		{
			ReportWarning(pListener, "Vertex color streams have different sizes - ignoring vertex colors.");
		}
		else if (streamSize == 0)
		{
			ReportWarning(pListener, "Vertex color streams are empty - ignoring vertex colors.");
		}
		else
		{
			vertexColours.resize(streamSize);
			for (size_t i = 0; i < streamSize; ++i)
			{
				vertexColours[i].r = streamData[0].empty() ?   0 : unsigned(255 * streamData[0][i]);
				vertexColours[i].g = streamData[1].empty() ?   0 : unsigned(255 * streamData[1][i]);
				vertexColours[i].b = streamData[2].empty() ?   0 : unsigned(255 * streamData[2][i]);
				vertexColours[i].a = streamData[3].empty() ? 255 : unsigned(255 * streamData[3][i]);
			}
			readVertexColoursSuccessfully = true;
		}
	}

	// Create the position and normal stream --------------------
	mesh->SetVertexCount(this->numIndices);

	newToOldPositionMapping.resize(this->numIndices);

	for (int i=0; i<this->numIndices; i++)
	{
		if (vertexIndices[i] >= positions.size() || normalIndices[i] >= normals.size())
		{
			ReportWarning(pListener, "Index is out of range.");
			return false;
		}
		mesh->m_pPositions[i] = positions[vertexIndices[i]];
		mesh->m_pNorms[i] = normals[normalIndices[i]];

		mesh->m_bbox.Add(mesh->m_pPositions[i]);

		newToOldPositionMapping[i] = vertexIndices[i];
	}

	// Create the texture coordinates stream --------------------
	if (readTextureCoordinatesSuccessfully)
	{
		// Resize the texture coordinates stream.
		mesh->SetTexCoordsCount(this->numIndices);

		for (int i=0; i<this->numIndices; i++)
		{
			if (texcoordIndices[i] >= int(uvs.size()))
			{
				ReportWarning(pListener, "Index out of range.");
				return false;
			}
			mesh->m_pTexCoord[i] = uvs[texcoordIndices[i]];
		}
	}

	// Create the color stream ----------------------------------
	if (readVertexColoursSuccessfully)
	{
		// Resize the vertex colors stream.
		mesh->SetColor0Count(this->numIndices);

		// Copy the vertex colors.
		for (int i=0; i<this->numIndices; i++)
		{
			if (colorIndices[i] >= int(vertexColours.size()))
			{
				ReportWarning(pListener, "Index out of range.");
				return false;
			}
			mesh->m_pColor0[i] = vertexColours[colorIndices[i]];
		}
	}

	return true;
}

void BasePrimitiveStream::SetVertexStream(ColladaVertexStream* vertexStream)
{
	this->vertexStream = vertexStream;
}

void BasePrimitiveStream::SetNormalSource(ColladaDataSource* normalSource)
{
	this->normalSource = normalSource;
}

void BasePrimitiveStream::SetTexCoordSource(ColladaDataSource* texCoordSource)
{
	this->texCoordSource = texCoordSource;
}

void BasePrimitiveStream::SetColourSource(ColladaDataSource* colourSource)
{
	this->colourSource = colourSource;
}

bool BasePolygonPrimitiveStream::CreateMesh(CMesh32* mesh, float const scale, IColladaLoaderListener* pListener, std::vector<int>& newToOldPositionMapping, const ColladaAssetMetaData::EUpAxis upAxis)
{
	// Call the base class version.
	if (!BasePrimitiveStream::CreateMesh(mesh, scale, pListener, newToOldPositionMapping, upAxis))
		return false;

	// Compute the number of faces we will have.
	int numFaces = 0;
	for (int i = 0; i < int(this->polygonSizes.size()); i++)
		numFaces += this->polygonSizes[i] - 2;

	// Allocate the new faces.
	mesh->SetFacesCount(numFaces);

	// Loop through all the polygons, generating triangular faces.
	int indexIndex = 0;
	int numVertices = this->numIndices;
	int faceIndex = 0;
	for (int polyIndex = 0; polyIndex < int(this->polygonSizes.size()); polyIndex++)
	{
		int firstIndex = indexIndex++;
		int lastIndex = indexIndex++;

		for (int vertexIndex = 2; vertexIndex < this->polygonSizes[polyIndex]; vertexIndex++)
		{
			int index = indexIndex++;

			SMeshFace32* pFace = &mesh->m_pFaces[faceIndex++];
			memset(pFace, 0, sizeof(SMeshFace32));
			if (firstIndex < 0 || firstIndex >= numVertices
				|| lastIndex < 0 || lastIndex >= numVertices
				|| index < 0 || index >= numVertices)
			{
				ReportError(pListener, "Mesh sub-set contains out-of-range indices.");
				return false;
			}
			pFace->v[0] = pFace->t[0] = firstIndex;
			pFace->v[1] = pFace->t[1] = lastIndex;
			pFace->v[2] = pFace->t[2] = index;
			pFace->nSubset = 0;
			lastIndex = index;
		}
	}

	return true;
}

bool PolyStripPrimitiveStream::ParseNode(XmlNodeRef& node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener)
{
	if (!BasePrimitiveStream::ParseNode(node, materials, vertexStreams, dataSources, pListener))
		return false;

	// Find out how many polygons there are and reserve space for them.
	this->polygonSizes.clear();
	int polycount = 0;
	if (node->haveAttr("count"))
		polycount = atol(node->getAttr("count"));

	if (polycount > 0)
	{
		this->polygonSizes.reserve(polycount);
	}
	else
	{
		ReportWarning(pListener, LINE_LOG" 'polylist' count attribute is zero - skipping geometry", node->getLine());
		return false;
	}

	// Check whether there is a vcount element.
	XmlNodeRef vcountNode = node->findChild("vcount");
	if (!vcountNode)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node has no 'vcount' element - skipping geometry.", node->getLine(), node->getTag());
		return false;
	}

	// Read the polygon sizes from the vcount node.
	IntStream stream(vcountNode->getContent());
	int polygonSize;
	numIndices = 0;
	bool polywarn = false;
	do 
	{
		polygonSize = stream.Read();
		if (polygonSize >= 0)
		{
			polygonSizes.push_back(polygonSize);
			numIndices += polygonSize;
			if (polygonSize < 3)
				polywarn = true;
		}
	}
	while(polygonSize >= 0);

	if (polywarn)
		ReportWarning(pListener, LINE_LOG" 'vcount' node has some 1 or 2 sided polygons", vcountNode->getLine());
	
	if (polycount != polygonSizes.size())
	{
		ReportWarning(pListener, LINE_LOG" The number of polygons of 'vcount' node does not match with the 'count' attribute in the 'polylist' header - skipping geometry", vcountNode->getLine());
		return false;
	}

	vertexIndices.clear();
	normalIndices.clear();
	texcoordIndices.clear();
	colorIndices.clear();

	// Read all the polygons.
	for (int childIndex = 0; childIndex < node->getChildCount(); ++childIndex)
	{
		XmlNodeRef primNode = node->getChild(childIndex);
		string primTag = primNode->getTag();
		std::transform(primTag.begin(), primTag.end(), primTag.begin(), tolower);
		if (primTag == "p")
		{
			vertexIndices.resize(numIndices);
			normalIndices.resize(numIndices);
			texcoordIndices.resize(numIndices);
			colorIndices.resize(numIndices);

			IntStream stream(primNode->getContent());
			for (int i=0; i<numIndices; i++)
			{
				for (int g=0; g<indexGroupSize; g++)
				{
					int index = stream.Read();
					if (index == -1)
					{
						ReportWarning(pListener, LINE_LOG" Number of indices of the node 'p' is less than required - skipping geometry", primNode->getLine());
						return false;
					}

					if (g == positionIndexOffset)
						vertexIndices[i] = index;
					else if (g == normalIndexOffset)
						normalIndices[i] = index;
					else if (g == texCoordIndexOffset)
						texcoordIndices[i] = index;
					else if (g == colourIndexOffset)
						colorIndices[i] = index;
				}
			}

			if (stream.Read() != -1)
			{
				ReportWarning(pListener, LINE_LOG" Number of indices of the node 'p' is more than required - skipping geometry", primNode->getLine());
				return false;
			}
		}
	}

	return true;
}

bool TrianglesPrimitiveStream::ParseNode(XmlNodeRef& node, MaterialMap& materials, VertexStreamMap& vertexStreams, DataSourceMap& dataSources, IColladaLoaderListener* pListener)
{
	if (!BasePrimitiveStream::ParseNode(node, materials, vertexStreams, dataSources, pListener))
		return false;

	// Find out how many polygons there are and reserve space for them.
	this->polygonSizes.clear();
	int tricount = 0;
	if (node->haveAttr("count"))
		tricount = atol(node->getAttr("count"));

	if (tricount > 0)
	{
		this->polygonSizes.resize(tricount);
		numIndices = tricount * 3;
		for (int i=0; i<tricount; i++)
			polygonSizes[i] = 3;
	}
	else
	{
		ReportWarning(pListener, LINE_LOG" 'triangles' count attribute is zero - skipping geometry", node->getLine());
		return false;
	}

	vertexIndices.clear();
	normalIndices.clear();
	texcoordIndices.clear();
	colorIndices.clear();

	// Read all the polygons.
	for (int childIndex = 0; childIndex < node->getChildCount(); ++childIndex)
	{
		XmlNodeRef primNode = node->getChild(childIndex);
		string primTag = primNode->getTag();
		std::transform(primTag.begin(), primTag.end(), primTag.begin(), tolower);
		if (primTag == "p")
		{
			vertexIndices.resize(numIndices);
			normalIndices.resize(numIndices);
			texcoordIndices.resize(numIndices);
			colorIndices.resize(numIndices);

			IntStream stream(primNode->getContent());
			for (int i=0; i<numIndices; i++)
			{
				for (int g=0; g<indexGroupSize; g++)
				{
					int index = stream.Read();
					if (index == -1)
					{
						ReportWarning(pListener, LINE_LOG" Number of indices of the node 'p' is less than required - skipping geometry", primNode->getLine());
						return false;
					}

					if (g == positionIndexOffset)
						vertexIndices[i] = index;
					else if (g == normalIndexOffset)
						normalIndices[i] = index;
					else if (g == texCoordIndexOffset)
						texcoordIndices[i] = index;
					else if (g == colourIndexOffset)
						colorIndices[i] = index;
				}
			}

			if (stream.Read() != -1)
			{
				ReportWarning(pListener, LINE_LOG" Number of indices of the node 'p' is more than required - skipping geometry", primNode->getLine());
				return false;
			}
		}
	}

	return true;
}

ColladaController::ColladaController(ColladaGeometry* pGeometry)
:   pGeometry(pGeometry)
{
	jointListSource = NULL;

	indexGroupSize = -1;
	jointIndexOffset = -1;
	weightIndexOffset = -1;
}

bool ColladaController::ParseVertexWeights(XmlNodeRef node, DataSourceMap& dataSources, IColladaLoaderListener* pListener)
{
	// Look through the child nodes for input nodes.
	int offset = 0;
	int numIndices = 0;
	for (int childIndex=0; childIndex<node->getChildCount(); childIndex++)
	{
		XmlNodeRef inputNode = node->getChild(childIndex);
		string inputTag = inputNode->getTag();
		std::transform(inputTag.begin(), inputTag.end(), inputTag.begin(), tolower);
		if (inputTag == "input")
		{
			// Check whether this input overrides the offset.
			const char* offsetAttr = 0;
			if (inputNode->haveAttr("idx"))
				offsetAttr = "idx";
			if (inputNode->haveAttr("offset"))
				offsetAttr = "offset";
			if (offsetAttr)
			{
				char* end;
				const char* buf = inputNode->getAttr(offsetAttr);
				int newOffset = strtol(buf, &end, 0);
				if (buf != end)
					offset = newOffset;
			}

			// Check whether tag has both a semantic and a source.
			if (inputNode->haveAttr("semantic") && inputNode->haveAttr("source"))
			{
				// Get the semantic of the input.
				string semantic = inputNode->getAttr("semantic");
				std::transform(semantic.begin(), semantic.end(), semantic.begin(), tolower);
				if (semantic == "joint")
				{
					// Look for a joint IDREF array of this id.
					string dataSourceID = inputNode->getAttr("source");
					if (dataSourceID[0] != '#')
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies external data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}

					dataSourceID = dataSourceID.substr(1, string::npos);
					DataSourceMap::iterator itDataSourceEntry = dataSources.find(dataSourceID);
					if (itDataSourceEntry == dataSources.end())
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies non-existent data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}

					ColladaDataSource* dataSource = (*itDataSourceEntry).second;

					if (this->jointListSource == NULL)
					{
						this->jointListSource = dataSource;
					}
					else if (this->jointListSource != dataSource)
					{
						ReportWarning(pListener, LINE_LOG" Joint list data source already defined. 'input' tag (semantic '%s') specifies another data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}
					
					if (this->jointList.size() == 0)
					{
						dataSource->GetDataAccessor()->ReadAsID("", this->jointList, pListener);
						this->jointIndexOffset = offset;
						if (GetNumJoints() > 255)
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies too much of joints (>255) source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
							this->jointList.clear();
							this->baseMatrices.clear();
							continue;
						}

						if (this->baseMatrices.size() && this->baseMatrices.size() != GetNumJoints())
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') wrong number of joints - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"));
							this->jointList.clear();
							this->baseMatrices.clear();
							continue;
						}
					}
				}
				// Check whether the input refers to another data stream.
				else if (semantic == "weight")
				{
					// Look for a joint IDREF array of this id.
					string dataSourceID = inputNode->getAttr("source");
					if (dataSourceID[0] != '#')
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies external data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}

					dataSourceID = dataSourceID.substr(1, string::npos);
					DataSourceMap::iterator itDataSourceEntry = dataSources.find(dataSourceID);
					if (itDataSourceEntry == dataSources.end())
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies non-existent data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}

					ColladaDataSource* dataSource = (*itDataSourceEntry).second;

					if (this->weightList.size() == 0)
					{
						dataSource->GetDataAccessor()->ReadAsFloat("", this->weightList, pListener);
						this->weightIndexOffset = offset;
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"));
					}
				}
			}
			else
			{
				ReportWarning(pListener, LINE_LOG" 'input' tag requires both 'semantic' and 'source' attributes.", inputNode->getLine());
			}
			
			offset++;
			if (offset > numIndices)
				numIndices = offset;
		}
	}

	this->indexGroupSize = numIndices;
	if (this->indexGroupSize < 2)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node has no 2 input tags", node->getLine(), node->getTag());
		return false;
	}

	if (this->indexGroupSize > 2)
		ReportWarning(pListener, LINE_LOG" '%s' node has more than 2 input tags", node->getLine(), node->getTag());

	return true;
}

bool ColladaController::ParseBoneMap(XmlNodeRef node, DataSourceMap& dataSources, IColladaLoaderListener* pListener)
{
	this->weightSizes.clear();
	int weightcount = 0;
	if (node->haveAttr("count"))
		weightcount = atol(node->getAttr("count"));

	if (weightcount > 0)
	{
		this->weightSizes.reserve(weightcount);
	}
	else
	{
		ReportWarning(pListener, LINE_LOG" 'vertex_weights' count attribute is zero - skipping controller", node->getLine());
		return false;
	}

	if (jointIndexOffset == -1 || weightIndexOffset == -1)
	{
		ReportError(pListener, LINE_LOG" Index offset error of input node.", node->getLine());
		return false;
	}

	// Check whether there is a vcount element.
	XmlNodeRef vcountNode = node->findChild("vcount");
	if (!vcountNode)
	{
		ReportWarning(pListener, LINE_LOG" '%s' node has no 'vcount' element - skipping controller.", node->getLine(), node->getTag());
		return false;
	}

	// Read the polygon sizes from the vcount node.
	IntStream stream(vcountNode->getContent());
	int weightSize;
	int numIndices = 0;
	bool warn = false;
	do 
	{
		weightSize = stream.Read();
		if (weightSize >= 0)
		{
			weightSizes.push_back(weightSize);
			numIndices += weightSize;
			if (weightSize == 0 || weightSize > jointList.size())
				warn = true;
		}
	}
	while(weightSize >= 0);

	if (warn)
		ReportWarning(pListener, LINE_LOG" 'vcount' node has some wrong value (must be 1..%d)", vcountNode->getLine(), jointList.size());
	
	if (weightcount != weightSizes.size())
	{
		ReportWarning(pListener, LINE_LOG" The number of weights of 'vcount' node does not match with the 'count' attribute in the 'vertex_weights' header - skipping controller", vcountNode->getLine());
		return false;
	}

	boneMapping.clear();

	// Read all the vertex weights
	for (int childIndex = 0; childIndex < node->getChildCount(); childIndex++)
	{
		XmlNodeRef weightNode = node->getChild(childIndex);
		string weightTag = weightNode->getTag();
		std::transform(weightTag.begin(), weightTag.end(), weightTag.begin(), tolower);
		if (weightTag == "v")
		{
			boneMapping.resize(weightcount);

			IntStream stream(weightNode->getContent());
			for (int i=0; i<weightcount; i++)
			{
				for (int b=0; b<weightSizes[i]; b++)
				{
					for (int g=0; g<indexGroupSize; g++)
					{
						int index = stream.Read();
						if (index < 0)
						{
							ReportWarning(pListener, LINE_LOG" Number of indices of the node 'v' is less than required - skipping controller", weightNode->getLine());
							return false;
						}

						if (g == jointIndexOffset)
						{
							if (index < jointList.size())
								boneMapping[i].boneIDs.push_back(index);
							else
								ReportWarning(pListener, LINE_LOG" Joint index out of range in 'v' node (index=%d, jointCount=%d)", weightNode->getLine(), index, jointList.size() );
						}
						else if (g == weightIndexOffset)
						{
							if (index < weightList.size())
								boneMapping[i].weights.push_back(this->weightList[index]);
							else
								ReportWarning(pListener, LINE_LOG" Weight index out of range in 'v' node", weightNode->getLine());
						}
					}
				}
			}

			if (stream.Read() != -1)
			{
				ReportWarning(pListener, LINE_LOG" Number of indices of the node 'v' is more than required - skipping controller", weightNode->getLine());
				return false;
			}
		}
	}
	
	return true;
}

bool ColladaController::ParseBoneMatrices(XmlNodeRef node, DataSourceMap& dataSources, float const scale, IColladaLoaderListener* pListener)
{
	// Look through the child nodes for input nodes.
	for (int childIndex=0; childIndex<node->getChildCount(); childIndex++)
	{
		XmlNodeRef inputNode = node->getChild(childIndex);
		string inputTag = inputNode->getTag();
		std::transform(inputTag.begin(), inputTag.end(), inputTag.begin(), tolower);
		if (inputTag == "input")
		{
			// Check whether tag has both a semantic and a source.
			if (inputNode->haveAttr("semantic") && inputNode->haveAttr("source"))
			{
				// Get the semantic of the input.
				string semantic = inputNode->getAttr("semantic");
				std::transform(semantic.begin(), semantic.end(), semantic.begin(), tolower);
				if (semantic == "inv_bind_matrix")
				{
					// Look for a matrix float array of this id.
					string dataSourceID = inputNode->getAttr("source");
					if (dataSourceID[0] != '#')
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies external data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}

					dataSourceID = dataSourceID.substr(1, string::npos);
					DataSourceMap::iterator itDataSourceEntry = dataSources.find(dataSourceID);
					if (itDataSourceEntry == dataSources.end())
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies non-existent data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}

					ColladaDataSource* dataSource = (*itDataSourceEntry).second;

					if (this->baseMatrices.size() == 0)
					{
						dataSource->GetDataAccessor()->ReadAsMatrix("", this->baseMatrices, pListener);

						if (GetNumJoints() && GetNumJoints() != this->baseMatrices.size())
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') wrong number of matrices - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"));
							this->baseMatrices.clear();
							this->jointList.clear();
							continue;
						}

						for (uint i=0; i<this->baseMatrices.size(); ++i)
						{
							this->baseMatrices[i].m03 *= scale;
							this->baseMatrices[i].m13 *= scale;
							this->baseMatrices[i].m23 *= scale;							
						}
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"));
					}
				}
				else if (semantic == "joint")
				{
					// Look for a joint IDREF array of this id.
					string dataSourceID = inputNode->getAttr("source");
					if (dataSourceID[0] != '#')
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies external data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}

					dataSourceID = dataSourceID.substr(1, string::npos);
					DataSourceMap::iterator itDataSourceEntry = dataSources.find(dataSourceID);
					if (itDataSourceEntry == dataSources.end())
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies non-existent data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}

					ColladaDataSource* dataSource = (*itDataSourceEntry).second;

					if (this->jointListSource == NULL)
					{
						this->jointListSource = dataSource;
					}
					else if (this->jointListSource != dataSource)
					{
						ReportWarning(pListener, LINE_LOG" Joint list data source already defined. 'input' tag (semantic '%s') specifies another data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
						continue;
					}
				}
			}
			else
			{
				ReportWarning(pListener, LINE_LOG" 'input' tag requires both 'semantic' and 'source' attributes.", inputNode->getLine());
			}
		}
	}

	return true;
}

void ColladaController::ReduceBoneMap(XmlNodeRef node, IColladaLoaderListener* pListener)
{
	meshBoneMapping.resize(boneMapping.size());

	bool moreThan4WeightFlag = false;
	bool negativeWeightFlag = false;
	bool tooBigSumFlag = false;
	for (int i=0; i<boneMapping.size(); i++)
	{
		int numweight = boneMapping[i].weights.size();
		assert(numweight == boneMapping[i].boneIDs.size());

		ColladaBoneMap tempMap = boneMapping[i];

		int num_real_weight = 0;
		for (int j=0; j<numweight; j++)
		{
			if (tempMap.weights[j] != 0.0f)
				num_real_weight++;
			if (tempMap.weights[j] < 0.0f)
				negativeWeightFlag = true;
			assert(tempMap.boneIDs[j] >= 0 && tempMap.boneIDs[j] <= 255);
		}
		if (num_real_weight > 4)
			moreThan4WeightFlag = true;

		std::vector<float> weight4;
		std::vector<int> index4;
		for (int b=0; b<4; b++)
		{
			float maxweight = 0.0f;
			int boneindex = -1;
			int top_index;
			for (int j=0; j<numweight; j++)
			{
				if (tempMap.weights[j] > maxweight)
				{
					maxweight = tempMap.weights[j];
					boneindex = tempMap.boneIDs[j];
					top_index = j;
				}
			}
			if (boneindex != -1)
			{
				weight4.push_back(maxweight);
				index4.push_back(boneindex);
				tempMap.weights[top_index] = 0.0f;
			}
			else
			{
				weight4.push_back(0.0f);
				index4.push_back(0);
			}
		}

		float weight_sum = weight4[0] + weight4[1] + weight4[2] + weight4[3];
		if (weight_sum > 1.001f)
			tooBigSumFlag = true;

		if (weight_sum > 0.0f)
		{
			float weight_mul = 1.0f / weight_sum;
			weight4[0] *= weight_mul;
			weight4[1] *= weight_mul;
			weight4[2] *= weight_mul;
			weight4[3] *= weight_mul;
		}

		for (int b=0; b<4; b++)
		{
			meshBoneMapping[i].boneIDs[b] = index4[b];
			meshBoneMapping[i].weights[b] = (int)(255.0f * weight4[b]);
		}
	}

	if (moreThan4WeightFlag)
		ReportWarning(pListener, LINE_LOG" Weights array contains too much bone references in controller '%s' - reduced to 4 bone per vertex.", node->getLine(), node->getAttr("id"));
	if (negativeWeightFlag)
		ReportWarning(pListener, LINE_LOG" Weights array contains negative weights in controller '%s'", node->getLine(), node->getAttr("id"));
	if (tooBigSumFlag)
		ReportWarning(pListener, LINE_LOG" Weights array contains wrong weights summary in controller '%s'", node->getLine(), node->getAttr("id"));
}

// [MichaelS] This function seems to make sure that the root bone is bone 0. I assume this is necessary because the loading code assumes it.
void ColladaController::ReIndexBoneIndices(const string& rootJoint)
{
	int rootIndex = -1;
	
	for (int i=0; i<GetNumJoints(); i++)
		if (GetJointID(i) == rootJoint)
			rootIndex = i;

	if (rootIndex == -1 || rootIndex == 0)
		return;

	std::swap(jointList[0],jointList[rootIndex]);
	std::swap(baseMatrices[0],baseMatrices[rootIndex]);

	for (int i=0; i<meshBoneMapping.size(); i++)
	{
		uint8& idx0 = meshBoneMapping[i].boneIDs.r;
		uint8& idx1 = meshBoneMapping[i].boneIDs.g;
		uint8& idx2 = meshBoneMapping[i].boneIDs.b;
		uint8& idx3 = meshBoneMapping[i].boneIDs.a;

		if (idx0 == 0) idx0 = rootIndex;
		else if (idx0 == rootIndex) idx0 = 0;

		if (idx1 == 0) idx1 = rootIndex;
		else if (idx1 == rootIndex) idx1 = 0;

		if (idx2 == 0) idx2 = rootIndex;
		else if (idx2 == rootIndex) idx2 = 0;

		if (idx3 == 0) idx3 = rootIndex;
		else if (idx3 == rootIndex) idx3 = 0;
	}
}

int ColladaController::GetJointIndex(const string& name)
{
	for (int i=0; i<GetNumJoints(); i++)
		if (GetJointID(i) == name)
			return i;
	
	return -1;
}

ColladaNode::ColladaNode(const string& a_id, const string& a_name, ColladaNode* const a_parent, int const a_nodeIndex)
{
	if (a_parent)
	{
		a_parent->AddChild(this);
	}

	name = a_name;

	// Replace any possible special name prefix to CryEngine's internal "$" prefix
	{
		static const char* const prefixes[] = 
		{
			"_", 
			"cry_",
			0
		};

		for (uint i=0; prefixes[i]; ++i)
		{
			if (StringHelpers::StartsWithIgnoreCase(name,prefixes[i]))
			{
				name = string("$") + name.substr(strlen(prefixes[i]));
				break;
			}
		}
	}

	id = a_id;
	parent = a_parent;
	nodeIndex = a_nodeIndex;

	type = NODE_NULL;

	translate = Vec3(0,0,0);
	rotationX = Vec4(1,0,0,0);
	rotationY = Vec4(0,1,0,0);
	rotationZ = Vec4(0,0,1,0);
	scale = Vec3(1,1,1);
	transform.SetIdentity();

	pGeometry = NULL;
	pController = NULL;
}

void ColladaNode::AddSubMaterial(ColladaMaterial* material)
{
	this->materials.push_back(material);
}

void ColladaNode::GetSubMaterials(std::vector<ColladaMaterial*>& materials)
{
	materials = this->materials;
}

ColladaAnimation::ColladaAnimation()
{
	targetNode = NULL;
}

ColladaAnimation::~ColladaAnimation()
{
}

void ColladaAnimation::GetKey(int index, ColladaKey& colladaKey)
{
	assert(index >=0 && index < GetKeys());

	colladaKey = this->colladaKeys[index];
}

float ColladaAnimation::GetDuration(void)
{
	assert(GetKeys() > 0);

	return (this->colladaKeys[GetKeys()-1].time - this->colladaKeys[0].time);
}

float ColladaAnimation::GetValue(float time)
{
	assert(GetKeys() > 0);
	int numKeys = GetKeys();
	int lastKey = GetKeys() - 1; 

	if (time <= colladaKeys[0].time)
		return colladaKeys[0].value;

	if (time >= colladaKeys[lastKey].time)
		return colladaKeys[lastKey].value;

	//TODO: write the interpolation function
	return 0.0f;
}

void ColladaAnimation::ParseName(const string& name)
{
	int pos = name.size();
	this->flags = 0;
	while (pos >= 0)
	{
		int const nextPos = name.rfind('-', pos - 1);
		string const flagString = name.substr(nextPos + 1, pos - nextPos - 1);
		if (_stricmp(flagString.c_str(), "noexport") == 0)
		{
			this->flags |= Flags_NoExport;
			break;
		}
		pos = nextPos;
	}
}

bool ColladaAnimation::ParseChannel(XmlNodeRef channelNode, XmlNodeRef samplerNode, SceneMap& scenes, DataSourceMap& dataSources, float scale, IColladaLoaderListener* pListener)
{
	if (!ParseTransformTarget(channelNode,scenes,pListener))
		return false;

	if (!ParseSamplerSource(samplerNode,dataSources,scale,pListener))
		return false;

	return true;
}

bool ColladaAnimation::ParseTransformTarget(XmlNodeRef node, SceneMap& scenes, IColladaLoaderListener* pListener)
{
	string targetID = node->getAttr("target");
	string nodeName,transformName,transformID,componentID;

	size_t slashPos = targetID.rfind('/');
	if (slashPos == string::npos)
	{
		ReportWarning(pListener, LINE_LOG" target attribute of 'channel' does not contains '/' separator.", node->getLine());
		return false;
	}

	nodeName = targetID.substr(0, slashPos);
	transformName = targetID.substr(slashPos+1, string::npos);

	size_t dotPos = transformName.rfind('.');

	transformID = transformName.substr(0, dotPos);
	if (dotPos != string::npos)
		componentID = transformName.substr(dotPos+1, string::npos);

	std::transform(transformID.begin(), transformID.end(), transformID.begin(), tolower);
	std::transform(componentID.begin(), componentID.end(), componentID.begin(), tolower);

	if (transformID == "translation")
		this->targetTransform = TRANS_TRANSLATION;
	else if (transformID == "rotation_x")
		this->targetTransform = TRANS_ROTATION_X;
	else if (transformID == "rotation_y")
		this->targetTransform = TRANS_ROTATION_Y;
	else if (transformID == "rotation_z")
		this->targetTransform = TRANS_ROTATION_Z;
	else if (transformID == "scale")
		this->targetTransform = TRANS_SCALE;
	else if (transformID == "near")
		this->targetTransform = TRANS_NEAR;
	else
	{
		ReportWarning(pListener, LINE_LOG" target attribute of 'channel' has unknown transformation '%s'", node->getLine(), transformID.c_str());
		return false;
	}

	if (componentID.length())
	{
		if (componentID == "x" || componentID == "y" || componentID == "z" ||
			componentID == "angle")
		{
			this->targetComponent = componentID;
		}
		else
		{
			ReportWarning(pListener, LINE_LOG" target attribute of 'channel' has unknown transformation component '%s'", node->getLine(), componentID.c_str());
			return false;
		}
	}

	for ( SceneMap::iterator scene = scenes.begin(), end = scenes.end(); scene != end; ++scene )
	{
		NameNodeMap * pNodes = (*scene).second->GetNodesNameMap();
		if ( pNodes )
		{
			NameNodeMap::const_iterator nodePos = pNodes->find(nodeName);
			if ( nodePos != pNodes->end() )
			{
				this->targetNode = (*nodePos).second;
				break;
			}
		}
	}
	
	if (!this->targetNode)
	{
		ReportWarning(pListener, LINE_LOG" target attribute of 'channel' referenced to undefined node '%s'", node->getLine(), nodeName.c_str());
		return false;
	}

	return true;
}

bool ColladaAnimation::ParseSamplerSource(XmlNodeRef node, DataSourceMap& dataSources, float scale, IColladaLoaderListener* pListener)
{
	// Look through the child nodes for input nodes.
	for (int childIndex=0; childIndex<node->getChildCount(); childIndex++)
	{
		XmlNodeRef inputNode = node->getChild(childIndex);
		string inputTag = inputNode->getTag();
		std::transform(inputTag.begin(), inputTag.end(), inputTag.begin(), tolower);
		if (inputTag == "input")
		{
			// Check whether tag has both a semantic and a source.
			if (inputNode->haveAttr("semantic") && inputNode->haveAttr("source"))
			{
				// Get the semantic of the input.
				string semantic = inputNode->getAttr("semantic");
				std::transform(semantic.begin(), semantic.end(), semantic.begin(), tolower);
				
				string dataSourceID = inputNode->getAttr("source");
				if (dataSourceID[0] != '#')
				{
					ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies external data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					continue;
				}

				dataSourceID = dataSourceID.substr(1, string::npos);
				DataSourceMap::iterator itDataSourceEntry = dataSources.find(dataSourceID);
				if (itDataSourceEntry == dataSources.end())
				{
					ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') specifies non-existent data source ('%s') - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					continue;
				}

				ColladaDataSource* dataSource = (*itDataSourceEntry).second;

				// "INPUT"
				if (semantic == "input")
				{
					if (this->inputData.size() == 0)
					{
						if (!dataSource->GetDataAccessor()->ReadAsFloat("time", this->inputData, pListener))
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') reading error - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
				}
				// "OUTPUT"
				else if (semantic == "output")
				{
					if (this->outputData.size() == 0)
					{
						if (!dataSource->GetDataAccessor()->ReadAsFloat("value", this->outputData, pListener))
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') reading error - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
				}
				// "IN_TANGENT"
				else if (semantic == "in_tangent")
				{
					if (this->inTangentData.size() == 0)
					{
						std::vector<float> inTangentX;
						std::vector<float> inTangentY;
						bool hasCorrectParams = true;
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("x", inTangentX, pListener);
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("y", inTangentY, pListener);
						int numTangents = inTangentX.size();
						
						if (!hasCorrectParams || inTangentX.size() != inTangentY.size())
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') reading error - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
							continue;
						}

						this->inTangentData.resize(numTangents);
						for (int i=0; i<numTangents; i++)
						{
							this->inTangentData[i].x = inTangentX[i];
							this->inTangentData[i].y = inTangentY[i];
						}
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
				}
				// "OUT_TANGENT"
				else if (semantic == "out_tangent")
				{
					if (this->outTangentData.size() == 0)
					{
						std::vector<float> outTangentX;
						std::vector<float> outTangentY;
						bool hasCorrectParams = true;
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("x", outTangentX, pListener);
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("y", outTangentY, pListener);
						int numTangents = outTangentX.size();

						if (!hasCorrectParams || outTangentX.size() != outTangentY.size())
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') reading error - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
							continue;
						}

						this->outTangentData.resize(numTangents);
						for (int i=0; i<numTangents; i++)
						{
							this->outTangentData[i].x = outTangentX[i];
							this->outTangentData[i].y = outTangentY[i];
						}
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
				}
				// "INTERPOLATION"
				else if (semantic == "interpolation")
				{
					if (this->interpData.size() == 0)
					{
						std::vector<string> interpNames;
						if (!dataSource->GetDataAccessor()->ReadAsName("interpolation", interpNames, pListener))
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') reading error - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
							continue;
						}

						for (int i=0; i<interpNames.size(); i++)
						{
							string interpID = interpNames[i];
							std::transform(interpID.begin(), interpID.end(), interpID.begin(), std::tolower);

							ColladaInterpType interpType;
							if (interpID == "const" || interpID == "constant")
								interpType = INTERP_CONST;
							else if (interpID == "linear")
								interpType = INTERP_LINEAR;
							else if (interpID == "bezier")
								interpType = INTERP_BEZIER;
							else
							{
								ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') reading error - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
								break;
							}

							this->interpData.push_back(interpType);
						}
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
				}
				// "TCB"
				else if (semantic == "tcb")
				{
					if (this->tcbData.size() == 0)
					{
						std::vector<float> tension;
						std::vector<float> continuity;
						std::vector<float> bias;
						bool hasCorrectParams = true;
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("tension", tension, pListener);
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("continuity", continuity, pListener);
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("bias", bias, pListener);
						int numTCBs = tension.size();

						if (!hasCorrectParams || tension.size() != continuity.size() || continuity.size() != bias.size())
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') reading error - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
							continue;
						}

						this->tcbData.resize(numTCBs);
						for (int i=0; i<numTCBs; i++)
						{
							this->tcbData[i].x = tension[i];
							this->tcbData[i].y = continuity[i];
							this->tcbData[i].z = bias[i];
						}
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
				}
				// "EASE_IN_OUT"
				else if (semantic == "ease_in_out")
				{
					if (this->easeInOutData.size() == 0)
					{
						std::vector<float> easeIn;
						std::vector<float> easeOut;
						bool hasCorrectParams = true;
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("ease_in", easeIn, pListener);
						hasCorrectParams = hasCorrectParams && dataSource->GetDataAccessor()->ReadAsFloat("ease_out", easeOut, pListener);
						int numEases = easeIn.size();

						if (!hasCorrectParams || easeIn.size() != easeOut.size())
						{
							ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') reading error - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
							continue;
						}

						this->easeInOutData.resize(numEases);
						for (int i=0; i<numEases; i++)
						{
							this->easeInOutData[i].x = easeIn[i];
							this->easeInOutData[i].y = easeOut[i];
						}
					}
					else
					{
						ReportWarning(pListener, LINE_LOG" 'input' tag (semantic '%s') already defined - skipping input.", inputNode->getLine(), inputNode->getAttr("semantic"), dataSourceID.c_str());
					}
				}
			}
			else
			{
				ReportWarning(pListener, LINE_LOG" 'input' tag requires both 'semantic' and 'source' attributes.", inputNode->getLine());
			}
		}
	}

	int numKeys = this->inputData.size();

	if (this->inputData.size() == 0 || this->outputData.size() == 0 || this->interpData.size() == 0)
	{
		ReportError(pListener, LINE_LOG" 'sampler' node requires an 'input', an 'output', and an 'interpolation' data source.", node->getLine());
		return false;
	}

	// numKeys, outputData, interpData sizes must be equal
	if (this->outputData.size() != numKeys || this->interpData.size() != numKeys)
	{
		ReportError(pListener, LINE_LOG" 'sampler' node data source sizes are not equal.", node->getLine());
		return false;
	}

	if (this->inTangentData.size())
	{
		// numKeys, inTangentData, outTangentData sizes must be equal
		if (this->inTangentData.size() != numKeys || this->outTangentData.size() != numKeys)
		{
			ReportError(pListener, LINE_LOG" 'sampler' node data source sizes are not equal.", node->getLine());
			return false;
		}
	}

	if (this->tcbData.size())
	{
		// numKeys, tcbData, easeInOutData sizes must be equal
		if (this->tcbData.size() != numKeys || this->easeInOutData.size() != numKeys)
		{
			ReportError(pListener, LINE_LOG" 'sampler' node data source sizes are not equal.", node->getLine());
			return false;
		}
	}

	this->colladaKeys.resize(numKeys);
	// fill out the time,values,interp members of colladaKeys
	float keyScale = 1.0f;
	if (this->targetTransform == TRANS_TRANSLATION)
		keyScale = scale;
	for (int i=0; i<numKeys; i++)
	{
		this->colladaKeys[i].time = this->inputData[i];
		this->colladaKeys[i].value = this->outputData[i] * keyScale;
		this->colladaKeys[i].interp = this->interpData[i];
		this->colladaKeys[i].tcb = Vec3(0,0,0);
		this->colladaKeys[i].easeInOut = Vec2(0,0);
	}
	
	// if tangents exist, copy them to colladaKeys
	for (int i=0; i<this->inTangentData.size(); i++)
	{
		this->colladaKeys[i].inTangent = this->inTangentData[i];
		this->colladaKeys[i].outTangent = this->outTangentData[i];
	}

	// if TCBs exist, copy them to colladaKeys
	for (int i=0; i<this->tcbData.size(); i++)
	{
		this->colladaKeys[i].tcb = this->tcbData[i];
		this->colladaKeys[i].easeInOut = this->easeInOutData[i];
	}

	return true;
}

ColladaAnimClip::ColladaAnimClip(float startTime, float endTime, const string& animClipID, ColladaScene* targetScene)
{
	this->startTime = startTime;
	this->endTime = endTime;
	this->animClipID = animClipID;
	this->targetScene = targetScene;
}

ColladaAnimClip::~ColladaAnimClip()
{
}

CNodeCGF* ColladaNode::CreateNodeCGF(CMaterialCGF* rootMaterial, CGFSubMaterialMap& cgfMaterials, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener)
{
	//assert(!IsNodeOfSkeleton());		// skeleton's nodes not allowed

	CMesh* mesh = NULL;

	CNodeCGF* const cgfNode = new CNodeCGF();

	strncpy(cgfNode->name,name,sizeof(cgfNode->name));
	cgfNode->name[sizeof(cgfNode->name)-1] = 0;

	cgfNode->pMaterial = rootMaterial;

	// set position,rotation,scale from transform
	cgfNode->localTM = transform;
	if (this->pController)
		cgfNode->localTM = this->pController->GetShapeMatrix() * cgfNode->localTM;
	DecomposeTransform(cgfNode->pos, cgfNode->rot, cgfNode->scl, cgfNode->localTM);

	cgfNode->pos_cont_id = -1;
	cgfNode->rot_cont_id = -1;
	cgfNode->scl_cont_id = -1;

	if (type == NODE_GEOMETRY || type == NODE_CONTROLLER)
	{
		if (pGeometry)
		{
			std::vector<SMeshBoneMapping> boneMapping;
			if (this->GetController())
				this->GetController()->GetMeshBoneMapping(boneMapping);
			float const scale = metaData.GetUnitSizeInMeters();
			mesh = pGeometry->CreateMesh(cgfMaterials, scale, pListener, std::vector<int>(), this->GetName(), boneMapping, metaData.GetUpAxis());
			if (mesh)
			{
				// Copy nPhysicalizeType from (CMaterialCGF)rootMaterial to mesh subsets
				for (int i=0; i<mesh->m_subsets.size(); i++) 
				{
					int matID = mesh->m_subsets[i].nMatID;
					assert(matID < rootMaterial->subMaterials.size());
					mesh->m_subsets[i].nPhysicalizeType = rootMaterial->subMaterials[matID]->nPhysicalizeType;
				}
			}
		}
		else
		{
			ReportWarning(pListener, "Node '%s' has no geometry", GetID().c_str());
		}
	}

	cgfNode->pMesh = mesh;

	if (StringHelpers::StartsWith(name,"$"))
	{
		cgfNode->type = CNodeCGF::NODE_HELPER;
		cgfNode->helperType = mesh ? HP_GEOMETRY : HP_DUMMY;
		cgfNode->helperSize = Vec3(100.0f,100.0f,100.0f);
	}

	switch (type)
	{
		case NODE_NULL:
			cgfNode->type = CNodeCGF::NODE_HELPER;
			if (helperData.m_eHelperType == SHelperData::eHelperType_Dummy)
			{
				cgfNode->helperType = HP_DUMMY;
				// convert helper size to centimeters, because it's what CgfNode expects.
				static float const metersToCentimeters = 100.0f;
				cgfNode->helperSize.Set(
					(helperData.m_boundBoxMax[0] - helperData.m_boundBoxMin[0]) * metersToCentimeters,
					(helperData.m_boundBoxMax[1] - helperData.m_boundBoxMin[1]) * metersToCentimeters,
					(helperData.m_boundBoxMax[2] - helperData.m_boundBoxMin[2]) * metersToCentimeters);
			}
			else
			{
				cgfNode->helperType = HP_POINT;
				cgfNode->helperSize.Set(0,0,0);
			}
			break;
		
		case NODE_GEOMETRY:
			cgfNode->type = CNodeCGF::NODE_MESH;
			break;

		case NODE_CONTROLLER:
			cgfNode->type = CNodeCGF::NODE_MESH;
			break;
		
		default:
			assert(false);
	}

	return cgfNode;
}

ColladaScene::~ColladaScene()
{
	//for (std::vector<ColladaNode*>::iterator itNode = this->nodes.begin(); itNode != this->nodes.end(); ++itNode)
	//	delete *itNode;
}

void ColladaScene::AddNode(ColladaNode* pNode)
{
	this->nodes.push_back(pNode);
}

static void RefineProperties(string& prop)
{
	if (prop.empty() || !PropertyHelpers::HasProperty(prop, "pieces"))
	{
		return;
	}

	string value;
	PropertyHelpers::GetPropertyValue(prop, "pieces", value);
	StringHelpers::Replace(value, ',', ' ');
	value = StringHelpers::Trim(StringHelpers::RemoveDuplicateSpaces(value));

	std::vector<string> parts;
	SplitString(value, ' ', parts);

	value = "";

	for (size_t i=0; i<parts.size(); ++i)
	{
		if (!parts[i].empty())
		{
			if (parts[i][0] == '_')
			{
				*parts[i].begin() = '$';	// implementation note: we cannot use "parts[i][0] = '$';" here because CryString has no "operator[] (size_t pos);"
			}
			else if (StringHelpers::StartsWithIgnoreCase(parts[i], "cry_"))
			{
				parts[i] = string("$") + parts[i].substr(4);
			}
		}	

		if (i > 0)
		{
			value += ',';
		}
		value += parts[i];
	}

	PropertyHelpers::SetPropertyValue(prop, "pieces", value.c_str());
}

CContentCGF* ColladaScene::CreateContentCGF(
	const string& filename, 
	const NameNodeMap& a_nodes, 
	NodeMaterialMap& nodeMaterialMap, 
	const ColladaAssetMetaData& metaData, 
	CGFMaterialMap& cgfMaterials, 
	IColladaLoaderListener* pListener, 
	int contentFlags)
{
	if (GetExportNodeName().empty())
	{
		ReportError(pListener, "CryExportNode's node name is missing.");
		return NULL;
	}

	if (this->nodes.empty())
	{
		ReportError(pListener, "Unable to load any nodes from the scene.");
		return NULL;
	}

	struct ColladaNodeInfo
	{
		int cgfNodeIndex;			  // -1 or >=0
		ColladaNode* pColladaNode;	  // always !=0
		int parentColladaNodeIndex;	  // -1 or >=0
	};

	struct CGFNodeInfo
	{
		int colladaNodeIndex;         // always >=0									  
		CNodeCGF* pCGFNode;           // always !=0
		int parentCGFNodeIndex;	      // -1 or >=0

		Matrix34 localTM;
		Matrix34 oldWorldTM;
		Matrix34 newWorldTM;
	};

	DynArray< ColladaNodeInfo > colladaNodes;
	DynArray< CGFNodeInfo > cgfNodes;

	{
		colladaNodes.resize( this->nodes.size() );

		for (int i=0; i<this->nodes.size(); ++i)
		{
			ColladaNodeInfo& c = colladaNodes[i];

			c.cgfNodeIndex = -1;

			c.pColladaNode = this->nodes[i];

			c.parentColladaNodeIndex = -1;

			ColladaNode* const parent = c.pColladaNode->GetParent();
			if (parent)
			{
				for (int j=0; j<this->nodes.size(); ++j)
				{
					if (this->nodes[j] == parent)
					{
						c.parentColladaNodeIndex = j;
						break;					
					}
				}
			}
		}
	}

	CContentCGF* cgf = new CContentCGF(filename.c_str());

	if (contentFlags & ContentFlags_Geometry)
	{
		CMaterialCGF* material = 0;

		this->CreateMaterial(cgfMaterials, a_nodes, nodeMaterialMap, pListener);

		for (CGFMaterialMap::const_iterator mtlMapPos = cgfMaterials.begin(), nodeMapEnd = cgfMaterials.end(); mtlMapPos != nodeMapEnd; ++mtlMapPos)
		{
			cgf->AddMaterial((*mtlMapPos).first);
			material = (*mtlMapPos).first;
		}
		cgf->SetCommonMaterial(material);
	}

	// Create a set of the nodes that are actually referred to as bones, and don't export their geometry.
	std::set<string> boneNodeNameSet;
	for (int i=0; i<colladaNodes.size(); ++i)
	{
		ColladaNode* colladaNode = colladaNodes[i].pColladaNode;
		ColladaController* controller = (colladaNode ? colladaNode->GetController() : 0);
		for (int jointIndex = 0, jointCount = (controller ? controller->GetNumJoints() : 0); jointIndex < jointCount; ++jointIndex)
		{
			string boneName = (controller ? controller->GetJointID(jointIndex) : "");
			boneNodeNameSet.insert(boneName);
		}
	}

	for (int i=0; i<colladaNodes.size(); ++i)
	{
		ColladaNode* colladaNode = colladaNodes[i].pColladaNode;
		
		CMaterialCGF* nodeMaterial = nodeMaterialMap[colladaNode];
		CGFSubMaterialMap& cgfSubMaterials = cgfMaterials[nodeMaterial];

		bool bCreateNodeCGF = true;

		if ((GetExportType() == EXPORT_CHR) || (GetExportType() == EXPORT_CHR_CAF))
		{
			// skeleton's node -> write to physical collision in skinninginfo (skip write to cgf's node)
			if (boneNodeNameSet.find(colladaNode->GetID()) != boneNodeNameSet.end())
			{
				// If there is a phys bone for this bone, then export the bone as a bone mesh.
				bCreateNodeCGF = false;
			}
		}

		if (bCreateNodeCGF)
		{
			CNodeCGF* const pCGFNode = colladaNode->CreateNodeCGF(nodeMaterial, cgfSubMaterials, metaData, pListener);
			if (pCGFNode)
			{
				CGFNodeInfo ci;
				ci.pCGFNode = pCGFNode;
				ci.colladaNodeIndex = i;
				cgfNodes.push_back(ci);

				colladaNodes[i].cgfNodeIndex = cgfNodes.size() - 1;
			}
			else
			{
				ReportError(pListener, "Failed to load node '%s'", colladaNode->GetID().c_str());
				SAFE_DELETE(cgf);
				return NULL;
			}
		}
	}

	// Set CGF node parents and LTMs
	for (int i=0; i<cgfNodes.size(); ++i)
	{
		CGFNodeInfo& node = cgfNodes[i];

		const ColladaNodeInfo& colladaNodeInfo = colladaNodes[ node.colladaNodeIndex ];
		
		const int parentColladaNodeIndex = colladaNodeInfo.parentColladaNodeIndex;

		node.parentCGFNodeIndex = 
			(parentColladaNodeIndex < 0)
			? -1
			: colladaNodes[parentColladaNodeIndex].cgfNodeIndex;

		node.localTM = node.pCGFNode->localTM;
	}

	// Set transformation of CryExportNode

	{
		int rootCount = 0;
		for (int i = 0; i < cgfNodes.size(); ++i)
		{
			if (cgfNodes[i].parentCGFNodeIndex < 0)
			{
				++rootCount;
			}
		}

		// Aligning root nodes
		for (int i = 0; i < cgfNodes.size(); ++i)
		{
			CGFNodeInfo& node = cgfNodes[i];

			if( metaData.GetUpAxis() == ColladaAssetMetaData::eUA_Y )
			{
				Matrix34 tempTM;

				Vec3 bX = node.localTM.GetColumn0();
				Vec3 bY = node.localTM.GetColumn1();
				Vec3 bZ = node.localTM.GetColumn2();
				Vec3 pos = node.localTM.GetColumn3();

				tempTM.SetFromVectors(  Vec3( bX.x,-bX.z, bX.y ),
										Vec3(-bZ.x, bZ.z,-bZ.y ),
										Vec3( bY.x,-bY.z, bY.y ),
										Vec3( pos.x, -pos.z, pos.y ) );

				node.localTM = tempTM;
			}

			if (node.parentCGFNodeIndex < 0)
			{
				// If there is only one root, move it to the origin.
				if ((rootCount == 1) && ((GetExportType() != EXPORT_CHR) && (GetExportType() != EXPORT_CHR_CAF)))
				{
					node.localTM.SetTranslation(Vec3(0,0,0));
				}
			}
		}

		if ((GetExportType() == EXPORT_CGF) || ((GetExportType() == EXPORT_CHR) || (GetExportType() == EXPORT_CHR_CAF)))
		{
			// Compute current world matrices (modeling CS -> world CS)
			for (int i = 0; i < cgfNodes.size(); ++i)
			{
				CGFNodeInfo& node = cgfNodes[i];
				node.oldWorldTM = node.localTM;
				for(int j = node.parentCGFNodeIndex; j >= 0; j = cgfNodes[j].parentCGFNodeIndex)
				{
					node.oldWorldTM = cgfNodes[j].localTM * node.oldWorldTM;
				}
			}

			// Form new (wanted) world matrices for nodes (the axes are same as world CS axes)
			for (int i = 0; i < cgfNodes.size(); ++i)
			{
				CGFNodeInfo& node = cgfNodes[i];
				node.newWorldTM.SetTranslationMat( node.oldWorldTM.GetTranslation() );
			}

			// Transform vertices and normals from their current local CS to new local CS
			for (int i = 0; i < cgfNodes.size(); ++i)
			{
				CGFNodeInfo& node = cgfNodes[i];

				if (node.pCGFNode->pMesh != 0)
				{
					Matrix34 tm = node.oldWorldTM;
					tm.SetTranslation(Vec3(0,0,0));

					// TODO: Sokov: why not recompute AABB using new vertex coordinates? It will give us tightest AABB.
					node.pCGFNode->pMesh->m_bbox.SetTransformedAABB(tm,node.pCGFNode->pMesh->m_bbox);

					for (int j = 0; j < node.pCGFNode->pMesh->GetVertexCount(); ++j)
					{
						node.pCGFNode->pMesh->m_pPositions[j] = tm.TransformPoint( node.pCGFNode->pMesh->m_pPositions[j] );
						node.pCGFNode->pMesh->m_pNorms[j] = tm.TransformVector( node.pCGFNode->pMesh->m_pNorms[j] ).GetNormalized();
					}
				}
			}

			// We've transformed vertices to new CSs, so we need to re-compute local transforms, to be consistent
			for (int i = 0; i < cgfNodes.size(); ++i)
			{
				CGFNodeInfo& node = cgfNodes[i];

				if (node.parentCGFNodeIndex < 0)
				{
					node.localTM = node.newWorldTM;
				}
				else
				{
					const Matrix34 parentWorldTM = cgfNodes[node.parentCGFNodeIndex].newWorldTM;
					const Vec3 parentPosition = parentWorldTM.GetTranslation();

					const Vec3 position = node.newWorldTM.GetTranslation();

					node.localTM = node.newWorldTM;
					node.localTM.SetTranslation(position - parentPosition);
				}

				/* Sokov: commented out because I cannot understand reason of HELPER rotation
// 				if (strstr(pNode->name.c_str(),"$joint"))
				if (pNode->type == CNodeCGF::NODE_HELPER)
				{
					worldTMs[i].SetTranslation(Vec3(worldTMs[i].GetTranslation() - parentWorldTM.GetTranslation()));
					if (isYAxisUp)
					{
						worldTMs[i] = worldTMs[i] * Matrix34::CreateRotationX(-PI/2);
					}
				}
				else
				{
					worldTMs[i] = Matrix34::CreateTranslationMat(Vec3(worldTMs[i].GetTranslation() - parentWorldTM.GetTranslation()));
				}
				*/
			}
		}
	}

	// Copy all computed data into resulting CContentCGF
	assert(cgf->GetNodeCount() == 0);
	for (int i = 0; i < cgfNodes.size(); ++i)
	{
		CGFNodeInfo& node = cgfNodes[i];

		node.newWorldTM = node.localTM;
		for (int j = node.parentCGFNodeIndex; j >= 0; j = cgfNodes[j].parentCGFNodeIndex)
		{
			node.newWorldTM = cgfNodes[j].localTM * node.newWorldTM;
		}

		node.pCGFNode->localTM = node.localTM;
		node.pCGFNode->worldTM = node.newWorldTM;

		node.pCGFNode->pParent = (node.parentCGFNodeIndex < 0) ? 0 : cgfNodes[ node.parentCGFNodeIndex ].pCGFNode;

		DecomposeTransform(node.pCGFNode->pos, node.pCGFNode->rot, node.pCGFNode->scl, node.pCGFNode->localTM);

		node.pCGFNode->bIdentityMatrix = node.pCGFNode->worldTM.IsEquivalent( Matrix34::CreateIdentity(), 0 );

		string prop = colladaNodes[node.colladaNodeIndex].pColladaNode->GetProperty();
		RefineProperties(prop);
		node.pCGFNode->properties = prop;

		cgf->AddNode( node.pCGFNode );
	}

	cgf->GetExportInfo()->bMergeAllNodes = NeedToMerge();

	return cgf;
}

void ColladaScene::SetPhysBones(PhysBoneMap& physBones)
{
	this->physBones = physBones;
}

void ColladaScene::SetPhysParentFrames(std::map<string, Matrix33>& physParentFrames)
{
	this->physParentFrames = physParentFrames;
}

void ColladaScene::AddBoneRecursive(ColladaNode *node, int parent_id, CSkinningInfo *pSkinningInfo, ColladaController* controller, const NameNodeMap& nodes, const std::map<string, int>& boneNameIndexMap)
{
	BONE_ENTITY bone_ent;
	memset(&bone_ent, 0, sizeof(bone_ent));

	int parentOfChildrenIndex = parent_id;
	if (nodes.find(node->GetID()) != nodes.end())
	{
		std::map<string, int>::const_iterator itBoneNameIndexPos = boneNameIndexMap.find(node->GetID());
		bone_ent.BoneID = (itBoneNameIndexPos != boneNameIndexMap.end() ? (*itBoneNameIndexPos).second : -1);
		bone_ent.ParentID = parent_id;
		bone_ent.ControllerID = g_crcGen.GetCRC32(GetSafeBoneName(node->GetName()).c_str());
		bone_ent.nChildren = node->GetNumChild();
		strcpy(bone_ent.prop, node->GetProperty().c_str());

		pSkinningInfo->m_arrBoneEntities.push_back(bone_ent);
		parentOfChildrenIndex = (itBoneNameIndexPos != boneNameIndexMap.end() ? (*itBoneNameIndexPos).second : parent_id);
	}

	for (int i=0; i<node->GetNumChild(); i++)
		AddBoneRecursive(node->GetChild(i), parentOfChildrenIndex, pSkinningInfo, controller, nodes, boneNameIndexMap);
}

bool ColladaScene::SetSkinningInfoCGF(CContentCGF* cgf, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener, const NameNodeMap& nameNodeMap)
{
	CSkinningInfo* pSkinningInfo = cgf->GetSkinningInfo();

	pSkinningInfo->m_arrBonesDesc.resize(0);
	for (int nodeIndex = 0, nodeCount = int(this->nodes.size()); nodeIndex < nodeCount; ++nodeIndex)
	{
		ColladaNode* colladaNode = this->nodes[nodeIndex];
		ColladaController* controller = (colladaNode ? colladaNode->GetController() : 0);
		int numJoints = (controller ? controller->GetNumJoints() : 0);

		// Add all the bones for this node to the list of bones, and create a mapping between the bone names and their indices.
		std::map<string, int> boneNameIndexMap;
		for (int i=0; i<numJoints; i++)
		{
			CryBoneDescData boneDesc;
			string boneID = (controller ? controller->GetJointID(i) : "");

			// Check whether the node referred to in the controller actually exists.
			NameNodeMap::const_iterator jointNodePos = nameNodeMap.find(boneID);
			if (jointNodePos != nameNodeMap.end())
			{
				string boneSafeName = GetSafeBoneName(boneID);
				strcpy(boneDesc.m_arrBoneName, boneSafeName.c_str());
				boneDesc.m_nControllerID = g_crcGen.GetCRC32(boneSafeName.c_str());

				if (controller == 0)
				{
					boneDesc.m_DefaultW2B = Matrix34::CreateIdentity();
					boneDesc.m_DefaultB2W = Matrix34::CreateIdentity();
				}
				else
				{
					boneDesc.m_DefaultW2B = controller->GetJointMatrix(i);
					boneDesc.m_DefaultB2W = boneDesc.m_DefaultW2B.GetInverted();

					if (metaData.GetUpAxis() == ColladaAssetMetaData::eUA_Y)
					{
						Matrix34 tempTM;

						Vec3 bX = boneDesc.m_DefaultB2W.GetColumn0();
						Vec3 bY = boneDesc.m_DefaultB2W.GetColumn1();
						Vec3 bZ = boneDesc.m_DefaultB2W.GetColumn2();
						Vec3 pos = boneDesc.m_DefaultB2W.GetColumn3();

						tempTM.SetFromVectors(  Vec3( bX.x,-bX.z, bX.y ),
												Vec3(-bZ.x, bZ.z,-bZ.y ),
												Vec3( bY.x,-bY.z, bY.y ),
												Vec3( pos.x, -pos.z, pos.y ) );

						boneDesc.m_DefaultB2W = tempTM;

						boneDesc.m_DefaultW2B = boneDesc.m_DefaultB2W.GetInverted();
					}
				}

				int boneIndex = int(pSkinningInfo->m_arrBonesDesc.size());
				pSkinningInfo->m_arrBonesDesc.push_back(boneDesc);

				boneNameIndexMap.insert(std::make_pair(boneID, boneIndex));
			}
		}

		// Find all the root nodes for this skeleton and then add that hierarchy.
		std::vector<ColladaNode*> rootBoneNodes;
		FindRootBones(rootBoneNodes, nameNodeMap, controller);
		for (int rootBoneIndex = 0, rootBoneCount = int(rootBoneNodes.size()); rootBoneIndex < rootBoneCount; ++rootBoneIndex)
			AddBoneRecursive(rootBoneNodes[rootBoneIndex], -1, pSkinningInfo, controller, nameNodeMap, boneNameIndexMap);
	}

	return true;
}

bool ColladaScene::SetBoneParentFrames(CContentCGF* cgf, IColladaLoaderListener* pListener)
{
	bool success = true;

	CSkinningInfo* pSkinningInfo = cgf->GetSkinningInfo();

	// Loop through each bone and update its parent frame matrices.
	for (int entityIndex = 0, entityCount = int(pSkinningInfo->m_arrBoneEntities.size()); entityIndex < entityCount; ++entityIndex)
	{
		// Check whether the bone has a parent frame.
		int boneIndex = pSkinningInfo->m_arrBoneEntities[entityIndex].BoneID;
		string boneName = GetSafeBoneName(pSkinningInfo->m_arrBonesDesc[boneIndex].m_arrBoneName);
		std::transform(boneName.begin(), boneName.end(), boneName.begin(), std::tolower);
		std::map<string, Matrix33>::iterator framePos = this->physParentFrames.find(boneName);
		if (framePos != this->physParentFrames.end())
		{
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[0][0] = (*framePos).second.m00;
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[0][1] = (*framePos).second.m01;
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[0][2] = (*framePos).second.m02;
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[1][0] = (*framePos).second.m10;
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[1][1] = (*framePos).second.m11;
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[1][2] = (*framePos).second.m12;
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[2][0] = (*framePos).second.m20;
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[2][1] = (*framePos).second.m21;
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[2][2] = (*framePos).second.m22;
		}
		else
		{
			pSkinningInfo->m_arrBoneEntities[entityIndex].phys.framemtx[0][0] = 100.0f; // This seems to be a way of specifying an invalid matrix.
		}
	}

	return success;
}

bool ColladaScene::SetIKParams(CContentCGF* cgf, IColladaLoaderListener* pListener)
{
	bool success = true;

	CSkinningInfo* pSkinningInfo = cgf->GetSkinningInfo();

	// Loop through each bone and update its ik information.
	for (int entityIndex = 0, entityCount = int(pSkinningInfo->m_arrBoneEntities.size()); entityIndex < entityCount; ++entityIndex)
	{
		BONE_ENTITY& e = pSkinningInfo->m_arrBoneEntities[entityIndex];
		CryBoneDescData& desc = pSkinningInfo->m_arrBonesDesc[e.BoneID];

		// Set the default values.
		e.phys.flags = -1;
		e.phys.min[0] = -10000000000.0f; e.phys.min[1] = -10000000000.0f; e.phys.min[2] = -10000000000.0f;
		e.phys.max[0] = 10000000000.0f; e.phys.max[1] = 10000000000.0f; e.phys.max[2] = 10000000000.0f;
		e.phys.spring_angle[0] = 0.0f; e.phys.spring_angle[1] = 0.0f; e.phys.spring_angle[2] = 0.0f;
		e.phys.spring_tension[0] = 1.0f; e.phys.spring_tension[1] = 1.0f; e.phys.spring_tension[2] = 1.0f;
		e.phys.damping[0] = 1.0f; e.phys.damping[1] = 1.0f; e.phys.damping[2] = 1.0f;

		// Override the defaults with the values that were specified.
		string boneName = GetSafeBoneName(desc.m_arrBoneName);
		std::transform(boneName.begin(), boneName.end(), boneName.begin(), std::tolower);
		PhysBoneMap::iterator physBonePos = this->physBones.find(boneName);
		if (physBonePos != this->physBones.end())
		{
			e.phys.flags = all_angles_locked;

			// Override the limits with the values that were specified.
			PhysBone& physBone = (*physBonePos).second;
			for (PhysBone::AxisLimitLimitMap::iterator limitPos = physBone.limits.begin(), limitEnd = physBone.limits.end(); limitPos != limitEnd; ++limitPos)
			{
				PhysBone::Axis axis = (*limitPos).first.first;
				PhysBone::Limit limit = (*limitPos).first.second;
				float value = (*limitPos).second;
				switch (limit)
				{
				case PhysBone::LimitMin: e.phys.min[axis] = DEG2RAD(value); break;
				case PhysBone::LimitMax: e.phys.max[axis] = DEG2RAD(value); break;
				}
				e.phys.flags &= ~(angle0_locked << axis);
			}

			// Override the other parameters with the values that were specified.
			typedef std::map<PhysBone::Axis, float> PhysBone::*PhysBoneAxisMapPtr;
			static const PhysBoneAxisMapPtr maps[] = {&PhysBone::dampings, &PhysBone::springAngles, &PhysBone::springTensions};
			typedef float (CryBonePhysics_Comp::*DOFArrayPtr)[3];
			static const DOFArrayPtr arrays[] = {&CryBonePhysics_Comp::damping, &CryBonePhysics_Comp::spring_angle, &CryBonePhysics_Comp::spring_tension};
			for (int paramIndex = 0; paramIndex < 3; ++paramIndex)
			{
				std::map<PhysBone::Axis, float>& map = (physBone.*(maps[paramIndex]));
				float* array = (e.phys.*(arrays[paramIndex]));
				for (std::map<PhysBone::Axis, float>::iterator valuePos = map.begin(), valueEnd = map.end(); valuePos != valueEnd; ++valuePos)
				{
					PhysBone::Axis axis = (*valuePos).first;
					float value = (*valuePos).second;
					array[axis] = value;
				}
			}
		}
	}

	return success;
}

bool ColladaScene::AddCollisionMeshesCGF(CContentCGF* cgf, const ColladaAssetMetaData& metaData, const NameNodeMap& nodes, CGFMaterialMap& cgfMaterials, NodeMaterialMap& nodeMaterialMap, IColladaLoaderListener* pListener)
{
	bool success = true;

	float const scale = metaData.GetUnitSizeInMeters();

	CSkinningInfo* pSkinningInfo = cgf->GetSkinningInfo();

	// Create a mapping between the bone names and the original bone meshes.
	NameNodeMap nameNodeMap;
	for (NameNodeMap::const_iterator itNode = nodes.begin(); itNode != nodes.end(); ++itNode)
	{
		ColladaNode* node = (*itNode).second;
		string boneName = GetSafeBoneName(node->GetName()).c_str();
		std::transform(boneName.begin(), boneName.end(), boneName.begin(), std::tolower);
		if (!nameNodeMap.insert(std::make_pair(boneName, node)).second)
			ReportWarning(pListener, "Multiple bones with name \"%s\".", GetSafeBoneName(node->GetName()));
	}

	// Create a mapping between the bone ids and the physics geom.
	typedef std::map<int, int> BonePhysMap;
	BonePhysMap bonePhysMap;

	for (int i=0, count = int(pSkinningInfo->m_arrBonesDesc.size()); i < count; i++)
	{
		// Check whether there is a phys bone for this bone.
		string boneName = GetSafeBoneName(pSkinningInfo->m_arrBonesDesc[i].m_arrBoneName);
		std::transform(boneName.begin(), boneName.end(), boneName.begin(), std::tolower);
		if (this->physBones.find(boneName) != this->physBones.end())
		{
			NameNodeMap::iterator nameNodeMapPos = nameNodeMap.find(boneName);
			ColladaNode* node = (nameNodeMapPos != nameNodeMap.end() ? (*nameNodeMapPos).second : 0);
			ColladaGeometry* geometry = (node ? node->GetGeometry() : 0);

			//CGFSubMaterialMap& subMaterials;

			CGFSubMaterialMap& cgfSubMaterials = cgfMaterials[nodeMaterialMap[node]];
			if (CMesh* mesh = (geometry ? geometry->CreateMesh(cgfSubMaterials, scale, pListener, std::vector<int>(), node->GetName(), std::vector<SMeshBoneMapping>(), metaData.GetUpAxis()) : 0))
			{
				int physIndex = int(pSkinningInfo->m_arrPhyBoneMeshes.size());
				pSkinningInfo->m_arrPhyBoneMeshes.push_back();
				PhysicalProxy& proxy = pSkinningInfo->m_arrPhyBoneMeshes.back();
				proxy.m_arrPoints.resize(mesh->GetVertexCount());
				//std::copy_n(mesh->m_pPositions, mesh->GetVertexCount(), proxy.m_arrPoints.begin());
				for (size_t j =0, end = mesh->GetVertexCount(); j < end; ++j) {
					proxy.m_arrPoints[j] = mesh->m_pPositions[j];
				}
				proxy.m_arrMaterials.resize(mesh->GetFacesCount());
				proxy.m_arrIndices.resize(mesh->GetFacesCount() * 3);
				for (int faceIndex = 0, faceCount = mesh->GetFacesCount(); faceIndex < faceCount; ++faceIndex)
				{
					proxy.m_arrMaterials[faceIndex] = mesh->m_pFaces[faceIndex].nSubset;
					for (int vertex = 0; vertex < 3; ++vertex)
						proxy.m_arrIndices[faceIndex * 3 + vertex] = mesh->m_pFaces[faceIndex].v[vertex];
				}

				bonePhysMap.insert(std::make_pair(i, physIndex));

				delete mesh;
			}
		}
	}

	// Update the bone entities.
	for (int boneEntityIndex = 0, boneEntityCount = int(pSkinningInfo->m_arrBoneEntities.size()); boneEntityIndex < boneEntityCount; ++boneEntityIndex)
	{
		BonePhysMap::iterator bonePhysMapPos = bonePhysMap.find(pSkinningInfo->m_arrBoneEntities[boneEntityIndex].BoneID);
		int physIndex = (bonePhysMapPos != bonePhysMap.end() ? (*bonePhysMapPos).second : -1);
		pSkinningInfo->m_arrBoneEntities[boneEntityIndex].phys.nPhysGeom = physIndex;
	}

	return success;
}

bool ColladaScene::AddMorphTargetsCGF(CContentCGF* cgf, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener)
{
	float const scale = metaData.GetUnitSizeInMeters();

	for (int nodeIndex = 0, nodeCount = int(this->nodes.size()); nodeIndex < nodeCount; ++nodeIndex)
	{
		ColladaNode* node = this->nodes[nodeIndex];
		ColladaController* controller = (node ? node->GetController() : 0);

		// Create the base mesh. We should do this again, even though it was probably done when creating one of the nodes.
		// This is because the skinning info is actually pretty separate from node creation throughout most of this
		// compilation process, and we probably shouldn't tie them together here.
		ColladaGeometry* baseGeometry = (controller ? controller->GetGeometry() : 0);
		std::vector<Vec3> baseVertexPositions;
		bool readBaseVertexPositions = (baseGeometry ? baseGeometry->GetVertexPositions(baseVertexPositions, scale, pListener, metaData.GetUpAxis()) : false);

		// Create the base mesh - we only need to do this again to get the remapping table.
		std::vector<int> meshToVertexListIndexMapping;
		if (baseGeometry)
		{
			CMesh* mesh = baseGeometry->CreateMesh(CGFSubMaterialMap(), scale, pListener, meshToVertexListIndexMapping, "morph target", std::vector<SMeshBoneMapping>(), metaData.GetUpAxis());
			if (mesh)
				delete mesh;
		}
		int meshVertexCount = int(meshToVertexListIndexMapping.size());

		int morphCount = (controller ? controller->GetMorphTargetCount() : 0);
		if (morphCount && meshVertexCount == 0)
		{
			ReportWarning(pListener, "Morph target controller contains no base geometry - morphs cannot be created.");
			return false;
		}

		for (int morphIndex = 0; morphIndex < morphCount; ++morphIndex)
		{
			CSkinningInfo* skinningInfo = (cgf ? cgf->GetSkinningInfo() : 0);

			// Compile the mesh for the geometry - assume that it will have the same layout as the base mesh!
			// TODO: Check this in exporter.
			ColladaGeometry* morphGeometry = (controller ? controller->GetMorphTargetGeometry(morphIndex) : 0);
			std::vector<Vec3> morphVertexPositions;
			bool readMorphVertexPositions = (morphGeometry ? morphGeometry->GetVertexPositions(morphVertexPositions, scale, pListener, metaData.GetUpAxis()) : false);

			if (!readMorphVertexPositions)
			{
				ReportWarning(pListener, "Error reading vertex positions for morph \"%s\".", (controller ? controller->GetMorphTargetName(morphIndex) : "UNKNOWN"));
				continue;
			}
			if (readMorphVertexPositions && readBaseVertexPositions && baseVertexPositions.size() != morphVertexPositions.size())
			{
				ReportWarning(pListener, "Morph target \"%s\" has different topology to base mesh.", (controller ? controller->GetMorphTargetName(morphIndex) : "UNKNOWN"));
				continue;
			}

			MorphTargets* morph = new MorphTargets;

			morph->MeshID = -1; // TODO: Get the actual value!
			morph->m_strName = (controller ? controller->GetMorphTargetName(morphIndex) : 0);

			for (int vertexIndex = 0; vertexIndex < meshVertexCount; ++vertexIndex)
			{
				int originalIndex = meshToVertexListIndexMapping[vertexIndex];

				if (morphVertexPositions[originalIndex] != baseVertexPositions[originalIndex])
				{
					SMeshMorphTargetVertex v;
					v.nVertexId = vertexIndex;
					v.ptVertex = morphVertexPositions[originalIndex] * 100.0f; // CGF is in cm (in these particular bytes at least...)
					morph->m_arrIntMorph.push_back(v);
				}
			}

			if (morph && skinningInfo)
				skinningInfo->m_arrMorphTargets.push_back(morph);
		}
	}

	return true;
}

void ColladaScene::GetAnimClipList(AnimClipMap& animclips, std::vector<ColladaAnimClip*>& animClipList, IColladaLoaderListener* pListener)
{
	// list that anim clips where the scene equal with this scene
	for (AnimClipMap::iterator itAnimClip = animclips.begin(); itAnimClip != animclips.end(); ++itAnimClip)
	{
		ColladaAnimClip* colladaAnimClip = (*itAnimClip).second;
		if (colladaAnimClip->GetTargetScene() == this)
			animClipList.push_back(colladaAnimClip);
	}
}

float DotProd (const CryQuat& q, const CryQuat& p)
{
	return q.w*p.w + q.v*p.v;
}

CInternalSkinningInfo* ColladaScene::CreateControllerTCBSkinningInfo(CContentCGF* pCGF, ColladaAnimClip* pAnimClip, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener)
{
	if (!pCGF || !pAnimClip)
	{
		return NULL;
	}

	CInternalSkinningInfo* pCtrlSkinningInfo = new CInternalSkinningInfo;

	float fps = 30.0f;

	float startTime = pAnimClip->GetStartTime();
	float endTime = pAnimClip->GetEndTime();
	pCtrlSkinningInfo->m_nStart = (int)(startTime * fps + 0.5f);
	pCtrlSkinningInfo->m_nEnd = (int)(endTime * fps + 0.5f);
	pCtrlSkinningInfo->m_nTicksPerFrame = TICKS_CONVERT;
	pCtrlSkinningInfo->m_secsPerTick = 1.0f / fps / TICKS_CONVERT;

	pCtrlSkinningInfo->m_arrControllers.resize(nodes.size()*3); // position, rotation, scale per node

	for (int nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
	{
		ColladaNode *pCurrentNode = nodes[nodeIndex];
		std::vector<ColladaAnimation *> currentNodeAnims;

		for (int i = 0; i < pAnimClip->GetNumAnim(); i++)
		{
			ColladaAnimation *pCurrentAnimation = pAnimClip->GetAnim(i);
			ColladaNode *pTargetNode = pCurrentAnimation->GetTargetNode();

			if (pTargetNode == pCurrentNode)
			{
				currentNodeAnims.push_back(pCurrentAnimation);
			}
		}

		if (currentNodeAnims.empty())
		{
			continue;
		}

		int numPosKeys = -1, numRotKeys = -1, numSclKeys = -1;

		for (int i = 0; i < currentNodeAnims.size(); i++)
		{
			ColladaAnimation *pCurrentAnimation = currentNodeAnims[i];
			ColladaTransformType transformType = pCurrentAnimation->GetTargetTransform();
			string componentType = pCurrentAnimation->GetTargetComponent();

			bool validAnimation = true;

			switch (transformType)
			{
				case TRANS_TRANSLATION:
					if (numPosKeys == -1)
						numPosKeys = pCurrentAnimation->GetKeys();
					else if (numPosKeys != pCurrentAnimation->GetKeys())
						validAnimation = false;

					if (componentType != "x" && componentType != "y" && componentType != "z")
						validAnimation = false;
					break;

				case TRANS_ROTATION_X:
				case TRANS_ROTATION_Y:
				case TRANS_ROTATION_Z:
					if (numRotKeys == -1)
						numRotKeys = pCurrentAnimation->GetKeys();
					else if (numRotKeys != pCurrentAnimation->GetKeys())
						validAnimation = false;

					if (componentType != "angle")
						validAnimation = false;
					break;

				case TRANS_SCALE:
					if (numSclKeys == -1)
						numSclKeys = pCurrentAnimation->GetKeys();
					else if (numSclKeys != pCurrentAnimation->GetKeys())
						validAnimation = false;
					
					if (componentType != "x" && componentType != "y" && componentType != "z")
						validAnimation = false;
					break;
			}

			if (validAnimation == false)
			{
				ReportError(pListener,"Number of keys consistency error in animated node '%s'",currentNodeAnims[i]->GetTargetNode()->GetID().c_str());
				return NULL;
			}
		}

		DynArray<CryTCB3Key> *pPositionTrack = NULL;
		DynArray<CryTCBQKey> *pRotationTrack = NULL;
		DynArray<CryTCB3Key> *pScaleTrack = NULL;

		int time;
		float tension, continuity, bias;
		float easeIn, easeOut;
		
		if (numPosKeys > 0)
		{
			pPositionTrack = new DynArray<CryTCB3Key>;
			pPositionTrack->resize(numPosKeys);

			int numTracks = pCtrlSkinningInfo->m_TrackVec3.size();
			pCtrlSkinningInfo->m_TrackVec3.push_back(pPositionTrack);
			pCtrlSkinningInfo->m_arrControllers[nodeIndex*3].type = 0x55;
			pCtrlSkinningInfo->m_arrControllers[nodeIndex*3].index = numTracks;

			for (int i = 0; i < numPosKeys; i++)
			{
				bool posx = false;
				bool posy = false;
				bool posz = false;

				Vec3 pos;

				for (int j = 0; j < currentNodeAnims.size(); j++)
				{
					ColladaAnimation *pCurrentAnimation = currentNodeAnims[j];
					ColladaTransformType transformType = pCurrentAnimation->GetTargetTransform();
					string componentType = pCurrentAnimation->GetTargetComponent();

					if (transformType == TRANS_TRANSLATION)
					{
						ColladaKey key;
						pCurrentAnimation->GetKey(i,key);

						time = (int)(key.time * fps * TICKS_CONVERT);
						tension = key.tcb.x;
						continuity = key.tcb.y;
						bias = key.tcb.z;
						easeIn = key.easeInOut.x;
						easeOut = key.easeInOut.y;
						if (componentType == "x")
						{
							pos.x = key.value;
							posx = true;
						}
						else if (componentType == "y")
						{
							pos.y = key.value;
							posy = true;
						}
						else if (componentType == "z")
						{
							pos.z = key.value;
							posz = true;
						}
					}
				}

				if (!posx)
				{
					pos.x = pCurrentNode->GetTranslate().x;
				}
				if (!posy)
				{
					pos.y = pCurrentNode->GetTranslate().y;
				}
				if (!posz)
				{
					pos.z = pCurrentNode->GetTranslate().z;
				}
				if( metaData.GetUpAxis() == ColladaAssetMetaData::eUA_Y )
				{
					pos.Set( pos.x, -pos.z, pos.y );
				}

				(*pPositionTrack)[i].time = time;
				(*pPositionTrack)[i].val = pos*100.0f;
				(*pPositionTrack)[i].t = tension;
				(*pPositionTrack)[i].c = continuity;
				(*pPositionTrack)[i].b = bias;
				(*pPositionTrack)[i].ein = easeIn;
				(*pPositionTrack)[i].eout = easeOut;
			}
		}

		if (numRotKeys > 0)
		{
			pRotationTrack = new DynArray<CryTCBQKey>;
			pRotationTrack->resize(numRotKeys);

			int numTracks = pCtrlSkinningInfo->m_TrackQuat.size();
			pCtrlSkinningInfo->m_TrackQuat.push_back(pRotationTrack);
			pCtrlSkinningInfo->m_arrControllers[nodeIndex*3+1].type = 0xaa;
			pCtrlSkinningInfo->m_arrControllers[nodeIndex*3+1].index = numTracks;

			CryQuat prevRotation(IDENTITY);

			for (int i = 0; i < numRotKeys; i++)
			{
				bool angx = false;
				bool angy = false;
				bool angz = false;

				Vec3 angles;

				for (int j = 0; j < currentNodeAnims.size(); j++)
				{
					ColladaAnimation *pCurrentAnimation = currentNodeAnims[j];
					ColladaTransformType transformType = pCurrentAnimation->GetTargetTransform();
					string componentType = pCurrentAnimation->GetTargetComponent();

					if (transformType != TRANS_ROTATION_X 
					&& transformType != TRANS_ROTATION_Y
					&& transformType != TRANS_ROTATION_Z)
						continue;

					ColladaKey key;
					pCurrentAnimation->GetKey(i,key);

					time = (int)(key.time * fps * TICKS_CONVERT);
					tension = key.tcb.x;
					continuity = key.tcb.y;
					bias = key.tcb.z;
					easeIn = key.easeInOut.x;
					easeOut = key.easeInOut.y;
					switch (transformType)
					{
					case TRANS_ROTATION_X:
						if (componentType == "angle")
						{
							angles.x = DEG2RAD(key.value);
							angx = true;
						}
						break;

					case TRANS_ROTATION_Y:
						if (componentType == "angle")
						{
							angles.y = DEG2RAD(key.value);
							angy = true;
						}
						break;

					case TRANS_ROTATION_Z:
						if (componentType == "angle")
						{
							angles.z = DEG2RAD(key.value);
							angz = true;
						}
						break;
					}
				}

				if (!angx)
				{
					angles.x = DEG2RAD(pCurrentNode->GetRotationX().w);
				}
				if (!angy)
				{
					angles.y = DEG2RAD(pCurrentNode->GetRotationY().w);
				}
				if (!angz)
				{
					angles.z = DEG2RAD(pCurrentNode->GetRotationZ().w);
				}

				CryQuat rotation(Ang3(angles.x,angles.y,angles.z));
				rotation = !rotation;

				if( metaData.GetUpAxis() == ColladaAssetMetaData::eUA_Y )
				{
					rotation.v.Set( rotation.v.x, -rotation.v.z, rotation.v.y);
				}

				CryQuat relRotation;
				CryQuat rotationAxisAngle;

				relRotation = (!prevRotation) * rotation;
				prevRotation = rotation;

				f32 angle = acos_tpl(relRotation.w);
				f32 sinAngle = sin_tpl(angle);

				if (fabs(sinAngle) < 0.00001f)
				{
					rotationAxisAngle.v.x = 1.0f;
					rotationAxisAngle.v.y = 0.0f;
					rotationAxisAngle.v.z = 0.0f;
					rotationAxisAngle.w = 0.0f;
				}
				else
				{
					rotationAxisAngle.v.x = relRotation.v.x/sinAngle;
					rotationAxisAngle.v.y = relRotation.v.y/sinAngle;
					rotationAxisAngle.v.z = relRotation.v.z/sinAngle;
					rotationAxisAngle.w = 2.0f*angle;
				}

				rotationAxisAngle.v.Normalize();

				if (rotationAxisAngle.w > PI)
				{
					rotationAxisAngle.w -= 2.0f*PI;
				}

				(*pRotationTrack)[i].time = time;
				(*pRotationTrack)[i].val = rotationAxisAngle;
				(*pRotationTrack)[i].t = tension;
				(*pRotationTrack)[i].c = continuity;
				(*pRotationTrack)[i].b = bias;
				(*pRotationTrack)[i].ein = easeIn;
				(*pRotationTrack)[i].eout = easeOut;
			}
		}

		if (numSclKeys > 0)
		{
			pScaleTrack = new DynArray<CryTCB3Key>;
			pScaleTrack->resize(numSclKeys);

			int numTracks = pCtrlSkinningInfo->m_TrackVec3.size();
			pCtrlSkinningInfo->m_TrackVec3.push_back(pScaleTrack);
			pCtrlSkinningInfo->m_arrControllers[nodeIndex*3+2].type = 0x55;
			pCtrlSkinningInfo->m_arrControllers[nodeIndex*3+2].index = numTracks;

			for (int i = 0; i < numSclKeys; i++)
			{
				bool sclx = false;
				bool scly = false;
				bool sclz = false;

				Vec3 scale;

				for (int j = 0; j < currentNodeAnims.size(); j++)
				{
					ColladaAnimation *pCurrentAnimation = currentNodeAnims[j];
					ColladaTransformType transformType = pCurrentAnimation->GetTargetTransform();
					string componentType = pCurrentAnimation->GetTargetComponent();

					if (transformType == TRANS_SCALE)
					{
						ColladaKey key;
						pCurrentAnimation->GetKey(i,key);

						time = (int)(key.time * fps * TICKS_CONVERT);
						tension = key.tcb.x;
						continuity = key.tcb.y;
						bias = key.tcb.z;
						easeIn = key.easeInOut.x;
						easeOut = key.easeInOut.y;
						if (componentType == "x")
						{
							scale.x = key.value;
							sclx = true;
						}
						else if (componentType == "y")
						{
							scale.y = key.value;
							scly = true;
						}
						else if (componentType == "z")
						{
							scale.z = key.value;
							sclz = true;
						}
					}
				}

				if (!sclx)
				{
					scale.x = pCurrentNode->GetScale().x;
				}
				if (!scly)
				{
					scale.y = pCurrentNode->GetScale().y;
				}
				if (!sclz)
				{
					scale.z = pCurrentNode->GetScale().z;
				}
				
				(*pScaleTrack)[i].time = time;
				(*pScaleTrack)[i].val = scale;
				(*pScaleTrack)[i].t = tension;
				(*pScaleTrack)[i].c = continuity;
				(*pScaleTrack)[i].b = bias;
				(*pScaleTrack)[i].ein = easeIn;
				(*pScaleTrack)[i].eout = easeOut;
			}
		}
	}

	return pCtrlSkinningInfo;
}

CInternalSkinningInfo* ColladaScene::CreateControllerSkinningInfo(CContentCGF* cgf, ColladaAnimClip* animClip, const NameNodeMap& nodes, const ColladaAssetMetaData& metaData, IColladaLoaderListener* pListener)
{
	if (!cgf)
		return NULL;

	CInternalSkinningInfo* pCtrlSkinningInfo = new CInternalSkinningInfo;

	float fps = 30.0f;		//TODO: set it properly

	float startTime = animClip->GetStartTime();
	float endTime = animClip->GetEndTime();
	pCtrlSkinningInfo->m_nStart = (int)(startTime * fps + 0.5f);
	pCtrlSkinningInfo->m_nEnd = (int)(endTime * fps + 0.5f);
	pCtrlSkinningInfo->m_nTicksPerFrame = TICKS_CONVERT;
	pCtrlSkinningInfo->m_secsPerTick = 1.0f / fps / TICKS_CONVERT;

	// Find which animations belong to which node.
	typedef std::map<ColladaNode*, std::vector<ColladaAnimation*> > NodeAnimationMap;
	NodeAnimationMap nodeAnimationMap;
	for (int i = 0, count = animClip->GetNumAnim(); i < count; ++i)
	{
		ColladaAnimation* anim = animClip->GetAnim(i);
		ColladaNode* targetNode = anim->GetTargetNode();
		nodeAnimationMap[targetNode].push_back(anim);
	}

	// Loop through all the nodes in the scene.
	for (int nodeIndex = 0, nodeCount = int(this->nodes.size()); nodeIndex < nodeCount; ++nodeIndex)
	{
		// Get the skin data for this node, if it has any.
		ColladaNode* skinNode = this->nodes[nodeIndex];
		ColladaController* controller = (skinNode ? skinNode->GetController() : 0);

		// Loop through all the bones for this node. We also want to output controllers for the joint itself (since in some cases
		// there is no skinning info and the bones are output simply as nodes in the scene). To do this we begin at node -1, which
		// represents the node itself.
		for (int jointIndex = -1, jointCount = (controller ? controller->GetNumJoints() : 0); jointIndex < jointCount; ++jointIndex)
		{
			ColladaNode* jointNode;
			if (jointIndex == -1)
			{
				jointNode = skinNode;
			}
			else
			{
				const string& jointID = (controller ? controller->GetJointID(jointIndex) : "");
				NameNodeMap::const_iterator jointNodePos = nodes.find(jointID);
				jointNode = (jointNodePos != nodes.end() ? (*jointNodePos).second : 0);
			}

			//TODO: ? wether the root joint's animation allowed?
			//if (node->IsRootOfJoints())
			//	continue;

			//if (!node->IsNodeOfSkeleton())
			//	continue;

			// collect all of the animation which belong to this node of skeleton
			NodeAnimationMap::iterator nodeAnimationPos = nodeAnimationMap.find(jointNode);
			if (nodeAnimationPos == nodeAnimationMap.end())
				continue;
			std::vector<ColladaAnimation*>& nodeAnims = (*nodeAnimationPos).second;

			CControllerPQLog *pController = new CControllerPQLog;

			pController->m_nControllerId = g_crcGen.GetCRC32(GetSafeBoneName(jointNode->GetName()));

			// check consistency of number of keys
			int numKeys = nodeAnims[0]->GetKeys();
			for (int i=0; i<nodeAnims.size(); i++)
			{
				if (nodeAnims[i]->GetKeys() != numKeys)
				{
					ReportError(pListener, "Number of keys consistency error in animated node '%s'", nodeAnims[i]->GetTargetNode()->GetID().c_str());
					return NULL;
				}
			}

			if (!numKeys)
				continue;

			// reserve the buffers
			pController->m_arrTimes.resize(numKeys);
			pController->m_arrKeys.resize(numKeys);

			// set times
			for (int i=0; i<numKeys; i++)
			{
				ColladaKey key;
				nodeAnims[0]->GetKey(i, key);

				pController->m_arrTimes[i] = (int)(key.time * fps * TICKS_CONVERT + 0.5f);
			}

			CryQuat qLast;
			qLast.SetIdentity();
			CryKeyPQLog keyLast;
			keyLast.nTime = 0;
			keyLast.vPos = Vec3(0,0,0);
			keyLast.vRotLog = Vec3(0,0,0);

			// create the controller keys
			bool missingComponent = false;
			bool scaleComponent = false;
			for (int keyIdx=0; keyIdx<numKeys; keyIdx++)
			{
				bool posx = false;
				bool posy = false;
				bool posz = false;
				bool angx = false;
				bool angy = false;
				bool angz = false;
				bool sclx = false;
				bool scly = false;
				bool sclz = false;

				Vec3 pos;
				Vec3 angle;

				for (int i=0; i<nodeAnims.size(); i++)
				{
					ColladaAnimation* anim = nodeAnims[i];
					ColladaKey key;
					anim->GetKey(keyIdx, key);
					ColladaTransformType transformType = anim->GetTargetTransform();
					string componentType = anim->GetTargetComponent();

					switch (transformType)
					{
						case TRANS_TRANSLATION:
							if (componentType == "x")
							{
								pos.x = key.value*100.0f; // TODO: The 100 factor was introduced in Budapest - is it necessary only for XSI? Breaks MB.
								posx = true;
							}
							else if (componentType == "y")
							{
								pos.y = key.value*100.0f;
								posy = true;
							}
							else if (componentType == "z")
							{
								pos.z = key.value*100.0f;
								posz = true;
							}
							break;

						case TRANS_ROTATION_X:
							if (componentType == "angle")
							{
								angle.x = DEG2RAD(key.value);
								angx = true;
							}
							break;

						case TRANS_ROTATION_Y:
							if (componentType == "angle")
							{
								angle.y = DEG2RAD(key.value);
								angy = true;
							}
							break;

						case TRANS_ROTATION_Z:
							if (componentType == "angle")
							{
								angle.z = DEG2RAD(key.value);
								angz = true;
							}
							break;

						case TRANS_SCALE:
							if (key.value != 1.0f)
							{
								if (componentType == "x")
									sclx = true;
								else if (componentType == "y")
									scly = true;
								else if (componentType == "z")
									sclz = true;
							}
							break;
					}
				}

				if (!posx) pos.x = jointNode->GetTranslate().x/**100.0f*/;
				if (!posy) pos.y = jointNode->GetTranslate().y/**100.0f*/;
				if (!posz) pos.z = jointNode->GetTranslate().z/**100.0f*/;

				if (!angx) angle.x = DEG2RAD(jointNode->GetRotationX().w);
				if (!angy) angle.y = DEG2RAD(jointNode->GetRotationY().w);
				if (!angz) angle.z = DEG2RAD(jointNode->GetRotationZ().w);

				if (!posx || !posy || !posz || !angx || !angy || !angz)
					missingComponent = true;

				if (sclx || scly || sclz)
					scaleComponent = true;

				CryQuat quat(Ang3(angle.x,angle.y,angle.z));

				quat = !quat;

				if( metaData.GetUpAxis() == ColladaAssetMetaData::eUA_Y )
				{
					quat.v.Set( quat.v.x, -quat.v.z, quat.v.y );
					pos.Set( pos.x, -pos.z, pos.y );
				}

				if (DotProd(qLast,quat) >= 0)
					qLast = quat;
				else
					qLast = -quat;

				CryKeyPQLog key;
				
				//key.nTime <- this is set already
				key.vPos = pos;
				key.vRotLog = log (qLast);

				AdjustRotLog (key.vRotLog, keyLast.vRotLog);

				keyLast = key;

				// add the key
				PQLog q;
				q.vPos = key.vPos;
				q.vRotLog = key.vRotLog;
				pController->m_arrKeys[keyIdx] = q;
			}

			/*
			if (missingComponent)
				ReportWarning(pListener, "Missing anim component in clip '%s' node '%s'", animClip->GetAnimClipID().c_str(), node->GetName().c_str());
				*/
			if (scaleComponent)
				ReportWarning(pListener, "Found scale component in clip '%s' node '%s'", animClip->GetAnimClipID().c_str(), jointNode->GetID().c_str());

			pCtrlSkinningInfo->m_pControllers.push_back(pController);
		}
	}

	return pCtrlSkinningInfo;
}

void ColladaScene::CreateMaterial(CGFMaterialMap& cgfMaterials, const NameNodeMap& nameNodeMap, NodeMaterialMap& nodeMaterialMap, IColladaLoaderListener* pListener)
{
	// We need to select the name of the parent material (ie the name of the *.MTL file). The convention is to take this name from
	// the name of the material library that is used. The material library name is given as the first part of the material name when
	// exported from DCC tool (ie a name is of the form <library>-<id>-<name>). It might be possible to have recursive libraries inside
	// the main one - for now we will use the name of the top-level library. It is possible for one object to contain materials from
	// different libraries - this is an error. We should also check whether the library name is DefaultLib - this is the name given
	// to the default library by XSI, and we should warn the user that he probably hasn't set the material library up properly.
	// (Actually, much better idea is to disallow exporting if library name is DefaultLib).
	typedef std::vector<ColladaMaterial*> MaterialList;
	typedef std::map<string, MaterialList> LibraryMap;
	LibraryMap libraries;
	typedef std::map<ColladaNode*, string> NodeLibraryNameMap;
	NodeLibraryNameMap nodeLibraryNameMap;
	std::set<string> addedMaterials;
	std::vector<ColladaNode*> nodesToAdd;
	for (int nodeIndex = 0, nodeCount = int(this->nodes.size()); nodeIndex < nodeCount; ++nodeIndex)
	{
		nodesToAdd.resize(0);

		// Add the materials belonging to this node, and also any materials belonging to any skeleton nodes.
		nodesToAdd.push_back(this->nodes[nodeIndex]);
		ColladaController* controller = (this->nodes[nodeIndex] ? this->nodes[nodeIndex]->GetController() : 0);
		for (int jointIndex = 0, jointCount = (controller ? controller->GetNumJoints() : 0); jointIndex < jointCount; ++jointIndex)
		{
			string jointID = controller->GetJointID(jointIndex);

			NameNodeMap::const_iterator jointNodePos = nameNodeMap.find(jointID);
			ColladaNode* jointNode = (jointNodePos != nameNodeMap.end() ? (*jointNodePos).second : 0);
			if (jointNode != 0)
				nodesToAdd.push_back(jointNode);
		}

		for (int nodeToAddIndex = 0, nodeToAddCount = int(nodesToAdd.size()); nodeToAddIndex < nodeToAddCount; ++nodeToAddIndex)
		{
			ColladaNode* node = nodesToAdd[nodeToAddIndex];
			std::vector<ColladaMaterial*> node_materials;
			node->GetSubMaterials(node_materials);
			for (int j=0;j<node_materials.size();j++)
			{
				string library = node_materials[j]->GetLibrary();
				library = (!library.empty() ? library : "library");

				NodeLibraryNameMap::const_iterator nodeLibraryNamePos = nodeLibraryNameMap.find(node);
				if (nodeLibraryNamePos != nodeLibraryNameMap.end() && library != (*nodeLibraryNamePos).second)
					ReportWarning(pListener, "Node \"%s\" refers to multiple material libraries. Selecting first one.", node->GetName().c_str());
				nodeLibraryNameMap.insert(std::make_pair(node, library));

				if (addedMaterials.insert(node_materials[j]->GetFullName()).second)
				{
					std::vector<ColladaMaterial*>& mtls = libraries[library];
					mtls.push_back(node_materials[j]);
				}
			}
		}
	}

	typedef std::map<string, CMaterialCGF*> NameMaterialMap;
	NameMaterialMap nameMaterialMap;
	for (LibraryMap::const_iterator libraryPos = libraries.begin(), libraryEnd = libraries.end(); libraryPos != libraryEnd; ++libraryPos)
	{
		string libraryName = (*libraryPos).first;
		const MaterialList& subMaterials = (*libraryPos).second;

		// Create a multi-material to hold the sub-materials.
		CMaterialCGF* material = new CMaterialCGF();
		material->nFlags = MTL_NAME_CHUNK_DESC_EXPORTER::FLAG_MULTI_MATERIAL;

		strncpy(material->name,libraryName,sizeof(material->name));
		material->name[sizeof(material->name)-1] = 0;

		material->nPhysicalizeType = PHYS_GEOM_TYPE_NONE;		// physicaliye type of material library (not used?)
		nameMaterialMap.insert(std::make_pair(material->name, material));

		// There is a naming convention for materials that specifies the material ID that the material should
		// receive in the engine. Names should be of the form <library>-<id>-<name>. To cater for materials that are
		// not of this form, we shall perform two passes over the material list. In the first pass, any materials
		// that fit the convention shall be assigned the materials they specify. In the second pass, any other
		// materials shall be given available ids.

		class MaterialEntry
		{
		public:
			MaterialEntry(int id, string name, const ColladaMaterial* colladaMaterial, const CCustomMaterialSettings& settings) : id(id), name(name), colladaMaterial(colladaMaterial), settings(settings) {}

			int id;
			string name;
			const ColladaMaterial* colladaMaterial;
			CCustomMaterialSettings settings;
		};

		typedef std::map<const ColladaMaterial*, MaterialEntry> MaterialCreationMap;
		MaterialCreationMap materialCreationMap;
		std::set<int> assignedIDs;

		// Pass 1.
		for (MaterialList::const_iterator itMaterial = subMaterials.begin(); itMaterial != subMaterials.end(); ++itMaterial)
		{
			const ColladaMaterial* colladaMaterial = (*itMaterial);

			// Check whether the name fits the naming convention.
			int id;
			string name;
			CCustomMaterialSettings settings;
			if (SplitMaterialName(colladaMaterial->GetFullName(), id, name, settings))
			{
				// Check whether the requested ID has already been assigned.
				if (assignedIDs.find(id) == assignedIDs.end())
				{
					materialCreationMap.insert(std::make_pair(colladaMaterial, MaterialEntry(id, name, colladaMaterial, settings)));
					assignedIDs.insert(id);
				}
				else
				{
					ReportWarning(pListener, "Material '%s' specifies duplicate ID - auto-assigning ID.", colladaMaterial->GetFullName().c_str());
				}
			}
			else
			{
				ReportWarning(pListener, "Material '%s' does not fit naming convention 'Library-ID-Name' - auto-assigning ID.", colladaMaterial->GetFullName().c_str());
			}
		}

		// Pass 2. Start assigning ids starting from 1, since material ids in the collada file are 1 based.
		int nextID = 1;
		for (MaterialList::const_iterator itMaterial = subMaterials.begin(); itMaterial != subMaterials.end(); ++itMaterial)
		{
			// Get the material.
			const ColladaMaterial* colladaMaterial = (*itMaterial);

			// Check whether the material has already been assigned an ID.
			if (materialCreationMap.find(colladaMaterial) == materialCreationMap.end())
			{
				// The material has not been assigned an ID - do so now. Choose the ID to assign.
				while (assignedIDs.find(nextID) != assignedIDs.end())
					++nextID;

				// Extract the material name.
				int id;
				string name;
				CCustomMaterialSettings settings;
				SplitMaterialName(colladaMaterial->GetFullName(), id, name, settings);

				// Assign the ID.
				id = nextID++;
				ReportInfo(pListener, "Assigning ID %d to material '%s'", id, colladaMaterial->GetFullName().c_str());
				materialCreationMap.insert(std::make_pair(colladaMaterial, MaterialEntry(id, name, colladaMaterial, settings)));
				assignedIDs.insert(id);
			}
		}

		// Subtract 1 from all IDs to convert from 1 based (UI) to 0 based (engine).
		for (MaterialCreationMap::iterator itMaterialCreationEntry = materialCreationMap.begin(); itMaterialCreationEntry != materialCreationMap.end(); ++itMaterialCreationEntry)
		{
			MaterialEntry& entry = (*itMaterialCreationEntry).second;
			entry.id -= 1;
		}

		// Create the entry in the map for this parent material.
		CGFSubMaterialMap& cgfSubMaterials = (*cgfMaterials.insert(std::make_pair(material, CGFSubMaterialMap())).first).second;

		// Now that all materials have been assigned IDs, create sub-materials for each of them and add them to the parent material.
		int maxMaterialID = 0;
		for (MaterialCreationMap::iterator itMaterialCreationEntry = materialCreationMap.begin(); itMaterialCreationEntry != materialCreationMap.end(); ++itMaterialCreationEntry)
		{
			MaterialEntry& entry = (*itMaterialCreationEntry).second;

			if (maxMaterialID < entry.id)
				maxMaterialID = entry.id;
		}
		material->subMaterials.resize(maxMaterialID + 1);
		std::fill(material->subMaterials.begin(), material->subMaterials.end(), (CMaterialCGF*)0);
		for (MaterialCreationMap::iterator itMaterialCreationEntry = materialCreationMap.begin(); itMaterialCreationEntry != materialCreationMap.end(); ++itMaterialCreationEntry)
		{
			MaterialEntry& entry = (*itMaterialCreationEntry).second;

			// Create the material.
			CMaterialCGF* subMaterial = new CMaterialCGF;

			strncpy(subMaterial->name,entry.name.c_str(),sizeof(subMaterial->name));
			subMaterial->name[sizeof(subMaterial->name)-1] = 0;

			subMaterial->nFlags = MTL_NAME_CHUNK_DESC_EXPORTER::FLAG_SUB_MATERIAL;

			// Apply the custom material settings.
			const CCustomMaterialSettings& settings = entry.settings;

			if (settings.physicalizeName == "None")
			{
				subMaterial->nPhysicalizeType = PHYS_GEOM_TYPE_NONE;
			}
			else if (settings.physicalizeName == "Default")
			{
				subMaterial->nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT;
			}
			else if (settings.physicalizeName == "ProxyNoDraw")
			{
				subMaterial->nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT_PROXY;
			}
			else if (settings.physicalizeName == "NoCollide")
			{
				subMaterial->nPhysicalizeType = PHYS_GEOM_TYPE_NO_COLLIDE;
			}
			else if (settings.physicalizeName == "Obstruct")
			{
				subMaterial->nPhysicalizeType = PHYS_GEOM_TYPE_OBSTRUCT;
			}
			else
			{
				subMaterial->nPhysicalizeType = PHYS_GEOM_TYPE_NONE;
				ReportWarning(pListener, "Invalid physicalization type '%s' in material name '%s'", settings.physicalizeName.c_str(), entry.colladaMaterial->GetFullName().c_str());
			}

			// SH settings
            /*
			if (settings.sh)
				subMaterial->nFlags |= MTL_NAME_CHUNK_DESC::FLAG_SH_COEFFS;
			if (settings.shTwoSided)
				subMaterial->nFlags |= MTL_NAME_CHUNK_DESC::FLAG_SH_2SIDED;
			if (settings.shAmbient)
				subMaterial->nFlags |= MTL_NAME_CHUNK_DESC::FLAG_SH_AMBIENT;
			int shOpacity = settings.shOpacity;
			if (shOpacity < 0 || shOpacity > 100)
			{
				ReportWarning(pListener, "SH Opacity invalid (%d) in material name '%s' - must be 0-100.", settings.shOpacity, entry.colladaMaterial->GetFullName().c_str());
				if (shOpacity < 0)
					shOpacity = 0;
				if (shOpacity > 100)
					shOpacity = 100;
			}
			subMaterial->shOpacity = settings.shOpacity / 100.0f;
            */
			subMaterial->shOpacity = 0;

			// Add the sub material to its parent.
			material->subMaterials[entry.id] = subMaterial;

			// Register the ID of this material, so we can look it up later.
			cgfSubMaterials.insert(std::make_pair(entry.colladaMaterial, CGFMaterialMapEntry(entry.id, subMaterial)));
		}
	}

	for (NodeLibraryNameMap::const_iterator nodeMapPos = nodeLibraryNameMap.begin(), nodeMapEnd = nodeLibraryNameMap.end(); nodeMapPos != nodeMapEnd; ++nodeMapPos)
	{
		nodeMaterialMap.insert(std::make_pair((*nodeMapPos).first, nameMaterialMap[(*nodeMapPos).second]));
	}
}

ColladaImage::ColladaImage(const string& name)
{
	this->name = name;
}

void ColladaImage::SetFileName(const string& name)
{
	this->filename = name;

	size_t pos = this->filename.find("/Game/");
	if (pos != string::npos)
	{
		this->rel_filename = this->filename.substr(pos+6,string::npos);
	}
	else
	{
		this->rel_filename = this->filename;
	}

}

ColladaEffect::ColladaEffect(const string& name)
{
	this->name = name;

	ambient.r = 0.0f; ambient.g = 0.0f; ambient.b = 0.0f; ambient.a = 1.0f;
	diffuse.r = 1.0f; diffuse.g = 1.0f; diffuse.b = 1.0f; diffuse.a = 0.0f; 
	emission.r = 0.0f; emission.g = 0.0f; emission.b = 0.0f; emission.a = 1.0f;
	specular.r = 0.0f; specular.g = 0.0f; specular.b = 0.0f; specular.a = 1.0f;
	reflective.r = 0.0f; reflective.g = 0.0f; reflective.b = 0.0f; reflective.a = 1.0f;
	transparent.r = 1.0f; transparent.g = 1.0f; transparent.b = 1.0f; transparent.a = 0.0f;

	shininess = 0.0f;
	reflectivity = 0.0f;
	transparency = 1.0f;
	
	diffuseImage = NULL;
	specularImage = NULL;
	normalImage = NULL;
}

ColladaMaterial::ColladaMaterial(const string& name)
{
	this->fullname = name;

	effect = NULL;

	// "Library__ID__Material__ParamOne__ParamTwo__Etc"
	std::vector<string> parts;
	SplitString(name,"__",parts);
	if (parts.size() >= 3)
	{
		library = parts[0];
		matname = parts[2];
	}
	else
	{
		library = "library";
		matname = name;
	}
}
