#ifndef PerceptionSystem_h
#define PerceptionSystem_h

#include "IPerceptionSystem.h"

//////////////////////////////////////////////////////////////////////////

class CPerceptionSystem : public IPerceptionSystem
{
	typedef std::map<string, SPerceptionConfig> PerceptionConfigsMap;
	typedef std::map<string, StimulusId > StimulusIdsMap;

public:
	CPerceptionSystem();
	~CPerceptionSystem();

	void Reset();
	void RegisterStimulusEvent(const SStimulusEvent& stimulusEvent);
	StimulusId GetStimulusId(const char* stimulusName) const;

private:

	void LoadConfigsFromXML( const char* xmlFilename );
	void LoadStimuliFromXML(const XmlNodeRef& stimuliNode, SPerceptionConfig& perceptionConfig);

	PerceptionConfigsMap m_perceptionConfigs;
	StimulusIdsMap m_stimulusIds;

};


#endif // PerceptionSystem_h