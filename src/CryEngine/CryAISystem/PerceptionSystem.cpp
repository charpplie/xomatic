#include "StdAfx.h"
#include "PerceptionSystem.h"
#include "StringUtils.h"

//////////////////////////////////////////////////////////////////////////

CPerceptionSystem::CPerceptionSystem()
{
}

CPerceptionSystem::~CPerceptionSystem()
{
}

void CPerceptionSystem::Reset()
{
	m_perceptionConfigs.clear();
	LoadConfigsFromXML( "Scripts/AI/PerceptionConfigurations.xml");
}

void CPerceptionSystem::LoadConfigsFromXML(const char* xmlFilename)
{
	XmlNodeRef xmlNode = GetISystem()->LoadXmlFile(xmlFilename);

	if (!xmlNode)
	{
		CryWarning( VALIDATOR_MODULE_AI, VALIDATOR_ERROR, "CPerceptionSystem::LoadConfigsFromXML: Failed to load'%s'", xmlFilename);
		return;
	}

	const int configCount = xmlNode->getChildCount();
	for (int i = 0; i < configCount; ++i)
	{
		const XmlNodeRef currentConfigNode = xmlNode->getChild(i); 

		string configName = currentConfigNode->getAttr("name");
		std::pair<PerceptionConfigsMap::iterator, bool> res = m_perceptionConfigs.insert(std::make_pair(configName, SPerceptionConfig()));

		SPerceptionConfig& perceptionConfig = res.first->second;

		XmlString templateName;
		if(currentConfigNode->getAttr("template", templateName))
		{
			PerceptionConfigsMap::iterator templateConfig = m_perceptionConfigs.find(templateName);
			if(templateConfig != m_perceptionConfigs.end())
			{
				perceptionConfig = templateConfig->second;
			}
			else
			{
				CryWarning( VALIDATOR_MODULE_AI, VALIDATOR_ERROR, "CPerceptionSystem::LoadConfigsFromXML: Failed to load template '%s' for the configuration '%s' ", templateName.c_str(), configName.c_str());
			}
		}

		XmlNodeRef paramsNode = currentConfigNode->findChild("Params");
		if(paramsNode)
			perceptionConfig.params.ReadParamsFromXML(paramsNode);

		XmlNodeRef stimuliNode = currentConfigNode->findChild("Stimuli");
		if(stimuliNode)
			LoadStimuliFromXML(stimuliNode, perceptionConfig);

	}
}

void CPerceptionSystem::LoadStimuliFromXML(const XmlNodeRef& stimuliNode, SPerceptionConfig& perceptionConfig)
{
	const int stimuliCount = stimuliNode->getChildCount();
	for (int i = 0; i < stimuliCount; ++i)
	{
		const XmlNodeRef currentStimulusNode = stimuliNode->getChild(i); 

		string stimulusName = currentStimulusNode->getAttr("name");

		StimulusId stimulusId  = CryStringUtils::CalculateHash(stimulusName);
		m_stimulusIds.insert(std::make_pair(stimulusName, stimulusId));

		std::pair<SPerceptionConfig::StimuliMap::iterator, bool> res = perceptionConfig.stimuli.insert(std::make_pair(stimulusId, SStimulus()));

		res.first->second.ReadStimulusFromXML(currentStimulusNode);
	}
}

StimulusId CPerceptionSystem::GetStimulusId( const char* stimulusName) const
{
	StimulusIdsMap::const_iterator result = m_stimulusIds.find(stimulusName);
	if(result != m_stimulusIds.end())
	{
		return result->second;
	}
	return 0;
}

void CPerceptionSystem::RegisterStimulusEvent( const SStimulusEvent& stimulusEvent )
{

}
