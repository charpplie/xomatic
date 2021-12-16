#include "BlendSpace.h"
#include "Serialization/StringList.h"
#include <ICryAnimation.h>

namespace CharacterTool
{

void BlendSpaceExample::Serialize(IArchive& ar)
{
	ar(AnimationAlias(animation), "animation", "<^");
	if (ar.IsEdit() && ar.IsOutput())
	{
		if (animation.empty())
		{
			ar.Warning(animation, "Example doesn't specify an animation name.");
		}
		else
		{
			if (ICharacterInstance* characterInstance = ar.FindContext<ICharacterInstance>())
			{
				IAnimationSet* animationSet = characterInstance->GetIAnimationSet();
				int animationId = animationSet->GetAnimIDByName(animation.c_str());
				if (animationId < 0)
				{
					ar.Error(animation, "Animation name is missing from the animation set.");
				}
				else
				{
					if (animationSet->IsAimPose(animationId, characterInstance->GetIDefaultSkeleton())|| 
						animationSet->IsLookPose(animationId, characterInstance->GetIDefaultSkeleton()))
					{
						ar.Error(animation, "BlendSpace example can not refer to Aim/LookPose");
					}
					else
					{
						int flags = animationSet->GetAnimationFlags(animationId);
						if (flags & CA_ASSET_LMG)
							ar.Error(animation, "BlendSpace example can not refer to another BlendSpace.");
					}
				}
			}
		}
	}

	BlendSpace* bspace = 0;
	if (ar.IsEdit())
		bspace = ar.FindContext<BlendSpace>();
	if (bspace && ar.IsEdit()) 
	{
		size_t numDimensions = bspace->m_dimensions.size();
		{
			ar(specified[0], "s0", ">^"); 
			ar(parameters.x, "p0", specified[0] ? ">^" : "!>^"); 
		}
		if (numDimensions > 1)
		{
			ar(specified[1], "s1", ">^"); 
			ar(parameters.y, "p1", specified[1] ? ">^" : "!>^"); 
		}
		if (numDimensions > 2)
		{
			ar(specified[2], "s2", ">^"); 
			ar(parameters.z, "p2", specified[2] ? ">^" : "!>^"); 
		}
		if (numDimensions > 3)
		{
			ar(specified[3], "s3", ">^"); 
			ar(parameters.w, "p3", specified[3] ? ">^" : "!>^"); 
		}
	}
	else
	{
		ar(parameters, "parameters");
		ar(specified, "specified");
	}
	ar(playbackScale, "playbackScale", "Playback Scale");
}
// ---------------------------------------------------------------------------

bool SerializeParameterName(string* str, IArchive& ar, const char* name, const char* label)
{
	static Serialization::StringList parameters;
	if (parameters.empty())
	{
		parameters.push_back("");
		for (int i = 0; i < eMotionParamID_COUNT; ++i)
		{
			SMotionParameterDetails details;
			gEnv->pCharacterManager->GetMotionParameterDetails(details, EMotionParamID(i));
			if ((details.flags & details.OBSOLETE) == 0)
				parameters.push_back(details.name);
		}
	}
	int index = parameters.find(str->c_str());
	Serialization::StringListValue value(parameters, index == -1 ? 0 : index);

	if (!ar(value, name, label))
		return false;

	*str = value.c_str();
	return true;
}

bool Serialize(IArchive& ar, BlendSpaceAdditionalExtraction& value, const char* name, const char* label)
{
	return SerializeParameterName(&value.parameterName, ar, name, label);
}

bool Serialize(IArchive& ar, BlendSpaceReference& ref, const char* name, const char* label)
{
	return ar(AnimationPath(ref.path), name, label);
}

void SerializeExampleIndex(int* index, float* weight, IArchive& ar, const char* name, const char* label)
{
	if (ar.OpenBlock(name, label))
	{
		if (BlendSpace* blendSpace = ar.FindContext<BlendSpace>())
		{
			Serialization::StringList stringList;
			for (size_t i = 0; i < blendSpace->m_examples.size(); ++i)
				stringList.push_back(blendSpace->m_examples[i].animation);
			Serialization::StringListValue value(stringList, size_t(*index) < size_t(stringList.size()) ? *index : 0);
			ar(value, "index", "^");
			if (ar.IsInput())
				*index = stringList.find(value.c_str());
			ar(*weight, "weight", ">^");
		}
		ar.CloseBlock();
	}
}

void BlendSpacePseudoExample::Serialize(IArchive& ar)
{
	if (ar.IsEdit())
	{
		SerializeExampleIndex(&i0, &w0, ar, "i0", "^<");
		SerializeExampleIndex(&i1, &w1, ar, "i1", "^<");
	}
	else
	{
		ar(i0, "i0");
		ar(i1, "i1");
		ar(w0, "w0");
		ar(w1, "w1");
	}
}

// ---------------------------------------------------------------------------{

void BlendSpaceDimension::Serialize(IArchive& ar)
{
	SerializeParameterName(&parameterName, ar, "parameterName", "<^");
	ar(minimal, "min", "^>Min");
	ar(maximal, "max", "^>Max");
	ar(cellCount, "cellCount", "Cell Count");
	ar(locked, "locked", "Locked");
}

// ---------------------------------------------------------------------------

static int ParameterIdByName(const char* name, bool additionalExtraction)
{
	for (size_t i = 0; i < eMotionParamID_COUNT; ++i)
	{
		SMotionParameterDetails details;
		gEnv->pCharacterManager->GetMotionParameterDetails(details, EMotionParamID(i));
		if (additionalExtraction && !((details.flags & details.ADDITIONAL_EXTRACTION) == 0))
			continue;
		if (strcmp(details.name, name) == 0)
			return i;
	}
	return -1;
}

bool BlendSpace::LoadFromXml(string& errorMessage, XmlNodeRef root, IAnimationSet* pAnimationSet)
{
	//---------------------------------------------------------
	//--- parse and verify XML 
	//---------------------------------------------------------
	const char* XMLTAG = root->getTag();
	if ( strcmp(XMLTAG,"ParaGroup") != 0) 
	{
		((errorMessage="Error: The XMLTAG is '") +=XMLTAG) +="'. It is expected to be 'ParaGroup'";
		return false;
	}

	uint32 numPseudo=0;
	uint32 numExamples=0;
	uint32 numChilds = root->getChildCount();
	for (uint32 c=0; c<numChilds; c++)
	{
		XmlNodeRef nodeList = root->getChild(c);
		const char* ListTag = nodeList->getTag();

		//load example-list 
		if ( strcmp(ListTag,"ExampleList")==0 )
		{ 
			numExamples = nodeList->getChildCount();
			if (numExamples==0)
			{
				errorMessage="Error: no examples in this ParaGroup.";
				return false;
			}

			// TODO: MAX_LMG_ANIMS
			if (numExamples>=40)
			{
				errorMessage="Error: too many examples in one ParaGroup. Only 40 are currently allowed! ";
				return false;
			}
		}

		//load pseudo example-list 
		if ( strcmp(ListTag,"ExamplePseudo")==0 ) 
			numPseudo = nodeList->getChildCount();
	}

	//----------------------------------------------------------------------

	for (uint32 c=0; c<numChilds; c++)
	{
		XmlNodeRef nodeList = root->getChild(c);
		const char* ListTag = nodeList->getTag();

		//----------------------------------------------------------------------------------
		//---   temporary helper flags to ensure compatibility with the old system       ---
		//---               they will disappear sooner or later                          ---
		//----------------------------------------------------------------------------------
		if (strcmp(ListTag,"THRESHOLD")==0) 
		{
			nodeList->getAttr( "tz", m_threshold );  //will go as soon as we have the Blend Nodes to combine VEGs
			continue;
		}
		if (strcmp(ListTag,"VEGPARAMS")==0) 
		{
			uint32 nFlags= 0; nodeList->getAttr( "Idle2Move",nFlags );    //will go as soon as CryMannegin is up and running
			if (nFlags)
				m_idleToMove = true; //this is a case for CryMannequin
			continue;
		}

		//-----------------------------------------------------------
		//---  define dimensions of the LMG                       ---
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"Dimensions")==0 ) 
		{
			int numDimensions=nodeList->getChildCount();
			m_dimensions.resize(numDimensions);
			if (m_dimensions.size() >3)
			{
				errorMessage="Error: More then 3 dimensions per Blend-Space are not supported";
				return false;
			}

			for (uint32 d=0; d<m_dimensions.size() ; d++ )
			{
				XmlNodeRef nodeExample = nodeList->getChild(d);
				const char* ExampleTag = nodeExample->getTag();
				if (strcmp(ExampleTag,"Param")==0) 
				{
					//each dimension must have a parameter-name
					m_dimensions[d].parameterName = nodeExample->getAttr( "name" );
					//check if the parameter-name is supported by the system
					m_dimensions[d].parameterId = ParameterIdByName(m_dimensions[d].parameterName.c_str(), false);
					if (m_dimensions[d].parameterId < 0)
					{
						((errorMessage="Error: The parameter '") += m_dimensions[d].parameterName.c_str()) +="' is currently not supported";
						return false;
					}

					//define the scope of the blend-space for each dimension
					nodeExample->getAttr( "min", m_dimensions[d].minimal );
					nodeExample->getAttr( "max", m_dimensions[d].maximal );

					nodeExample->getAttr( "cells", m_dimensions[d].cellCount );	
					m_dimensions[d].cellCount=m_dimensions[d].cellCount<3 ? 3 : m_dimensions[d].cellCount; 

					nodeExample->getAttr( "scale",m_dimensions[d].debugVisualScale );  //just for visual-debugging
					m_dimensions[d].debugVisualScale=max(0.01f,m_dimensions[d].debugVisualScale); 

					//from which joint do we wnat to extract the parameters to initialize the patameter-space??
					m_dimensions[d].jointName = nodeExample->getAttr( "JointName" );
					nodeExample->getAttr( "skey",m_dimensions[d].startKey );
					nodeExample->getAttr( "ekey",m_dimensions[d].endKey );

					//special flags per-dimension
					nodeExample->getAttr( "locked",m_dimensions[d].locked );
				}
			}
			continue;
		}

		//-----------------------------------------------------------
		//---  define the additional extraction parameters        ---
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"AdditionalExtraction")==0 ) 
		{
			int numExtractionParams = nodeList->getChildCount();

			if (numExtractionParams>4)
			{
				errorMessage="Error: More then 4 additional extraction parameters are not supported";
				return false;
			}
			m_additionalExtraction.resize(numExtractionParams);
			for (uint32 d=0; d<numExtractionParams; d++ )
			{
				XmlNodeRef nodeExample = nodeList->getChild(d);
				const char* ExampleTag = nodeExample->getTag();
				if (strcmp(ExampleTag,"Param")==0) 
				{
					//each dimension must have a parameter-name
					m_additionalExtraction[d].parameterName = nodeExample->getAttr( "name" );
					//check if the parameter-name is supported by the system
					m_additionalExtraction[d].parameterId = ParameterIdByName(m_additionalExtraction[d].parameterName.c_str(), true);
					if (m_additionalExtraction[d].parameterId<0)
					{
						((errorMessage="Error: The parameter '") +=m_additionalExtraction[d].parameterName.c_str()) +="' is currently not supported";
						return false;
					}
				}
			}
			continue;
		}


		//-----------------------------------------------------------
		//load example-list 
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"ExampleList")==0 ) 
		{
			int numExamples=nodeList->getChildCount();
			m_examples.resize(numExamples);
			for (uint32 i=0; i<numExamples; i++) 
			{
				XmlNodeRef nodeExample = nodeList->getChild(i);
				const char* ExampleTAG = nodeExample->getTag();
				if ( strcmp(ExampleTAG,"Example") ) 
				{
					((errorMessage="Error: The ExampleTAG for an example is '") +=ExampleTAG) +="'. It is expected to be 'Example'";
					return false;
				}

				if (strcmp(ExampleTAG,"Example")==0) 
				{
					m_examples[i].animation = nodeExample->getAttr( "AName" );

					nodeExample->getAttr( "PlaybackScale", m_examples[i].playbackScale );

					//Pre-initialized parameters should be an exception. Only use them if real extraction is impossible
					m_examples[i].specified[0] = nodeExample->getAttr( "SetPara0",m_examples[i].parameters.x );
					m_examples[i].specified[1]=nodeExample->getAttr( "SetPara1",m_examples[i].parameters.y );
					if (m_examples[i].specified[1] && m_dimensions.size()<2)
					{
						errorMessage="Error: SetPara1 is not allowed on BSpaces with less than 2 m_dimensions";
						return false;
					}

					m_examples[i].specified[2]=nodeExample->getAttr( "SetPara2",m_examples[i].parameters.z );
					if (m_examples[i].specified[2] && m_dimensions.size()<3)
					{
						errorMessage="Error: SetPara2 is not allowed on BSpaces with less than 3 m_dimensions";
						return false;
					}

					m_examples[i].specified[3]=nodeExample->getAttr( "SetPara3",m_examples[i].parameters.w );
					if (m_examples[i].specified[3] && m_dimensions.size()<4)
					{
						errorMessage="Error: SetPara3 is not allowed on BSpaces with less than 4 m_dimensions";
						return false;
					}

					nodeExample->getAttr( "UseDirectlyForDeltaMotion0", m_examples[i].useDirectlyForDeltaMotion[0] );
					nodeExample->getAttr( "UseDirectlyForDeltaMotion1", m_examples[i].useDirectlyForDeltaMotion[1] );
					nodeExample->getAttr( "UseDirectlyForDeltaMotion2", m_examples[i].useDirectlyForDeltaMotion[2] );
					nodeExample->getAttr( "UseDirectlyForDeltaMotion3", m_examples[i].useDirectlyForDeltaMotion[3] );
				}
			}
			continue;
		}


		//-----------------------------------------------------------
		//load pseudo example-list 
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"ExamplePseudo")==0 ) 
		{
			uint32 num = nodeList->getChildCount();
			m_pseudoExamples.resize(num);
			for (uint32 i=0; i<num; i++) 
			{
				XmlNodeRef nodeExample = nodeList->getChild(i);
				const char* ExampleTag = nodeExample->getTag();
				if (strcmp(ExampleTag,"Pseudo")==0) 
				{
					uint32 i0=-1; f32 w0=1.0f;
					uint32 i1=-1; f32 w1=1.0f;

					nodeExample->getAttr( "p0",i0 );
					assert(i0<numExamples);
					nodeExample->getAttr( "p1",i1 );
					assert(i1<numExamples);

					nodeExample->getAttr( "w0",w0 );
					nodeExample->getAttr( "w1",w1 );

					f32 sum=w0+w1;
					assert(fabsf(1.0f-sum)<0.00001f);

					m_pseudoExamples[i].i0=i0;
					m_pseudoExamples[i].w0=w0;
					m_pseudoExamples[i].i1=i1;
					m_pseudoExamples[i].w1=w1;
				}
			}
			continue;
		}



		//-----------------------------------------------------------
		//---  load blend-annotation-list                          --
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"Blendable")==0 ) 
		{
			uint32 num = nodeList->getChildCount();
			m_annotations.clear();
			m_annotations.reserve(num);
			const char* facePointNames[8] = { "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7" };
			for (uint32 i=0; i<num; i++) 
			{
				XmlNodeRef nodeExample = nodeList->getChild(i);
				const char* ExampleTag = nodeExample->getTag();
				if (strcmp(ExampleTag,"Face")==0) 
				{
					BlendSpaceAnnotation face;

					uint8 index;
					for (size_t i = 0; i < sizeof(facePointNames)/sizeof(facePointNames[0]); ++i)
						if (nodeExample->getAttr(facePointNames[i], index))
							face.indices.push_back(index);
					
					m_annotations.push_back(face);
				}
			}
			continue;
		}


		//-----------------------------------------------------------
		//---  load precomputed example grid                       --
		//-----------------------------------------------------------
		// TODO
		// if ( strcmp(ListTag,"VGrid")==0 ) 
		// {
		// 	VirtualExampleXML::ReadVGrid(nodeList, *this);
		// 	continue;
		// }


		//-----------------------------------------------------------
		//---    check of Motion-Combination examples             ---
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"MotionCombination")==0 ) 
		{
			uint32 num = nodeList->getChildCount();
			m_motionCombinations.resize(num);
			for (uint32 i=0; i<num; i++) 
			{
				XmlNodeRef nodeExample = nodeList->getChild(i);
				const char* ExampleTag = nodeExample->getTag();

				if (strcmp(ExampleTag,"NewStyle")==0) 
				{
					m_motionCombinations[i].animation = nodeExample->getAttr( "Style" );
				}
				else
				{
					((errorMessage="Error: The XML-Tag '") +=ExampleTag) +="' is currently not supported";
					return 0;
				}
			}
			continue;
		}

		//-----------------------------------------------------------
		//-- joint mask
		//-----------------------------------------------------------
		if (strcmp(ListTag, "JointList") == 0)
		{
			uint32 num = nodeList->getChildCount();
			m_joints.resize(num);
			for (uint32 i=0; i<num; ++i)
			{
				XmlNodeRef node = nodeList->getChild(i);
				const char* tag = node->getTag();
				if (strcmp(tag, "Joint") != 0)
				{
					((errorMessage="Error: The XML-Tag '") +=tag) +="' is currently not supported";
					return 0;
				}

				m_joints[i].name = node->getAttr("Name");
			}

			std::sort(m_joints.begin(), m_joints.end());
			continue;
		}


		if ( strcmp(ListTag,"VGrid")==0 ) 
		{
			if (m_dimensions.size() == 1)
			{
				uint32 num = nodeList->getChildCount();
				m_virtualExamples1d.resize(num);
				for (uint32 i=0; i<num; ++i)
				{
					BlendSpaceVirtualExample1D& example = m_virtualExamples1d[i];
					XmlNodeRef node = nodeList->getChild(i);
					const char* tag = node->getTag();
					if (strcmp(tag, "VExample") != 0)
					{
						((errorMessage="Error: The XML-Tag '") +=tag) +="' is currently not supported";
						return false;
					}

					node->getAttr("i0", example.i0);
					node->getAttr("i1", example.i1);
					node->getAttr("w0", example.w0);
					node->getAttr("w1", example.w1);
				}
			}
			if (m_dimensions.size() == 2)
			{
				uint32 num = nodeList->getChildCount();
				m_virtualExamples2d.resize(num);
				for (uint32 i=0; i<num; ++i)
				{
					BlendSpaceVirtualExample2D& example = m_virtualExamples2d[i];
					XmlNodeRef node = nodeList->getChild(i);
					const char* tag = node->getTag();
					if (strcmp(tag, "VExample") != 0)
					{
						((errorMessage="Error: The XML-Tag '") +=tag) +="' is currently not supported";
						return false;
					}

					node->getAttr("i0", example.i0);
					node->getAttr("i1", example.i1);
					node->getAttr("i2", example.i2);
					node->getAttr("i3", example.i3);
					node->getAttr("w0", example.w0);
					node->getAttr("w1", example.w1);
					node->getAttr("w2", example.w2);
					node->getAttr("w3", example.w3);
				}
			}
			if (m_dimensions.size() == 3)
			{
				uint32 num = nodeList->getChildCount();
				m_virtualExamples3d.resize(num);
				for (uint32 i=0; i<num; ++i)
				{
					BlendSpaceVirtualExample3D& example = m_virtualExamples3d[i];
					XmlNodeRef node = nodeList->getChild(i);
					const char* tag = node->getTag();
					if (strcmp(tag, "VExample") != 0)
					{
						((errorMessage="Error: The XML-Tag '") +=tag) +="' is currently not supported";
						return false;
					}

					node->getAttr("i0", example.i0);
					node->getAttr("i1", example.i1);
					node->getAttr("i2", example.i2);
					node->getAttr("i3", example.i3);
					node->getAttr("i4", example.i4);
					node->getAttr("i5", example.i5);
					node->getAttr("i6", example.i6);
					node->getAttr("i7", example.i7);
					node->getAttr("w0", example.w0);
					node->getAttr("w1", example.w1);
					node->getAttr("w2", example.w2);
					node->getAttr("w3", example.w3);
					node->getAttr("w4", example.w4);
					node->getAttr("w5", example.w5);
					node->getAttr("w6", example.w6);
					node->getAttr("w7", example.w7);
				}
			}
		}
	}

	if (m_dimensions.size() ==0)
	{
		errorMessage="Error: ParaGroup has no m_dimensions specified";
		return false;
	}

	return true;
}

XmlNodeRef BlendSpace::SaveToXml() const
{
	XmlNodeRef root = gEnv->pSystem->CreateXmlNode("ParaGroup");

	if (m_threshold > -99999.0f)
	{
		XmlNodeRef threshold = root->newChild("THRESHOLD");
		threshold->setAttr("tz", m_threshold);
	}

	if (m_idleToMove)
	{
		XmlNodeRef vegparams = root->newChild("VEGPARAMS");
		vegparams->setAttr("Idle2Move", "1");
	}

	XmlNodeRef nodeList = root->newChild("Dimensions");
	uint32 numDimensions = m_dimensions.size();
	for (uint32 d = 0; d < numDimensions; ++d)
	{
		XmlNodeRef nodeExample = nodeList->newChild("Param");
		nodeExample->setAttr("Name", m_dimensions[d].parameterName.c_str());
		nodeExample->setAttr("Min", m_dimensions[d].minimal);
		nodeExample->setAttr("Max", m_dimensions[d].maximal);
		nodeExample->setAttr("Cells", m_dimensions[d].cellCount);	
		nodeExample->setAttr("Scale",m_dimensions[d].debugVisualScale);
		nodeExample->setAttr("SKey",m_dimensions[d].startKey);
		nodeExample->setAttr("EKey",m_dimensions[d].endKey);
		if (m_dimensions[d].locked)
			nodeExample->setAttr( "locked",m_dimensions[d].locked );
	}

	uint32 numExtractionParams = m_additionalExtraction.size();
	nodeList = root->newChild("AdditionalExtraction");
	for (uint32 d=0; d<numExtractionParams; d++ )
	{
		XmlNodeRef nodeExample = nodeList->newChild("Param");
		nodeExample->setAttr("name", m_additionalExtraction[d].parameterName.c_str());
	}

	nodeList = root->newChild("ExampleList");
	int numExamples = m_examples.size();
	for (uint32 i = 0; i < numExamples; ++i) 
	{
		XmlNodeRef nodeExample = nodeList->newChild("Example");

		nodeExample->setAttr("AName", m_examples[i].animation.c_str());
		nodeExample->setAttr("PlaybackScale", m_examples[i].playbackScale);

		if (m_examples[i].specified[0]) nodeExample->setAttr("SetPara0",m_examples[i].parameters.x);
		if (m_examples[i].specified[1]) nodeExample->setAttr("SetPara1",m_examples[i].parameters.y);
		if (m_examples[i].specified[2]) nodeExample->setAttr("SetPara2",m_examples[i].parameters.z);
		if (m_examples[i].specified[3]) nodeExample->setAttr("SetPara3",m_examples[i].parameters.w);

		nodeExample->setAttr("UseDirectlyForDeltaMotion0", m_examples[i].useDirectlyForDeltaMotion[0]);
		nodeExample->setAttr("UseDirectlyForDeltaMotion1", m_examples[i].useDirectlyForDeltaMotion[1]);
		nodeExample->setAttr("UseDirectlyForDeltaMotion2", m_examples[i].useDirectlyForDeltaMotion[2]);
		nodeExample->setAttr("UseDirectlyForDeltaMotion3", m_examples[i].useDirectlyForDeltaMotion[3]);
	}

	nodeList = root->newChild("ExamplePseudo");
	uint32 num = m_pseudoExamples.size();
	for (uint32 i = 0; i < num; ++i)
	{
		XmlNodeRef nodeExample = nodeList->newChild("Pseudo");
		nodeExample->setAttr("p0",m_pseudoExamples[i].i0);
		nodeExample->setAttr("p1",m_pseudoExamples[i].i1);
		nodeExample->setAttr("w0",m_pseudoExamples[i].w0);
		nodeExample->setAttr("w1",m_pseudoExamples[i].w1);
	}

	nodeList = root->newChild("Blendable");
	num = m_annotations.size();
	const char* facePointNames[8] = { "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7" };
	for (uint32 i = 0; i < num; ++i) 
	{
		XmlNodeRef nodeFace = nodeList->newChild("Face");
		const BlendSpaceAnnotation& face = m_annotations[i];
		uint32 pointCount = face.indices.size();
		if (pointCount > 8)
			pointCount = 8;
		for (uint32 j = 0; j < pointCount; ++j)
			nodeFace->setAttr(facePointNames[j], face.indices[j]);
	}


	nodeList = root->newChild("MotionCombination");
	num = m_motionCombinations.size();
	for (uint32 i = 0; i < num; ++i) 
	{
		XmlNodeRef nodeExample = nodeList->newChild("NewStyle");
		nodeExample->setAttr("Style", m_motionCombinations[i].animation.c_str());
	}

	nodeList = root->newChild("JointList");
	num = m_joints.size();
	for (uint32 i=0; i<num; ++i)
	{
		XmlNodeRef node = nodeList->newChild("Joint");
		node->setAttr("Name", m_joints[i].name.c_str());
	}

	if (!m_virtualExamples1d.empty() ||
			!m_virtualExamples2d.empty() ||
			!m_virtualExamples3d.empty())
	{
		nodeList = root->newChild("VGrid");
		if (!m_virtualExamples1d.empty())
		{
			for (uint32 i = 0; i < m_virtualExamples1d.size(); ++i)
			{
				const BlendSpaceVirtualExample1D& example = m_virtualExamples1d[i];
				XmlNodeRef node = nodeList->newChild("VExample");
				node->setAttr("i0", example.i0);
				node->setAttr("i1", example.i1);
				node->setAttr("w0", example.w0);
				node->setAttr("w1", example.w1);
			}
		}
		else if (!m_virtualExamples2d.empty())
		{
			for (uint32 i = 0; i < m_virtualExamples2d.size(); ++i)
			{
				const BlendSpaceVirtualExample2D& example = m_virtualExamples2d[i];
				XmlNodeRef node = nodeList->newChild("VExample");
				node->setAttr("i0", example.i0);
				node->setAttr("i1", example.i1);
				node->setAttr("i2", example.i2);
				node->setAttr("i3", example.i3);
				node->setAttr("w0", example.w0);
				node->setAttr("w1", example.w1);
				node->setAttr("w2", example.w2);
				node->setAttr("w3", example.w3);
			}
		}
		else if (!m_virtualExamples3d.empty())
		{
			for (uint32 i = 0; i < m_virtualExamples3d.size(); ++i)
			{
				const BlendSpaceVirtualExample3D& example = m_virtualExamples3d[i];
				XmlNodeRef node = nodeList->newChild("VExample");
				node->setAttr("i0", example.i0);
				node->setAttr("i1", example.i1);
				node->setAttr("i2", example.i2);
				node->setAttr("i3", example.i3);
				node->setAttr("i4", example.i4);
				node->setAttr("i5", example.i5);
				node->setAttr("i6", example.i6);
				node->setAttr("i7", example.i7);
				node->setAttr("w0", example.w0);
				node->setAttr("w1", example.w1);
				node->setAttr("w2", example.w2);
				node->setAttr("w3", example.w3);
				node->setAttr("w4", example.w4);
				node->setAttr("w5", example.w5);
				node->setAttr("w6", example.w6);
				node->setAttr("w7", example.w7);
			}
		}
	}

	return root;
}

void BlendSpace::Serialize(Serialization::IArchive& ar)
{
	Serialization::SContext<BlendSpace> bspaceContext(ar, this);
	ar(m_threshold, "threshold", 0);
	ar(m_idleToMove, "idleToMove", "Idle To Move");

	ar(m_dimensions, "dimensions", "Dimensions");
	if (m_dimensions.empty()) 
		ar.Warning(m_dimensions, "At least one dimension is required for the BlendSpace");
	else if (m_dimensions.size() > 3)
		ar.Error(m_dimensions, "At most 3 dimensions are supported");

	ar(m_examples, "examples", "Examples");
	const int maxBlendSpaceAnimations = 40;
	if (m_examples.size() > maxBlendSpaceAnimations)
		ar.Error(m_examples, "Number of examples should not excceed %d.", maxBlendSpaceAnimations);

	ar(m_pseudoExamples, "pseudoExamples", "[+]Pseudo Examples");

	ar(m_additionalExtraction, "additionalExtraction", "Additional Extraction");
	if (m_additionalExtraction.size() > 4)
		ar.Error(m_additionalExtraction, "More than 4 additional extraction parameters are not supported");

	ar(m_motionCombinations, "motionCombinations", "Motion Combinations");
	ar(m_joints, "joints", "Joints");
	ar(m_annotations, "annotations", "Annotations");

	if (!ar.IsEdit())
	{
		ar(m_virtualExamples1d, "virtualExamples1d", "Virtual Examples (1D)");
		ar(m_virtualExamples2d, "virtualExamples2d", "Virtual Examples (2D)");
		ar(m_virtualExamples3d, "virtualExamples3d", "Virtual Examples (3D)");
	}
}

// ---------------------------------------------------------------------------

void CombinedBlendSpaceDimension::Serialize(IArchive& ar)
{
	SerializeParameterName(&parameterName, ar, "parameterName", "<^");
	ar(locked, "locked", "^Locked");
	ar(chooseBlendSpace, "chooseBlendSpace", "^Selects BSpace");
	ar(parameterScale, "parameterScale", ">^ x");
}

bool CombinedBlendSpace::LoadFromXml(string& errorMessage, XmlNodeRef root, IAnimationSet* pAnimationSet)
{
	//---------------------------------------------------------
	//--- parse and verify XML 
	//---------------------------------------------------------
	const char* XMLTAG = root->getTag();
	if ( strcmp(XMLTAG,"CombinedBlendSpace") != 0 ) 
	{
		((errorMessage="Error: The XMLTAG is '") +=XMLTAG) +="'. It is expected to be 'ParaGroup' or 'CombinedBlendSpace'";
		return false;
	}

	//----------------------------------------------------------------------
	//----      check of this is a combined Blend-Space          -----------
	//----------------------------------------------------------------------
	uint32 numChilds = root->getChildCount();
	for (uint32 c=0; c<numChilds; c++)
	{
		XmlNodeRef nodeList = root->getChild(c);
		const char* ListTag = nodeList->getTag();

		if (strcmp(ListTag,"VEGPARAMS")==0) 
		{
			uint32 nFlags=0;
			nodeList->getAttr( "Idle2Move",nFlags );    //will go as soon as CryMannegin is up and running
			m_idleToMove = nFlags != 0;
			continue;
		}

		//-----------------------------------------------------------
		//---  define m_dimensions of the LMG                       ---
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"Dimensions")==0 ) 
		{
			int numDimensions=nodeList->getChildCount();
			if (numDimensions>4)
			{
				errorMessage="Error: More then 4 m_dimensions per ParaGroup are not supported";
				return false;
			}

			m_dimensions.resize(numDimensions);
			for (uint32 d=0; d<numDimensions; d++ )
			{
				XmlNodeRef nodeExample = nodeList->getChild(d);
				const char* ExampleTag = nodeExample->getTag();
				if (strcmp(ExampleTag,"Param")==0) 
				{
					//each dimension must have a parameter-name
					m_dimensions[d].parameterName = nodeExample->getAttr( "name" );
					m_dimensions[d].parameterId = ParameterIdByName(m_dimensions[d].parameterName.c_str(), false);

					if (m_dimensions[d].parameterId < 0)
					{
						((errorMessage="Error: The parameter '") +=m_dimensions[d].parameterName.c_str()) +="' is currently not supported";
						return false;
					}

					if (!nodeExample->getAttr( "ParaScale", m_dimensions[d].parameterScale ))
						m_dimensions[d].parameterScale = 1.0f;
					if (!nodeExample->getAttr( "ChooseBlendSpace", m_dimensions[d].chooseBlendSpace ))
						m_dimensions[d].chooseBlendSpace = false;
					//special flags per-dimension
					if (!nodeExample->getAttr( "locked", m_dimensions[d].locked ))
						m_dimensions[d].locked = false;
				}
			}
			continue;
		}


		//-----------------------------------------------------------
		//---  define the additional extraction parameters        ---
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"AdditionalExtraction")==0 ) 
		{
			int numExtractionParams = nodeList->getChildCount();
			if (numExtractionParams>4)
			{
				errorMessage="Error: More then 4 additional extraction parameters are not supported";
				return false;
			}
			m_additionalExtraction.resize(numExtractionParams);
			for (uint32 d=0; d<numExtractionParams; d++ )
			{
				XmlNodeRef nodeExample = nodeList->getChild(d);
				const char* ExampleTag = nodeExample->getTag();
				if (strcmp(ExampleTag,"Param")==0) 
				{
					//each dimension must have a parameter-name
					m_additionalExtraction[d].parameterName = nodeExample->getAttr( "name" );
					//check if the parameter-name is supported by the system
					m_additionalExtraction[d].parameterId = ParameterIdByName(m_additionalExtraction[d].parameterName.c_str(), true);
					if (m_additionalExtraction[d].parameterId<0)
					{
						((errorMessage="Error: The parameter '") +=m_additionalExtraction[d].parameterName.c_str()) +="' is currently not supported";
						return false;
					}
				}
			}
			continue;
		}



		if ( strcmp(ListTag,"BlendSpaces")==0 ) 
		{
			uint32 num = nodeList->getChildCount();
			m_blendSpaces.resize(num);
			for (uint32 i=0; i<num; i++) 
			{
				XmlNodeRef nodeExample = nodeList->getChild(i);
				const char* ExampleTag = nodeExample->getTag();
				if (strcmp(ExampleTag,"BlendSpace")==0) 
				{
					string strFilePath = nodeExample->getAttr( "aname");
					m_blendSpaces[i].path			= strFilePath;
				}
				else
				{
					((errorMessage="Error: The XML-Tag '") +=ExampleTag) +="' is currently not supported";
					return 0;
				}
			}
			continue;
		}

		//-----------------------------------------------------------
		//---    check of Motion-Combination examples             ---
		//-----------------------------------------------------------
		if ( strcmp(ListTag,"MotionCombination")==0 ) 
		{
			uint32 num = nodeList->getChildCount();
			m_motionCombinations.resize(num);
			for (uint32 i=0; i<num; i++) 
			{
				XmlNodeRef nodeExample = nodeList->getChild(i);
				const char* ExampleTag = nodeExample->getTag();

				if (strcmp(ExampleTag,"NewStyle")==0) 
				{
					m_motionCombinations[i].animation = nodeExample->getAttr( "Style" );
				}
				else
				{
					((errorMessage="Error: The XML-Tag '") +=ExampleTag) +="' is currently not supported";
					return 0;
				}
			}
			continue;
		}

		//-----------------------------------------------------------
		//-- joint mask
		//-----------------------------------------------------------
		if (strcmp(ListTag, "JointList") == 0)
		{
			uint32 num = nodeList->getChildCount();
			m_joints.resize(num);
			for (uint32 i=0; i<num; ++i)
			{
				XmlNodeRef node = nodeList->getChild(i);
				const char* tag = node->getTag();
				if (strcmp(tag, "Joint") != 0)
				{
					((errorMessage="Error: The XML-Tag '") +=tag) +="' is currently not supported";
					return 0;
				}

				m_joints[i].name = node->getAttr("Name");
			}

			std::sort(m_joints.begin(), m_joints.end());
			continue;
		}
	}

	return true;
}

XmlNodeRef CombinedBlendSpace::SaveToXml() const
{
	XmlNodeRef root = gEnv->pSystem->CreateXmlNode("CombinedBlendSpace");

	if (m_idleToMove)
	{
		XmlNodeRef node = root->newChild("VEGPARAMS");
		node->setAttr("Idle2Move", "1");
	}

	XmlNodeRef dimensions = root->newChild("Dimensions");
	int numDimensions=m_dimensions.size();
	for (uint32 d = 0; d < numDimensions; d++)
	{
		XmlNodeRef nodeExample = dimensions->newChild("Param");
		nodeExample->setAttr("Name", m_dimensions[d].parameterName.c_str());
		nodeExample->setAttr("ParaScale", m_dimensions[d].parameterScale );
		if (m_dimensions[d].chooseBlendSpace)
			nodeExample->setAttr("ChooseBlendSpace", m_dimensions[d].chooseBlendSpace);
		if (m_dimensions[d].locked )
			nodeExample->setAttr("Locked", m_dimensions[d].locked);
	}

	XmlNodeRef nodeList = root->newChild("AdditionalExtraction");
	int numExtractionParams = m_additionalExtraction.size();
	for (uint32 d = 0; d < numExtractionParams; ++d)
	{
		XmlNodeRef nodeExample = nodeList->newChild("Param");
		nodeExample->setAttr("Name", m_additionalExtraction[d].parameterName.c_str());
	}

	nodeList = root->newChild("BlendSpaces");
	uint32 num = m_blendSpaces.size();
	for (uint32 i = 0; i < num; ++i) 
	{
		XmlNodeRef nodeExample = nodeList->newChild("BlendSpace");
		nodeExample->setAttr("AName", m_blendSpaces[i].path.c_str());
	}

	nodeList = root->newChild("MotionCombination");
	num = m_motionCombinations.size();
	for (uint32 i = 0; i < num; ++i) 
	{
		XmlNodeRef nodeExample = nodeList->newChild("NewStyle");
		nodeExample->setAttr("Style", m_motionCombinations[i].animation.c_str());
	}

	nodeList = root->newChild("JointList");
	num = m_joints.size();
	for (uint32 i=0; i<num; ++i)
	{
		XmlNodeRef node = nodeList->newChild("Joint");
		node->setAttr("Name", m_joints[i].name.c_str());
	}

	return root;
}

}
