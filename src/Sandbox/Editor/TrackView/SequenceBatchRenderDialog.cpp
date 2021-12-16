//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : SequenceBatchRenderDialog.cpp
//  Author           : Jaewon Jung
//  Time of creation : 3/1/2011   11:29
//  Compilers        : VS2008
//  Description      : A dialog for batch-rendering sequences
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SequenceBatchRenderDialog.h"
#include "CustomResolutionDlg.h"
#include "ViewPane.h"
#include "GameEngine.h"
#include <afxbutton.h>
#include <ppl.h>
#include <agents.h>

namespace
{

int resolutions[][2] = { {1280,720}, {1920,1080}, {1998,1080}, {2048,858}, {2560,1440} };
struct SFPSPair
{
	int fps;
	const char *fpsDesc;
};
SFPSPair fps[] = { {24,"Film(24)"}, {25,"PAL(25)"}, {30,"NTSC(30)"}, 
									{48,"Show(48)"}, {50,"PAL Field(50)"}, {60,"NTSC Field(60)"} };
const char *imageFormats[] = { "jpg", "bmp", "tga", "hdr" };
const char *buffersToCapture[] = { "Color", "HDR_depth_shadow_AO", "Stereo" };

const char defaultPresetFilename[] = "defaultBatchRender.preset";

const char customResFormat[] = "Custom(%d x %d)...";

const int kBatchRenderFileVersion = 1;	// This version number should be incremented 
																				// every time available options like the list of formats,
																				// the list of buffers change.

const int kMaxRes = 8192;  // Renderer checks the actual maximum resolution (after all it is affected also by supersampling)

const int kWarmingUpDuration = 1000; // 1000ms == 1s
}

CSequenceBatchRenderDialog::CSequenceBatchRenderDialog(float fps, CWnd *pParent /* = NULL */)
							: m_fpsForTimeToFrameConversion(fps), CDialog(CSequenceBatchRenderDialog::IDD, pParent),
							m_customResH(0), m_customResW(0), m_customFPS(0), m_bFFMPEGCommandAvailable(false)

{
	m_pGoBtn = new CMFCButton();
}

CSequenceBatchRenderDialog::~CSequenceBatchRenderDialog()
{
	delete m_pGoBtn;
}

void CSequenceBatchRenderDialog::DoDataExchange(CDataExchange *pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_BATCH_RENDER_SEQ_COMBO, m_sequenceCombo);
	DDX_Control(pDX, IDC_BATCH_RENDER_SHOT_COMBO, m_shotCombo);
	DDX_Control(pDX, IDC_BATCH_RENDER_SEQ_LIST, m_renderList);
	DDX_Control(pDX, IDC_BATCH_RENDER_RESOLUTION_COMBO, m_resolutionCombo);
	DDX_Control(pDX, IDC_BATCH_RENDER_FPS_COMBO, m_fpsCombo);
	DDX_Control(pDX, IDC_BATCH_RENDER_IMAGE_FORMAT, m_imageFormatCombo);
	DDX_Control(pDX, IDC_BATCH_RENDER_BUFFERS, m_buffersToCaptureCombo);
	DDX_Control(pDX, IDC_BATCH_RENDER_PROGRESS_MSG, m_progressStatusMsg);
	DDX_Control(pDX, IDOK, *m_pGoBtn);
	DDX_Control(pDX, IDC_BATCH_RENDER_PROGRESS, m_progressBar);
	DDX_Control(pDX, IDC_BATCH_RENDER_CVARS, m_cvarsEdit);
	DDX_Control(pDX, IDC_BATCH_RENDER_UPDATE_SEQ, m_updateBtn);
	DDX_Control(pDX, IDC_BATCH_RENDER_DESTINATION_EDIT, m_destinationEdit);
	DDX_Control(pDX, IDC_BATCH_RENDER_CREATE_VIDEO, m_createVideoCheckBox);
}

BEGIN_MESSAGE_MAP(CSequenceBatchRenderDialog, CDialog)
	ON_BN_CLICKED(IDC_BATCH_RENDER_ADD_SEQ, OnAddRenderItem)
	ON_BN_CLICKED(IDC_BATCH_RENDER_REMOVE_SEQ, OnRemoveRenderItem)
	ON_BN_CLICKED(IDC_BATCH_RENDER_CLEAR_SEQ, OnClearRenderItems)
	ON_BN_CLICKED(IDC_BATCH_RENDER_UPDATE_SEQ, OnUpdateRenderItem)
	ON_BN_CLICKED(IDC_BATCH_RENDER_LOAD_PRESET, OnLoadPreset)
	ON_BN_CLICKED(IDC_BATCH_RENDER_SAVE_PRESET, OnSavePreset)
	ON_BN_CLICKED(IDC_BATCH_RENDER_LOAD_BATCH, OnLoadBatch)
	ON_BN_CLICKED(IDC_BATCH_RENDER_SAVE_BATCH, OnSaveBatch)
	ON_BN_CLICKED(IDOK, OnGo)
	ON_BN_CLICKED(IDCANCEL, OnDone)
	ON_CBN_SELENDOK(IDC_BATCH_RENDER_SEQ_COMBO, OnSequenceSelected)
	ON_CBN_EDITCHANGE(IDC_BATCH_RENDER_FPS_COMBO, OnFPSEditChange)
	ON_LBN_SELCHANGE(IDC_BATCH_RENDER_SEQ_LIST, OnRenderItemSelChange)
	ON_CBN_SELENDOK(IDC_BATCH_RENDER_RESOLUTION_COMBO, OnResolutionSelected)
	ON_CBN_SELENDOK(IDC_BATCH_RENDER_IMAGE_FORMAT, OnImageFormatSelected)
	ON_CBN_SELENDOK(IDC_BATCH_RENDER_BUFFERS, OnBuffersSelected)
	ON_EN_CHANGE(IDC_BATCH_RENDER_START_FRAME, OnStartFrameChange)
	ON_EN_CHANGE(IDC_BATCH_RENDER_END_FRAME, OnEndFrameChange)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
END_MESSAGE_MAP()

BOOL CSequenceBatchRenderDialog::OnInitDialog()
{
	__super::OnInitDialog();

	m_destinationEdit.EnableFolderBrowseButton();

	const float bigEnoughNumber = 1000000.0f;
	m_startFrame.Create(this, IDC_BATCH_RENDER_START_FRAME);
	m_startFrame.SetRange(0.0f, bigEnoughNumber);
	m_startFrame.SetInteger(true);

	m_endFrame.Create(this, IDC_BATCH_RENDER_END_FRAME);
	m_endFrame.SetRange(0.0f, bigEnoughNumber);
	m_endFrame.SetInteger(true);
	
	// Fill the sequence combo box.
	for(int k=0; k<GetIEditor()->GetMovieSystem()->GetNumSequences(); ++k)
	{
		IAnimSequence *pSequence = GetIEditor()->GetMovieSystem()->GetSequence(k);
		m_sequenceCombo.AddString(pSequence->GetName());
	}
	m_sequenceCombo.SetCurSel(0);

	// Fill the shot combos and the default frame range.
	OnSequenceSelected();

	// Fill the resolution combo box.
	CString resText;
	for(int i=0; i<arraysize(resolutions); ++i)
	{
		resText.Format("%d x %d", resolutions[i][0], resolutions[i][1]);
		m_resolutionCombo.AddString(resText);
	}
	m_resolutionCombo.AddString("Custom...");
	m_resolutionCombo.SetCurSel(0);

	// Fill the FPS combo box.
	for(int i=0; i<arraysize(fps); ++i)
	{
		m_fpsCombo.AddString(fps[i].fpsDesc);
	}
	m_fpsCombo.SetCurSel(0);

	// Fill the image format combo box.
	for(int i=0; i<arraysize(imageFormats); ++i)
	{
		m_imageFormatCombo.AddString(imageFormats[i]);
	}
	m_imageFormatCombo.SetCurSel(0);

	// Fill the buffers-to-capture combo box.
	for(int i=0; i<arraysize(buffersToCapture); ++i)
	{
		m_buffersToCaptureCombo.AddString(buffersToCapture[i]);
	}
	m_buffersToCaptureCombo.SetCurSel(0);
	
	GetDlgItem(IDC_BATCH_RENDER_FILE_PREFIX)->SetWindowText("Frame");

	m_progressStatusMsg.SetWindowText("Not running");

	GetDlgItem(IDC_BATCH_RENDER_REMOVE_SEQ)->EnableWindow(FALSE);
	m_updateBtn.EnableWindow(FALSE);
	m_pGoBtn->EnableWindow(FALSE);
	m_pGoBtn->SetImage(IDB_CLAPPERBOARD_GO);

	m_progressBar.SetRange(0, 100);

	CString frameInFpsText;
	frameInFpsText.Format("In %d FPS", int(m_fpsForTimeToFrameConversion));
	GetDlgItem(IDC_BATCH_RENDER_FRAME_IN_FPS)->SetWindowText(frameInFpsText);

	m_bFFMPEGCommandAvailable = GetIEditor()->GetICommandManager()->IsRegistered("plugin", "ffmpeg_encode");
	m_ffmpegPluginStatusMsg = m_bFFMPEGCommandAvailable ? 
		"" : 
		"FFMPEG plug-in isn't found(creating a video isn't supported).";
	GetDlgItem(IDC_BATCH_RENDER_PRESS_ESC_TO_CANCEL)->SetWindowText(m_ffmpegPluginStatusMsg);

	// Load previously saved options, if any.
	CString defaultPresetPath = Path::GetUserSandboxFolder();
	defaultPresetPath += defaultPresetFilename;
	if(CFileUtil::FileExists(defaultPresetPath))
		LoadOutputOptions(defaultPresetPath);

	UpdateCreateVideoCheckBox();

	return TRUE;
}

void CSequenceBatchRenderDialog::OnRenderItemSelChange()
{
	/// Enable/disable the 'remove'/'update' button properly.
	bool bNoSelection = LB_ERR == m_renderList.GetCurSel();
	GetDlgItem(IDC_BATCH_RENDER_REMOVE_SEQ)->EnableWindow(
																	bNoSelection?FALSE:TRUE);
	m_updateBtn.EnableWindow(bNoSelection?FALSE:TRUE);

	if(bNoSelection)
		return;

	/// Apply the settings of the selected one to the dialog.
	const SRenderItem& item = m_renderItems[m_renderList.GetCurSel()];
	// sequence
	CString sequenceName;
	for(int i=0; i<m_sequenceCombo.GetCount(); ++i)
	{
		m_sequenceCombo.GetLBText(i, sequenceName);
		if(sequenceName == item.pSequence->GetName())
		{
			m_sequenceCombo.SetCurSel(i);
			OnSequenceSelected();
			break;
		}
	}
	// director
	CString directorName;
	for(int i=0; i<m_shotCombo.GetCount(); ++i)
	{
		m_shotCombo.GetLBText(i, directorName);
		if(directorName == item.pDirectorNode->GetName())
		{
			m_shotCombo.SetCurSel(i);
			break;
		}
	}
	// frame range
	m_startFrame.SetValue(item.frameRange.start * m_fpsForTimeToFrameConversion);
	m_endFrame.SetValue(item.frameRange.end * m_fpsForTimeToFrameConversion);
	// folder
	m_destinationEdit.SetWindowText(item.folder);
	// fps
	bool bFound = false;
	for(int i=0; i<arraysize(fps); ++i)
	{
		if(item.fps == fps[i].fps)
		{
			m_fpsCombo.SetCurSel(i);
			bFound = true;
			break;
		}
	}
	if(bFound == false)
	{
		m_customFPS = item.fps;
		CString fpsText;
		fpsText.Format("%d", item.fps);
		m_fpsCombo.SetCurSel(-1);
		m_fpsCombo.SetWindowText(fpsText);
	}
	// buffer
	m_buffersToCaptureCombo.SetCurSel(item.bufferIndex);
	// prefix
	GetDlgItem(IDC_BATCH_RENDER_FILE_PREFIX)->SetWindowText(item.prefix);
	// format
	m_imageFormatCombo.SetCurSel(item.formatIndex);
	OnBuffersSelected();
	// create_video
	m_createVideoCheckBox.SetCheck(item.bCreateVideo?BST_CHECKED:BST_UNCHECKED);
	// resolution
	bFound = false;
	for(int i=0; i<arraysize(resolutions); ++i)
	{
		if(item.resW == resolutions[i][0] && item.resH == resolutions[i][1])
		{
			m_resolutionCombo.SetCurSel(i);
			bFound = true;
			break;
		}
	}
	if(bFound == false)
	{
		int indexOfCustomRes = arraysize(resolutions);
		CString resText;
		resText.Format(customResFormat, item.resW, item.resH);
		m_customResW = item.resW;
		m_customResH = item.resH;
		m_resolutionCombo.DeleteString(indexOfCustomRes);
		m_resolutionCombo.AddString(resText);
		m_resolutionCombo.SetCurSel(indexOfCustomRes);
	}
	// cvars
	CString cvarsText = "";
	for(size_t i=0; i<item.cvars.size(); ++i)
	{
		cvarsText += item.cvars[i];
		cvarsText += "\r\n";
	}
	m_cvarsEdit.SetWindowText(cvarsText);

	m_updateBtn.EnableWindow(FALSE);
}

void CSequenceBatchRenderDialog::OnAddRenderItem()
{
	// If there is no director node, it cannot be added.
	if(m_shotCombo.GetCount() == 0)
	{
		MessageBox("No director available!", "Cannot add", MB_OK|MB_ICONERROR);
		return;
	}

	/// Set up a new render item.
	SRenderItem item;
	if(SetUpNewRenderItem(item) == false)
		return;

	/// Check a duplication before adding.
	for(size_t i=0; i<m_renderItems.size(); ++i)
	{
		if(m_renderItems[i] == item)
		{
			MessageBox("The same item already exists!", "Cannot add", MB_OK|MB_ICONERROR);
			return;
		}
	}

	AddItem(item);
}

void CSequenceBatchRenderDialog::OnRemoveRenderItem()
{
	int index = m_renderList.GetCurSel();
	assert(index != CB_ERR);
	m_renderList.DeleteString(index);
	m_renderItems.erase(m_renderItems.begin()+index);

	if(m_renderItems.empty())
	{
		GetDlgItem(IDC_BATCH_RENDER_REMOVE_SEQ)->EnableWindow(FALSE);
		m_updateBtn.EnableWindow(FALSE);
		m_pGoBtn->EnableWindow(FALSE);
	}
	else
	{
		m_renderList.SetCurSel(0);
		OnRenderItemSelChange();
	}
}

void CSequenceBatchRenderDialog::OnClearRenderItems()
{
	m_renderList.ResetContent();
	m_renderItems.clear();

	GetDlgItem(IDC_BATCH_RENDER_REMOVE_SEQ)->EnableWindow(FALSE);
	m_updateBtn.EnableWindow(FALSE);
	m_pGoBtn->EnableWindow(FALSE);
}

void CSequenceBatchRenderDialog::OnUpdateRenderItem()
{
	int index = m_renderList.GetCurSel();
	assert(index != CB_ERR);

	/// Set up a new render item.
	SRenderItem item;
	SetUpNewRenderItem(item);

	/// Check a duplication before updating.
	for(size_t i=0; i<m_renderItems.size(); ++i)
	{
		if(m_renderItems[i] == item)
		{
			MessageBox("The same item already exists!", "Cannot update", MB_OK|MB_ICONERROR);
			return;
		}
	}

	/// Update the item.
	m_renderItems[index] = item;

	/// Update the list box, too.
	m_renderList.DeleteString(index);
	m_renderList.InsertString(index, GetCaptureItemString(item));
	m_renderList.SetCurSel(index);

	m_updateBtn.EnableWindow(FALSE);
}

void CSequenceBatchRenderDialog::OnLoadPreset()
{
	CString loadPath;
	if(CFileUtil::SelectFile("Preset Files (*.preset)|*.preset||", 
													Path::GetUserSandboxFolder(), loadPath))
	{
		if(LoadOutputOptions(loadPath) == false)
			MessageBox("The file version is different!", "Cannot load", MB_OK|MB_ICONERROR);
	}
}

void CSequenceBatchRenderDialog::OnSavePreset()
{
	CString savePath;
	if(CFileUtil::SelectSaveFile("Preset Files (*.preset)|*.preset||", "preset", 
															Path::GetUserSandboxFolder(), savePath))
	{
		SaveOutputOptions(savePath);
	}
}

void CSequenceBatchRenderDialog::OnGo()
{
	if(m_renderContext.IsInRendering())
	{
		if(m_renderContext.bWarmingUpAfterResChange 
			|| m_renderContext.bFFMPEGProcessing) // No cancellation in these two phases
			return;
		/// Cancel the batch.
		GetIEditor()->GetMovieSystem()->AbortSequence(m_renderItems[m_renderContext.currentItemIndex].pSequence);
	}
	else
	{
		/// Start a new batch.
		m_pGoBtn->SetWindowText("Cancel");
		m_pGoBtn->SetImage(IDB_CLAPPERBOARD_CANCEL);
		// Inform the movie system that it soon will be in a batch-rendering mode.
		GetIEditor()->GetMovieSystem()->EnableBatchRenderMode(true);

		// Initialize the context.
		InitializeContext();

		// Trigger the first item.
		OnMovieEvent(IMovieListener::eMovieEvent_Stopped, NULL);
	}
}

void CSequenceBatchRenderDialog::OnMovieEvent(IMovieListener::EMovieEvent event, IAnimSequence* pSequence)
{
	if(event == IMovieListener::eMovieEvent_Stopped
			|| event == IMovieListener::eMovieEvent_Aborted)
	{

		/// Finalize the current one, if any.
		if(pSequence)
		{
			EndCaptureItem(pSequence);

			bool bDone = m_renderContext.currentItemIndex == m_renderItems.size()-1;
			bool bCancelled = event == IMovieListener::eMovieEvent_Aborted;
			if(bDone || bCancelled)
			{
				// Display the final progress message.
				if(bCancelled)
				{
					m_progressBar.SetPos(0);
					GetDlgItem(IDC_BATCH_RENDER_PROGRESS_MSG)->SetWindowText("Rendering cancelled");
				}
				else
				{
					m_progressBar.SetPos(100);
					GetDlgItem(IDC_BATCH_RENDER_PROGRESS_MSG)->SetWindowText("Rendering finished");
				}
				/// End the batch.
				m_pGoBtn->SetWindowText("Start");
				m_pGoBtn->SetImage(IDB_CLAPPERBOARD_GO);
				GetIEditor()->GetMovieSystem()->EnableBatchRenderMode(false);
				m_renderContext.currentItemIndex = -1;
				GetDlgItem(IDC_BATCH_RENDER_PRESS_ESC_TO_CANCEL)->SetWindowText(m_ffmpegPluginStatusMsg);
				return;
			}

			/// Update the context.
			m_renderContext.spentTime += m_renderContext.captureOptions.duration;
			++m_renderContext.currentItemIndex;
		}

		/// Trigger the next item.
		StartCaptureItem();
	}
}

void CSequenceBatchRenderDialog::OnDone()
{
	if(m_renderContext.IsInRendering())
	{
		if(m_renderContext.bWarmingUpAfterResChange 
			|| m_renderContext.bFFMPEGProcessing) // No cancellation in these two phases
			return;
		/// Cancel the batch.
		GetIEditor()->GetMovieSystem()->AbortSequence(m_renderItems[m_renderContext.currentItemIndex].pSequence);
	}
	else
	{
		// Save options when closed.
		CString defaultPresetPath = Path::GetUserSandboxFolder();
		defaultPresetPath += defaultPresetFilename;
		SaveOutputOptions(defaultPresetPath);

		OnCancel();
	}
}

void CSequenceBatchRenderDialog::OnSequenceSelected()
{
	// Get the selected sequence.
	CString seqName;
	m_sequenceCombo.GetWindowText(seqName);
	IAnimSequence *pSequence = GetIEditor()->GetMovieSystem()->FindSequence(seqName.GetString());

	// Adjust the frame range.
	float sFrame = pSequence->GetTimeRange().start * m_fpsForTimeToFrameConversion;
	float eFrame = pSequence->GetTimeRange().end * m_fpsForTimeToFrameConversion;
	m_startFrame.SetRange(0.0f, eFrame);
	m_endFrame.SetRange(0.0f, eFrame);

	// Set the default start/end frames properly.
	m_startFrame.SetValue(sFrame);
	m_endFrame.SetValue(eFrame);
	
	m_shotCombo.ResetContent();
	// Fill the shot combo box with the names of director nodes.
	for(int i=0; i<pSequence->GetNodeCount(); ++i)
	{
		if(pSequence->GetNode(i)->GetType() == eAnimNodeType_Director)
		{
			m_shotCombo.AddString(pSequence->GetNode(i)->GetName());
		}
	}
	m_shotCombo.SetCurSel(0);
}

void CSequenceBatchRenderDialog::OnFPSEditChange()
{
	CString fpsText;
	m_fpsCombo.GetWindowText(fpsText);
	int fps;
	bool bInvalidInput = false;
	if(sscanf(fpsText.GetString(), "%d", &fps)<=0)
	{
		bInvalidInput = true;
	}
	else if(fps <= 0)
	{
		bInvalidInput = true;
	}

	if(bInvalidInput)
		m_fpsCombo.SetCurSel(0);
	else
		m_customFPS = fps;
}

void CSequenceBatchRenderDialog::OnResolutionSelected()
{
	int indexOfCustomRes = arraysize(resolutions);
	if(m_resolutionCombo.GetCurSel() == indexOfCustomRes)
	{
		int defaultW = resolutions[0][0];
		int defaultH = resolutions[0][1];
		CString currentCustomResText;
		m_resolutionCombo.GetWindowText(currentCustomResText);
		sscanf(currentCustomResText.GetString(), customResFormat, &defaultW, &defaultH);
		CCustomResolutionDlg resDlg(defaultW, defaultH, this);
		if(resDlg.DoModal() == IDOK)
		{
			m_customResW = min(resDlg.GetWidth(), kMaxRes);
			m_customResH = min(resDlg.GetHeight(), kMaxRes);
			CString resText;
			resText.Format(customResFormat, m_customResW, m_customResH);
			m_resolutionCombo.DeleteString(indexOfCustomRes);
			m_resolutionCombo.AddString(resText);
			m_resolutionCombo.SetCurSel(indexOfCustomRes);
		}
		else
		{
			m_resolutionCombo.SetCurSel(0);
		}
	}
}

void CSequenceBatchRenderDialog::SaveOutputOptions(const char *pathname) const
{
	XmlNodeRef batchRenderOptionsNode = XmlHelpers::CreateXmlNode("batchrenderoptions");
	batchRenderOptionsNode->setAttr("version", kBatchRenderFileVersion);
	
	// Resolution
	XmlNodeRef resolutionNode = batchRenderOptionsNode->newChild("resolution");
	resolutionNode->setAttr("cursel", m_resolutionCombo.GetCurSel());
	if(m_resolutionCombo.GetCurSel() == arraysize(resolutions))
	{
		CString resText;
		m_resolutionCombo.GetWindowText(resText);
		resolutionNode->setContent(resText.GetString());
	}

	// FPS
	XmlNodeRef fpsNode = batchRenderOptionsNode->newChild("fps");
	fpsNode->setAttr("cursel", m_fpsCombo.GetCurSel());
	if(m_fpsCombo.GetCurSel() == CB_ERR)
	{
		CString fpsText;
		m_fpsCombo.GetWindowText(fpsText);
		fpsNode->setContent(fpsText.GetString());
	}

	// Capture options (format, buffer, prefix, create_video)
	XmlNodeRef imageNode = batchRenderOptionsNode->newChild("image");
	imageNode->setAttr("format", m_imageFormatCombo.GetCurSel() % arraysize(imageFormats));
	imageNode->setAttr("bufferstocapture", m_buffersToCaptureCombo.GetCurSel());
	CString prefix;
	GetDlgItem(IDC_BATCH_RENDER_FILE_PREFIX)->GetWindowText(prefix);
	imageNode->setAttr("prefix", prefix.GetString());
	bool bCreateVideoOn = m_createVideoCheckBox.GetCheck() == BST_CHECKED;
	imageNode->setAttr("createvideo", bCreateVideoOn);

	// Custom configs
	XmlNodeRef cvarsNode = batchRenderOptionsNode->newChild("cvars");
	const int kCVarNameMaxSize = 256;
	char buf[kCVarNameMaxSize]; buf[kCVarNameMaxSize-1] = 0;
	for(int i=0; i<m_cvarsEdit.GetLineCount(); ++i)
	{
		m_cvarsEdit.GetLine(i, buf, kCVarNameMaxSize-1);
		cvarsNode->newChild("cvar")->setContent(buf);
	}

	// Destination
	XmlNodeRef destinationNode = batchRenderOptionsNode->newChild("destination");
	CString destinationText;
	m_destinationEdit.GetWindowText(destinationText);
	destinationNode->setContent(destinationText.GetString());

	XmlHelpers::SaveXmlNode(batchRenderOptionsNode, pathname);
}

bool CSequenceBatchRenderDialog::LoadOutputOptions(const char *pathname)
{
	XmlNodeRef batchRenderOptionsNode = XmlHelpers::LoadXmlFromFile(pathname);
	if(batchRenderOptionsNode == NULL)
	{
		return true;
	}
	int version = 0;
	batchRenderOptionsNode->getAttr("version", version);
	if(version != kBatchRenderFileVersion)
	{
		return false;
	}

	// Resolution
	XmlNodeRef resolutionNode = batchRenderOptionsNode->findChild("resolution");
	if(resolutionNode)
	{
		int curSel = CB_ERR;
		resolutionNode->getAttr("cursel", curSel);
		if(curSel == arraysize(resolutions))
		{
			CString customResText = resolutionNode->getContent();
			m_resolutionCombo.DeleteString(curSel);
			m_resolutionCombo.AddString(customResText);
			sscanf(customResText, customResFormat, &m_customResW, &m_customResH);
		}
		m_resolutionCombo.SetCurSel(curSel);
	}

	// FPS
	XmlNodeRef fpsNode = batchRenderOptionsNode->findChild("fps");
	if(fpsNode)
	{
		int curSel = CB_ERR;
		fpsNode->getAttr("cursel", curSel);
		if (curSel == CB_ERR)
		{
			m_fpsCombo.SetCurSel(-1);
			m_fpsCombo.SetWindowText(fpsNode->getContent());
			sscanf(fpsNode->getContent(), "%d", &m_customFPS);
		}
		else
		{
			m_fpsCombo.SetCurSel(curSel);
		}
	}

	// Capture options (format, buffer, prefix, create_video)
	XmlNodeRef imageNode = batchRenderOptionsNode->findChild("image");
	if(imageNode)
	{
		int curSel = CB_ERR;
		imageNode->getAttr("format", curSel);
		m_imageFormatCombo.SetCurSel(curSel);
		curSel = CB_ERR;
		imageNode->getAttr("bufferstocapture", curSel);
		m_buffersToCaptureCombo.SetCurSel(curSel);
		OnBuffersSelected();
		GetDlgItem(IDC_BATCH_RENDER_FILE_PREFIX)->SetWindowText(imageNode->getAttr("prefix"));
		bool bCreateVideoOn = false;
		imageNode->getAttr("createvideo", bCreateVideoOn);
		m_createVideoCheckBox.SetCheck(bCreateVideoOn?BST_CHECKED:BST_UNCHECKED);
	}

	// Custom configs
	XmlNodeRef cvarsNode = batchRenderOptionsNode->findChild("cvars");
	if(cvarsNode)
	{
		CString cvarsText = "";
		for(int i=0; i<cvarsNode->getChildCount(); ++i)
		{
			cvarsText += cvarsNode->getChild(i)->getContent();
			if(i < cvarsNode->getChildCount()-1)
				cvarsText += "\r\n";
		}
		m_cvarsEdit.SetWindowText(cvarsText);
	}

	// Destination
	XmlNodeRef destinationNode = batchRenderOptionsNode->findChild("destination");
	if(destinationNode)
	{
		m_destinationEdit.SetWindowText(destinationNode->getContent());
	}

	return true;
}

void CSequenceBatchRenderDialog::OnStartFrameChange()
{
	if(m_startFrame.GetValue() >= m_endFrame.GetValue())
		m_endFrame.SetValue(m_startFrame.GetValue()+1);
}

void CSequenceBatchRenderDialog::OnEndFrameChange()
{
	if(m_startFrame.GetValue() >= m_endFrame.GetValue())
		m_startFrame.SetValue(m_endFrame.GetValue()-1);
}

void CSequenceBatchRenderDialog::InitializeContext()
{
	m_renderContext.currentItemIndex = 0;
	m_renderContext.spentTime = 0;
	m_renderContext.expectedTotalTime = 0;
	for(size_t i=0; i<m_renderItems.size(); ++i)
	{
		Range rng = m_renderItems[i].frameRange;
		m_renderContext.expectedTotalTime += rng.end - rng.start;
	}
	m_renderContext.captureOptions.once = false;

	GetDlgItem(IDC_BATCH_RENDER_PRESS_ESC_TO_CANCEL)->SetWindowText("Press ESC to cancel");
}

void CSequenceBatchRenderDialog::StartCaptureItem()
{
	SRenderItem renderItem = m_renderItems[m_renderContext.currentItemIndex];
	IAnimSequence *pNextSequence = renderItem.pSequence;
	/// Initialize the next one for the batch rendering.
	// Set the active shot.
	m_renderContext.pActiveDirectorBU = pNextSequence->GetActiveDirector();
	pNextSequence->SetActiveDirector(renderItem.pDirectorNode);

	// Back up flags and range of the sequence.
	m_renderContext.flagBU = pNextSequence->GetFlags();
	m_renderContext.rangeBU = pNextSequence->GetTimeRange();

	// Change flags and range of the sequence so that it automatically starts
	// once the game mode kicks in with the specified range.
	pNextSequence->SetFlags(m_renderContext.flagBU|IAnimSequence::eSeqFlags_PlayOnReset);
	// A margin value to capture the precise number of frames
	const float someMargin = 2.5f/30.0f;	
	Range newRange = renderItem.frameRange;
	newRange.end += someMargin;
	pNextSequence->SetTimeRange(newRange);

	// Set up the custom config cvars for this item.
	for(size_t i=0; i<renderItem.cvars.size(); ++i)
		GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(renderItem.cvars[i]);

	// Set specific capture options for this item.
	m_renderContext.captureOptions.timeStep = 1.0f / renderItem.fps;
	m_renderContext.captureOptions.bufferToCapture = renderItem.bufferIndex;
	strcpy_s(m_renderContext.captureOptions.prefix, 
		arraysize(m_renderContext.captureOptions.prefix),
		renderItem.prefix);
	CString formatText = imageFormats[renderItem.formatIndex];
	if(formatText == "jpg")
		m_renderContext.captureOptions.FormatJPG();
	else if(formatText == "tga")
		m_renderContext.captureOptions.FormatTGA();
	else if(formatText == "bmp")
		m_renderContext.captureOptions.FormatBMP();
	else if(formatText == "hdr")
		m_renderContext.captureOptions.FormatHDR();
	Range rng = pNextSequence->GetTimeRange();
	m_renderContext.captureOptions.duration = rng.end - rng.start;
	CString folder, itemText, finalFolder;
	folder = renderItem.folder;
	m_renderList.GetText(m_renderContext.currentItemIndex, itemText);
	itemText.Replace('/', '-');	// A full sequence name can have slash characters which aren't suitable for a file name.
	folder += "\\";
	folder += itemText;
	finalFolder = folder;
	int i = 2;
	while(CFileUtil::PathExists(finalFolder))
	{
		finalFolder = folder;
		CString suffix;
		suffix.Format("_v%d", i);
		finalFolder += suffix;
		++i;
	}
	strcpy_s(m_renderContext.captureOptions.folder, arraysize(m_renderContext.captureOptions.folder), finalFolder);

	/// Change the resolution.
	ICVar *pCVarCustomResWidth = gEnv->pConsole->GetCVar("r_CustomResWidth");
	ICVar *pCVarCustomResHeight = gEnv->pConsole->GetCVar("r_CustomResHeight");
	if(pCVarCustomResWidth && pCVarCustomResHeight)
	{
		// If available, use the custom resolution cvars.
		m_renderContext.cvarCustomResWidthBU = pCVarCustomResWidth->GetIVal();
		m_renderContext.cvarCustomResHeightBU = pCVarCustomResHeight->GetIVal();
		pCVarCustomResWidth->Set(renderItem.resW);
		pCVarCustomResHeight->Set(renderItem.resH);
	}
	else
	{
		// Otherwise, try to adjust the viewport resolution accordingly.
		GetIEditor()->ExecuteCommand("general.resize_viewport %d %d", renderItem.resW, renderItem.resH);
	}

	// The capturing doesn't actually start here. It just flags the warming-up and
	// once it's done, then the capturing really begins.
	// The warming-up is necessary to settle down some post-fxs after the resolution change.
	m_renderContext.bWarmingUpAfterResChange = true;
	m_renderContext.timeWarmingUpStarted = GetTickCount();
}

void CSequenceBatchRenderDialog::ReallyStartCaptureItem()
{
	SRenderItem renderItem = m_renderItems[m_renderContext.currentItemIndex];
	IAnimSequence *pNextSequence = renderItem.pSequence;

	GetIEditor()->GetMovieSystem()->StartCapture(m_renderContext.captureOptions);
	GetIEditor()->SetInGameMode(true);
	GetIEditor()->GetMovieSystem()->AddMovieListener(pNextSequence, this);
}

void CSequenceBatchRenderDialog::EndCaptureItem(IAnimSequence* pSequence)
{
	GetIEditor()->GetMovieSystem()->RemoveMovieListener(pSequence, this);
	GetIEditor()->SetInGameMode(false);
	GetIEditor()->GetMovieSystem()->EndCapture();
	GetIEditor()->GetMovieSystem()->ControlCapture();

	ICVar *pCVarCustomResWidth = gEnv->pConsole->GetCVar("r_CustomResWidth");
	ICVar *pCVarCustomResHeight = gEnv->pConsole->GetCVar("r_CustomResHeight");
	if(pCVarCustomResWidth && pCVarCustomResHeight)
	{
		// Restore the custom resolution cvars.
		pCVarCustomResWidth->Set(m_renderContext.cvarCustomResWidthBU);
		pCVarCustomResHeight->Set(m_renderContext.cvarCustomResHeightBU);
	}
	
	// Restore flags, range and the active director of the sequence.
	pSequence->SetFlags(m_renderContext.flagBU);
	pSequence->SetTimeRange(m_renderContext.rangeBU);
	pSequence->SetActiveDirector(m_renderContext.pActiveDirectorBU);

	SRenderItem renderItem = m_renderItems[m_renderContext.currentItemIndex];
	if(m_bFFMPEGCommandAvailable
		&& renderItem.bCreateVideo)
	{
		// Create a video using the ffmpeg plug-in from captured images.
		m_renderContext.bFFMPEGProcessing = true;
		CString outputFolder = m_renderContext.captureOptions.folder;
		Concurrency::task_group ffmpegTask;
		Concurrency::single_assignment<bool> done;
		ffmpegTask.run(
			[&renderItem, &outputFolder, &done] 
			{ 
				CString inputFile, outputFile = outputFolder;
				outputFile += "\\";
				outputFile += renderItem.prefix;
				inputFile = outputFile;
				outputFile += ".mp4";
				inputFile += "%06d.jpg";
				GetIEditor()->ExecuteCommand(
					"plugin.ffmpeg_encode '%s' '%s' '%s' %d '-r %d -vf crop=%d:%d:0:0'", 
					inputFile.GetString(), outputFile.GetString(), "mpeg4", 
					10240, renderItem.fps, renderItem.resW, renderItem.resH);
				Concurrency::send(done, true);
			}
		);
		bool dummy;
		do
		{
			OnKickIdle(0, 0);
		} 
		while(Concurrency::try_receive(done, dummy) == false);
		m_renderContext.bFFMPEGProcessing = false;
	}
}

LRESULT CSequenceBatchRenderDialog::OnKickIdle(WPARAM, LPARAM lCount)
{
	static int count = 0;

	if(m_renderContext.IsInRendering())
	{
		if(m_renderContext.bWarmingUpAfterResChange)  /// A warming-up phase
		{
			char * rotatingCursor[] =  { "|", "/", "-", "\\" };
			CString msg = "Warming up ";
			msg += rotatingCursor[(count++)%arraysize(rotatingCursor)];
			GetDlgItem(IDC_BATCH_RENDER_PROGRESS_MSG)->SetWindowText(msg);
			GetIEditor()->GetGameEngine()->Update();
			GetIEditor()->Notify( eNotify_OnIdleUpdate );

			if(GetTickCount() > m_renderContext.timeWarmingUpStarted + kWarmingUpDuration)
			{
				// The warming-up done.
				m_renderContext.bWarmingUpAfterResChange = false;
				count = 0;
				ReallyStartCaptureItem();
			}
		}
		else if(m_renderContext.bFFMPEGProcessing)    /// A ffmpeg-processing phase
		{
			char * rotatingCursor[] =  { "|", "/", "-", "\\" };
			CString msg = "FFMPEG processing ";
			msg += rotatingCursor[(count++)%arraysize(rotatingCursor)];
			GetDlgItem(IDC_BATCH_RENDER_PROGRESS_MSG)->SetWindowText(msg);
			GetIEditor()->GetGameEngine()->Update();
			GetIEditor()->Notify( eNotify_OnIdleUpdate );
		}
		else                                          /// A capturing phase
		{
			// Progress bar
			IAnimSequence *pCurSeq = m_renderItems[m_renderContext.currentItemIndex].pSequence;
			Range rng = pCurSeq->GetTimeRange();
			float elapsedTime = GetIEditor()->GetMovieSystem()->GetPlayingTime(pCurSeq) - rng.start;
			int percentage 
				= int(100.0f * (m_renderContext.spentTime+elapsedTime) / m_renderContext.expectedTotalTime);
			m_progressBar.SetPos(percentage);

			// Progress message
			CString msg, itemText;
			m_renderList.GetText(m_renderContext.currentItemIndex, itemText);
			msg.Format("Rendering '%s'...(%d%%)", itemText, int(100.0f*elapsedTime/(rng.end-rng.start)));
			GetDlgItem(IDC_BATCH_RENDER_PROGRESS_MSG)->SetWindowText(msg);

			GetIEditor()->GetGameEngine()->Update();
		}

		return TRUE;
	}
	else
	{
		bool bItemSelected = LB_ERR != m_renderList.GetCurSel();
		if(bItemSelected)
		{
			// If any of settings changed, then enable the 'update button'.
			// Otherwise, disable it.
			SRenderItem item;
			bool bSettingChanged 
				= SetUpNewRenderItem(item) && !(item == m_renderItems[m_renderList.GetCurSel()]);
			m_updateBtn.EnableWindow(bSettingChanged?TRUE:FALSE);
		}
	}

	return FALSE;
}

void CSequenceBatchRenderDialog::OnLoadBatch()
{
	CString loadPath;
	if(CFileUtil::SelectFile("Render Batch Files (*.batch)|*.batch||", 
		Path::GetUserSandboxFolder(), loadPath))
	{
		XmlNodeRef batchRenderListNode = XmlHelpers::LoadXmlFromFile(loadPath);
		if(batchRenderListNode == NULL)
			return;
		int version = 0;
		batchRenderListNode->getAttr("version", version);
		if(version != kBatchRenderFileVersion)
		{
			MessageBox("The file version is different!", "Cannot load", MB_OK|MB_ICONERROR);
			return;
		}

		OnClearRenderItems();

		for(int i=0; i<batchRenderListNode->getChildCount(); ++i)
		{
			/// Get an item.
			SRenderItem item;
			XmlNodeRef itemNode = batchRenderListNode->getChild(i);

			// sequence
			CString seqName = itemNode->getAttr("sequence");
			item.pSequence = GetIEditor()->GetMovieSystem()->FindSequence(seqName.GetString());
			if(item.pSequence == NULL)
			{
				CString warningMsg;
				warningMsg.Format("A sequence of '%s' not found! This'll be skipped.", seqName.GetString());
				MessageBox(warningMsg, "Sequence not found", MB_OK|MB_ICONWARNING);
				continue;
			}

			// director node
			CString directorName = itemNode->getAttr("director");
			for(int k=0; k<item.pSequence->GetNodeCount(); ++k)
			{
				IAnimNode *pNode = item.pSequence->GetNode(k);
				if(pNode->GetType() == eAnimNodeType_Director && directorName.Compare(pNode->GetName()) == 0)
				{
					item.pDirectorNode = pNode;
					break;
				}
			}
			if(item.pDirectorNode == NULL)
			{
				CString warningMsg;
				warningMsg.Format("A director node of '%s' not found in the sequence of '%s'! This'll be skipped.", 
													directorName.GetString(), seqName.GetString());
				MessageBox(warningMsg, "Director node not found", MB_OK|MB_ICONWARNING);
				continue;
			}
			
			// frame range
			itemNode->getAttr("startframe", item.frameRange.start);
			itemNode->getAttr("endframe", item.frameRange.end);

			// resolution
			itemNode->getAttr("width", item.resW);
			itemNode->getAttr("height", item.resH);

			// fps
			itemNode->getAttr("fps", item.fps);

			// format
			itemNode->getAttr("format", item.formatIndex);

			// buffer
			itemNode->getAttr("bufferstocapture", item.bufferIndex);

			// prefix
			item.prefix = itemNode->getAttr("prefix");

			// create_video
			itemNode->getAttr("createvideo", item.bCreateVideo);

			// folder
			item.folder = itemNode->getAttr("folder");

			// cvars
			for(int k=0; k<itemNode->getChildCount(); ++k)
			{
				CString cvar = itemNode->getChild(k)->getContent();
				item.cvars.push_back(cvar);
			}

			AddItem(item);
		}
	}
}

void CSequenceBatchRenderDialog::OnSaveBatch()
{
	CString savePath;
	if(CFileUtil::SelectSaveFile("Render Batch Files (*.batch)|*.batch||", "batch", 
		Path::GetUserSandboxFolder(), savePath))
	{
		XmlNodeRef batchRenderListNode = XmlHelpers::CreateXmlNode("batchrenderlist");
		batchRenderListNode->setAttr("version", kBatchRenderFileVersion);

		for(size_t i=0; i<m_renderItems.size(); ++i)
		{
			const SRenderItem& item = m_renderItems[i];
			XmlNodeRef itemNode = batchRenderListNode->newChild("item");

			// sequence
			itemNode->setAttr("sequence", item.pSequence->GetName());

			// director node
			itemNode->setAttr("director", item.pDirectorNode->GetName());
			
			// frame range
			itemNode->setAttr("startframe", item.frameRange.start);
			itemNode->setAttr("endframe", item.frameRange.end);

			// resolution
			itemNode->setAttr("width", item.resW);
			itemNode->setAttr("height", item.resH);

			// fps
			itemNode->setAttr("fps", item.fps);

			// format
			itemNode->setAttr("format", item.formatIndex);

			// buffer
			itemNode->setAttr("bufferstocapture", item.bufferIndex);

			// prefix
			itemNode->setAttr("prefix", item.prefix.GetString());

			// create_video
			itemNode->setAttr("createvideo", item.bCreateVideo);

			// folder
			itemNode->setAttr("folder", item.folder.GetString());

			// cvars
			for(size_t k=0; k<item.cvars.size(); ++k)
			{
				itemNode->newChild("cvar")->setContent(item.cvars[k].GetString());
			}
		}

		XmlHelpers::SaveXmlNode(batchRenderListNode, savePath);
	}
}

bool CSequenceBatchRenderDialog::SetUpNewRenderItem(SRenderItem& item)
{
	CString seqName, shotName;
	m_sequenceCombo.GetWindowText(seqName);
	m_shotCombo.GetWindowText(shotName);
	// folder
	m_destinationEdit.GetWindowText(item.folder);
	if(item.folder.IsEmpty())
	{
		MessageBox("The output folder should be specified!", "Cannot add", MB_OK|MB_ICONERROR);
		return false;
	}
	// sequence
	item.pSequence = GetIEditor()->GetMovieSystem()->FindSequence(seqName.GetString());
	assert(item.pSequence);
	// director
	for(int i=0; i<item.pSequence->GetNodeCount(); ++i)
	{
		IAnimNode *pNode = item.pSequence->GetNode(i);
		if(pNode->GetType() == eAnimNodeType_Director && shotName.Compare(pNode->GetName()) == 0)
		{
			item.pDirectorNode = pNode;
			break;
		}
	}
	if(item.pDirectorNode == NULL)
		return false;
	// frame range
	item.frameRange = Range(m_startFrame.GetValue()/m_fpsForTimeToFrameConversion, 
		m_endFrame.GetValue()/m_fpsForTimeToFrameConversion);
	// fps
	if(m_fpsCombo.GetCurSel() == CB_ERR)
		item.fps = m_customFPS;
	else
		item.fps = fps[m_fpsCombo.GetCurSel()].fps;
	// buffer
	item.bufferIndex = m_buffersToCaptureCombo.GetCurSel();
	// prefix
	GetDlgItem(IDC_BATCH_RENDER_FILE_PREFIX)->GetWindowText(item.prefix);
	// format
	item.formatIndex = m_imageFormatCombo.GetCurSel() % arraysize(imageFormats);
	// create_video
	item.bCreateVideo = m_createVideoCheckBox.GetCheck() == BST_CHECKED;
	// resolution
	int curResSel = m_resolutionCombo.GetCurSel();
	if(curResSel < arraysize(resolutions))
	{
		item.resW = resolutions[curResSel][0];
		item.resH = resolutions[curResSel][1];
	}
	else
	{
		item.resW = m_customResW;
		item.resH = m_customResH;
	}
	// cvars
	const int kCVarNameMaxSize = 256;
	char buf[kCVarNameMaxSize]; buf[kCVarNameMaxSize-1] = 0;
	for(int i=0; i<m_cvarsEdit.GetLineCount(); ++i)
	{
		m_cvarsEdit.GetLine(i, buf, kCVarNameMaxSize-1);
		if(strlen(buf) > 0)
			item.cvars.push_back(CString(buf));
	}

	return true;
}

void CSequenceBatchRenderDialog::AddItem(const SRenderItem& item)
{
	// Add the item.
	m_renderItems.push_back(item);

	// Add it to the list box, too.
	m_renderList.AddString(GetCaptureItemString(item));

	m_pGoBtn->EnableWindow(TRUE);
}

CString CSequenceBatchRenderDialog::GetCaptureItemString(const SRenderItem& item) const
{
	CString itemText;
	itemText.Format("%s_%s_%d-%d(%dx%d,%d,%s)%s", item.pSequence->GetName(), 
		item.pDirectorNode->GetName(), 
		int(item.frameRange.start * m_fpsForTimeToFrameConversion), 
		int(item.frameRange.end * m_fpsForTimeToFrameConversion), 
		item.resW, item.resH, item.fps, buffersToCapture[item.bufferIndex],
		item.bCreateVideo?"[v]":"");

	return itemText;
}
void CSequenceBatchRenderDialog::OnBuffersSelected()
{
	const int kColorBuffer = 0;
	const int kHDREtcBuffer = 1;
	const int kStereoBuffer = 2;
	int curSel = m_buffersToCaptureCombo.GetCurSel();
	if(curSel == kColorBuffer)
	{
		while(m_imageFormatCombo.GetCount() > arraysize(imageFormats))
			m_imageFormatCombo.DeleteString(m_imageFormatCombo.GetCount()-1);
		m_imageFormatCombo.EnableWindow(TRUE);
		m_imageFormatCombo.SetCurSel(0);
	}
	else
	{
		if(curSel == kHDREtcBuffer)
		{
			int index = m_imageFormatCombo.AddString("multi");
			m_imageFormatCombo.SetCurSel(index);
		}
		else if(curSel == kStereoBuffer)
		{
			m_imageFormatCombo.SetCurSel(2);	// Stereo capturing uses .tga ATM.
		}
		m_imageFormatCombo.EnableWindow(FALSE);
	}

	UpdateCreateVideoCheckBox();
}

void CSequenceBatchRenderDialog::OnImageFormatSelected()
{
	UpdateCreateVideoCheckBox();
}

void CSequenceBatchRenderDialog::UpdateCreateVideoCheckBox()
{
	const int kColorBuffer = 0, kJPG = 0;
	bool bColorBufferSelected = m_buffersToCaptureCombo.GetCurSel() == kColorBuffer;
	bool bJPGFormatSelected = m_imageFormatCombo.GetCurSel() == kJPG;
	if(bColorBufferSelected && bJPGFormatSelected)
		m_createVideoCheckBox.EnableWindow(TRUE);
	else
		m_createVideoCheckBox.EnableWindow(FALSE);
}