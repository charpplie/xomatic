#include "stdafx.h"
#include "AnimEventLoader.h"
#include "Model.h"
#include "CharacterManager.h"

// loads the data from the animation event database file (.animevent) - this is usually
// specified in the CAL file.
bool AnimEventLoader::loadAnimationEventDatabase( CCharacterModel* pModel, const char* pFileName )
{
	if (pFileName == 0 || pFileName[0] == 0)
		return false;

	// Parse the xml.
	XmlNodeRef root = g_pISystem->LoadXmlFile(pFileName);
	if (!root)
	{
		//AnimWarning("Animation Event Database (.animevent) file \"%s\" could not be read (associated with model \"%s\")", pFileName, pModel->GetFile() );
		return false;
	}

	// Load the events from the xml.
	uint32 numAnimations = root->getChildCount();
	for (uint32 nAnimationNode=0; nAnimationNode<numAnimations; ++nAnimationNode)
	{
		XmlNodeRef animationRoot = root->getChild(nAnimationNode);
		// Check whether this is an animation.
		if (string("animation") != animationRoot->getTag())
			continue;

		// Get the name of the animation.
		XmlString sName = animationRoot->getAttr("name");

		// Look up the animation id in the animation set.
		int nGlobalAnimID = g_AnimationManager.GetGlobalIDbyFilePath_CAF(sName.c_str());
		if (nGlobalAnimID>=0)
		{
			uint32 HowManyEvents = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_AnimEventsCAF.size();
			if (HowManyEvents==0)
			{
				// Loop through the events for this animation.
				uint32 numEvents = animationRoot->getChildCount();
				for (uint32 nEventNode = 0; nEventNode<numEvents; ++nEventNode)
				{
					XmlNodeRef eventNode = animationRoot->getChild(nEventNode);

					// Check whether this is an event.
					if (string("event") != eventNode->getTag())
						continue;

					// Read the attributes of the event.
					XmlString sEventName;
					if (!(sEventName = eventNode->getAttr("name")))
						continue;
					
					float fTime;
					if (!eventNode->getAttr("time", fTime))
						continue;
					XmlString sEventNameLow = sEventName;
					sEventNameLow.MakeLower();

					uint32 ind = -1;
					if(sEventNameLow == "segment1") // Motion segmentation event
						ind = 1;
					else if(sEventNameLow == "segment2") 
						ind = 2;
					else if(sEventNameLow == "segment3") 
						ind = 3;
					else if(sEventNameLow == "segment4")
					{
						ind = 4;
						assert( fabs(fTime - 1.0f) < 1e-5f ); // The time for the last segment should be 1.0
					}

					if(ind != -1)
					{
						GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
						rGAH.m_SegmentsTime[ind] = fTime;
						++ rGAH.m_Segments;
					}

					XmlString sParameter;
					if (!(sParameter = eventNode->getAttr("parameter")))
						continue;
					XmlString sBoneName;
					if (!(sBoneName = eventNode->getAttr("bone")))
						continue;
					Vec3 vOffset(0,0,0);
					eventNode->getAttr("offset", vOffset);
					Vec3 vDir(0,0,0);
					eventNode->getAttr("dir", vDir);

					AnimEvents aevents;
					aevents.m_time=fTime;
					aevents.m_strCustomParameter=sParameter;
					aevents.m_strEventName=sEventName;
					aevents.m_strBoneName=sBoneName;
					aevents.m_vOffset=vOffset;
					aevents.m_vDir=vDir;
					g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_AnimEventsCAF.push_back(aevents);
				}
			}
		}	

		//------------------------------------------------------------------------------------------------------
		// Look up the animation id in the animation set.
		nGlobalAnimID = g_AnimationManager.GetGlobalIDbyFilePath_LMG(sName.c_str());
		if (nGlobalAnimID>=0)
		{
			uint32 HowManyEvents = g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID].m_AnimEventsLMG.size();
			if (HowManyEvents==0)
			{
				// Loop through the events for this animation.
				uint32 numEvents = animationRoot->getChildCount();
				for (uint32 nEventNode = 0; nEventNode<numEvents; ++nEventNode)
				{
					XmlNodeRef eventNode = animationRoot->getChild(nEventNode);

					// Check whether this is an event.
					if (string("event") != eventNode->getTag())
						continue;

					// Read the attributes of the event.
					XmlString sEventName;
					if (!(sEventName = eventNode->getAttr("name")))
						continue;
					float fTime;
					if (!eventNode->getAttr("time", fTime))
						continue;
					XmlString sParameter;
					if (!(sParameter = eventNode->getAttr("parameter")))
						continue;
					XmlString sBoneName;
					if (!(sBoneName = eventNode->getAttr("bone")))
						continue;
					Vec3 vOffset(0,0,0);
					eventNode->getAttr("offset", vOffset);
					Vec3 vDir(0,0,0);
					eventNode->getAttr("dir", vDir);

					AnimEvents aevents;
					aevents.m_time=fTime;
					aevents.m_strCustomParameter=sParameter;
					aevents.m_strEventName=sEventName;
					aevents.m_strBoneName=sBoneName;
					aevents.m_vOffset=vOffset;
					aevents.m_vDir=vDir;
					g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID].m_AnimEventsLMG.push_back(aevents);
				}
			}
		}	

	}

	return true;
}
