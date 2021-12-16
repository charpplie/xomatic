//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#include "stdafx.h"
#include "Utility.h"
#include "..\Xml\Xml.h"
//#include "CryShader\CfgFile.h"
//#include "PathUtil.h"
#include "ResourceCompilerHelper.h"
#include <sstream>

#define CRYENGINE_CONFIG_CLASS_ID Class_ID(0x37157994, 0x7b926c58)

void CSExportUtility::ConfigLoad(bool autosave)
{
	//////////////////////////////////////////////////////////////////////////
	// Load config .ini file.
	//////////////////////////////////////////////////////////////////////////

	std::string moduleName = "CryExport";
	CResourceCompilerHelper rch(moduleName.c_str());
	m_sEditorExe = rch.GetEditorExecutable().c_str();
}

//////////////////////////////////////////////////////////////////////////
static std::string HexEncode(const std::string& sString)
{
	std::string sEncoded;
	sEncoded.resize(sString.size() * 2);
	for (int i = 0; i < sString.size(); ++i)
	{
		int nHighNibble = sString[i] >> 4;
		int nLowNibble = sString[i] & 0xF;
		char cHighNibbleChar = ((nHighNibble < 10) ? nHighNibble + '0' : nHighNibble + 'A' - 10);
		char cLowNibbleChar = ((nLowNibble < 10) ? nLowNibble + '0' : nLowNibble + 'A' - 10);
		sEncoded[2 * i] = cHighNibbleChar;
		sEncoded[2 * i + 1] = cLowNibbleChar;
	}

	return sEncoded;
}

//////////////////////////////////////////////////////////////////////////
static std::string HexDecode(const std::string& sEncoded)
{
	// First we check whether the string is a valid hex encoding.
	if (sEncoded.size() & 1)
		return "<corrupt hex encoding>";
	for (int i = 0; i < sEncoded.size(); ++i)
	{
		if ((sEncoded[i] < '0' || sEncoded[i] > '9') && (sEncoded[i] < 'A' || sEncoded[i] > 'F'))
		{
			return "<corrupt hex encoding>";
		}
	}

	std::string sString;
	sString.resize(sEncoded.size() >> 1);
	for (int i = 0; i < sString.size(); ++i)
	{
		int nHighNibbleChar = sEncoded[2 * i];
		int nLowNibbleChar = sEncoded[2 * i + 1];
		int nHighNibble = ((nHighNibbleChar < 'A') ? nHighNibbleChar - '0' : nHighNibbleChar - 'A' + 10);
		int nLowNibble = ((nLowNibbleChar < 'A') ? nLowNibbleChar - '0' : nLowNibbleChar - 'A' + 10);
		sString[i] = (nHighNibble << 4) + nLowNibble;
	}

	return sString;
}

//////////////////////////////////////////////////////////////////////////
static void ParamBlockToXml( IParamBlock2 *pBlock,XmlNodeRef &node )
{
	for (int i = 0; i < pBlock->NumParams(); i++)
	{
		ParamID paramId = pBlock->IndextoID(i);
		ParamDef &paramDef = pBlock->GetParamDef(paramId);
		const char *name = NULL;
		if (paramDef.int_name)
			name = paramDef.int_name;
		if (name)
		{
			switch (paramDef.type)
			{
			case TYPE_BOOL:
				{
					bool bOn = pBlock->GetInt(paramId) != 0;
					node->setAttr(name,bOn);
				}
				break;
			case TYPE_INT:
			case TYPE_RGBA:
				node->setAttr(name,pBlock->GetInt(paramId));
				break;
			case TYPE_FLOAT:
				node->setAttr(name,pBlock->GetFloat(paramId));
				break;

			case TYPE_STRING:
			case TYPE_FILENAME:

				// Simply storing the string in the attribute would be dangerous, as the
				// string may contain illegal characters. Therefore we encode the string in
				// hex.
				const TCHAR* szString = pBlock->GetStr(paramId);
				if (szString != 0)
				{
					XmlString sEncoded = HexEncode(szString).c_str();
					node->setAttr(name, sEncoded);
				}

				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
static void XmlToParamBlock( IParamBlock2 *pBlock,XmlNodeRef &node )
{
	if (!node)
		return;
	bool bValue;
	int nValue;
	float fValue;
	XmlString sValue;
	for (int i = 0; i < pBlock->NumParams(); i++)
	{
		ParamID paramId = pBlock->IndextoID(i);
		ParamDef &paramDef = pBlock->GetParamDef(paramId);
		const char *name = NULL;
		if (paramDef.int_name)
			name = paramDef.int_name;
		if (name)
		{
			switch (paramDef.type)
			{
			case TYPE_BOOL:
				if (node->getAttr(name,bValue))
					pBlock->SetValue(paramId,0,(bValue)?TRUE:FALSE);
				break;
			case TYPE_INT:
			case TYPE_RGBA:
				if (node->getAttr(name,nValue))
					pBlock->SetValue(paramId,0,nValue);
				break;
			case TYPE_FLOAT:
				if (node->getAttr(name,fValue))
					pBlock->SetValue(paramId,0,fValue);
				break;

			case TYPE_STRING:
			case TYPE_FILENAME:

				// Simply storing the string in the attribute would be dangerous, as the
				// string may contain illegal characters. Therefore we encode the string in
				// hex.
				if (node->getAttr(name, sValue))
				{
					std::string sString = HexDecode(sValue);
					pBlock->SetValue(paramId, 0, const_cast<TCHAR*>(sString.c_str()));
				}

				break;
			}
		}
	}
}

static void NamedRangesToXML(std::vector<NamedRange>& ranges, XmlNodeRef& node)
{
	if (node == 0)
		return;

	// Loop through all the ranges, adding them to the xml tree.
	for (int nRange = 0; nRange < ranges.size(); ++nRange)
	{
		NamedRange& range = ranges[nRange];

		// Create a new node to represent this range.
		XmlNodeRef rangeNode = node->newChild("Range");

		// Store the attributes of the range in the node.
		rangeNode->setAttr("Start", range.nStart);
		rangeNode->setAttr("End", range.nEnd);
		XmlString sEncoded = HexEncode(range.sName).c_str();
		rangeNode->setAttr("Name", sEncoded.c_str());
	}
}

static void XmlToNamedRanges(std::vector<NamedRange>& ranges, XmlNodeRef& node)
{
	// Clear out the list of ranges.
	ranges.clear();

	if (node == 0)
		return;

	// Read all the node elements from the xml node.
	int nUnnamedCount = 0;
	for (int nNode = 0; nNode < node->getChildCount(); ++nNode)
	{
		XmlNodeRef rangeNode = node->getChild(nNode);
		if (stricmp("Range", rangeNode->getTag()) != 0)
			continue;

		// Read the attributes into the node.
		float fStart;
		if (!rangeNode->getAttr("Start", fStart))
			fStart = 0.0f;
		float fEnd;
		if (!rangeNode->getAttr("End", fEnd))
			fEnd = 0.0f;

		std::string sName;
		const char* szNameAttr = rangeNode->getAttr("Name");
		if (szNameAttr != 0)
		{
			sName = HexDecode(szNameAttr);
		}
		else
		{
			std::ostringstream name;
			name << "unnamed" << (++nUnnamedCount);
			sName = name.str();
		}

		// Add a new range to the list.
		ranges.resize(ranges.size() + 1);
		ranges.back().nStart = fStart;
		ranges.back().nEnd = fEnd;
		ranges.back().sName = sName.c_str();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSExportUtility::SaveConfigToMaxFile()
{
	XmlNodeRef root = new CXmlNode( "Config" );

	ParamBlockToXml( pb_nodes,root->newChild( "Nodes" ) );
	ParamBlockToXml( pb_bones,root->newChild( "Bones" ) );
	ParamBlockToXml( pb_options,root->newChild( "Options" ) );
	ParamBlockToXml( pb_timing,root->newChild( "Timing" ) );
	ParamBlockToXml( pb_physics,root->newChild( "Physics" ) );

	NamedRangesToXML(Ranges, root->newChild("NamedRanges"));

	XmlNodeRef nodes = root->newChild("NodeList");
	int i;
	for(i=0;i<pb_nodes->Count(PB_NODES);i++)
	{
		INode *node=pb_nodes->GetINode(PB_NODES,0,i);
		if (node)
			nodes->newChild("Node")->setAttr("Name",node->GetName());
	}

	XmlNodeRef bones = root->newChild("BoneList");
	for(i=0;i<pb_bones->Count(PB_BONES);i++)
	{
		INode *node=pb_bones->GetINode(PB_BONES,0,i);
		if (node)
			bones->newChild("Node")->setAttr("Name",node->GetName());
	}

	INode *pRootNode = ip->GetRootNode();
	if (pRootNode)
	{
		if (pRootNode->GetAppDataChunk(CRYENGINE_CONFIG_CLASS_ID,UTILITY_CLASS_ID,0))
			pRootNode->RemoveAppDataChunk( CRYENGINE_CONFIG_CLASS_ID,UTILITY_CLASS_ID,0 );
		XmlString xml = root->getXML();
		if (!xml.empty())
		{
			// [MichaelS 24/8/2006] It turns out that AddAppDataChunk simply stores the pointer you pass it - which
			// will then be free()ed later on - therefore it is vital that the chunk we pass in be allocated using
			// MAX_malloc()!
			CoreExport void *	(__cdecl *MAX_malloc)(size_t);
			char* data = (char*)MAX_malloc(xml.size() + 1);
			strcpy(data, xml.c_str());

			pRootNode->AddAppDataChunk( CRYENGINE_CONFIG_CLASS_ID,UTILITY_CLASS_ID,0,xml.length()+10,(void*)data );
			AppDataChunk *pAppDataChunk = pRootNode->GetAppDataChunk( CRYENGINE_CONFIG_CLASS_ID,UTILITY_CLASS_ID,0 );
			
			XmlParser xmlParser;
			XmlNodeRef root = xmlParser.parseBuffer( (const char*)pAppDataChunk->data );
			if (!root)
				return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSExportUtility::LoadConfigFromMaxFile()
{
	INode *pRootNode = ip->GetRootNode();
	if (!pRootNode)
		return;
	AppDataChunk *pAppDataChunk = pRootNode->GetAppDataChunk( CRYENGINE_CONFIG_CLASS_ID,UTILITY_CLASS_ID,0 );
	if (!pAppDataChunk)
		return;

	if (pAppDataChunk->length == 0)
		return;

	XmlParser xmlParser;
	XmlNodeRef root = xmlParser.parseBuffer( (const char*)pAppDataChunk->data );
	if (!root)
		return;

	XmlToParamBlock( pb_nodes,root->findChild("Nodes") );
	XmlToParamBlock( pb_bones,root->findChild( "Bones" ) );
	XmlToParamBlock( pb_options,root->findChild( "Options" ) );
	XmlToParamBlock( pb_timing,root->findChild( "Timing" ) );
	XmlToParamBlock( pb_physics,root->findChild( "Physics" ) );

	XmlToNamedRanges(Ranges, root->findChild("NamedRanges"));

	pb_nodes->ZeroCount(PB_NODES);
	pb_bones->ZeroCount(PB_BONES);

	XmlString nodeName;

	XmlNodeRef nodes = root->findChild("NodeList");
	int i;
	for(i=0;i<nodes->getChildCount();i++)
	{
		if (nodes->getChild(i)->getAttr("Name",nodeName))
		{
			INode *node = ip->GetINodeByName(nodeName.c_str());
			if (node)
				pb_nodes->Append(PB_NODES,1,&node);
		}
	}

	XmlNodeRef bones = root->findChild("BoneList");
	//save Node List
	for(i=0;i<bones->getChildCount();i++)
	{
		if (bones->getChild(i)->getAttr("Name",nodeName))
		{
			INode *node = ip->GetINodeByName(nodeName.c_str());
			AddBoneToList(node);
		}
	}

	if (hwnd_nodes)
	{
		int nCurSel = pb_nodes->GetInt(PB_EXPORT_TO);
		HWND hComboBox = GetDlgItem(hwnd_nodes,IDC_EXPORT_TO);
		if (hComboBox)
			SendMessage( hComboBox,CB_SETCURSEL,nCurSel,0 );
	}
}
