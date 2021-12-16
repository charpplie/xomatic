#ifndef IPerceptionSystem_h
#define IPerceptionSystem_h

//////////////////////////////////////////////////////////////////////////

struct SPerceptionConfigParams
{
	SPerceptionConfigParams() : 
		test(0.0f)
	{
	}

	void ReadParamsFromXML(const XmlNodeRef& paramsNode)
	{
		paramsNode->getAttr("test", test);
	}

	float test;
};

//////////////////////////////////////////////////////////////////////////

typedef uint32 StimulusId;

struct SStimulus
{
	SStimulus()	: 
		attack(0.0f),
		sustain(0.0f)
	{
	}

	void ReadStimulusFromXML(const XmlNodeRef& stimulusNode)
	{
		stimulusNode->getAttr("attack", attack);
		stimulusNode->getAttr("sustain", sustain);
	}

	float attack;
	float sustain;
};

//////////////////////////////////////////////////////////////////////////

struct SPerceptionConfig
{
	typedef std::map<StimulusId, SStimulus> StimuliMap;

	SPerceptionConfigParams params;
	StimuliMap stimuli;
};

//////////////////////////////////////////////////////////////////////////

struct SStimulusEvent
{
	StimulusId stimulusId;
};

//////////////////////////////////////////////////////////////////////////

struct IPerceptionSystem
{
	virtual void Reset() = 0;
	virtual void RegisterStimulusEvent(const SStimulusEvent& stimulus) = 0;
	virtual StimulusId GetStimulusId(const char* stimulusName) const = 0;
};

#endif