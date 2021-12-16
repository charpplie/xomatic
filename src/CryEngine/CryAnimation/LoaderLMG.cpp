#include "stdafx.h"

#include "LoaderLMG.h"
#include "LMG.h"

const char* const TAG_LOCOMOTION_GROUP = "LocomotionGroup";

const char* const TAG_EXAMPLE_LIST = "ExampleList";

const char* const TAG_EXAMPLE = "Example";
const char* const ATTR_EXAMPLE_ANIMATION_NAME = "AName";
const char* const ATTR_EXAMPLE_POSITION = "Position";

const char* const TAG_BLEND_TYPE = "BlendType";
const char* const ATTR_BLEND_TYPE_TYPE = "Type";

const char* const TAG_CAPS = "Caps";
const char* const ATTR_CAPS_CODE = "Code";

const char* const TAG_MOTION_COMBINATION = "MotionCombination";
const char* const TAG_NEW_STYLE = "NewStyle";
const char* const ATTR_NEW_STYLE_STYLE = "Style";

const char* const TAG_JOINT_LIST = "JointList";
const char* const TAG_JOINT = "Joint";
const char* const ATTR_JOINT_NAME = "Name";


// TODO: Unify Lmg loading code path.
CLoaderLMG::CLoaderLMG( CAnimationSet* pAnimationSet )
: m_pAnimationSet( pAnimationSet )
{
}


CLoaderLMG::~CLoaderLMG()
{

}


LMGLoadResult CLoaderLMG::Load( const char* resourceName, const char* animationName, const char* xmlData )
{
	if ( m_pAnimationSet == NULL )
	{
		return LMG_LOAD_ERROR_ANIMATION_SET_NULL;
	}

	if ( resourceName == NULL )
	{
		return LMG_LOAD_ERROR_RESOURCE_NAME_NULL;
	}

	if ( animationName == NULL )
	{
		return LMG_LOAD_ERROR_ANIMATION_NAME_NULL;
	}

	if ( xmlData == NULL )
	{
		return LMG_LOAD_ERROR_XML_DATA_NULL;
	}

	int globalAnimationLmgId = g_AnimationManager.CreateGAH_LMG( resourceName );

	m_pAnimationSet->CreateOrSetAnimationLMG( animationName, globalAnimationLmgId );
	
	GlobalAnimationHeaderLMG& globalAnimHeader = g_AnimationManager.m_arrGlobalLMG[ globalAnimationLmgId ];
	globalAnimHeader.OnAssetLMG();
	globalAnimHeader.m_arrBSAnimations.clear();
	globalAnimHeader.m_strSpliceAnim.clear();
	globalAnimHeader.m_jointList.clear();

	LMGLoadResult result = Load( globalAnimHeader, xmlData );
	if ( result == LMG_LOAD_SUCCESS )
	{
		globalAnimHeader.OnAssetCreated();
		globalAnimHeader.OnAssetLoaded();
	}

	return result;
}


LMGLoadResult CLoaderLMG::Load( GlobalAnimationHeaderLMG& globalAnim, const char* xmlData )
{
	uint32 numAnims = globalAnim.m_arrBSAnimations.size();
	if ( numAnims != 0 )
	{
		return LMG_LOAD_ERROR_ANIMATION_ALREADY_LOADED;
	}

	const char* pathname = globalAnim.GetFilePath(); 

	g_pILog->LogToFile( "Loading LMG %s", pathname );
	g_pILog->UpdateLoadingScreen( 0 );

	XmlNodeRef xmlNode = g_pISystem->LoadXmlFromString( xmlData );

	LMGLoadResult result = ParseLocomotionGroupNode( globalAnim, xmlNode );

	return result;
}


LMGLoadResult CLoaderLMG::ParseLocomotionGroupNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode )
{
	if ( xmlNode == 0 )
	{
		g_pILog->LogError( "locomotion-group not found: %s", globalAnim.GetFilePath() );

		return LMG_LOAD_ERROR_XML_NOT_FOUND;
	}

	const char* xmlTag = xmlNode->getTag();

	bool isLocomotionGroupXml = ( stricmp( xmlTag, TAG_LOCOMOTION_GROUP ) == 0 );
	if ( ! isLocomotionGroupXml ) 
	{
		return LMG_LOAD_ERROR_XML_IS_NOT_LOCOMOTION_GROUP;
	}

	globalAnim.m_arrBSAnimations.reserve( MAX_LMG_ANIMS );

	uint32 childCount = xmlNode->getChildCount();
	for ( uint32 i = 0; i < childCount; i++ )
	{
		XmlNodeRef childNode = xmlNode->getChild( i );

		const char* childNodeName = childNode->getTag();

		LMGLoadResult result = LMG_LOAD_SUCCESS;

		bool isExampleListNode = ( stricmp( childNodeName, TAG_EXAMPLE_LIST ) == 0 );
		bool isBlendTypeNode = ( stricmp( childNodeName, TAG_BLEND_TYPE ) == 0 );
		bool isCapsNodeType = ( stricmp( childNodeName, TAG_CAPS ) == 0 );
		bool isMotionCombinationNodeType = ( stricmp( childNodeName, TAG_MOTION_COMBINATION ) == 0 );
		bool isJointListNode = ( stricmp( childNodeName, TAG_JOINT_LIST ) == 0 );

		if ( isExampleListNode ) 
		{
			result = ParseExampleListNode( globalAnim, childNode );
		}
		else if ( isBlendTypeNode )
		{
			result = ParseBlendTypeNode( globalAnim, childNode );
		}
		else if ( isCapsNodeType )
		{
			result = ParseCapsNode( globalAnim, childNode );
		}
		else if ( isMotionCombinationNodeType )
		{
			result = ParseMotionCombinationNode( globalAnim, childNode );
		}
		else if ( isJointListNode )
		{
			result = ParseJointListNode( globalAnim, childNode );
		}

		if ( result != LMG_LOAD_SUCCESS )
		{
			return result;
		}
	}

	return LMG_LOAD_SUCCESS;
}


LMGLoadResult CLoaderLMG::ParseExampleListNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode )
{
	uint32 childCount = xmlNode->getChildCount();
	for ( uint32 i = 0; i < childCount; i++ )
	{
		XmlNodeRef childNode = xmlNode->getChild( i );

		LMGLoadResult result = ParseExampleNode( globalAnim, childNode );

		if ( result != LMG_LOAD_SUCCESS )
		{
			return result;
		}
	}

	return LMG_LOAD_SUCCESS;
}


LMGLoadResult CLoaderLMG::ParseExampleNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode )
{
	const char* xmlNodeName = xmlNode->getTag();
	bool isExampleNode = ( stricmp( xmlNodeName, TAG_EXAMPLE ) == 0 );
	if ( ! isExampleNode )
	{
		return LMG_LOAD_ERROR_EXAMPLE_NODE_EXPECTED;
	}

	BSAnimationLMG bsAnimation;

	bsAnimation.m_strAnimName = xmlNode->getAttr( ATTR_EXAMPLE_ANIMATION_NAME );
	xmlNode->getAttr( ATTR_EXAMPLE_POSITION, bsAnimation.m_Position );

	globalAnim.m_arrBSAnimations.push_back( bsAnimation );

	return LMG_LOAD_SUCCESS;
}


LMGLoadResult CLoaderLMG::ParseBlendTypeNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode )
{
	bool hasType = xmlNode->haveAttr( ATTR_BLEND_TYPE_TYPE );
	if ( hasType )
	{
		const char* type = xmlNode->getAttr( ATTR_BLEND_TYPE_TYPE );
		uint32 blendCode = *( uint32* )( type );
		globalAnim.m_nBlendCodeLMG = blendCode;

		bool lmgCodeValid = LMG::IsValidBlendCode( blendCode );
		if ( ! lmgCodeValid )
		{
			return LMG_LOAD_ERROR_BLEND_CODE_NOT_VALID;
		}
	}

	return LMG_LOAD_SUCCESS;
}


LMGLoadResult CLoaderLMG::ParseCapsNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode )
{
	bool hasSelectionCapsCode = xmlNode->haveAttr( ATTR_CAPS_CODE );
	if ( hasSelectionCapsCode )
	{
		const char* selectionCapsCode = xmlNode->getAttr( ATTR_CAPS_CODE );
		globalAnim.m_nSelectionCapsCode = *( uint32* )( selectionCapsCode );
	}

	return LMG_LOAD_SUCCESS;
}


LMGLoadResult CLoaderLMG::ParseMotionCombinationNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode )
{
	uint32 childCount = xmlNode->getChildCount();
	globalAnim.m_strSpliceAnim.resize( childCount );

	for ( uint32 i = 0; i < childCount; i++ ) 
	{
		XmlNodeRef childNode = xmlNode->getChild( i );
		
		LMGLoadResult result = ParseNewStyleNode( globalAnim, i, childNode );
		if ( result != LMG_LOAD_SUCCESS )
		{
			return result;
		}
	}

	return LMG_LOAD_SUCCESS;
}


LMGLoadResult CLoaderLMG::ParseNewStyleNode( GlobalAnimationHeaderLMG& globalAnim, uint32 index, XmlNodeRef xmlNode )
{
	const char* childNodeName = xmlNode->getTag();
	bool isNewStyleNode = ( stricmp( childNodeName, TAG_NEW_STYLE ) == 0 );
	if ( ! isNewStyleNode )
	{
		return LMG_LOAD_ERROR_NEW_STYLE_NODE_EXPECTED;
	}

	globalAnim.m_strSpliceAnim[ index ] = xmlNode->getAttr( ATTR_NEW_STYLE_STYLE );

	return LMG_LOAD_SUCCESS;
}


LMGLoadResult CLoaderLMG::ParseJointListNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode )
{
	uint32 childCount = ( uint32 )( xmlNode->getChildCount() );
	globalAnim.m_jointList.resize( childCount );

	for ( uint32 i = 0; i < childCount; ++i )
	{
		XmlNodeRef childnode = xmlNode->getChild( i );

		LMGLoadResult result = ParseJointNode( globalAnim, i, childnode );
		if ( result != LMG_LOAD_SUCCESS )
		{
			return result;
		}
	}

	std::sort( globalAnim.m_jointList.begin(), globalAnim.m_jointList.end() );

	return LMG_LOAD_SUCCESS;
}


LMGLoadResult CLoaderLMG::ParseJointNode( GlobalAnimationHeaderLMG& globalAnim, uint32 index, XmlNodeRef xmlNode )
{
	const char* childNodeName = xmlNode->getTag();
	bool isJointNode = ( stricmp( childNodeName, TAG_JOINT ) == 0 );
	if ( ! isJointNode )
	{
		return LMG_LOAD_ERROR_JOINT_NODE_EXPECTED;
	}

	const char* name = xmlNode->getAttr( ATTR_JOINT_NAME );
	globalAnim.m_jointList[ index ] = g_pCrc32Gen->GetCRC32( name );

	return LMG_LOAD_SUCCESS;
}
