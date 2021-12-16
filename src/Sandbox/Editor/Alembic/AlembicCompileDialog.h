//////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012
// ------------------------------------------------------------------------
//  File name:   AlembicCompileDialog.cpp
//  Created:     26/10/2012 by Axel Gneiting
//
////////////////////////////////////////////////////////////////////////////

#pragma once

class CAlembicCompileDialog : public CDialog
{
	DECLARE_DYNAMIC(CAlembicCompileDialog)

public:
	CAlembicCompileDialog(const XmlNodeRef config);

	virtual BOOL OnInitDialog() override;
	
	CString GetUpAxis() const;
	CString GetPlaybackFromMemory() const;
	CString GetBlockCompressionFormat() const;
	CString GetMeshPrediction() const;	
	CString GetUseBFrames() const;
	uint GetIndexFrameDistance() const;
	double GetPositionPrecision() const;

private:
	struct SConfig
	{
		SConfig() : m_upAxis("Y"), m_playbackFromMemory("0"),
			m_blockCompressionFormat("deflate"), m_meshPrediction("1"), 
			m_useBFrames("1"), m_indexFrameDistance(10), m_positionPrecision(1.0) {}

		bool operator ==(const SConfig &other) const
		{
			return m_blockCompressionFormat == other.m_blockCompressionFormat
				&& m_upAxis == other.m_upAxis
				&& m_playbackFromMemory == other.m_playbackFromMemory
				&& m_meshPrediction == other.m_meshPrediction
				&& m_useBFrames == other.m_useBFrames
				&& m_indexFrameDistance == other.m_indexFrameDistance
				&& m_positionPrecision == other.m_positionPrecision;
		}

		CString m_name;
		CString m_blockCompressionFormat;
		CString m_upAxis;
		CString m_playbackFromMemory;
		CString m_meshPrediction;
		CString m_useBFrames;
		uint m_indexFrameDistance;
		double m_positionPrecision;
	};

	void SetFromConfig(const SConfig &config);
	void UpdateEnabledStates();
	void UpdatePresetSelection();
	SConfig LoadConfig(const CString &fileName, XmlNodeRef xml) const;	

	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnRadioYUp();
	afx_msg void OnRadioZUp();
	afx_msg void OnPlaybackFromMemory();
	afx_msg void OnBlockCompressionSelected();
	afx_msg void OnMeshPredictionCheckBox();
	afx_msg void OnUseBFramesCheckBox();
	afx_msg void OnIndexFrameDistanceChanged();	
	afx_msg void OnPositionPrecisionChanged();
	afx_msg void OnPresetSelected();

	CComboBox m_presetComboBox;
	CComboBox m_blockCompressionFormatCombo;
	CButton m_yUpRadio;
	CButton m_zUpRadio;
	CButton m_playbackFromMemoryCheckBox;
	CButton m_meshPredictionCheckBox;
	CButton m_useBFramesCheckBox;	
	CEdit m_indexFrameDistanceEdit;
	CEdit m_positionPrecisionEdit;

	SConfig m_config;
	std::vector<SConfig> m_presets;

	DECLARE_MESSAGE_MAP()
};