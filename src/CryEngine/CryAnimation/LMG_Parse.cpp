///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   LMG_Parse.cpp
//  Version:     v1.00
//  Created:     07/10/2009 by Jaewon Jung
//  Description: Locomotion Group utility functions
// -------------------------------------------------------------------------
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMG.h"
#include <map>
#include <algorithm>
#include <functional>

#include "crc32.h"

namespace LMG
{
	bool ParseXML(GlobalAnimationHeaderLMG& globalAnim)
	{
		uint32 numAnims = globalAnim.m_arrBSAnimations.size();
		if (numAnims)
			return true; //already loaded

		const char* pathname = globalAnim.GetFilePath(); 
		//g_pILog->LogToFile("Loading LMG %s", pathname);				// to file only so console is cleaner
		//g_pILog->UpdateLoadingScreen(0);
		XmlNodeRef root		= g_pISystem->LoadXmlFile(pathname);	
		if (root==0)
		{
			g_pILog->LogError ("locomotion-group not found: %s", pathname);
			return false;
		}

		const char* XMLTAG = root->getTag();

		if (strcmp(XMLTAG,"LocomotionGroup")==0) 
		{
			globalAnim.m_arrBSAnimations.reserve( MAX_LMG_ANIMS );

			uint32 numChilds = root->getChildCount();
			for (uint32 c=0; c<numChilds; c++)
			{
				XmlNodeRef nodeList = root->getChild(c);
				const char* ListTag = nodeList->getTag();

				//-----------------------------------------------------------
				//load example-list 
				//-----------------------------------------------------------
				if ( strcmp(ListTag,"ExampleList")==0 ) 
				{
					uint32 num = nodeList->getChildCount();
					for (uint32 i=0; i<num; i++) 
					{
						XmlNodeRef nodeExample = nodeList->getChild(i);
						const char* ExampleTag = nodeExample->getTag();
						if (strcmp(ExampleTag,"Example")==0) 
						{
							BSAnimationLMG bsa;
							bsa.m_strAnimName = nodeExample->getAttr( "AName" );
							nodeExample->getAttr( "Position",bsa.m_Position );
							globalAnim.m_arrBSAnimations.push_back(bsa);
						}
						else 
							return false;
					}
				}
				//-----------------------------------------------------------
				//---                load blending-type                   ---
				//-----------------------------------------------------------
			 else if (strcmp(ListTag,"BLENDTYPE")==0) 
				{
					uint32 sum = nodeList->haveAttr("type");
					assert(sum);
					if (sum)
					{
						const char *type = nodeList->getAttr("type");
						globalAnim.m_nBlendCodeLMG = *(uint32*)type;
					}
				}
				else if (strcmp(ListTag,"CAPS")==0) 
				{
					uint32 sum = nodeList->haveAttr("code");
					assert(sum);
					if (sum)
					{
						const char *code = nodeList->getAttr("code");
						globalAnim.m_nSelectionCapsCode = *(uint32*)code;
					}
				}
				//-----------------------------------------------------------
				//-- check of Motion-Combination examples 
				//-----------------------------------------------------------
				else if ( strcmp(ListTag,"MotionCombination")==0 ) 
				{
					uint32 num = nodeList->getChildCount();
					globalAnim.m_strSpliceAnim.resize(num);
					for (uint32 i=0; i<num; i++) 
					{
						XmlNodeRef nodeExample = nodeList->getChild(i);
						const char* ExampleTag = nodeExample->getTag();

						if (strcmp(ExampleTag,"NewStyle")==0) 
						{
							globalAnim.m_strSpliceAnim[i]=nodeExample->getAttr( "Style" );

							const char* test2 = globalAnim.m_strSpliceAnim[i];
						}
						else
						{
							assert(0); //illegal Tag found
							return 0;
						}
					}
				}
				//-----------------------------------------------------------
				//-- joint mask
				//-----------------------------------------------------------
				else if (strcmp(ListTag, "JointList") == 0)
				{
					uint32 num = nodeList->getChildCount();
					globalAnim.m_jointList.resize(num);
					for (uint32 i=0; i<num; ++i)
					{
						XmlNodeRef node = nodeList->getChild(i);
						const char* tag = node->getTag();
						if (strcmp(tag, "Joint") != 0)
						{
							assert(0);
							return 0;
						}

						const char* name = node->getAttr("Name");
						globalAnim.m_jointList[i] = g_pCrc32Gen->GetCRC32(name);
					}

					std::sort(globalAnim.m_jointList.begin(), globalAnim.m_jointList.end());
				}
			}
			assert(globalAnim.m_nBlendCodeLMG);
		}

		return true;
	}
} // namespace LMG
