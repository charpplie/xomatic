//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : TVCustomizeTrackColors.cpp
//  Author           : Jaewon Jung
//  Time of creation : 9/20/2011   12:04
//  Compilers        : VS2010
//  Description      : A dialog for customizing track colors
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <algorithm>
#include "TVCustomizeTrackColorsDlg.h"
#include "TrackViewDialog.h"
#define max(a,b) (((a) > (b)) ? (a) : (b))
#include <afxcolorbutton.h>
#undef max

#define TRACKCOLOR_ENTRY_PREFIX _T("TrackColor")
#define TRACKCOLOR_FOR_OTHERS_ENTRY _T("TrackColorForOthers")
#define TRACKCOLOR_FOR_DISABLED_ENTRY _T("TrackColorForDisabled")
#define TRACKCOLOR_FOR_MUTED_ENTRY _T("TrackColorForMuted")

struct STrackEntry
{
	CAnimParamType paramType;
	const char *name;
	COLORREF defaultColor;
};

namespace
{
	const STrackEntry g_trackEntries[] = {
		// Color for tracks
		{ eAnimParamType_FOV, "FOV", RGB(220,220,220) },
		{ eAnimParamType_Position, "Pos", RGB(90,150,90) },
		{ eAnimParamType_Rotation, "Rot", RGB(90,150,90) },
		{ eAnimParamType_Scale, "Scale", RGB(90,150,90) },
		{ eAnimParamType_Event, "Event", RGB(220,220,220) },
		{ eAnimParamType_Visibility, "Visibility", RGB(220,220,220) },
		{ eAnimParamType_Camera, "Camera", RGB(220,220,220) },
		{ eAnimParamType_Sound, "Sound", RGB(220,220,220) },
		{ eAnimParamType_Animation, "Animation", RGB(220,220,220) },
		{ eAnimParamType_Sequence, "Sequence", RGB(220,220,220) },
		{ eAnimParamType_Expression, "Expression", RGB(220,220,220) },
		{ eAnimParamType_Console, "Console", RGB(220,220,220) },
		{ eAnimParamType_Music, "Music", RGB(220,220,220) },
		{ eAnimParamType_FaceSequence, "FaceSequence", RGB(220,220,220) },
		{ eAnimParamType_LookAt, "LookAt", RGB(220,220,220) },
		{ eAnimParamType_TrackEvent, "TrackEvent", RGB(220,220,220) },
		{ eAnimParamType_ShakeMultiplier, "ShakeMult", RGB(90,150,90) },
		{ eAnimParamType_TransformNoise, "Noise", RGB(90,150,90) },
		{ eAnimParamType_TimeWarp, "Timewarp", RGB(220,220,220) },
		{ eAnimParamType_FixedTimeStep, "FixedTimeStep", RGB(220,220,220) },
		{ eAnimParamType_DepthOfField, "DepthOfField", RGB(90,150,90) },
		{ eAnimParamType_CommentText, "CommentText", RGB(220,220,220) },
		{ eAnimParamType_ScreenFader, "ScreenFader", RGB(220,220,220) },
		{ eAnimParamType_LightDiffuse, "LightDiffuseColor", RGB(90,150,90) },
		{ eAnimParamType_LightRadius, "LightRadius", RGB(220,220,220) },
		{ eAnimParamType_LightDiffuseMult, "LightDiffuseMult", RGB(220,220,220) },
		{ eAnimParamType_LightHDRDynamic, "LightHDRDynamic", RGB(220,220,220) },
		{ eAnimParamType_LightSpecularMult, "LightSpecularMult", RGB(220,220,220) },
		{ eAnimParamType_LightSpecPercentage, "LightSpecularPercent", RGB(220,220,220) },
		{ eAnimParamType_FocusDistance, "FocusDistance", RGB(220,220,220) },
		{ eAnimParamType_FocusRange, "FocusRange", RGB(220,220,220) },
		{ eAnimParamType_BlurAmount, "BlurAmount", RGB(220,220,220) },
		{ eAnimParamType_PositionX, "PosX", RGB(220,220,220) },
		{ eAnimParamType_PositionY, "PosY", RGB(220,220,220) },
		{ eAnimParamType_PositionZ, "PosZ", RGB(220,220,220) },
		{ eAnimParamType_RotationX, "RotX", RGB(220,220,220) },
		{ eAnimParamType_RotationY, "RotY", RGB(220,220,220) },
		{ eAnimParamType_RotationZ, "RotZ", RGB(220,220,220) },
		{ eAnimParamType_ScaleX, "ScaleX", RGB(220,220,220) },
		{ eAnimParamType_ScaleY, "ScaleY", RGB(220,220,220) },
		{ eAnimParamType_ScaleZ, "ScaleZ", RGB(220,220,220) },
		{ eAnimParamType_ShakeAmpAMult, "ShakeMultAmpA", RGB(220,220,220) },
		{ eAnimParamType_ShakeAmpBMult, "ShakeMultAmpB", RGB(220,220,220) },
		{ eAnimParamType_ShakeFreqAMult, "ShakeMultFreqA", RGB(220,220,220) },
		{ eAnimParamType_ShakeFreqBMult, "ShakeMultFreqB", RGB(220,220,220) },
		{ eAnimParamType_ColorR, "ColorR", RGB(220,220,220) },
		{ eAnimParamType_ColorG, "ColorG", RGB(220,220,220) },
		{ eAnimParamType_ColorB, "ColorB", RGB(220,220,220) },
		{ eAnimParamType_MaterialOpacity, "MaterialOpacity", RGB(220,220,220) },
		{ eAnimParamType_MaterialSmoothness, "MaterialGlossiness", RGB(220,220,220) },
		{ eAnimParamType_MaterialEmissive, "MaterialEmission", RGB(220,220,220) },
		{ eAnimParamType_NearZ, "NearZ", RGB(220,220,220) },
		
		{ eAnimParamType_User, "", RGB(0,0,0) }, // An empty string means a separator row.
		
		// Misc colors for special states of a track
		{ eAnimParamType_User, "Others", RGB(220,220,220) },
		{ eAnimParamType_User, "Disabled/Inactive", RGB(255,224,224) },
		{ eAnimParamType_User, "Muted", RGB(255,224,224) },
	};
	
	CMFCColorButton g_colorButtons[arraysize(g_trackEntries)];

	const int kButtonsIdBase = 0x7fff;
	const int kMaxRows = 20;
	const int kColumnWidth = 300;
	const int kRowHeight = 24;
	
	const int kOthersEntryIndex = arraysize(g_trackEntries)-3;
	const int kDisabledEntryIndex = arraysize(g_trackEntries)-2;
	const int kMutedEntryIndex = arraysize(g_trackEntries)-1;
}

std::map<CAnimParamType, COLORREF> CTVCustomizeTrackColorsDlg::s_trackColors;
COLORREF CTVCustomizeTrackColorsDlg::s_colorForDisabled;
COLORREF CTVCustomizeTrackColorsDlg::s_colorForMuted;
COLORREF CTVCustomizeTrackColorsDlg::s_colorForOthers;

CTVCustomizeTrackColorsDlg::CTVCustomizeTrackColorsDlg(CWnd *pParent)
	: CDialog(CTVCustomizeTrackColorsDlg::IDD, pParent),
	m_aLabels(new CStatic[arraysize(g_trackEntries)])
{
}

CTVCustomizeTrackColorsDlg::~CTVCustomizeTrackColorsDlg()
{
	::delete [] m_aLabels;
}

void CTVCustomizeTrackColorsDlg::DoDataExchange(CDataExchange *pDX)
{
	__super::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTVCustomizeTrackColorsDlg, CDialog)
	ON_BN_CLICKED(IDC_APPLY, OnApply)
	ON_BN_CLICKED(IDC_RESET_ALL, OnResetAll)
	ON_BN_CLICKED(IDC_EXPORT, OnExport)
	ON_BN_CLICKED(IDC_IMPORT, OnImport)
END_MESSAGE_MAP()

BOOL CTVCustomizeTrackColorsDlg::OnInitDialog()
{
	__super::OnInitDialog();
	
	CRect labelRect(30, 30, 150, 50);
	CRect buttonRect(180, 30, 280, 50);
	// Create a label and a color button for each track.
	int col=0, i=0;
	std::for_each(g_trackEntries, g_trackEntries+arraysize(g_trackEntries), [&](const STrackEntry& entry)
	{
		CString labelText = entry.name;

		if(labelText.IsEmpty() == false)
		{
			m_aLabels[i].Create(labelText, WS_CHILD|WS_VISIBLE, labelRect, this);
			m_aLabels[i].SetFont(CFont::FromHandle(gSettings.gui.hSystemFont));

			g_colorButtons[i].Create("", WS_CHILD|WS_VISIBLE, buttonRect, this, kButtonsIdBase+i);
			g_colorButtons[i].EnableOtherButton(_T("Other..."));
			if(entry.paramType.GetType() == eAnimParamType_User)
			{
				assert(kOthersEntryIndex <= i);
				if(i == kOthersEntryIndex)
					g_colorButtons[i].SetColor(s_colorForOthers);
				else if(i == kDisabledEntryIndex)
					g_colorButtons[i].SetColor(s_colorForDisabled);
				else if(i == kMutedEntryIndex)
					g_colorButtons[i].SetColor(s_colorForMuted);
			}
			else
			{
				g_colorButtons[i].SetColor(s_trackColors[entry.paramType]);
			}
			g_colorButtons[i].SetColumnsNumber(5);
		}

		if(i % kMaxRows == kMaxRows - 1)
		{
			++col;
			labelRect.MoveToXY(30+kColumnWidth*col, 30);
			buttonRect.MoveToXY(180+kColumnWidth*col, 30);
		}
		else
		{
			labelRect.OffsetRect(0, kRowHeight);
			buttonRect.OffsetRect(0, kRowHeight);
		}
		++i;
	});

	// Resize this dialog properly.
	CRect rect(0, 0, 60+kColumnWidth*(col+1), 100+kMaxRows*kRowHeight);
	MoveWindow(rect);
	const int kButtonWidth = 75, kButtonHeight = 21;
	const int kSpaceBetweenButtons = 80;
	const int kStartX = rect.Width()-82, kStartY = rect.Height()-50;
	GetDlgItem(IDC_APPLY)->MoveWindow(kStartX, kStartY, kButtonWidth, kButtonHeight);
	GetDlgItem(IDCANCEL)->MoveWindow(kStartX-kSpaceBetweenButtons, kStartY, kButtonWidth, kButtonHeight);
	GetDlgItem(IDOK)->MoveWindow(kStartX-2*kSpaceBetweenButtons, kStartY, kButtonWidth, kButtonHeight);
	GetDlgItem(IDC_RESET_ALL)->MoveWindow(kStartX-10-3*kSpaceBetweenButtons, kStartY, kButtonWidth, kButtonHeight);
	GetDlgItem(IDC_EXPORT)->MoveWindow(kStartX-10-4*kSpaceBetweenButtons, kStartY, kButtonWidth, kButtonHeight);
	GetDlgItem(IDC_IMPORT)->MoveWindow(kStartX-10-5*kSpaceBetweenButtons, kStartY, kButtonWidth, kButtonHeight);
	CenterWindow();

	return TRUE;
}

void CTVCustomizeTrackColorsDlg::OnOK()
{
	OnApply();
	__super::OnOK();
}

void CTVCustomizeTrackColorsDlg::OnApply()
{
	int i=0;
	std::for_each(g_trackEntries, g_trackEntries+arraysize(g_trackEntries), [&](const STrackEntry& entry)
	{
		if(entry.paramType.GetType() != eAnimParamType_User)
		{
			s_trackColors[entry.paramType] = static_cast<CMFCColorButton*>(this->GetDlgItem(kButtonsIdBase+i))->GetColor();
		}
		++i;
	});

	s_colorForOthers = static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kOthersEntryIndex))->GetColor();
	s_colorForDisabled = static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kDisabledEntryIndex))->GetColor();
	s_colorForMuted = static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kMutedEntryIndex))->GetColor();

	CTrackViewDialog::GetCurrentInstance()->InvalidateDopeSheet();
}

void CTVCustomizeTrackColorsDlg::OnResetAll()
{
	int i=0;
	std::for_each(g_trackEntries, g_trackEntries+arraysize(g_trackEntries), [&](const STrackEntry& entry)
	{
		CString labelText = entry.name;
		if(labelText.IsEmpty() == false)
		{
			static_cast<CMFCColorButton*>(this->GetDlgItem(kButtonsIdBase+i))->SetColor(entry.defaultColor);
		}
		++i;
	});
}

void CTVCustomizeTrackColorsDlg::SaveColors(const char *sectionName)
{
	std::for_each(begin(s_trackColors), end(s_trackColors), 
								[=](const std::pair<CAnimParamType, COLORREF>& pair)
	{
		CString trackColorEntry;
		trackColorEntry.Format("%s%d", TRACKCOLOR_ENTRY_PREFIX, pair.first);
		AfxGetApp()->WriteProfileInt(sectionName, trackColorEntry, pair.second);
	});

	AfxGetApp()->WriteProfileInt(sectionName, TRACKCOLOR_FOR_OTHERS_ENTRY, s_colorForOthers);
	AfxGetApp()->WriteProfileInt(sectionName, TRACKCOLOR_FOR_DISABLED_ENTRY, s_colorForDisabled);
	AfxGetApp()->WriteProfileInt(sectionName, TRACKCOLOR_FOR_MUTED_ENTRY, s_colorForMuted);
}

void CTVCustomizeTrackColorsDlg::LoadColors(const char *sectionName)
{
	std::for_each(g_trackEntries, g_trackEntries+arraysize(g_trackEntries), [&](const STrackEntry& entry)
	{
		if(entry.paramType.GetType() != eAnimParamType_User)
		{
			CString trackColorEntry;
			trackColorEntry.Format("%s%d", TRACKCOLOR_ENTRY_PREFIX, entry.paramType);
			s_trackColors[entry.paramType] = AfxGetApp()->GetProfileInt(sectionName, trackColorEntry, entry.defaultColor);
		}
	});

	s_colorForOthers = AfxGetApp()->GetProfileInt(sectionName, TRACKCOLOR_FOR_OTHERS_ENTRY, g_trackEntries[kOthersEntryIndex].defaultColor);
	s_colorForDisabled = AfxGetApp()->GetProfileInt(sectionName, TRACKCOLOR_FOR_DISABLED_ENTRY, g_trackEntries[kDisabledEntryIndex].defaultColor);
	s_colorForMuted = AfxGetApp()->GetProfileInt(sectionName, TRACKCOLOR_FOR_MUTED_ENTRY, g_trackEntries[kMutedEntryIndex].defaultColor);
}

void CTVCustomizeTrackColorsDlg::OnExport()
{
	CString savePath;
	if(CFileUtil::SelectSaveFile("Custom Track Colors Files (*.ctc)|*.ctc||", "ctc", 
		Path::GetUserSandboxFolder(), savePath))
	{	
		Export(savePath);
	}
}

void CTVCustomizeTrackColorsDlg::OnImport()
{
	CString loadPath;
	if(CFileUtil::SelectFile("Custom Track Colors Files (*.ctc)|*.ctc||", 
		Path::GetUserSandboxFolder(), loadPath))
	{
		if(Import(loadPath) == false)
			MessageBox("The file format is invalid!", "Cannot import", MB_OK|MB_ICONERROR);
	}
}

void CTVCustomizeTrackColorsDlg::Export(const char *fullPath) const
{
	XmlNodeRef customTrackColorsNode = XmlHelpers::CreateXmlNode("customtrackcolors");

	int i=0;
	std::for_each(g_trackEntries, g_trackEntries+arraysize(g_trackEntries), [&](const STrackEntry& entry)
	{
		if(entry.paramType.GetType() != eAnimParamType_User)
		{
			XmlNodeRef entryNode = customTrackColorsNode->newChild("entry");

			// Serialization is const safe
			CAnimParamType &paramType = const_cast<CAnimParamType&>( entry.paramType );
			paramType.Serialize( entryNode, false );
			entryNode->setAttr("color", static_cast<CMFCColorButton*>(this->GetDlgItem(kButtonsIdBase+i))->GetColor());
		}
		++i;
	});

	XmlNodeRef othersNode = customTrackColorsNode->newChild("others");
	othersNode->setAttr("color", static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kOthersEntryIndex))->GetColor());
	XmlNodeRef disabledNode = customTrackColorsNode->newChild("disabled");
	disabledNode->setAttr("color", static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kDisabledEntryIndex))->GetColor());
	XmlNodeRef mutedNode = customTrackColorsNode->newChild("muted");
	mutedNode->setAttr("color", static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kMutedEntryIndex))->GetColor());

	XmlHelpers::SaveXmlNode(customTrackColorsNode, fullPath);
}

bool CTVCustomizeTrackColorsDlg::Import(const char *fullPath)
{
	XmlNodeRef customTrackColorsNode = XmlHelpers::LoadXmlFromFile(fullPath);
	if(customTrackColorsNode == NULL)
	{
		return false;
	}

	for(int i=0; i<customTrackColorsNode->getChildCount(); ++i)
	{
		XmlNodeRef childNode = customTrackColorsNode->getChild(i);
		if(CString(childNode->getTag()) != "entry")
			continue;
		
		CAnimParamType paramType;		
		if(childNode->haveAttr("paramtype"))
		{
			int paramId;
			childNode->getAttr("paramtype", paramId);
			paramType = paramId;
		}
		else
			paramType.Serialize( childNode, true );

		// Get the entry index for this param type.
		const STrackEntry *pEntry = std::find_if(g_trackEntries, g_trackEntries+arraysize(g_trackEntries), 
		[=](const STrackEntry& entry) 
		{ 
			return entry.paramType == paramType;
		});
		int entryIndex = pEntry - g_trackEntries;
		if(entryIndex >= arraysize(g_trackEntries)) // If not found, skip this.
			continue;
		COLORREF color = -1;
		childNode->getAttr("color", color);
		static_cast<CMFCColorButton*>(this->GetDlgItem(kButtonsIdBase+entryIndex))->SetColor(color);
	}

	XmlNodeRef othersNode = customTrackColorsNode->findChild("others");
	if(othersNode)
	{
		COLORREF color = -1;
		othersNode->getAttr("color", color);
		static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kOthersEntryIndex))->SetColor(color);
	}

	XmlNodeRef disabledNode = customTrackColorsNode->findChild("disabled");
	if(disabledNode)
	{
		COLORREF color = -1;
		disabledNode->getAttr("color", color);
		static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kDisabledEntryIndex))->SetColor(color);
	}

	XmlNodeRef mutedNode = customTrackColorsNode->findChild("muted");
	if(mutedNode)
	{
		COLORREF color = -1;
		mutedNode->getAttr("color", color);
		static_cast<CMFCColorButton*>(GetDlgItem(kButtonsIdBase + kMutedEntryIndex))->SetColor(color);
	}

	return true;
}
