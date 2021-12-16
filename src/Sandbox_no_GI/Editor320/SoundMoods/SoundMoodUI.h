//같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같�
//  File name:      SoundMoodUI.h
//  Version:        v1.00
//  Last modified:  (11/07/05)
//  Compilers:      Visual C++.NET
//  Description:    
// -------------------------------------------------------------------------
//  Author: Tomas Neumann
// -------------------------------------------------------------------------
//
//  You are not permitted to distribute, sell or use any part of
//  this source for your software without written permision of author.
//
//같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같�

#ifndef __SoundMoodUI__
#define __SoundMoodUI__
#pragma once

//#include "Util/Variable.h"

//////////////////////////////////////////////////////////////////////////
// Base class for SoundMood UIs
class CSoundMoodUI
{
public:
	CVariableArray m_mainTable;
	CVarBlockPtr m_vars;
	IVariable::OnSetCallback m_onSetCallback;

	virtual CVarBlock* CreateVars() = 0;
protected:
	//////////////////////////////////////////////////////////////////////////
	void AddVariable( CVariableArray &varArray,CVariableBase &var,const char *varName,unsigned char dataType=IVariable::DT_SIMPLE )
	{
		var.AddRef(); // Variables are local and must not be released by CVarBlock.
		if (varName)
			var.SetName(varName);
		var.SetDataType(dataType);
		if (m_onSetCallback)
			var.AddOnSetCallback(m_onSetCallback);
		varArray.AddChildVar(&var);
	}
	//////////////////////////////////////////////////////////////////////////
	void AddVariable( CVarBlock *vars,CVariableBase &var,const char *varName,unsigned char dataType=IVariable::DT_SIMPLE )
	{
		var.AddRef(); // Variables are local and must not be released by CVarBlock.
		if (varName)
			var.SetName(varName);
		var.SetDataType(dataType);
		if (m_onSetCallback)
			var.AddOnSetCallback(m_onSetCallback);
		vars->AddVariable(&var);
	}
};

//////////////////////////////////////////////////////////////////////////
class CSoundMoodUI_CatProps : public CSoundMoodUI
{
public:
	
	// default properties
	CVariableArray m_Properties;
	CVariable<float> m_fVolume;
	CVariable<float> m_fPitch;
	// Lowpass
	CVariable<float> m_fLowPassCutoff;
	// Highpass
	CVariable<float> m_fHighPassCutoff;
	CVariable<float> m_fHighPassResonance;
	// Compressor
	CVariable<float> m_fCompressorThreshold;
	CVariable<float> m_fCompressorAttack;
	CVariable<float> m_fCompressorRelease;
	CVariable<float> m_fCompressorGainMakeup;
	// ParamEQ1
	CVariable<float> m_fParamEQ1Gain;
	CVariable<float> m_fParamEQ1Center;
	CVariable<float> m_fParamEQ1Bandwidth;
	// ParamEQ2
	CVariable<float> m_fParamEQ2Gain;
	CVariable<float> m_fParamEQ2Center;
	CVariable<float> m_fParamEQ2Bandwidth;

	//CVariable<int> m_nProbability;
	//CVariable<int> m_nTimeOutMS;
	//CVariable<bool> m_bLooped;
	//CVariable<CString> m_sFileName;

	//////////////////////////////////////////////////////////////////////////
	CVarBlock* CreateVars()
	{
		if (m_vars)
			return m_vars;
		m_vars = new CVarBlock;
		
		// Properties
		AddVariable( m_vars,m_Properties,"Properties" );
		AddVariable( m_Properties,m_fVolume,"Volume [0..1]" );
		m_fVolume.SetLimits(0.0f, 1.0f);
		m_fVolume.Set(1.0f);
		AddVariable( m_Properties,m_fPitch,"Pitch [-4..4]" );
		m_fPitch.SetLimits(-4.0f, 4.0f);
		// Lowpass
		AddVariable( m_Properties,m_fLowPassCutoff,"LowPass Filter Cutoff [10..22,000]" );
		m_fLowPassCutoff.SetLimits(10.0f, 22000.0f);
		// Highpass
		AddVariable( m_Properties,m_fHighPassCutoff,"HighPass Filter Cutoff [10..22,000]" );
		m_fHighPassCutoff.SetLimits(10.0f, 22000.0f);
		AddVariable( m_Properties,m_fHighPassResonance,"HighPass Filter Resonance [1..10]" );
		m_fHighPassResonance.SetLimits(1.0f, 10.0f);
		// Compressor
		AddVariable( m_Properties,m_fCompressorThreshold,"Compressor Threshold [-60..0]" );
		m_fCompressorThreshold.SetLimits(-60.0f, 0.0f);
		AddVariable( m_Properties,m_fCompressorAttack,"Compressor Attack [10..200]" );
		m_fCompressorAttack.SetLimits(10.0f, 200.0f);
		AddVariable( m_Properties,m_fCompressorRelease,"Compressor Release [20..1000]" );
		m_fCompressorRelease.SetLimits(20.0f, 1000.0f);
		AddVariable( m_Properties,m_fCompressorGainMakeup,"Compressor GainMakeup [0..30]" );
		m_fCompressorGainMakeup.SetLimits(0.0f, 30.0f);
		// ParamEQ1
		AddVariable( m_Properties,m_fParamEQ1Gain,"ParamEQ-1 Gain [0.05..3]" );
		m_fParamEQ1Gain.SetLimits(0.05f, 3.0f);
		AddVariable( m_Properties,m_fParamEQ1Center,"ParamEQ-1 Center [20..22000]" );
		m_fParamEQ1Center.SetLimits(20.0f, 22000.0f);
		AddVariable( m_Properties,m_fParamEQ1Bandwidth,"ParamEQ-1 Bandwidth [0.2..5]" );
		m_fParamEQ1Bandwidth.SetLimits(0.2f, 5.0f);
		// ParamEQ2
		AddVariable( m_Properties,m_fParamEQ2Gain,"ParamEQ-2 Gain [0.05..3]" );
		m_fParamEQ2Gain.SetLimits(0.05f, 3.0f);
		AddVariable( m_Properties,m_fParamEQ2Center,"ParamEQ-2 Center [20..22000]" );
		m_fParamEQ2Center.SetLimits(20.0f, 22000.0f);
		AddVariable( m_Properties,m_fParamEQ2Bandwidth,"ParamEQ-2 Bandwidth [0.2..5]" );
		m_fParamEQ2Bandwidth.SetLimits(0.2f, 5.0f);

		//AddVariable( m_Properties,m_sFileName,"FileName",IVariable::DT_FILE );
		//AddVariable( m_Properties,m_bLooped,"Looped" );

		return m_vars;
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes the current state of SoundGroups to the UI
	void ReadFromPtr( ICategory *pElement )
	{
		enumSG_SEARCHDIRECTION eSearchDirection = eELEMENTONLY;
		//int										nFlags		= 0;
		//int32									nIntValue = 0;
		//ptParamINT32					ParamInt32(nIntValue);
		//string								sStringValue;
		//ptParamCRYSTRING			ParamString(sStringValue);
		//bool									bBoolValue = true;
		//ptParamBOOL						ParamBool(bBoolValue);
		
		// Volume
		{
			f32 							fValue = 1.0f;
			ptParamF32				ParamF32(fValue);

			//if (pElement->GetParam(gspFVOLUME, &ParamF32))

			// TODO Fix n / f
			pElement->GetParam(gspFVOLUME, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fVolume.Set( fValue );
		}

		// Pitch
		{
			f32 							fValue = 0.0f;
			ptParamF32				ParamF32(fValue);

			//if (pElement->GetParam(gspFPITCH, &ParamF32))

				// TODO Fix n / f
			pElement->GetParam(gspFPITCH, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fPitch.Set( fValue );
		}


		// LowPass Cutoff
		{
			f32 							fValue = 22000.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFLOWPASSCUTOFF, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fLowPassCutoff.Set( fValue );
		}

		// HighPass Cutoff
		{
			f32 							fValue = 10.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFHIGHPASSCUTOFF, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fHighPassCutoff.Set( fValue );
		}

		// HighPass Resonance
		{
			f32 							fValue = 1.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFHIGHPASSRESONANCE, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fHighPassResonance.Set( fValue );
		}

		// Compressor Threshold
		{
			f32 							fValue = 0.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFCOMPRESSORTHRESHOLD, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fCompressorThreshold.Set( fValue );
		}

		// Compressor Attack
		{
			f32 							fValue = 50.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFCOMPRESSORATTACK, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fCompressorAttack.Set( fValue );
		}

		// Compressor Release
		{
			f32 							fValue = 50.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFCOMPRESSORRELEASE, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fCompressorRelease.Set( fValue );
		}

		// Compressor GainMakeup
		{
			f32 							fValue = 0.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFCOMPRESSORGAINMAKEUP, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fCompressorGainMakeup.Set( fValue );
		}

		// ParamEQ1 Gain
		{
			f32 							fValue = 1.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFPARAMEQ1GAIN, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fParamEQ1Gain.Set( fValue );
		}

		// ParamEQ1 Center
		{
			f32 							fValue = 8000.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFPARAMEQ1CENTER, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fParamEQ1Center.Set( fValue );
		}

		// ParamEQ1 Bandwidth
		{
			f32 							fValue = 1.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFPARAMEQ1BANDWIDTH, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fParamEQ1Bandwidth.Set( fValue );
		}

		// ParamEQ2 Gain
		{
			f32 							fValue = 1.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFPARAMEQ2GAIN, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fParamEQ2Gain.Set( fValue );
		}

		// ParamEQ2 Center
		{
			f32 							fValue = 8000.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFPARAMEQ2CENTER, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fParamEQ2Center.Set( fValue );
		}

		// ParamEQ2 Bandwidth
		{
			f32 							fValue = 1.0f;
			ptParamF32				ParamF32(fValue);

			pElement->GetParam(gspFPARAMEQ2BANDWIDTH, &ParamF32);
			ParamF32.GetValue(fValue);
			m_fParamEQ2Bandwidth.Set( fValue );
		}

	}

	//////////////////////////////////////////////////////////////////////////
	// Gives the current state of the UI to the SoundGroup manager
	ICategory* WriteToPtr( ICategory *pElement )
	{
		XmlNodeRef ElementNode = CreateXmlNode("Category");
		ElementNode->setAttr("Name", pElement->GetName());
		f32 fValue = 0.0f;

		// VolumeDB
		// insert parameter to xml node
		fValue = 1.0f;
		m_fVolume.Get(fValue);
		ElementNode->setAttr("Volume", fValue);

		// Priority
		// insert parameter to xml node
		fValue = 0.0f;
		m_fPitch.Get(fValue);
		ElementNode->setAttr("Pitch", fValue);

		// Lowpass
		fValue = 22000.0f;
		m_fLowPassCutoff.Get(fValue);
		ElementNode->setAttr("LowPassCutoff", fValue);

		// Highpass
		fValue = 10.0f;
		m_fHighPassCutoff.Get(fValue);
		ElementNode->setAttr("HighPassCutoff", fValue);

		fValue = 1.0f;
		m_fHighPassResonance.Get(fValue);
		ElementNode->setAttr("HighPassResonance", fValue);

		// Compressor
		fValue = 0.0f;
		m_fCompressorThreshold.Get(fValue);
		ElementNode->setAttr("CompressorThreshold", fValue);

		fValue = 50.0f;
		m_fCompressorAttack.Get(fValue);
		ElementNode->setAttr("CompressorAttack", fValue);

		fValue = 50.0f;
		m_fCompressorRelease.Get(fValue);
		ElementNode->setAttr("CompressorRelease", fValue);

		fValue = 0.0f;
		m_fCompressorGainMakeup.Get(fValue);
		ElementNode->setAttr("CompressorGainMakeup", fValue);

		// ParamEQ1
		fValue = 1.0f;
		m_fParamEQ1Gain.Get(fValue);
		ElementNode->setAttr("ParamEQ1Gain", fValue);
		
		fValue = 8000.0f;
		m_fParamEQ1Center.Get(fValue);
		ElementNode->setAttr("ParamEQ1Center", fValue);

		fValue = 1.0f;
		m_fParamEQ1Bandwidth.Get(fValue);
		ElementNode->setAttr("ParamEQ1Bandwidth", fValue);

		// ParamEQ2
		fValue = 1.0f;
		m_fParamEQ2Gain.Get(fValue);
		ElementNode->setAttr("ParamEQ2Gain", fValue);

		fValue = 8000.0f;
		m_fParamEQ2Center.Get(fValue);
		ElementNode->setAttr("ParamEQ2Center", fValue);

		fValue = 1.0f;
		m_fParamEQ2Bandwidth.Get(fValue);
		ElementNode->setAttr("ParamEQ2Bandwidth", fValue);

		pElement->Serialize(ElementNode, true);
		
		ISoundMoodManager *pManager = pElement->GetIMoodManager();

		if (pManager)
			pManager->Update();
		
		return pElement;
	}
};


//////////////////////////////////////////////////////////////////////////
class CSoundMoodUI_MoodProps : public CSoundMoodUI
{
public:

	// default properties
	CVariable<float> m_fPriority;
	CVariable<float> m_fMusicVolume;

	//////////////////////////////////////////////////////////////////////////
	CVarBlock* CreateVars()
	{
		if (m_vars)
			return m_vars;
		m_vars = new CVarBlock;

		// Properties
		AddVariable( m_vars,m_fPriority,"Priority" );
		m_fPriority.SetLimits(1.0f, 100.0f);
		m_fPriority.Set(1.0f);

		AddVariable( m_vars,m_fMusicVolume,"MusicVolume" );
		m_fMusicVolume.SetLimits(0.0f, 1.0f);
		m_fMusicVolume.Set(0.8f);

		return m_vars;
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes the current state of SoundGroups to the UI
	void ReadFromPtr( IMood *pElement )
	{
		m_fPriority.Set(pElement->GetPriority());
		m_fMusicVolume.Set(pElement->GetMusicVolume());
	}

	//////////////////////////////////////////////////////////////////////////
	// Gives the current state of the UI to the SoundGroup manager
	IMood* WriteToPtr( IMood *pElement )
	{
		XmlNodeRef ElementNode = CreateXmlNode("Mood");
		ElementNode->setAttr("Name", pElement->GetName());

		float fValue = 1.0f;
		m_fPriority.Get(fValue);
		ElementNode->setAttr("Priority", fValue);

		fValue = 1.0f;
		m_fMusicVolume.Get(fValue);
		ElementNode->setAttr("MusicVolume", fValue);

		pElement->Serialize(ElementNode, true);
		return pElement;
	}
};

#endif
