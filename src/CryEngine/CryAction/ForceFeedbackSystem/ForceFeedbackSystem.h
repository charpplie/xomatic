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

#pragma once 

#ifndef _FORCEFEEDBACKSYSTEM_H_
#define _FORCEFEEDBACKSYSTEM_H_

#include "../IForceFeedbackSystem.h"

#if !defined(_RELEASE)
#define DEBUG_FORCEFEEDBACK_SYSTEM
#endif

#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
struct IDebugHistoryManager;
#endif

#define FFSYSTEM_MAX_PATTERN_SAMPLES 32
#define FFSYSTEM_MAX_PATTERN_SAMPLES_FLOAT 32.0f
#define FFSYSTEM_PATTERN_SAMPLE_STEP 0.03125f

#define FFSYSTEM_MAX_ENVELOPE_SAMPLES 16
#define FFSYSTEM_MAX_ENVELOPE_SAMPLES_FLOAT 16.0f
#define FFSYSTEM_ENVELOPE_SAMPLE_STEP 0.0625f

#define FFSYSTEM_UINT16_TO_FLOAT 0.0000152590218967f

struct SForceFeedbackSystemCVars
{
	SForceFeedbackSystemCVars();
	~SForceFeedbackSystemCVars();

	int ffs_debug;
};

class CForceFeedBackSystem : public IForceFeedbackSystem
{
private:

	struct SPattern
	{
		float SamplePattern(float time) const
		{
			assert((time >= 0.0f) && (time <= 1.0f));
			
			const float fSampleIdx = cry_floorf(time / FFSYSTEM_PATTERN_SAMPLE_STEP);
			int sampleIdx1 = (int)fSampleIdx;
			assert((sampleIdx1 >= 0) && (sampleIdx1 < FFSYSTEM_MAX_PATTERN_SAMPLES));
			int sampleIdx2 = (sampleIdx1 >= (FFSYSTEM_MAX_PATTERN_SAMPLES - 1)) ?  0 : sampleIdx1 + 1;
			const float delta = clamp((FFSYSTEM_PATTERN_SAMPLE_STEP - (time - (FFSYSTEM_PATTERN_SAMPLE_STEP * fSampleIdx))) * FFSYSTEM_MAX_PATTERN_SAMPLES_FLOAT, 0.0f, 1.0f);

			return ((SampleToFloat(sampleIdx1) * delta) + (SampleToFloat(sampleIdx2) * (1.0f - delta)));
		}

		ILINE float SampleToFloat(int idx) const
		{
			return ((float)m_patternSamples[idx] * FFSYSTEM_UINT16_TO_FLOAT);
		}

		void ResetToDefault()
		{
			for (int i = 0; i < FFSYSTEM_MAX_PATTERN_SAMPLES; ++i)
			{
				m_patternSamples[i] = 0x0000;
			}
		}

		uint16 m_patternSamples[FFSYSTEM_MAX_PATTERN_SAMPLES];
	};

	typedef VectorMap<string, SPattern> TPatternsMap;

	struct SEnvelope
	{
		float SampleEnvelope(float time) const
		{
			assert((time >= 0.0f) && (time <= 1.0f));

			const float fSampleIdx = cry_floorf(time / FFSYSTEM_ENVELOPE_SAMPLE_STEP);
			int sampleIdx1 = (int)fSampleIdx;
			sampleIdx1 = (sampleIdx1 >= (FFSYSTEM_MAX_ENVELOPE_SAMPLES - 1)) ? (FFSYSTEM_MAX_ENVELOPE_SAMPLES - 2) : sampleIdx1;
			int sampleIdx2 = sampleIdx1 + 1;
			const float delta = clamp((FFSYSTEM_ENVELOPE_SAMPLE_STEP - (time - (FFSYSTEM_ENVELOPE_SAMPLE_STEP * fSampleIdx))) * FFSYSTEM_MAX_ENVELOPE_SAMPLES_FLOAT, 0.0f, 1.0f);

			return ((SampleToFloat(sampleIdx1) * delta) + (SampleToFloat(sampleIdx2) * (1.0f - delta)));
		}

		ILINE float SampleToFloat(int idx) const
		{
			return ((float)m_envelopeSamples[idx] * FFSYSTEM_UINT16_TO_FLOAT);
		}

		void ResetToDefault()
		{
			for (int i = 0; i < FFSYSTEM_MAX_ENVELOPE_SAMPLES; ++i)
			{
				m_envelopeSamples[i] = (0x0000);
			}
		}

		uint16 m_envelopeSamples[FFSYSTEM_MAX_ENVELOPE_SAMPLES];
	};

	typedef VectorMap<string, SEnvelope> TEnvelopesMap;

	struct SEffect
	{
		SEffect()
			: time(1.0f)
			, frequencyA(1.0f)
			, frequencyB(1.0f)
		{

		}

		string patternA;
		string envelopeA;
		string patternB;
		string envelopeB;

		float frequencyA;
		float frequencyB;
		float time;
	};

	typedef VectorMap<string, int> TEffectToIndexMap;
	typedef std::vector<SEffect>	TEffectArray;

	struct SFFOutput
	{
		SFFOutput()
			: forceFeedbackA(0.0f)
			, forceFeedbackB(0.0f)
		{

		}

		void operator += (const SFFOutput& operand2)
		{
			forceFeedbackA += operand2.forceFeedbackA;
			forceFeedbackB += operand2.forceFeedbackB;
		}

		ILINE float GetClampedFFA() const
		{
			return clamp(forceFeedbackA, 0.0f, 1.0f);
		}

		ILINE float GetClampedFFB() const
		{
			return clamp(forceFeedbackB, 0.0f, 1.0f);
		}

		ILINE void ZeroIt()
		{
			forceFeedbackA = forceFeedbackB = 0.0f;
		}

		float forceFeedbackA;
		float forceFeedbackB;
	};


	struct SActiveEffect
	{
		SActiveEffect()
			: effectIdx(-1)
			, effectTime(1.0f)
			, runningTime(0.0f)
			, frequencyA(1.0f)
			, frequencyB(1.0f)
			, intensity(1.0f)
		{
			
		}

		ILINE bool HasFinished() const
		{
			return (effectTime > 0.0f) ? (runningTime > effectTime) : false;
		}

		SFFOutput Update(float frameTime);

		int effectIdx;
		float effectTime;
		float runningTime;
		float frequencyA;
		float frequencyB;
		float intensity;
		SPattern m_patternA;
		SEnvelope m_envelopeA;
		SPattern m_patternB;
		SEnvelope m_envelopeB;
	};

	typedef std::vector<SActiveEffect> TActiveEffectsArray;

	typedef CryFixedStringT<128>	TSamplesBuffer;

public:

	CForceFeedBackSystem();
	~CForceFeedBackSystem();

	//IForceFeedbackSystem
	VIRTUAL void PlayForceFeedbackEffect(const char* effectName, const float intensity);
	VIRTUAL void StopForceFeedbackEffect(const char* effectName);
	VIRTUAL void StopAllEffects();

	VIRTUAL void AddFrameCustomForceFeedback(const float amplifierA, const float amplifierB);
	VIRTUAL void EnumerateEffects( IFFSPopulateCallBack* pCallBack );  // intended to be used only from the editor
	//~IForceFeedbackSystem

	void Initialize();
	void Reload();

	void Update(float frameTime);

private:

	SFFOutput UpdateEffect(SActiveEffect &effect, float frameTime);

	void InitializePredefinedPatterns();
	void InitializePredefinedEnvelopes();

	void LoadXmlData();

	void LoadPatters(XmlNodeRef& patternsNode);
	void LoadEnvelopes(XmlNodeRef& envelopesNode);
	void LoadEffects(XmlNodeRef& effectsNode);

	int ParseSampleBuffer(const TSamplesBuffer& buffer, float* outputValues, const int maxOutputValues);
	void DistributeSamples(const float* sampleInput, const int sampleInputCount, uint16* sampleOutput, const int sampleOutputCount);

	void UpdateInputSystem(const float amplifierA, const float amplifierB);

#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
	void DebugFFOutput(const SFFOutput& output);
#endif

	SPattern	m_defaultPattern;
	SEnvelope m_defaultEnvelope;

	TPatternsMap m_patters;
	TEnvelopesMap m_envelopes;

	TEffectToIndexMap m_effectToIndexMap;
	TEffectArray			m_effects;
	TActiveEffectsArray m_activeEffects;

	SFFOutput m_frameCustomForceFeedback;

	SForceFeedbackSystemCVars m_cvars;

#ifdef DEBUG_FORCEFEEDBACK_SYSTEM
	IDebugHistoryManager* m_pDebugHistoryManager;
#endif
};
#endif