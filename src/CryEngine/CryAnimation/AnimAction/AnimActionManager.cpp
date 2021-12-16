#include "stdafx.h"
#include "AnimActionManager.h"
#include "AARandom.h"
#include "AASequence.h"
#include "AALocomotion.h"
#include "AnimActionActor.h"

// TEMP: From CryAnimation
#define AnimWarning(...)

CAnimActionManager g_AnimActionManager; 

CAnimActionManager::CAnimActionManager()
{
	RegisterAAClass(new AAClass_CLocomotion());
	RegisterAAClass(new AAClass_CSequence());
	RegisterAAClass(new AAClass_CRandom());
}

bool CAnimActionManager::RegisterAAClass(IAnimActionClass* pClass)
{
	assert (pClass != NULL);

	// check if we already have a class with the same name registered
	if (GetAAClassByName(pClass->GetAAClassName()))
	{
		// TODO: raise a more 'serious' error at this point
		return false;
	}

	int newIndex = m_AAClasses.size();
	m_AAClasses.push_back(pClass);
	m_classToIndex[pClass->GetAAClassName()] = newIndex;
	return true;
}

size_t CAnimActionManager::GetAAClassCount() const
{
	return m_AAClasses.size();
}

IAnimActionClass* CAnimActionManager::GetAAClass(size_t index)
{
	assert(index < m_AAClasses.size());

	if (index < m_AAClasses.size())
		return m_AAClasses[index];
	else
		return NULL;
}

IAnimActionClass* CAnimActionManager::GetAAClassByName(const char* name)
{
	TClassNameToIndexMap::const_iterator i = m_classToIndex.find(name);
	
	if (i == m_classToIndex.end())
		return NULL;

	size_t index = i->second;

	uint32 numClasses = m_AAClasses.size();
	assert(index<numClasses);

	return m_AAClasses[index];
}

int32 CAnimActionManager::GetAAClassIndexByName(const char* name)
{
	TClassNameToIndexMap::const_iterator i = m_classToIndex.find(name);

	if (i == m_classToIndex.end())
		return -1;

	size_t index = i->second;

	uint32 numClasses = m_AAClasses.size();
	assert(index<numClasses);

	return index;
}

IAnimActionClass* CAnimActionManager::CreatePrototypeFromXml( const char *sActionName,XmlNodeRef &root )
{
	uint32 IsAnimAction = root->isTag("AnimAction");
	if (IsAnimAction==0)
	{
		AnimWarning( "AnimAction XML tag not found" );
		return NULL;
	}

	const char * strActionType = root->getAttr( "class" );
	IAnimActionClass* pClass = GetAAClassByName( strActionType );
	if ( pClass == 0 )
	{
		AnimWarning( "Undefined AnimAction class \"%s\"", strActionType );
		return NULL;
	}

	IAnimActionClass* pPrototype = pClass->CreatePrototype();
	pPrototype->SetActionName( sActionName );

	size_t numProps = 0;
	const SAnimActionPropertyDesc* descriptor = pPrototype->GetPropertiesDesc();
	
	LoadAnimActionPropertiesFromXML( root, descriptor, pPrototype->GetProperties() );

	// now process all child entries
	int childCount = root->getChildCount();
	for ( int i=0; i<childCount; i++ )
	{
		XmlNodeRef childNode = root->getChild(i);
		if (childNode->isTag("Param"))
			continue;

		SAnimActionAnimation childAnimation;
		childAnimation.animation = childNode->getAttr( "name" );

		const SAnimActionPropertyDesc* childDescriptor = pPrototype->GetChildPropertiesDesc();
		LoadAnimActionPropertiesFromXML( childNode, childDescriptor, childAnimation.properties );

		if ( childNode->isTag("AnimAction") )
		{
			stack_string sChildAnimActionName = sActionName;
			sChildAnimActionName += ":";
			sChildAnimActionName += childAnimation.animation.c_str();
			childAnimation.animation.clear();
			childAnimation.prototype = CreatePrototypeFromXml( sChildAnimActionName.c_str(),childNode );
		}
		else 
		{
			if ( childNode->isTag("Animation") == false )
			{
				assert( !"Unrecognized XML tag in a CAA file!" );
			}
		}

		childAnimation.fParam[0] = 1.0f;
		childAnimation.fParam[1] = 0.0f;
		childAnimation.fParam[2] = 0.0f;

		const char *strWeight = childNode->getAttr( "Weight" );
		if (*strWeight)
		{
			f32 weight = f32(atof(strWeight));
			childAnimation.fParam[0] = weight;
		}
		const char *strTransitionTime = childNode->getAttr( "TransitionTime" );
		if (*strTransitionTime)
		{
			f32 ttime = f32(atof(strTransitionTime));
			childAnimation.fParam[1] = ttime;
		}
		const char *strLookAt = childNode->getAttr( "Allow_LookAt" );
		if (*strLookAt)
		{
			f32 lookat = f32(atof(strLookAt));
			childAnimation.fParam[2] = lookat;
		}

		// anonymous child entries go at the end of the list of child animations
		DynArray<SAnimActionAnimation>& childAnimations = pPrototype->GetChildAnimations();
		childAnimations.push_back( childAnimation );
	}

	if ( pPrototype == NULL )
	{
		assert( !"Error loading AnimAction from XML!" );
		return NULL;
	}

	return pPrototype;
}

void CAnimActionManager::LoadAnimActionPropertiesFromXML( XmlNodeRef node, const SAnimActionPropertyDesc* descriptor, TAnimActionPropertyValues& properties )
{
	if ( descriptor != NULL )
	{
		const SAnimActionPropertyDesc* counter = descriptor;
		while ( counter->propertyType != eAAPT_null )
			++counter;
		properties.resize(int(counter-descriptor));

		TAnimActionPropertyValues::iterator it, itEnd = properties.end();
		for ( it = properties.begin(); it != itEnd; ++it, ++descriptor )
		{
			// find if the XML node has a parameter which matches our current property name
			int childCount = node->getChildCount();
			for ( int i = 0; i < childCount; ++i )
			{
				XmlNodeRef childNode = node->getChild(i);
				if (childNode->isTag("Param"))
				{
					const char* id = childNode->getAttr("id");
					if (id && !strcmp(id, descriptor->propertyName))
					{
						const char* value = childNode->getAttr("value");
						it->SetAsString( descriptor, value );
						break;
					}
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////
// finds the animation-asset by path-name. 
// Returns -1 if no animation was found.
// Returns the animation ID if it was found.
/////////////////////////////////////////////////////////////////////
int CAnimActionManager::GetGlobalIDbyFilePathAAC(const char * sAnimFileName)
{
	size_t id = m_AnimationMap.GetValue(sAnimFileName);
	if ( id != -1 && !stricmp(m_arrGlobalAAC[id].GetPathName(),sAnimFileName))
		return id;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////
// loads the animation with the specified name; if the animation is already loaded,
// then just returns its id
// The caller MUST TAKE CARE to bind the animation if it's already loaded before it has registered itself within this manager
// RETURNS:
//   The global animation id.
//   -1 if the animation couldn't be loaded 
// SIDE EFFECT NOTES:
//   This function does not put up a warning in the case the file was not found.
//   The caller must do so. But if the file was found and was corrupted, more detailed
//   error information (like where the error occured etc.) may be put into the log
int CAnimActionManager::CreateGlobalMotionHeaderAAC(const char* strFilePath)
{
	int nGlobalAnimId = GetGlobalIDbyFilePathAAC (strFilePath);
	bool bRecordExists = (nGlobalAnimId >= 0);
	if (bRecordExists && m_arrGlobalAAC[nGlobalAnimId].IsAssetLoaded())
	{
		// we have already such file loaded
		return nGlobalAnimId;
	}
	else
	{
		//selfValidate();
		if (bRecordExists==0)
		{
			// add a new animation structure that will hold the info about the new animation.
			// the new animation id is the index of this structure in the array
			m_arrGlobalAAC.push_back(GlobalHeaderAAC());

			nGlobalAnimId = (int)m_arrGlobalAAC.size() - 1;
			m_arrGlobalAAC[nGlobalAnimId].SetPathName(strFilePath);

			m_AnimationMap.InsertValue(m_arrGlobalAAC[nGlobalAnimId].GetCRC32(), nGlobalAnimId);
		}

		//selfValidate();
	}

	return nGlobalAnimId;
}

//----------------------------------------------------------------------------------------

void CAnimActionManager::CreateAAPrototype(const char* szFilePath, const char* szAnimName,uint32 nGlobalAnimID)
{
	m_arrGlobalAAC[nGlobalAnimID].AddRef();

	uint32 loaded=m_arrGlobalAAC[nGlobalAnimID].IsAssetCreated();//Loaded();
	if (loaded==0)
	{
		GlobalHeaderAAC& rGlobalHeaderAAC = m_arrGlobalAAC[nGlobalAnimID];
		rGlobalHeaderAAC.OnAssetAAC();

		XmlNodeRef root = gEnv->pSystem->LoadXmlFile( szFilePath );
		if (root)
			rGlobalHeaderAAC.m_pAnimActionPrototype = CreatePrototypeFromXml( szAnimName,root );

		if ( rGlobalHeaderAAC.m_pAnimActionPrototype != NULL )
		{
			rGlobalHeaderAAC.m_pAnimActionPrototype->SetFileName(szFilePath);

			rGlobalHeaderAAC.OnAssetCreated();
			rGlobalHeaderAAC.OnAssetLoaded();
			rGlobalHeaderAAC.OnAssetAACValid();
		}
		else
		{
		 rGlobalHeaderAAC.OnAssetNotFound();
		}
	}
}

//----------------------------------------------------------------------------------------

uint32 CAnimActionManager::GetFlagsAAC(uint32 nGlobalAnimID)
{
	// lets assume this is an AAC
	uint32 numGlobalAAC = m_arrGlobalAAC.size();
	if (nGlobalAnimID < numGlobalAAC)
	{
		GlobalHeaderAAC& rGlobalHeaderAAC = m_arrGlobalAAC[nGlobalAnimID];
		return rGlobalHeaderAAC.m_nFlags; //is an AAC
	}
	return 0;
}

