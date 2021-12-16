// AnimationBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "AnimationBrowser.h"
#include "AnimationInfoLoader.h"
#include <I3DEngine.h>
#include <ICryAnimation.h>
#include "ModelViewport.h"
#include "ModelViewportCE.h"
#include "CryCharMorphParams.h"
#include "CharacterEditor.h"
#include "Material/MaterialManager.h"
#include "CharPanel_Preset.h"
#include "Clipboard.h"

#include "Util\UIEnumerations.h"

#define IDC_REPORT_CONTROL 1

#define COLUMN_ANIM_NAME 0
#define COLUMN_ANIM_LENGTH 1
#define COLUMN_ANIM_SIZE 2
#define COLUMN_ANIM_POS_KEY_NUM 3
#define COLUMN_ANIM_ROT_KEY_NUM 4

#define ID_COPY_TO_CLIPBOARD 10

static int MAXIMUM_HISTORY_SIZE = 50;
enum ECE_Panel_Animations_Icons
{
	ICON_AIMPOSE,
	ICON_FOLDER,
	ICON_LOOPING,
	ICON_LOOPINGIDLE,
	ICON_NONLOOPING,
	ICON_MISSING,
	ICON_ONDEMAND,
	ICON_LMG,
	ICON_AMC,
	ICON_ADDITIVE
};

class AnimationBrowser_AnimRecord : public CXTPReportRecord
{
	DECLARE_DYNAMIC(AnimationBrowser_AnimRecord)

public:
	AnimationBrowser_AnimRecord(AnimationBrowser_AnimRecord* pParent, bool bIsGroup, bool bIsMorph, const CString& groupName, const CString& name,int icon, 
		const char* toolTips, const size_t animSize, const uint32 numPosKeys, const uint32 numRotKeys, uint32 iLength=0)
		: m_pParent(pParent), m_groupName(groupName), m_name(name), m_bIsGroup(bIsGroup)
	{
		CXTPReportRecordItem* pNewItem = new CXTPReportRecordItemText( (bIsGroup) ? groupName : name );
		pNewItem->SetTooltip(toolTips);
		pNewItem->SetIconIndex(icon);
	
		AddItem(pNewItem);
		if (!bIsGroup && !bIsMorph)
		{
			AddItem( new CXTPReportRecordItemNumber( (double)iLength,_T("%.0f") ) );
			AddItem( new CXTPReportRecordItemNumber( (double)animSize ,_T("%.0f") ) );
			AddItem( new CXTPReportRecordItemNumber( (double)numPosKeys ,_T("%.0f") ) );
			AddItem( new CXTPReportRecordItemNumber( (double)numRotKeys ,_T("%.0f") ) );
		}
	}
	bool IsGroup() const { return m_bIsGroup; }
	const CString& GetName() const { return m_name; }
	const CString& GetGroupName() const { return m_groupName; }
	AnimationBrowser_AnimRecord* GetParent() const { return m_pParent; }
protected:
	AnimationBrowser_AnimRecord* m_pParent;
	CString m_groupName;
	CString m_name;
	bool m_bIsGroup;
};
IMPLEMENT_DYNAMIC(AnimationBrowser_AnimRecord,CXTPReportRecord);


CAnimationBrowser::CAnimationBrowser()
{
	bSelMotionFromHistory = false;

	m_pModelViewportCE = 0;
	m_pAnimationGraphDialog = 0;
	m_pCharacterEditor = 0;
	m_pCharacterInstance = NULL;

	GetIEditor()->RegisterNotifyListener( this );
}

CAnimationBrowser::~CAnimationBrowser()
{
	GetIEditor()->UnregisterNotifyListener( this );
}

BEGIN_MESSAGE_MAP(CAnimationBrowser, CDialog)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_NOTIFY(NM_RCLICK, IDC_REPORT_CONTROL, OnReportItemRClick)
	ON_NOTIFY(XTP_NM_REPORT_SELCHANGED, IDC_REPORT_CONTROL, OnReportSelChange)
	ON_NOTIFY(NM_DBLCLK, IDC_REPORT_CONTROL, OnReportItemDblClick)
	ON_NOTIFY(EN_CHANGE, ID_CE_FILTER_TEXT, OnFilterText)
	ON_XTP_EXECUTE(ID_MOTION_BROWSER_SELECT_CHARACTER, OnSelectCharacters)
	ON_XTP_EXECUTE(ID_MOTION_BROWSER_HISTORY, OnSelectMotionFromHistory)
	ON_COMMAND( ID_CE_RELOAD,ReloadAnimations )
END_MESSAGE_MAP()

void CAnimationBrowser::OnSelectMotionFromHistory(NMHDR* pNMHDR, LRESULT* pResult)
{
	bSelMotionFromHistory = true;
	std::vector<CString> anims;

	if (GetSelectedAnimations(anims))
	{
		PlaySelectedAnimations(anims);
	}
}

BOOL CAnimationBrowser::CreateBitmapToolbarDynamic(CImageList& imageList)
{
	imageList.DeleteImageList();

	uint32 iType[] = {IDB_ANIMATIONS_TREE_LMG, IDB_ANIMATIONS_TREE_AIMPOSE, IDB_ANIMATIONS_TREE_AMC, IDB_ANIMATIONS_TREE_NORMAL};
	int32 iTypeSize = 4;

	uint32 iLoop[] = {IDB_ANIMATIONS_TREE_LOOPING, IDB_ANIMATIONS_TREE_LOOP_IDLE, IDB_ANIMATIONS_TREE_NONLOOPING};
	int32 iLoopsize = 3;

	uint32 iFeature[] = {IDB_ANIMATIONS_TREE_ADDITIVE, IDB_ANIMATIONS_TREE_OVERRIDE};
	int32 iFeatureSize = 2;

	uint32 iOnDemand[] = {IDB_ANIMATIONS_TREE_ON_DEMAND, IDB_ANIMATIONS_TREE_STATIC};
	int32 iOnDemandSize = 2;

	std::vector<uint32> iconIds;
	for(int32 i=0; i<4; ++i)
		iconIds.push_back(IDB_ANIMATIONS_TREE_MISSING);

	for(int32 i=0; i< iTypeSize; ++i)
	{
		for(int32 j=0; j< iLoopsize; ++j)
		{
			for(int32 m=0; m< iFeatureSize; ++m)
			{
				for(int32 n=0; n< iOnDemandSize; ++n)
				{
					iconIds.push_back(iType[i]);
					iconIds.push_back(iLoop[j]);
					iconIds.push_back(iFeature[m]);
					iconIds.push_back(iOnDemand[n]);
				}
			}
		}
	}

	LoadAndCombineTrueColorImages( imageList, iconIds, 16, 16, RGB(255,0,255) );
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
BOOL CAnimationBrowser::OnInitDialog()
{
	BOOL bRes = __super::OnInitDialog();
	if (!bRes)
		return bRes;

	// Create filter Toolbar.
	{
		VERIFY(m_wndToolBar.CreateToolBar(WS_VISIBLE|WS_CHILD|CBRS_TOOLTIPS|CBRS_GRIPPER, this, AFX_IDW_TOOLBAR));
		VERIFY(m_wndToolBar.LoadToolBar(IDR_MOTION_BROWSER_TOOLBAR));
		m_wndToolBar.SetFlags(xtpFlagAlignTop|xtpFlagStretched);
		
		// Filter control
		CXTPControl *pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_CE_FILTER_TEXT, TRUE, FALSE);
		if (pCtrl)
		{
			int nIndex = pCtrl->GetIndex();
			CXTPControlEdit *pEdit = (CXTPControlEdit*)m_wndToolBar.GetControls()->SetControlType(nIndex,xtpControlEdit);
			pEdit->SetFlags(xtpFlagManualUpdate);
			pEdit->SetWidth(80);
		}

		// Filter label
		pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_CE_FILTER_LABEL, TRUE, FALSE);
		if (pCtrl)
		{
			int nIndex = pCtrl->GetIndex();
			CXTPControlLabel *pLabelCtrl = (CXTPControlLabel*)m_wndToolBar.GetControls()->SetControlType(nIndex,xtpControlLabel);
			pLabelCtrl->SetCaption( "Filter" );
			pLabelCtrl->SetFlags(xtpFlagManualUpdate);
			pLabelCtrl->SetStyle(xtpButtonCaption);
		}

		//Add "choose character" ComboBox
		pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_MOTION_BROWSER_SELECT_CHARACTER, TRUE, FALSE);
		if(pCtrl)
		{
			int nIndex = pCtrl->GetIndex();
			CXTPControlComboBox *pComboCtrl = static_cast<CXTPControlComboBox*> (m_wndToolBar.GetControls()->SetControlType(nIndex, xtpControlComboBox) );
			pComboCtrl->SetWidth(100);
		}


		// "motion history" label
		pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_MOTION_HISTORY_LABEL, TRUE, FALSE);
		if(pCtrl)
		{
			int nIndex = pCtrl->GetIndex();
			CXTPControlLabel *pLabelCtrl = static_cast<CXTPControlLabel*> (m_wndToolBar.GetControls()->SetControlType(nIndex, xtpControlLabel) );
			pLabelCtrl->SetCaption("History");
			pLabelCtrl->SetWidth(60);
		}

		//Add "motion history" ComboBox
		pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_MOTION_BROWSER_HISTORY, TRUE, FALSE);
		if(pCtrl)
		{
			int nIndex = pCtrl->GetIndex();
			CXTPControlComboBox *pComboCtrl = static_cast<CXTPControlComboBox*> (m_wndToolBar.GetControls()->SetControlType(nIndex, xtpControlComboBox) );
			pComboCtrl->SetWidth(230);
		}
	}

	CRect rc(0,0,0,0);

	m_wndReport.Create(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN, rc, this, IDC_REPORT_CONTROL );
	m_wndReport.ModifyStyleEx( 0, WS_EX_STATICEDGE );

	CreateBitmapToolbarDynamic(m_imageList);
	
	m_wndReport.SetImageList(&m_imageList);


	CXTPReportColumn* col = new CXTPReportColumn(COLUMN_ANIM_NAME, _T("Animation"), 300, TRUE, XTP_REPORT_NOICON, TRUE, TRUE);
	col->SetMinWidth(300);
	CXTPReportColumn *pTreeCol = m_wndReport.AddColumn(col);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_ANIM_LENGTH, _T("Frame"), 10, TRUE, XTP_REPORT_NOICON, TRUE, TRUE))->SetAlignment(DT_CENTER);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_ANIM_SIZE, _T("Size"), 20, TRUE, XTP_REPORT_NOICON, TRUE, TRUE))->SetAlignment(DT_CENTER);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_ANIM_POS_KEY_NUM, _T("Controller"), 20, TRUE, XTP_REPORT_NOICON, TRUE, TRUE))->SetAlignment(DT_CENTER);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_ANIM_ROT_KEY_NUM, _T("Controller"), 20, TRUE, XTP_REPORT_NOICON, TRUE, TRUE))->SetAlignment(DT_CENTER);

	pTreeCol->SetTreeColumn(true);
	pTreeCol->SetSortable(TRUE);
	m_wndReport.GetColumns()->SetSortColumn(pTreeCol, true);
	m_wndReport.GetReportHeader()->AllowColumnRemove(FALSE);
	m_wndReport.ShadeGroupHeadings(FALSE);
	m_wndReport.SkipGroupsFocus(TRUE);
	m_wndReport.SetMultipleSelection(TRUE);
	m_wndReport.SetSortRecordChilds(TRUE); //Make the files in a group sorted.

	class CAnimationBrowser_PaintManager : public CXTPReportPaintManager
	{
	public:
		virtual int GetRowHeight(CDC* pDC, CXTPReportRow* pRow)
		{
			return __super::GetRowHeight(pDC,pRow)-1;
		}
	};

	CXTPReportPaintManager* pPMgr = new CAnimationBrowser_PaintManager();
	pPMgr->m_nTreeIndent = 0x0a;
	pPMgr->m_bShadeSortColumn = false;
	pPMgr->m_strNoItems = _T("No Animations");
	pPMgr->SetGridStyle(FALSE, xtpGridNoLines);
	pPMgr->SetGridStyle(TRUE, xtpGridNoLines);
	m_wndReport.SetPaintManager( pPMgr );

	LayOutControls();

	return bRes;
}

BOOL CAnimationBrowser::CreateBitmap(CBitmap& bitmap, const int32 width, const int32 height, const BYTE* pData)
{
	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth  = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	
	CPaintDC dc(this);
	bitmap.CreateCompatibleBitmap(&dc, width, height);
	::SetDIBits(dc.m_hDC,bitmap,0,height,pData,&bmi,DIB_RGB_COLORS);
	CDC dcMemory;
	dcMemory.CreateCompatibleDC(&dc);
	CBitmap * pOldBitmap = dcMemory.SelectObject(&bitmap);
	dc.BitBlt(0, 0, width, height, &dcMemory, 0, 0, SRCCOPY);
	dcMemory.SelectObject(pOldBitmap);

	return TRUE;
}
//------------------------------------------------------------------------------
// Combine several bitmap to make a bigger bitmap
// Assume all the icons has the same sizes
BOOL CAnimationBrowser::LoadAndCombineTrueColorImages( CImageList& imageList, std::vector<UINT>& nIDResources, int nIconWidth, int nIconHeight, COLORREF colMaskColor)
{
	int32 count = (UINT)nIDResources.size();
	int32 finalWidth = nIconWidth * 4;
	int32 finalHeight = nIconHeight;

	int32 bytesWidth = nIconWidth * 3;
	int32 finalBytesWidth = finalWidth * 3;

	BYTE* finalData = new BYTE[3*finalWidth*finalHeight];
	BYTE* pFinal = finalData;

	imageList.DeleteImageList();

	if (!imageList.Create(finalWidth, finalHeight, ILC_COLOR24|ILC_MASK, 1, 0))
	{
		delete[] finalData;
		finalData = NULL;
		return FALSE;
	}

	assert(count % 4 == 0);

	for(int32 i=0; i<count; i+=4)
	{
		for(int32 j=0; j<4; ++j)
		{
			CBitmap bitmap;
			BITMAP bmBitmap;
			if (!bitmap.Attach(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResources[i+j]),IMAGE_BITMAP, 0, 0,LR_DEFAULTSIZE|LR_CREATEDIBSECTION)))
			{
				delete[] finalData;
				finalData = NULL;
				return FALSE;
			}

			if (!bitmap.GetBitmap(&bmBitmap))
			{
				delete[] finalData;
				finalData = NULL;
				return FALSE;
			}

			assert(bmBitmap.bmWidth == nIconWidth && bmBitmap.bmHeight == nIconHeight);

			BYTE*	pRead= (BYTE*)(bmBitmap.bmBits);

			pFinal = finalData + j*bytesWidth;
			// Add current image data to the final one
			for(int32 k=0; k<nIconHeight; ++k)
			{
				memcpy(pFinal, pRead, bytesWidth);
				//memset(pFinal, 255, bytesWidth);
				pFinal += finalBytesWidth;
				pRead += bytesWidth;
			}		
		}	
	

		CBitmap bitmap;
		CreateBitmap(bitmap, finalWidth, finalHeight, finalData);

		if (imageList.Add(&bitmap,colMaskColor) == -1)
		{
			delete[] finalData;
			finalData = NULL;
			return FALSE;
		}
		
	}

	delete[] finalData;
	finalData = NULL;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnSize( UINT nType, int cx, int cy )
{
	if (m_wndReport)
	{
		LayOutControls();
	}
	__super::OnSize( nType,cx,cy );
}

//////////////////////////////////////////////////////////////////////////
int32 CAnimationBrowser::GetAnimIcon(const int32 nAnimId, IAnimationSet* pAnimations)
{
	int32 iconId = -1;

	//IAnimationSet* pAnimations = m_pModelViewportCE->GetCharacterAnim()->GetIAnimationSet();
	static int32 typeSize[4] = {48, 12, 4, 2}; // calculated from [3, 3, 2, 2]
	int32 typeId[4] = {-1, -1, -1, -1};

	if (pAnimations)
	{
		const char* name = pAnimations->GetNameByAnimID(nAnimId);
		if (name[0]!='#' && name[0]!='!')
		{
			uint32 flags = pAnimations->GetAnimationFlags(nAnimId);

			if ( (flags&CA_ASSET_CREATED) == 0 )
				return 0; // motion missing

			uint32 loaded = flags & CA_ASSET_LOADED;

			uint32 created = flags & CA_ASSET_CREATED;

			uint32 lmg				= flags & CA_ASSET_LMG;
			uint32 lmgvalid		= flags & CA_ASSET_LMG_VALID;

			lmg				|= (flags & CA_ASSET_PMG);
			lmgvalid	|= (flags & CA_ASSET_PMG_VALID);

			uint32 aimPose = flags & CA_AIMPOSE;

			
			if(lmg && created && lmgvalid)
				typeId[0] = 0;
			else if(aimPose)
				typeId[0] = 1;
			else 
				typeId[0] = 3;

			if ( flags & CA_ASSET_CYCLE ) // looping
				typeId[1] = 0;
			else // non-looping
				typeId[1] = 2;
			// loopIdle is not used

			if ( flags & CA_ASSET_ADDITIVE ) // additive
				typeId[2] = 0;
			else
				typeId[2] = 1; //override
			
			if ( flags & CA_ASSET_ONDEMAND ) // on demand
				typeId[3] = 0;
			else // static
				typeId[3] = 1;
			
		}

		iconId = typeId[0] * typeSize[1] + typeId[1]*typeSize[2] + typeId[2]*typeSize[3] + typeId[3];
	}

	return iconId +1; // skip the first 64x16 "missing icon"
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::ReloadAnimations()
{
	if(m_pModelViewportCE)
		UpdateAnimations(m_pModelViewportCE->GetCharacterBase());
	else if(m_pCharacterInstance)
		UpdateAnimations(m_pCharacterInstance);
}

void CAnimationBrowser::RemoveAll()
{
	m_wndReport.BeginUpdate();
	m_wndReport.GetRecords()->RemoveAll();
	m_wndReport.EndUpdate();
	m_wndReport.Populate();

}
void CAnimationBrowser::UpdateCharacterComboBoxSelection(const int ind)
{
	//------------------------------------------------------------------------------
	// Set combobox content
	CXTPControls* pCtrls = NULL;
	pCtrls = m_wndToolBar.GetControls();
	if(!pCtrls)
		return;

	CXTPControlComboBox* pComboCtrl = static_cast<CXTPControlComboBox*> ( ( pCtrls->FindControl(xtpControlComboBox, ID_MOTION_BROWSER_SELECT_CHARACTER, TRUE, FALSE)) );

	pComboCtrl->SetCurSel(ind);

	if(m_pModelViewportCE)
	{
		m_pModelViewportCE->SetCharacterAnim(m_pModelViewportCE->GetCharacterAnim());
		m_pModelViewportCE->UpdateAnimationList();
	}
}

void CAnimationBrowser::UpdateCharacterComboBox(const std::vector<const char*>& attachmentNames, const std::vector<int>& posInAttachBrowser)
{
	if(attachmentNames.size()!=posInAttachBrowser.size())
		return;

	//------------------------------------------------------------------------------
	// Set combobox content
	CXTPControls* pCtrls = NULL;
	pCtrls = m_wndToolBar.GetControls();
	if(!pCtrls)
		return;

	CXTPControlComboBox* pComboCtrl = static_cast<CXTPControlComboBox*> ( ( pCtrls->FindControl(xtpControlComboBox, ID_MOTION_BROWSER_SELECT_CHARACTER, TRUE, FALSE)) );

	pComboCtrl->ResetContent();
	pComboCtrl->AddString("Base Character");

	for(int i=0; i<(int)attachmentNames.size(); ++i)
	{
		pComboCtrl->AddString(attachmentNames[i]);
	}

	//------------------------------------------------------------------------------
	// Set name id maps

	SAttachmentIDs id;
	int indCombo = 1;
	for(int i=0; i<(int)attachmentNames.size(); ++i)
	{
		id.iIndexAttachBrowser = posInAttachBrowser[i];
		id.iIndexComboBox = indCombo;
		++indCombo;
		m_attachmentIDs.push_back(id);
	}

	pComboCtrl->SetCurSel(0);
}
//////////////////////////////////////////////////////////////////////////
CAnimationBrowser*&	CAnimationBrowser::GetCurrentInstance()
{
	static CAnimationBrowser*	poCurrentInstance(NULL);
	return poCurrentInstance;
}
//////////////////////////////////////////////////////////////////////////
bool CAnimationBrowser::GetSelectedAnimations( TDSelectedAnimations &anims )
{
	anims.clear();

	if(!bSelMotionFromHistory){
		POSITION pos = m_wndReport.GetSelectedRows()->GetFirstSelectedRowPosition();
		while (pos)
		{
			AnimationBrowser_AnimRecord *pRec = DYNAMIC_DOWNCAST(AnimationBrowser_AnimRecord,m_wndReport.GetSelectedRows()->GetNextSelectedRow( pos )->GetRecord());

			if (pRec && pRec->GetName().GetLength() >0)
			{
				anims.push_back(pRec->GetName());
			}
		}
	}
	else{

		//------------------------------------------------------------------------------
		//  Unselect all the motions in motion browser

		CXTPReportSelectedRows* rows = m_wndReport.GetSelectedRows();
		for(int i=0; i<rows->GetCount(); ++i)
		{
			rows->GetAt(i)->SetSelected(FALSE);
		}

		//------------------------------------------------------------------------------
		//  
		CXTPControls* pCtrls = NULL;
		pCtrls = m_wndToolBar.GetControls();
		if(!pCtrls)
			return false;
		CXTPControlComboBox* pComboCtrl = static_cast<CXTPControlComboBox*> ( ( pCtrls->FindControl(xtpControlComboBox, ID_MOTION_BROWSER_HISTORY, TRUE, FALSE)) );

		CString animName;
		pComboCtrl->GetLBText(pComboCtrl->GetCurSel(), animName);
		if(animName.GetLength()>0)
			anims.push_back(animName);
	}

	//------------------------------------------------------------------------------
	// If user selected two motions (non-LMG, non-PMG), enable dummyPMG
	if(anims.size() == 2 && m_pModelViewportCE)
	{
		IAnimationSet* pAnimations = m_pModelViewportCE->GetCharacterBase()->GetIAnimationSet();
		int32 globalAnimId[2] = {-1, -1};
		if(pAnimations)
		{
			globalAnimId[0] = pAnimations->GetGlobalIDByName(anims[0]);
			globalAnimId[1] = pAnimations->GetGlobalIDByName(anims[1]);
			uint32 flag[2];
			flag[0] = pAnimations->GetAnimationFlags( pAnimations->GetAnimIDByName(anims[0]));
			flag[1] = pAnimations->GetAnimationFlags( pAnimations->GetAnimIDByName(anims[1]));
			bool bLmg[2], bPmg[2];
			bLmg[0] = flag[0] & CA_ASSET_LMG;
			bLmg[1] = flag[1] & CA_ASSET_LMG;
			bPmg[0] = flag[0] & CA_ASSET_PMG;
			bPmg[1] = flag[1] & CA_ASSET_PMG;

			if(!bLmg[0] && !bPmg[0] && !bLmg[1] && !bPmg[1])
			{
				int32 globalDummyPMGId = pAnimations->GetGlobalIDByName("_PMG_Dummy");
				pAnimations->SetDummyPMGAnimName(globalDummyPMGId, anims[0].GetBuffer(), anims[1].GetBuffer());
				anims.clear();
				anims.push_back("_PMG_Dummy");
			}
		}
	}
	
	return !anims.empty();
}

void CAnimationBrowser::OnSelectedPMG(const std::vector<CString> anims)
{
	//------------------------------------------------------------------------------
	// Reset PMG sliders and edits
	if(anims.empty())
		return;

	IAnimationSet* pAnimations = m_pModelViewportCE->GetCharacterBase()->GetIAnimationSet();
	int32 globalAnimId = -1;
	if(pAnimations)
		globalAnimId = pAnimations->GetGlobalIDByName(anims[0]);

	uint32 ind = 0;
	f32 minV = .0f, maxV = .0f;
	uint32 pmgParamDim = pAnimations->GetPMGParamDim(globalAnimId);

	ISkeletonAnim* pISkeletonAnim = m_pModelViewportCE->GetCharacterAnim()->GetISkeletonAnim();

	std::map<string, f32>& prePmgParamMap = m_pModelViewportCE->GetModelPanelA()->GetPrePmgParamMap();
	std::map<string, f32> prePmgParamMapOld = prePmgParamMap;

	prePmgParamMap.clear();
	if(pISkeletonAnim)
		pISkeletonAnim->ResetControlParam();	

	for(uint32 i=0; i<pmgParamDim; ++i)
	{
		pAnimations->GetPMGParamLimits(globalAnimId, i, minV, maxV);
		string name = pAnimations->GetPMGParamName(globalAnimId, i);
		m_pModelViewportCE->GetModelPanelA()->m_ControlParamName[i].SetWindowText(name.c_str());
		m_pModelViewportCE->GetModelPanelA()->m_ControlParamName[i].ShowWindow(TRUE);

		//f32 preVal = m_pModelViewportCE->GetModelPanelA()->m_ControlParamEdit[i].GetValue();
		f32 preVal = 0.0f;
		preVal = stl::find_in_map(prePmgParamMapOld, name, -1e+20f);
		if(fabs (preVal+1e+20f) < 1e-6f) // Doesn't exist
		{
			preVal = 0.0f;
			if(name == "MoveSpeed")
				preVal = 3.0f;
		}				

		if(preVal < minV)
			preVal = minV;
		else if(preVal > maxV)
			preVal = maxV;

		prePmgParamMap[name] = preVal;

		m_pModelViewportCE->GetModelPanelA()->m_ControlParam[i].SetRangeFloat(minV, maxV);
		m_pModelViewportCE->GetModelPanelA()->m_ControlParamEdit[i].SetRange(minV, maxV);

		m_pModelViewportCE->GetModelPanelA()->m_ControlParam[i].SetValue(preVal);
		m_pModelViewportCE->GetModelPanelA()->m_ControlParamEdit[i].SetValue(preVal);

		if(pISkeletonAnim)
			pISkeletonAnim->SetControlParam(preVal, name.c_str(), i);

		m_pModelViewportCE->GetModelPanelA()->m_ControlParam[i].EnableWindow(TRUE);
		m_pModelViewportCE->GetModelPanelA()->m_ControlParamEdit[i].EnableWindow(TRUE);	

		m_pModelViewportCE->GetModelPanelA()->m_ControlParam[i].ShowWindow(TRUE);
		m_pModelViewportCE->GetModelPanelA()->m_ControlParamEdit[i].ShowWindow(TRUE);	
	}

	for(uint32 i=pmgParamDim; i<MAX_PMG_PARAM_DIM; ++i)
	{
		m_pModelViewportCE->GetModelPanelA()->m_ControlParam[i].ShowWindow(FALSE);
		m_pModelViewportCE->GetModelPanelA()->m_ControlParamEdit[i].ShowWindow(FALSE);	
		m_pModelViewportCE->GetModelPanelA()->m_ControlParamName[i].ShowWindow(FALSE);	
	}


}

void CAnimationBrowser::OnReportSelChange( NMHDR * pNotifyStruct, LRESULT *result )
{ 
	bSelMotionFromHistory = false;

	m_wndReport.GetToolTipContext()->SetStyle(xtpToolTipStandard);
	std::vector<CString> anims;

	GetSelectedAnimations(anims);

	if(anims.empty())
		return;

	// Updating the proxy.
	CUIEnumerations::GetUIEnumerationsInstance().GetSelectedAnimations()=anims;

	CXTPControls* pCtrls = NULL;
	pCtrls = m_wndToolBar.GetControls();
	if(!pCtrls)
		return;

	CXTPControlComboBox* pComboCtrl = static_cast<CXTPControlComboBox*> ( ( pCtrls->FindControl(xtpControlComboBox, ID_MOTION_BROWSER_HISTORY, TRUE, FALSE)) );

	//------------------------------------------------------------------------------
	// if it's full, remove anims.size() items from the box
	if(pComboCtrl->GetCount() >= MAXIMUM_HISTORY_SIZE)
	{
		for(int i=0; i< (int)anims.size(); ++i)
		{
			pComboCtrl->DeleteItem (pComboCtrl->GetCount()-1);
		}				
	}

	//------------------------------------------------------------------------------
	// Add new history at the beginning
	for(int i=0; i< (int)anims.size(); ++i){
		if(pComboCtrl->FindString(-1, anims[i]) == LB_ERR)
			pComboCtrl->InsertString(0, anims[i].GetBuffer());
	}

	pComboCtrl->SetCurSel(0);

	if(m_pModelViewportCE)
	{
		IAnimationSet* pAnimations = m_pModelViewportCE->GetCharacterBase()->GetIAnimationSet();
		int32 globalAnimId = -1;
		if(pAnimations)
			globalAnimId = pAnimations->GetGlobalIDByName(anims[0]);
		uint32 flag = pAnimations->GetAnimationFlags( pAnimations->GetAnimIDByName(anims[0]));
		bool bPmg = flag & CA_ASSET_PMG;

		if(bPmg)
			OnSelectedPMG(anims);	
		else // If not PMG, hide all the PMG controls
		{
			for(uint32 i=0; i<MAX_PMG_PARAM_DIM; ++i)
			{
				m_pModelViewportCE->GetModelPanelA()->m_ControlParam[i].ShowWindow(FALSE);
				m_pModelViewportCE->GetModelPanelA()->m_ControlParamEdit[i].ShowWindow(FALSE);
				m_pModelViewportCE->GetModelPanelA()->m_ControlParamName[i].ShowWindow(FALSE);
			}
		}

		if(!m_pModelViewportCE->GetPaused())
			PlaySelectedAnimations(anims);
		else
			if( !(GetKeyState(VK_LCONTROL)& 0x80) && !(GetKeyState(VK_RCONTROL)&0x80) )
			{
				m_pModelViewportCE->SetPaused(false);
				PlaySelectedAnimations(anims);
			}
	}

	*result = 0;
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnReportItemDblClick( NMHDR * pNotifyStruct, LRESULT *result )
{
	std::vector<CString> anims;
	GetSelectedAnimations(anims);
	PlaySelectedAnimations(anims);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::PlaySelectedAnimations(std::vector<CString>& anims)
{
	if (m_pCharacterEditor && !anims.empty())
	{
		m_pCharacterEditor->PlayAnimations( anims );
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnFilterText(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = FALSE; //Unhandled

	NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;

	m_filterText = "";

	CXTPControlEdit* pControl = (CXTPControlEdit*)m_wndToolBar.GetControls()->FindControl(xtpControlEdit, ID_CE_FILTER_TEXT, TRUE, FALSE);
	if (pControl && pControl->GetType() == xtpControlEdit)
	{
		CString str;
		pControl->GetEditCtrl()->GetWindowText(str);
		m_filterText = (const char*)str;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (event == eNotify_OnIdleUpdate)
	{
		if (m_filterTextPrev != m_filterText)
		{
			m_filterTextPrev = m_filterText;
			if(m_pModelViewportCE)
				m_pModelViewportCE->UpdateAnimationList();
		}
	}
}

void CAnimationBrowser::OnSelectCharacters(NMHDR* pNMHDR, LRESULT* pResult)
{
	CXTPControls* pCtrls = NULL;
	CXTPControlComboBox *pComboCtrl = NULL;

	pCtrls = m_wndToolBar.GetControls();
	if(!pCtrls)
		return;

	pComboCtrl = static_cast<CXTPControlComboBox*> ( ( pCtrls->FindControl(xtpControlComboBox, ID_MOTION_BROWSER_SELECT_CHARACTER, TRUE, FALSE) ) );
	if(!pComboCtrl)
		return;

	if(m_pModelViewportCE == NULL || m_pCharacterEditor == NULL)
		return;

	int iIndCombo = pComboCtrl->GetCurSel();
	pComboCtrl->SetCurSel(iIndCombo);
	if(iIndCombo == 0)
	{
		m_pModelViewportCE->SetCharacterAnim(m_pModelViewportCE->GetCharacterBase());
		UpdateAnimations(m_pModelViewportCE->GetCharacterBase());
		return;
	}
	else
	{
		m_pModelViewportCE->SetCharacterAnim(m_pModelViewportCE->GetCharacterAnim());
		m_pModelViewportCE->UpdateAnimationList();
	}

	//------------------------------------------------------------------------------
	// update the selected item in attachment dialog
	int iIndAttach;

	for(int i=0; i<(int)m_attachmentIDs.size(); ++i)
	{
		if(m_attachmentIDs[i].iIndexComboBox == iIndCombo)
			iIndAttach = m_attachmentIDs[i].iIndexAttachBrowser;
	}

	m_pCharacterEditor->GetAttachmentDlg()->m_attachmentsList.SetCurSel(iIndAttach);
	m_pCharacterEditor->GetAttachmentDlg()->OnAttachmentSelect();
}

CString CAnimationBrowser::GenerateToolTips(IAnimationSet* pAnimations, const int animId)
{
	CString text;
	const char* name = pAnimations->GetNameByAnimID(animId);
	if (name)
	{
		if (name[0]=='#')
		{
			text.Format( "%s \n Morph Target)", name );
		}
		else
		{
			float length    = pAnimations->GetDuration_sec(animId);
			uint32 frames   = 1 + uint32(length * 30.0f + 0.5f);
			uint32 flags    = pAnimations->GetAnimationFlags(animId);

			uint32 aimpose  = flags & CA_AIMPOSE;

			uint32 lmg      = flags & CA_ASSET_LMG;
			uint32 lmgvalid = flags & CA_ASSET_LMG_VALID;

			uint32 created  = flags & CA_ASSET_CREATED;
			uint32 ondemand = flags & CA_ASSET_ONDEMAND;
			uint32 cycle    = flags & CA_ASSET_CYCLE;

			if (lmg)
			{
				if (created && lmgvalid)
					text.Format( "%s \n Locomotion Group (LMG)", name );
				if (created==0)
					text.Format( "%s \n XML-file for LMG does not exist", name );
				if (created && lmgvalid==0)
					text.Format( "%s \n Invalid LMG (could be an XML problem)", name );
			} 
			else
			{
				if (aimpose && created)
					text.Format( "%s This is an Aim-Pose", name );
				if (created==0)
					text.Format( "%s \n CAF-file does not exist", name );
				if (created && ondemand)
					text.Format( "%s \n file is loaded or streamed \nLength: %f (%d frames)", name, length, frames );
				if (created && ondemand==0 && cycle)
					text.Format( "%s \n cycles asset \nLength: %f (%d frames)", name, length, frames  );
				if (created && ondemand==0 && cycle==0)
					text.Format( "%s \n transition asset \nLength: %f (%d frames)", name, length, frames  );
				if (created && ondemand && cycle)
					text.Format( "%s \n streamed cycles asset \nLength: %f (%d frames)", name, length, frames  );
				if (created && ondemand && cycle==0)
					text.Format( "%s \n streamed transition asset \nLength: %f (%d frames)", name, length, frames  );
			}
		}
	}

	return text;
}
void CAnimationBrowser::UpdateAnimations(ICharacterInstance* characterInstantce)
{
	if(NULL == characterInstantce)
		return;

	std::map<CString,AnimationBrowser_AnimRecord*,stl::less_stricmp<CString> > groupMap;

	m_wndReport.BeginUpdate();
	m_wndReport.GetRecords()->RemoveAll();

	m_pCharacterInstance = characterInstantce;

	//Add all animations to the animation list box/view
	IAnimationSet* pAnimations = characterInstantce->GetIAnimationSet();

	if (!pAnimations)
		return;

	char sFolder[MAX_PATH];

	uint32 numAnims = pAnimations->GetAnimationCount();
	uint32 numMorphs = pAnimations->numMorphTargets();

	for (int nAnimId = 0; nAnimId < numAnims + numMorphs; nAnimId++)
	{
		AnimationBrowser_AnimRecord* pItemGroupRec = 0;

		float fLength = 0;
		uint32 frames = 0;
		const char* pName = pAnimations->GetNameByAnimID(nAnimId);

		// Don't disclose this dummy PMG to users
		if(stricmp(pName, "_PMG_Dummy") == 0)
			continue;

		bool bMorph = ( pName[0]=='#'? true:false);

		if (!bMorph)
		{
			fLength = pAnimations->GetDuration_sec(nAnimId);
			frames   = 1 + uint32(fLength * 30.0f + 0.5f);
		}

		int32 icon = GetAnimIcon(nAnimId, pAnimations);

		//------------------------------------------------------------------------------
		// Get size of animation
		uint32 flags = pAnimations->GetAnimationFlags(nAnimId);

		uint32 aimPose    = flags & CA_AIMPOSE;
		uint32 lmg				= flags & CA_ASSET_LMG;
		uint32 pmg				= flags & CA_ASSET_LMG;

		size_t animationSize = 0;
		uint32 numPosKeys = 0;
		uint32 numRotKeys = 0;
		int32 globalAnimId = pAnimations->GetGlobalIDByName(pName);

		ISkeletonAnim* pISkeletonAnim = characterInstantce->GetISkeletonAnim();

		if(pISkeletonAnim && globalAnimId >= 0)
		{
			animationSize = pISkeletonAnim->GetAnimationSize(aimPose, lmg, pmg, globalAnimId);
			numPosKeys = pISkeletonAnim->GetToltalPosKeys(!lmg&&!pmg, globalAnimId);
			numRotKeys = pISkeletonAnim->GetToltalRotKeys(!lmg&&!pmg, globalAnimId);
		}


		const char* name = pAnimations->GetNameByAnimID(nAnimId);
		strcpy(sFolder, name);

		if (!m_filterText.IsEmpty())
		{
			if (strstri(name,m_filterText) == 0)
				continue;
		}

		char *ch = sFolder;
		while (*ch!=0 && *ch=='_')
			ch++;
		while(*ch!=0 && *ch!='_')
			ch++;

		if(!*ch) // only name no folder, e.g, "default"
		{
			*ch = 0;

			CString strToolTip = GenerateToolTips(pAnimations, nAnimId);

			AnimationBrowser_AnimRecord* pRec = new AnimationBrowser_AnimRecord(pItemGroupRec, false, bMorph, sFolder, name, icon, strToolTip.GetBuffer(), animationSize, numPosKeys, numRotKeys, frames);
			if (pItemGroupRec)
				pItemGroupRec->GetChilds()->Add(pRec);
			else
				m_wndReport.AddRecord(pRec);
		}

		if(*ch)
		{
			*ch = 0;

			AnimationBrowser_AnimRecord* pGroupRec = stl::find_in_map( groupMap,sFolder,0 );
			if (pGroupRec == 0) // Group doesn't exist, add group record
			{
				pGroupRec = new AnimationBrowser_AnimRecord(pItemGroupRec, true, bMorph, sFolder, "", -1, NULL, animationSize, numPosKeys, numRotKeys);
				if (pItemGroupRec != 0)
				{
					pItemGroupRec->GetChilds()->Add(pGroupRec);
				}
				else
				{
					m_wndReport.AddRecord(pGroupRec);
				}
				groupMap[sFolder] = pGroupRec;
				pItemGroupRec = pGroupRec;
			}
			else
			{
				pItemGroupRec = pGroupRec;
			}

			CString strToolTip = GenerateToolTips(pAnimations, nAnimId);

			AnimationBrowser_AnimRecord* pRec = new AnimationBrowser_AnimRecord(pItemGroupRec, false, bMorph, sFolder, name, icon, strToolTip.GetBuffer(), animationSize, numPosKeys, numRotKeys, frames);
			if (pItemGroupRec)
				pItemGroupRec->GetChilds()->Add(pRec);
			else
				m_wndReport.AddRecord(pRec);
		}
	}

	// Reset sort priority so that "__" >"_">others
	std::map<CString,AnimationBrowser_AnimRecord*,stl::less_stricmp<CString> >::iterator it = groupMap.begin();
	int i = 0;
	while(it != groupMap.end())
	{
		AnimationBrowser_AnimRecord* pGroupRec = it->second;
		pGroupRec->GetItem(0)->SetSortPriority(i);
		++it;
		++i;
	}

	m_wndReport.EndUpdate();
	m_wndReport.Populate();

	// Fixed the problem of blank combobox text when starting up animation browser.
	// Supposed to show "Base Character" in that box.

	CXTPControls* pCtrls = NULL;
	pCtrls = m_wndToolBar.GetControls();
	if(!pCtrls)
		return;

	CXTPControlComboBox* pComboCtrl = static_cast<CXTPControlComboBox*> ( ( pCtrls->FindControl(xtpControlComboBox, ID_MOTION_BROWSER_SELECT_CHARACTER, TRUE, FALSE)) );
	if(pComboCtrl->GetCurSel() <=0)
		pComboCtrl->SetCurSel(0);
}

void CAnimationBrowser::OnReportItemRClick( NMHDR* pNotifyStruct, LRESULT* result )
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;

	if(!pItemNotify->pColumn) // If there is no animations in the control just return.
		return;

	ASSERT(pItemNotify->pColumn);
	CPoint ptClick = pItemNotify->pt;
	
	CMenu menu;
	VERIFY(menu.CreatePopupMenu());

	CXTPReportSelectedRows* pRows = m_wndReport.GetSelectedRows();
	if(!pRows) // No rows selected
		return;

	AnimationBrowser_AnimRecord *pRec = DYNAMIC_DOWNCAST(AnimationBrowser_AnimRecord,m_wndReport.GetSelectedRows()->GetAt(0)->GetRecord()); 
	string name = pRec->GetName();
		
	// create main menu items
	menu.AppendMenu(MF_STRING, ID_COPY_TO_CLIPBOARD, _T("Copy"));

	// track menu
	int nMenuResult = CXTPCommandBars::TrackPopupMenu(&menu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN |TPM_RIGHTBUTTON, ptClick.x, ptClick.y, this, NULL);

	// other general items
	switch (nMenuResult)
	{
	case ID_COPY_TO_CLIPBOARD:
		CClipboard bd;
		bd.PutString(CString(name.c_str()));

		break;
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnClose()
{
	m_wndReport.GetColumns()->Clear();
	m_wndReport.ResetContent(FALSE);
	m_imageList.DeleteImageList();
	DestroyWindow();
	m_filterText = "";
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::LayOutControls()
{
	CRect rcClient;
	GetClientRect(rcClient);

	DWORD dwMode = LM_HORZ|LM_HORZDOCK|LM_STRETCH|LM_COMMIT;
	CSize sz = m_wndToolBar.CalcDockingLayout(32000, dwMode);

	CRect rctb = rcClient;
	rctb.bottom = rctb.top + sz.cy;
	m_wndToolBar.MoveWindow(rctb);

	rcClient.top = rctb.bottom + 1;

	CRect rctree = rcClient;
	m_wndReport.MoveWindow( rctree );
}

