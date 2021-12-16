/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-20010.
-------------------------------------------------------------------------

Implementation for force feedback system

* Effect definition (shape, time, ...) are defined in xml 
* Effects are invoked by name, and updated here internally, feeding 
input system in a per frame basis

-------------------------------------------------------------------------
History:
- 18-02-2010:	Created by Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "ForceFeedbackSystem.h"

#define MAX_FORCE_FEEDBACK_EFFECTS 8

#define FORCE_FEEDBACK_PREDEFINED_PATTERNS 3
#define FORCE_FEEDBACK_PREDEFINED_ENVELOPES 4

#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
	#include <IDebugHistory.h>
#endif
CForceFeedBackSystem::CForceFeedBackSystem()
{
#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
	m_pDebugHistoryManager = NULL;
#endif

	m_defaultPattern.ResetToDefault();
	m_defaultEnvelope.ResetToDefault();

	m_activeEffects.reserve(MAX_FORCE_FEEDBACK_EFFECTS); 
}

CForceFeedBackSystem::~CForceFeedBackSystem()
{
#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
	SAFE_RELEASE(m_pDebugHistoryManager);
#endif
}

void CForceFeedBackSystem::PlayForceFeedbackEffect( const char* effectName, const float intensity )
{
	const int activeEffectSize = m_activeEffects.size();
	bool freeSlotsAvailable = (activeEffectSize < MAX_FORCE_FEEDBACK_EFFECTS);

	if (freeSlotsAvailable)
	{
		TEffectToIndexMap::const_iterator cit = m_effectToIndexMap.find(CONST_TEMP_STRING(effectName));

		if (cit != m_effectToIndexMap.end())
		{
			const int effectIdx = cit->second;
			assert((effectIdx >= 0) && (effectIdx < m_effects.size()));

			const SEffect& effect = m_effects[effectIdx];
			TPatternsMap::const_iterator patternACit = m_patters.find(effect.patternA);
			TEnvelopesMap::const_iterator envelopeACit = m_envelopes.find(effect.envelopeA); 
			TPatternsMap::const_iterator patternBCit = (effect.patternA == effect.envelopeB) ? patternACit : m_patters.find(effect.patternB);
			TEnvelopesMap::const_iterator envelopeBCit = (effect.envelopeA == effect.envelopeB) ? envelopeACit : m_envelopes.find(effect.envelopeB);

			TPatternsMap::const_iterator patternEndIt = m_patters.end();
			TEnvelopesMap::const_iterator envelopeEndIt = m_envelopes.end();

			SActiveEffect newActiveEffect;
			m_activeEffects.push_back(newActiveEffect);

			SActiveEffect& justAddedEffect = m_activeEffects[activeEffectSize];
			justAddedEffect.effectIdx = effectIdx;
			justAddedEffect.effectTime = effect.time;
			justAddedEffect.runningTime = 0.0f;
			justAddedEffect.frequencyA = effect.frequencyA;
			justAddedEffect.frequencyB = effect.frequencyB;
			justAddedEffect.intensity = intensity;

			//Patters are copied, for faster access when loop-processing, instead of being a pointer
			//Since we have a very small amount of fixed FX it should not be a big deal.
			justAddedEffect.m_patternA = (patternACit != patternEndIt) ? patternACit->second : m_defaultPattern;
			justAddedEffect.m_envelopeA = (envelopeACit != envelopeEndIt) ? envelopeACit->second : m_defaultEnvelope;
			justAddedEffect.m_patternB = (patternBCit != patternEndIt) ? patternBCit->second : m_defaultPattern;
			justAddedEffect.m_envelopeB = (envelopeBCit != envelopeEndIt) ? envelopeBCit->second : m_defaultEnvelope;
		}
#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
		else
		{
			GameWarning("ForceFeedback (Play) system could not found effect '%s'", effectName);
		}
#endif
	}
	else
	{
		GameWarning("ForceFeedback system: Too many effects already running, could not execute '%s'", effectName);
	}
}

void CForceFeedBackSystem::StopForceFeedbackEffect( const char* effectName )
{
	TEffectToIndexMap::const_iterator cit = m_effectToIndexMap.find(CONST_TEMP_STRING(effectName));

	if (cit != m_effectToIndexMap.end())
	{
		const int effectIdx = cit->second;
		assert((effectIdx >= 0) && (effectIdx < m_effects.size()));

		TActiveEffectsArray::iterator activeEffectIt = m_activeEffects.begin();

		while (activeEffectIt != m_activeEffects.end())
		{
			if (activeEffectIt->effectIdx != effectIdx)
			{
				++activeEffectIt;
			}
			else
			{
				TActiveEffectsArray::iterator next = m_activeEffects.erase(activeEffectIt);
				activeEffectIt = next;
			}
		}
	}
#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
	else
	{
		GameWarning("ForceFeedback (Stop) system could not found effect '%s'", effectName);
	}
#endif
}

void CForceFeedBackSystem::StopAllEffects()
{
	m_activeEffects.clear();

	UpdateInputSystem(0.0f, 0.0f);
}

void CForceFeedBackSystem::AddFrameCustomForceFeedback(const float amplifierA, const float amplifierB)
{
	m_frameCustomForceFeedback.forceFeedbackA += amplifierA;
	m_frameCustomForceFeedback.forceFeedbackB += amplifierB;
}

void CForceFeedBackSystem::Update( float frameTime )
{
	SFFOutput forceFeedback;

	TActiveEffectsArray::iterator activeEffectIt = m_activeEffects.begin();

	while (activeEffectIt != m_activeEffects.end())
	{
		SActiveEffect& currentEffect = *activeEffectIt;

		forceFeedback += currentEffect.Update(frameTime);

		if (currentEffect.HasFinished())
		{
			TActiveEffectsArray::iterator next = m_activeEffects.erase(activeEffectIt);
			activeEffectIt = next;
		}
		else
		{
			++activeEffectIt;
		}
	}

	forceFeedback += m_frameCustomForceFeedback;
	m_frameCustomForceFeedback.ZeroIt();

	UpdateInputSystem(forceFeedback.GetClampedFFA(), forceFeedback.GetClampedFFB());

#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
		DebugFFOutput(forceFeedback);
#endif
}

void CForceFeedBackSystem::UpdateInputSystem( const float amplifierA, const float amplifierB )
{
	if (gEnv->pInput)
	{
		SFFOutputEvent ffEvent;
		ffEvent.deviceId = eDI_XI;
		ffEvent.eventId = eFF_Rumble_Frame;
		ffEvent.amplifierA = amplifierA;
		ffEvent.amplifierS = amplifierB;
		gEnv->pInput->ForceFeedbackEvent(ffEvent);
	}
}

void CForceFeedBackSystem::InitializePredefinedPatterns()
{
	SPattern pattern;

	//Square pattern
	for (int i = 0; i < FFSYSTEM_MAX_PATTERN_SAMPLES; ++i)
	{
		pattern.m_patternSamples[i] = (i < FFSYSTEM_MAX_PATTERN_SAMPLES/2) ? 0xFFFF : 0x0000;
	}
	m_patters.insert(TPatternsMap::value_type("square", pattern));

	//Triangle pattern
	const int halfTriangle = FFSYSTEM_MAX_PATTERN_SAMPLES / 2;
	for (int i = 0; i < halfTriangle; ++i)
	{
		uint16 sample = (uint16)(clamp(i * FFSYSTEM_PATTERN_SAMPLE_STEP * 2.0f, 0.0f, 1.0f) * 65535.0f);
		pattern.m_patternSamples[i] = sample;
		pattern.m_patternSamples[FFSYSTEM_MAX_PATTERN_SAMPLES - 1 - i] = sample; ;
	}
	m_patters.insert(TPatternsMap::value_type("triangle", pattern));

	//Sin pattern
	for (int i = 0; i < FFSYSTEM_MAX_PATTERN_SAMPLES; ++i)
	{
		pattern.m_patternSamples[i] = (uint16)(clamp(cry_sinf(i * FFSYSTEM_PATTERN_SAMPLE_STEP * gf_PI), 0.0f, 1.0f) * 65535.0f);
	}
	m_patters.insert(TPatternsMap::value_type("sin", pattern));

}

void CForceFeedBackSystem::InitializePredefinedEnvelopes()
{
	SEnvelope envelope;

	//Linear decrease
	for (int i = 0; i < FFSYSTEM_MAX_ENVELOPE_SAMPLES; ++i)
	{
		envelope.m_envelopeSamples[i] = (uint16)(clamp(1.0f - ((i + 1) * FFSYSTEM_ENVELOPE_SAMPLE_STEP), 0.0f, 1.0f) * 65535.0f);
	}
	m_envelopes.insert(TEnvelopesMap::value_type("linearDecrease", envelope));

	//Squared decrease
	for (int i = 0; i < FFSYSTEM_MAX_ENVELOPE_SAMPLES; ++i)
	{
		envelope.m_envelopeSamples[i] = (uint16)(pow(clamp(1.0f - (i * FFSYSTEM_ENVELOPE_SAMPLE_STEP), 0.0f, 1.0f), 2.0f) * 65535.0f);
	}
	m_envelopes.insert(TEnvelopesMap::value_type("squaredDecrease", envelope));

	//100% constant
	for (int i = 0; i < FFSYSTEM_MAX_ENVELOPE_SAMPLES; ++i)
	{
		envelope.m_envelopeSamples[i] = (0xFFFF);
	}
	m_envelopes.insert(TEnvelopesMap::value_type("constant", envelope));

	//50% constant
	for (int i = 0; i < FFSYSTEM_MAX_ENVELOPE_SAMPLES; ++i)
	{
		envelope.m_envelopeSamples[i] = (0x7FFF);
	}
	m_envelopes.insert(TEnvelopesMap::value_type("50constant", envelope));
}

void CForceFeedBackSystem::Initialize()
{
	LoadXmlData();
}

void CForceFeedBackSystem::Reload()
{
	StopAllEffects();

	m_patters.clear();
	m_envelopes.clear();
	m_effects.clear();
	m_effectToIndexMap.clear();

	LoadXmlData();
}

void CForceFeedBackSystem::LoadXmlData()
{
	const char* xmlDataFile = "Libs/GameForceFeedback/ForceFeedbackEffects.xml";
	XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFile(xmlDataFile);

	if (!rootNode || strcmpi(rootNode->getTag(), "ForceFeedback"))
	{
		GameWarning("Could not load force feedback system data. Invalid XML file '%s'! ", xmlDataFile);
		return;
	}

	const int childCount = rootNode->getChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef childNode = rootNode->getChild(i);

		const char* childTag = childNode->getTag();
		if (strcmp(childTag, "Patterns") == 0)
		{
			LoadPatters(childNode);
		}
		else if (strcmp(childTag, "Envelopes") == 0)
		{
			LoadEnvelopes(childNode);
		}
		else if (strcmp(childTag, "Effects") == 0)
		{
			LoadEffects(childNode);
		}

	}	
}

void CForceFeedBackSystem::LoadPatters( XmlNodeRef& patternsNode )
{
	const int predefinedPatters = FORCE_FEEDBACK_PREDEFINED_PATTERNS;
	const int patterCount = patternsNode->getChildCount();

	m_patters.reserve(patterCount + predefinedPatters);
	
	InitializePredefinedPatterns();

	TSamplesBuffer samplesBuffer;
	const int maxSampleCount = FFSYSTEM_MAX_PATTERN_SAMPLES / 2;
	float readValues[maxSampleCount];

	for (int i = 0; i < patterCount; ++i)
	{
		XmlNodeRef childPatternNode = patternsNode->getChild(i);

		const char* customPatternName = childPatternNode->getAttr("name");
		if (!customPatternName || (customPatternName[0] == '\0'))
		{
			GameWarning("ForceFeedbackSystem: Could not load pattern without name (at line %d)", childPatternNode->getLine());
			continue;
		}

		samplesBuffer = childPatternNode->haveAttr("name") ? childPatternNode->getAttr("samples") : "";

		int samplesFound = ParseSampleBuffer(samplesBuffer, &readValues[0], maxSampleCount);

		if (samplesFound != 0)
		{
			SPattern customPattern;
			customPattern.ResetToDefault();

			DistributeSamples(&readValues[0], samplesFound, &customPattern.m_patternSamples[0], FFSYSTEM_MAX_PATTERN_SAMPLES);

			m_patters.insert(TPatternsMap::value_type(customPatternName, customPattern));		
		}
		else
		{
			GameWarning("ForceFeedbackSystem: Pattern '%s' (at line %d) has not samples, skipping", customPatternName, childPatternNode->getLine());
		}
	}
}

void CForceFeedBackSystem::LoadEnvelopes( XmlNodeRef& envelopesNode )
{
	const int predefinedEnvelopes = FORCE_FEEDBACK_PREDEFINED_ENVELOPES;
	const int envelopesCount = envelopesNode->getChildCount();

	m_envelopes.reserve((envelopesCount + predefinedEnvelopes));

	InitializePredefinedEnvelopes();

	TSamplesBuffer samplesBuffer;
	const int maxSampleCount = FFSYSTEM_MAX_ENVELOPE_SAMPLES / 2;
	float readValues[maxSampleCount];

	for (int i = 0; i < envelopesCount; ++i)
	{
		XmlNodeRef envelopeChildNode = envelopesNode->getChild(i);

		const char* customEnvelopeName = envelopeChildNode->getAttr("name");
		if (!customEnvelopeName || (customEnvelopeName[0] == '\0'))
		{
			GameWarning("ForceFeedbackSystem: Could not load envelope without name (at line %d)", envelopeChildNode->getLine());
			continue;
		}

		samplesBuffer = envelopeChildNode->haveAttr("name") ? envelopeChildNode->getAttr("samples") : "";

		int samplesFound = ParseSampleBuffer(samplesBuffer, &readValues[0], maxSampleCount);

		if (samplesFound != 0)
		{
			SEnvelope customEnvelope;
			customEnvelope.ResetToDefault();

			DistributeSamples(&readValues[0], samplesFound, &customEnvelope.m_envelopeSamples[0], FFSYSTEM_MAX_ENVELOPE_SAMPLES);

			m_envelopes.insert(TEnvelopesMap::value_type(customEnvelopeName, customEnvelope));
		}
		else
		{
			GameWarning("ForceFeedbackSystem: Envelope '%s' (at line %d) has not samples, skipping", customEnvelopeName, envelopeChildNode->getLine());
		}
	}
}

void CForceFeedBackSystem::LoadEffects( XmlNodeRef& effectsNode )
{
	const int effectsCount = effectsNode->getChildCount();

	m_effectToIndexMap.reserve(effectsCount);
	m_effects.reserve(effectsCount);

	for (int i = 0; i < effectsCount; ++i)
	{
		XmlNodeRef childEffectNode = effectsNode->getChild(i);

		SEffect newEffect;
		const int effectDataCount = childEffectNode->getChildCount();

		const char* effectName = childEffectNode->getAttr("name");

		//Check for invalid name
		if ((effectName == NULL) || (effectName[0] == '\0'))
		{
			GameWarning("ForceFeedbackSystem: Could not load effect without name (at line %d)", childEffectNode->getLine());
			continue;
		}

		//Check for duplicates
		if (m_effectToIndexMap.find(effectName) != m_effectToIndexMap.end())
		{
			GameWarning("ForceFeedbackSystem: Effect '%s' does already exists, skipping", effectName);
			continue;
		}

		childEffectNode->getAttr("time", newEffect.time);	
		newEffect.envelopeA = "";
		newEffect.envelopeB = "";
		newEffect.patternA = "";
		newEffect.patternB = "";

		for (int j = 0; j < effectDataCount; ++j)
		{
			XmlNodeRef motorNode = childEffectNode->getChild(j);

			const char* motorTag = motorNode->getTag();

			if (strcmp(motorTag, "MotorAB") == 0)
			{
				newEffect.patternA = motorNode->haveAttr("pattern") ? motorNode->getAttr("pattern") : "";
				newEffect.patternB = newEffect.patternA;
				newEffect.envelopeA = motorNode->haveAttr("envelope") ? motorNode->getAttr("envelope") : "";
				newEffect.envelopeB = newEffect.envelopeA;
				motorNode->getAttr("frequency", newEffect.frequencyA);
				newEffect.frequencyB = newEffect.frequencyA;
			}
			else if (strcmp(motorTag, "MotorA") == 0)
			{
				newEffect.patternA = motorNode->haveAttr("pattern") ? motorNode->getAttr("pattern") : "";
				newEffect.envelopeA = motorNode->haveAttr("envelope") ? motorNode->getAttr("envelope") : "";
				motorNode->getAttr("frequency", newEffect.frequencyA);
			}
			else if (strcmp(motorTag, "MotorB") == 0)
			{
				newEffect.patternB = motorNode->haveAttr("pattern") ? motorNode->getAttr("pattern") : "";
				newEffect.envelopeB = motorNode->getAttr("envelope");
				motorNode->getAttr("frequency", newEffect.frequencyB);
			}
		}

		newEffect.frequencyA = (float)__fsel(-newEffect.frequencyA, 1.0f, newEffect.frequencyA);
		newEffect.frequencyB = (float)__fsel(-newEffect.frequencyB, 1.0f, newEffect.frequencyB);
		m_effects.push_back(newEffect);
		m_effectToIndexMap.insert(TEffectToIndexMap::value_type(effectName, ((int)m_effects.size() - 1)));
	}
}

int CForceFeedBackSystem::ParseSampleBuffer(const TSamplesBuffer& buffer, float* outputValues, const int maxOutputValues)
{
	int tokenStart = 0;
	int tokenEnd = 0;
	int samplesFound = 0;

	for (int i = 0; i < maxOutputValues; ++i)
	{
		tokenEnd = buffer.find(",", tokenStart);
		if (tokenEnd != buffer.npos)
		{
			const int charCount = tokenEnd - tokenStart;
			samplesFound++;
			outputValues[i] = (float)atof(buffer.substr(tokenStart, charCount).c_str());
			tokenStart = tokenEnd + 1;
		}
		else
		{
			//Last token
			const int charCount = buffer.length() - tokenStart;
			samplesFound++;
			outputValues[i] = (float)atof(buffer.substr(tokenStart, charCount).c_str());
			break;
		}
	}

	return samplesFound;
}

void CForceFeedBackSystem::DistributeSamples(const float* sampleInput, const int sampleInputCount, uint16* sampleOutput, const int sampleOutputCount)
{
	const int sampleDistributionStep = (sampleOutputCount / sampleInputCount);
	const int sampleIterations = ((sampleInputCount % 2) == 0) ? (sampleInputCount / 2) : (sampleInputCount / 2) + 1;
	
	int lastStartOffsetIdx = 0;
	int lastEndOffsetIdx = (sampleOutputCount - 1);

	for (int i = 0; i < sampleIterations; ++i)
	{
		const int startOffsetIdx = sampleDistributionStep * i;
		const int endOffsetIdx = sampleOutputCount - 1 - startOffsetIdx;

		CRY_ASSERT((startOffsetIdx >= 0) && (startOffsetIdx < sampleOutputCount));
		CRY_ASSERT((endOffsetIdx >= 0) && (endOffsetIdx < sampleOutputCount));

		sampleOutput[startOffsetIdx] = (uint16)(clamp(sampleInput[i], 0.0f, 1.0f) * 65535.0f);
		sampleOutput[endOffsetIdx] = (uint16)(clamp(sampleInput[sampleInputCount - 1 - i], 0.0f, 1.0f) * 65535.0f);

		// Fill values in between, left side
		if (lastStartOffsetIdx < startOffsetIdx)
		{
			const int startValue = sampleOutput[lastStartOffsetIdx];
			const int step = (sampleOutput[startOffsetIdx] - sampleOutput[lastStartOffsetIdx]) / (startOffsetIdx - lastStartOffsetIdx);
			for (int j = lastStartOffsetIdx; j < startOffsetIdx; ++j)
			{
				sampleOutput[j] = startValue + ((j - lastStartOffsetIdx) * step);
			}
		}

		// ...and right side
		if (endOffsetIdx < lastEndOffsetIdx)
		{
			const int startValue = sampleOutput[endOffsetIdx];
			const int step = (sampleOutput[lastEndOffsetIdx] - sampleOutput[endOffsetIdx]) / (lastEndOffsetIdx - endOffsetIdx);
			for (int j = endOffsetIdx; j < lastEndOffsetIdx; ++j)
			{
				sampleOutput[j] = startValue + ((j - endOffsetIdx) * step);
			}
		}

		//Last iteration, requires to fill the remaining ones in the middle
		if (i == (sampleIterations - 1))
		{
			if (startOffsetIdx < endOffsetIdx)
			{
				const int startValue = sampleOutput[startOffsetIdx];
				const int step = (sampleOutput[endOffsetIdx] - sampleOutput[startOffsetIdx]) / (endOffsetIdx - startOffsetIdx);
				for (int j = startOffsetIdx; j < endOffsetIdx; ++j)
				{
					sampleOutput[j] = startValue + ((j - startOffsetIdx) * step);
				}
			}
		}

		lastStartOffsetIdx = startOffsetIdx;
		lastEndOffsetIdx = endOffsetIdx;

	}
}

#ifdef DEBUG_FORCEFEEDBACK_SYSTEM

void CForceFeedBackSystem::DebugFFOutput( const SFFOutput& output )
{
	if(m_cvars.ffs_debug == 0)
		return;

	if (m_pDebugHistoryManager == NULL)
	 m_pDebugHistoryManager= CCryAction::GetCryAction()->CreateDebugHistoryManager();

	CRY_ASSERT(m_pDebugHistoryManager);

	m_pDebugHistoryManager->LayoutHelper("FFMotorA_HighFreq", NULL, true, -20, 20, 0, 1, 0.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("FFMotorB_LowFreq", NULL, true, -20, 20, 0, 1, 1.0f, 0.0f);

	IDebugHistory* pDHMotorA = m_pDebugHistoryManager->GetHistory("FFMotorA_HighFreq");
	if (pDHMotorA != NULL)
	{
		pDHMotorA->AddValue(output.GetClampedFFA());
	}

	IDebugHistory* pDHMotorB = m_pDebugHistoryManager->GetHistory("FFMotorB_LowFreq");
	if (pDHMotorB != NULL)
	{
		pDHMotorB->AddValue(output.GetClampedFFB());
	}
}

#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

namespace
{
	void FFSReload(IConsoleCmdArgs* pArgs)
	{
		CForceFeedBackSystem* pFFS = static_cast<CForceFeedBackSystem*>(CCryAction::GetCryAction()->GetIForceFeedbackSystem());

		if(pFFS)
		{
			pFFS->Reload();
		}
	}

	void FFSPlayEffect(IConsoleCmdArgs* pArgs)
	{
		CForceFeedBackSystem* pFFS = static_cast<CForceFeedBackSystem*>(CCryAction::GetCryAction()->GetIForceFeedbackSystem());

		if(pFFS)
		{
			if(pArgs->GetArgCount() == 2)
			{
				pFFS->PlayForceFeedbackEffect(pArgs->GetArg(1) , 1.0f);
			}
		}
	}

	void FFSStopAllEffects(IConsoleCmdArgs* pArgs)
	{
		CForceFeedBackSystem* pFFS = static_cast<CForceFeedBackSystem*>(CCryAction::GetCryAction()->GetIForceFeedbackSystem());

		if(pFFS)
		{
			pFFS->StopAllEffects();
		}
	}

};

SForceFeedbackSystemCVars::SForceFeedbackSystemCVars()
{
	REGISTER_CVAR(ffs_debug, 0, 0, "Turns on/off force feedback system debug." );

	REGISTER_COMMAND("ffs_PlayEffect", FFSPlayEffect, 0, "Play force feedback effect, passed by name as first parameter");
	REGISTER_COMMAND("ffs_StopAllEffects", FFSStopAllEffects, 0, "Stop force feedback effect, passed by name as first parameter");
	REGISTER_COMMAND("ffs_Reload", FFSReload, 0, "Reload force feedback system data");
}

SForceFeedbackSystemCVars::~SForceFeedbackSystemCVars()
{
	IConsole *pConsole = gEnv->pConsole;

	if (pConsole)
	{
		pConsole->UnregisterVariable("ffs_debug", true);

		pConsole->RemoveCommand("ffs_PlayEffect");
		pConsole->RemoveCommand("ffs_StopEffect");
		pConsole->RemoveCommand("ffs_Reload");
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CForceFeedBackSystem::SFFOutput CForceFeedBackSystem::SActiveEffect::Update( float frameTime )
{
	SFFOutput effectFF;

	bool isLoopingEffect = (effectTime <= 0.0f); 

	const float effectTimeInv = !isLoopingEffect ? (float)__fres(effectTime) : 1.0f;
	const float sampleTime = runningTime * effectTimeInv;

	const float sampleTimeAAtFreq = sampleTime * frequencyA;
	const float sampleTimeAAtFreqNorm = clamp(sampleTimeAAtFreq - cry_floorf(sampleTimeAAtFreq), 0.0f, 1.0f);

	const float sampleTimeBAtFreq = sampleTime * frequencyB;
	const float sampleTimeBAtFreqNorm = clamp(sampleTimeBAtFreq - cry_floorf(sampleTimeBAtFreq), 0.0f, 1.0f);

	effectFF.forceFeedbackA = m_patternA.SamplePattern(sampleTimeAAtFreqNorm) * m_envelopeA.SampleEnvelope(sampleTime) * intensity;
	effectFF.forceFeedbackB = m_patternB.SamplePattern(sampleTimeBAtFreqNorm) * m_envelopeB.SampleEnvelope(sampleTime) * intensity;
	runningTime += frameTime;

	//Re-start the loop
	if (isLoopingEffect)
	{
		runningTime = (float)__fsel(1.0f - runningTime, runningTime, 0.0f);
	}

	return effectFF;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// calls callback once for each effect

void CForceFeedBackSystem::EnumerateEffects( IFFSPopulateCallBack* pCallBack )
{
	TEffectToIndexMap::iterator iter = m_effectToIndexMap.begin();

	while (iter!=m_effectToIndexMap.end())
	{
		const char* const pName = iter->first.c_str();
		pCallBack->AddFFSEffectName( pName );
		++iter;
	}
}


#include UNIQUE_VIRTUAL_WRAPPER(IForceFeedbackSystem) 