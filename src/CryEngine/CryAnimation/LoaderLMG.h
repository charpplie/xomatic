#ifndef __LoaderLMG__h__
#define __LoaderLMG__h__
#pragma once

typedef enum 
{
	LMG_LOAD_SUCCESS = 0,

	LMG_LOAD_ERROR_ANIMATION_SET_NULL,
	LMG_LOAD_ERROR_RESOURCE_NAME_NULL,
	LMG_LOAD_ERROR_ANIMATION_NAME_NULL,
	LMG_LOAD_ERROR_XML_DATA_NULL,

	LMG_LOAD_ERROR_ANIMATION_ALREADY_LOADED,
	LMG_LOAD_ERROR_XML_NOT_FOUND,
	LMG_LOAD_ERROR_XML_IS_NOT_LOCOMOTION_GROUP,
	LMG_LOAD_ERROR_EXAMPLE_NODE_EXPECTED,
	LMG_LOAD_ERROR_NEW_STYLE_NODE_EXPECTED,

	LMG_LOAD_ERROR_JOINT_NODE_EXPECTED,
	LMG_LOAD_ERROR_JOINT_NAME_NULL,
	
	LMG_LOAD_ERROR_BLEND_CODE_NOT_VALID
} LMGLoadResult;

class CLoaderLMG
{
public:
	CLoaderLMG( CAnimationSet* pAnimationSet );
	virtual ~CLoaderLMG();

	LMGLoadResult Load( const char* resourceName, const char* animationName, const char* xmlData );

private:
	LMGLoadResult Load( GlobalAnimationHeaderLMG& globalAnim, const char* xmlData );

	LMGLoadResult ParseLocomotionGroupNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode );
	LMGLoadResult ParseExampleListNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode );
	LMGLoadResult ParseExampleNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode );
	LMGLoadResult ParseBlendTypeNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode );
	LMGLoadResult ParseCapsNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode );
	LMGLoadResult ParseMotionCombinationNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode );
	LMGLoadResult ParseNewStyleNode( GlobalAnimationHeaderLMG& globalAnim, uint32 index, XmlNodeRef xmlNode );
	LMGLoadResult ParseJointListNode( GlobalAnimationHeaderLMG& globalAnim, XmlNodeRef xmlNode );
	LMGLoadResult ParseJointNode( GlobalAnimationHeaderLMG& globalAnim, uint32 index, XmlNodeRef xmlNode );

	CAnimationSet* m_pAnimationSet;
};

#endif
