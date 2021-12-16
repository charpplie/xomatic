//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : TVCustomizeTrackColors.h
//  Author           : Jaewon Jung
//  Time of creation : 9/20/2011   12:04
//  Compilers        : VS2010
//  Description      : A dialog for customizing track colors
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __TV_CUSTOMIZE_TRACK_COLORS_DLG_H__
#define __TV_CUSTOMIZE_TRACK_COLORS_DLG_H__

#pragma once

class CTVCustomizeTrackColorsDlg : public CDialog
{
	friend class CTrackViewDialog;
public:
	CTVCustomizeTrackColorsDlg(CWnd *pParent=NULL);
	virtual ~CTVCustomizeTrackColorsDlg();

	enum { IDD = IDD_TV_CUSTOMIZETRACKCOLORS };

	static COLORREF GetTrackColor(CAnimParamType paramType)
	{
		auto itr = s_trackColors.find(paramType);
		if(itr != end(s_trackColors))
			return itr->second;
		else
			return s_colorForOthers;
	}
	static COLORREF GetColorForDisabledTracks()
	{ return s_colorForDisabled; }
	static COLORREF GetColorForMutedTracks()
	{ return s_colorForMuted; }

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnApply();
	afx_msg void OnResetAll();
	afx_msg void OnExport();
	afx_msg void OnImport();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

	void Export(const char *fullPath) const;
	bool Import(const char *fullPath);

	static void SaveColors(const char *sectionName);
	static void LoadColors(const char *sectionName);

	CStatic *m_aLabels;
	
	static std::map<CAnimParamType, COLORREF> s_trackColors;
	static COLORREF s_colorForDisabled;
	static COLORREF s_colorForMuted;
	static COLORREF s_colorForOthers;
};

#endif // __TV_CUSTOMIZE_TRACK_COLORS_DLG_H__