#include "StdAfx.h"
#include "AnimationGraph2_Randomizer.h"
#include "AnimationGraph_2.h"


#define SLOT_OFFSET_FROM_TOP 180

//IMPLEMENT_DYNCREATE(CAnimationGraph_RandomizerDialog, CDialog);
CAnimationGraph_RandomizerDialog::CAnimationGraph_RandomizerDialog()
: CDialog(IDD_AG2_RANDOMIZER_DIALOG)
, m_slotsCreated(0)
{
}


BEGIN_MESSAGE_MAP(CAnimationGraph_RandomizerDialog, CDialog)
	ON_WM_SIZE()
	ON_WM_VSCROLL()

	ON_BN_CLICKED(IDC_BUTTON_ADD_SLOT, OnAddSlot)
	ON_BN_CLICKED(IDC_CHECK_SWITCH_ASSETS, OnGeneralParamsChanged)
	
	ON_BN_CLICKED(IDC_CHECK_TIME_ALIGN_ASSETS, OnGeneralParamsChanged)
	ON_BN_CLICKED(IDC_CHECK_SWITCH_ON_NON_LOOP_FINISH, OnGeneralParamsChanged)
	ON_EN_CHANGE(IDC_EDIT_SWITCH_TIMER, OnGeneralParamsChanged)
	
	ON_BN_CLICKED(IDC_CHECK_USE_DELAY, OnGeneralParamsChanged)
	ON_EN_CHANGE(IDC_EDIT_INITIAL_DELAY, OnGeneralParamsChanged)

	//ON_EN_CHANGE(IDC_EDIT_RANDOMIZER_NAME, OnRandomizerNameChanged)

	//	ON_EN_KILLFOCUS(IDC_ANIMATION_NAME, OnAnimationNameChanged)
END_MESSAGE_MAP()


void CAnimationGraph_RandomizerDialog::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_BUTTON_ADD_SLOT, m_addSlot );
	DDX_Control( pDX, IDC_CHECK_SWITCH_ASSETS, m_switchAssetsOverTime );
	DDX_Control( pDX, IDC_CHECK_TIME_ALIGN_ASSETS, m_timeAlignBetweenAssets );
	DDX_Control( pDX, IDC_CHECK_SWITCH_ON_NON_LOOP_FINISH, m_switchOnNonLoopFinish);
	DDX_Control( pDX, IDC_EDIT_SWITCH_TIMER, m_switchProbabilitySeconds );
	DDX_Control( pDX, IDC_CHECK_USE_DELAY, m_useInitialDelay);
	DDX_Control( pDX, IDC_EDIT_INITIAL_DELAY, m_initialDelaySeconds);
	DDX_Control( pDX, IDC_EDIT_RANDOMIZER_NAME, m_randomizerName);
}

BOOL CAnimationGraph_RandomizerDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CreateFromRandomizerData();

	GrayOutItems();

	GetParent()->RedrawWindow();

	return TRUE;
}

void CAnimationGraph_RandomizerDialog::CreateFromRandomizerData()
{
	// Write all the data provided into the dialog interfaces
	// and create slots as needed

	CString tempString = "";

	// name
	tempString = m_randomizerData->m_name;	
	tempString.Delete(0, 11);
	tempString.Delete(tempString.GetLength() -1, 1);
	m_randomizerName.SetWindowText(tempString);

	// Initial Delay
	m_useInitialDelay.SetCheck(m_randomizerData->m_randomParams.initialDelay > 0.0f);
	tempString.Format("%.2f", m_randomizerData->m_randomParams.initialDelay);
	m_initialDelaySeconds.SetWindowText(tempString);

	// Asset Switching over time
	m_switchAssetsOverTime.SetCheck(m_randomizerData->m_randomParams.switchAssetsOverTime);
	tempString.Format("%.2f", m_randomizerData->m_randomParams.switchProbabilitySeconds);
	m_switchProbabilitySeconds.SetWindowText(tempString);

	// Asset time-alignment
	m_timeAlignBetweenAssets.SetCheck(m_randomizerData->m_randomParams.timeAlignBetweenAssets);

	// Switch to next asset if a non-looping Asset finished
	m_switchOnNonLoopFinish.SetCheck(m_randomizerData->m_randomParams.switchOnNonLoopFinish);

	// Now create as many animation slots as needed and fill their data
	int slotCount = m_randomizerData->m_randomAnims.size();
	for (int i = 0; i < slotCount; ++i)
	{
		OnAddSlot();
		m_animSlots[i]->SetData(m_randomizerData->m_randomAnims[i]);
	}

	// Check whether at least one slot has been created, and if not, create a default one
	if (m_animSlots.size() < 1)
	{
		OnAddSlot();

		// TODO: set the first animation's name to state (parameterized) animation name (convenience)
	}
}

void CAnimationGraph_RandomizerDialog::ReadBackToRandomizerData()
{
	CString tempString = "";

	// name
	m_randomizerName.GetWindowText(tempString);
	m_randomizerData->m_name.Format("<RandomSet:%s>", tempString);
	// Initial Delay
	m_initialDelaySeconds.GetWindowText(tempString);
	if (!m_useInitialDelay.GetCheck())
		m_randomizerData->m_randomParams.initialDelay = 0.0f;
	else
		m_randomizerData->m_randomParams.initialDelay = atof(tempString);

	// Asset Switching over time
	m_randomizerData->m_randomParams.switchAssetsOverTime = m_switchAssetsOverTime.GetCheck();
	m_switchProbabilitySeconds.GetWindowText(tempString);
	m_randomizerData->m_randomParams.switchProbabilitySeconds = atof(tempString);

	// Asset time-alignment
	m_randomizerData->m_randomParams.timeAlignBetweenAssets = m_timeAlignBetweenAssets.GetCheck();

	// Switch to next asset if a non-looping asset finishes
	m_randomizerData->m_randomParams.switchOnNonLoopFinish = m_switchOnNonLoopFinish.GetCheck();

	// clear the previous randomizer data
	int animCount = m_randomizerData->m_randomAnims.size();
	for (int i = 0; i < animCount; ++i)
		SAFE_DELETE(m_randomizerData->m_randomAnims[i]);
	m_randomizerData->m_randomAnims.clear();

	// go through all slots and create assets for them
	int slotCount = m_animSlots.size();
	for (int i = 0; i < slotCount; ++i)
	{
		CRandomAnim* newAnim = new CRandomAnim();
		m_animSlots[i]->GetData(*newAnim);
		m_randomizerData->m_randomAnims.push_back(newAnim);
	}

}

void CAnimationGraph_RandomizerDialog::SetRandomizerData( CRandomizerData* randomizerData )
{
	CRY_ASSERT_MESSAGE(randomizerData, "Randomizer Data is a NULL Pointer, this is not acceptable. I will crash. Fix your code.");
	m_randomizerData = randomizerData;
}


void CAnimationGraph_RandomizerDialog::OnSize(UINT nType, int cx, int cy)
{
	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );
	int offset = si.nPos;

	__super::OnSize(nType, cx, cy);

	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );
	offset -= si.nPos;
	ScrollWindow( 0, offset );
}

void CAnimationGraph_RandomizerDialog::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	__super::OnVScroll(nSBCode, nPos, pScrollBar);
/*
	if ( m_bIgnoreVScroll )
		return;
*/

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_ALL;
	GetScrollInfo( SB_VERT, &si );
	int offset = si.nPos;

	switch (nSBCode)
	{
	case SB_BOTTOM: // Scroll to bottom
		si.nPos = si.nMax;
		break;
	case SB_ENDSCROLL: // End scroll
		break;
	case SB_LINEDOWN: // Scroll one line down
		si.nPos += 8;
		break;
	case SB_LINEUP: // Scroll one line up
		si.nPos -= 8;
		break;
	case SB_PAGEDOWN: // Scroll one page down
		si.nPos += si.nPage;
		break;
	case SB_PAGEUP: // Scroll one page up
		si.nPos -= si.nPage;
		break;
	case SB_THUMBPOSITION: // Scroll to the absolute position. The current position is provided in nPos
	case SB_THUMBTRACK: // Drag scroll box to specified position. The current position is provided in nPos
		si.nPos = nPos;
		break;
	case SB_TOP: // Scroll to top
		si.nPos = 0;
		break;
	}

	si.fMask = SIF_POS;
	si.nPos = max(min(si.nPos, si.nMax-(int)si.nPage),0);
	SetScrollInfo( SB_VERT, &si );

	offset -= si.nPos;
	ScrollWindow( 0, offset );

}

CAnimationGraph_RandomizerDialog::~CAnimationGraph_RandomizerDialog()
{
	m_animSlots.clear();
}

void CAnimationGraph_RandomizerDialog::OnOK()
{
	// Update everything once again before closing (all slots, too)
	OnGeneralParamsChanged();

	// Remove empty slots
	RemoveEmtpySlots();

	// Read back
	ReadBackToRandomizerData();

	__super::OnOK();
}

void CAnimationGraph_RandomizerDialog::OnGeneralParamsChanged()
{
	// TODO: Sanity Check settings
	// positive numbers

	// Gray out items if appropriate
	GrayOutItems();
}

void CAnimationGraph_RandomizerDialog::OnAddSlot()
{
	++m_slotsCreated;

	// Create a new slot
	CRandomAnimationSlot* newSlot = new CRandomAnimationSlot(this, m_slotsCreated);
	newSlot->SetRandomizerDialog(this);

	// show it
	newSlot->ShowWindow(SW_SHOW);

	// add it to the window
	m_animSlots.push_back(newSlot);

	// fill with default data
	CRandomAnim anim;
	newSlot->SetData(&anim);

	// give it a good position
	PositionSlotPanel(newSlot, (m_animSlots.size() - 1));

	AdjustScroll();

	RedrawWindow();
}

void CAnimationGraph_RandomizerDialog::GrayOutItems()
{
	// Switching Assets over time toggles some controls
	bool isOn = (m_switchAssetsOverTime.GetCheck() != 0);
	m_switchProbabilitySeconds.EnableWindow(isOn);
	m_timeAlignBetweenAssets.EnableWindow(isOn);
	m_switchOnNonLoopFinish.EnableWindow(isOn);
	GetDlgItem(IDC_STATIC_SWITCHPROBABILITY)->EnableWindow(isOn); 
	GetDlgItem(IDC_STATIC_SECONDS)->EnableWindow(isOn); 

	// Initial Delay
	isOn = (m_useInitialDelay.GetCheck() != 0);
	m_initialDelaySeconds.EnableWindow(isOn);
	GetDlgItem(IDC_STATIC_SECONDS2)->EnableWindow(isOn); 
}

void CAnimationGraph_RandomizerDialog::RemoveSlot( int slotID )
{
	// don't allow removal of the last single slot
	if (m_animSlots.size() == 1)
	{
		AfxMessageBox( "There always has to be at least one Asset Slot.", MB_OK|MB_ICONWARNING );
		return;
	}

	// run through all slots and find the right one
	std::vector<CRandomAnimationSlot*>::iterator it = m_animSlots.begin();
	std::vector<CRandomAnimationSlot*>::iterator endIt = m_animSlots.end();
	for ( ; it != endIt; ++it)
	{
		if (slotID == (*it)->GetSlotID())
		{
			delete (*it);
			m_animSlots.erase(it);
			break;
		}
	}

	// Recalculate the positions for each slot
	it = m_animSlots.begin();
	endIt = m_animSlots.end();
	for (int i = 0; it != endIt; ++it, ++i)
	{
		PositionSlotPanel(*it, i);
	}

	// Recalculate the probabilities
	CalculateAnimationProbabilities();

	//AdjustScroll();
	RedrawWindow();
}

void CAnimationGraph_RandomizerDialog::PositionSlotPanel( CRandomAnimationSlot* pSlot, int index )
{
	CRY_ASSERT_MESSAGE((index >= 0) && (index < m_animSlots.size()), "RandomizerDialog: Wrong index specified, slot panel positioning will be wrong!");
	CRect wRect;
	pSlot->GetClientRect(&wRect);
	int posY = SLOT_OFFSET_FROM_TOP + (index * (wRect.Height() + 5));
	int scrollPos = GetScrollingPos();
	posY -= scrollPos;
	pSlot->MoveWindow(23, posY, wRect.Width(), wRect.Height());
}

void CAnimationGraph_RandomizerDialog::AdjustScroll()
{
	CRect wRect;
	m_animSlots[0]->GetClientRect(&wRect);
	int maxY = 120 + (m_animSlots.size() * (wRect.Height() + 25));

	CRect rc;
	GetClientRect( rc );
	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_RANGE;
	si.nMin = 0;
	si.nMax = maxY + si.nPos; // since we started at -si.nPos
	si.nPage = rc.bottom;
	//m_bIgnoreVScroll = true;
	SetScrollInfo( SB_VERT, &si );
	// ScrollWindow( 0, maxY );
	//m_bIgnoreVScroll = false;
}

int CAnimationGraph_RandomizerDialog::GetScrollingPos()
{
	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );
	return si.nPos;
}

void CAnimationGraph_RandomizerDialog::CalculateAnimationProbabilities()
{
	// Go through all animation slots
	// calculate a sum of all probabilities
	// go through again and calculate 100% percentages

	int slotcount = m_animSlots.size();
	int probabilitySum = 0;
	for (int i = 0; i < slotcount; ++i)
	{
		probabilitySum += m_animSlots[i]->GetProbability();
	}
	if (probabilitySum <= 0)
		probabilitySum = 1;

	float probability = 0.0f;
	for (int i = 0; i < slotcount; ++i)
	{
		probability = (float)m_animSlots[i]->GetProbability() / ((float)probabilitySum);
		m_animSlots[i]->SetPercentageProbability(probability);
	}
}

void CAnimationGraph_RandomizerDialog::RemoveEmtpySlots()
{
	for (int i = 0; i < m_animSlots.size(); ++i)
	{
		// there always has to be at least one slot
		if (m_animSlots.size() == 1)
			return;

		if (!m_animSlots[i]->IsValidSlot())
		{
			RemoveSlot(m_animSlots[i]->GetSlotID());
			i--;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Slot Implementation

IMPLEMENT_DYNAMIC( CRandomAnimationSlot, CXTResizeDialog )

BEGIN_MESSAGE_MAP( CRandomAnimationSlot, CXTResizeDialog )
	ON_EN_CHANGE( IDC_EDIT_ASSETNAME, OnAssetDataUpdated )
	ON_EN_CHANGE( IDC_EDIT_TRANSITIONTIME, OnAssetDataUpdated )
	ON_EN_CHANGE( IDC_EDIT_PROBABILITY, OnAssetDataUpdated )
	ON_BN_CLICKED(IDC_CHECK_ALLOWRESELECT, OnAssetDataUpdated)
	ON_BN_CLICKED(IDC_CHECK_LOOP, OnAssetDataUpdated)
	ON_BN_CLICKED(IDC_CHECK_NO_INTERRUPT, OnAssetDataUpdated)
	ON_BN_CLICKED(IDC_BUTTON_REMOVESLOT, OnRemoveSlot)

	//ON_EN_KILLFOCUS(IDC_XXX, OnXXX)
END_MESSAGE_MAP()

void CRandomAnimationSlot::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_EDIT_ASSETNAME, m_animationName );
	DDX_Control( pDX, IDC_EDIT_TRANSITIONTIME, m_transitionTime );
	DDX_Control( pDX, IDC_EDIT_PROBABILITY, m_probability );
	DDX_Control( pDX, IDC_CHECK_LOOP, m_loop );
	DDX_Control( pDX, IDC_CHECK_ALLOWRESELECT, m_allowReselection );
	DDX_Control( pDX, IDC_CHECK_NO_INTERRUPT, m_preventCutoff );
}



CRandomAnimationSlot::CRandomAnimationSlot( CWnd* pParent, int slotId )
: CXTResizeDialog( IDD_AG2_RANDOMIZER_SLOT, pParent )
, m_pParent(NULL)
, m_slotID(slotId)
{
	Create( IDD_AG2_RANDOMIZER_SLOT, pParent );
}

CRandomAnimationSlot::~CRandomAnimationSlot()
{

}

	
BOOL CRandomAnimationSlot::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Reset all values

	GetParent()->RedrawWindow();

	return TRUE;
}

void CRandomAnimationSlot::OnAssetDataUpdated()
{
	// TODO: sanity check
	// positive probabilities
	// positive transition times

	// Looping disables the allowInterrupt option
	bool isOn = (m_loop.GetCheck() != 0);
	m_preventCutoff.EnableWindow(!isOn);

	// Recalculate all asset probabilities
	m_pParent->CalculateAnimationProbabilities();
}

void CRandomAnimationSlot::OnRemoveSlot()
{
	CRY_ASSERT(m_pParent);

	m_pParent->RemoveSlot(m_slotID);
}

void CRandomAnimationSlot::SetData( CRandomAnim* data )
{
	CString tempString = "";

	// Fill in all the data from the randomizer data

	m_animationName.SetWindowText(data->animName.c_str());

	m_allowReselection.SetCheck(data->canBeStartedTwiceInARow);

	m_loop.SetCheck(data->looping);

	m_preventCutoff.SetCheck(!data->allowInterrupt);

	tempString.Format("%.i", data->probability);
	m_probability.SetWindowText(tempString);

	tempString.Format("%.2f", data->transitionTime);
	m_transitionTime.SetWindowText(tempString);
}

void CRandomAnimationSlot::GetData( CRandomAnim& data )
{
	CString tempString = "";

	// Fill in all the data from the dialog window

	m_animationName.GetWindowText(tempString);
	data.animName = tempString;

	data.canBeStartedTwiceInARow = m_allowReselection.GetCheck();

	data.looping = m_loop.GetCheck();

	data.allowInterrupt = !m_preventCutoff.GetCheck();

	m_probability.GetWindowText(tempString);
	data.probability = atoi(tempString);

	m_transitionTime.GetWindowText(tempString);
	data.transitionTime = atof(tempString);
}

int CRandomAnimationSlot::GetProbability()
{
	CString tempString = "";
	m_probability.GetWindowText(tempString);
	int i = atoi(tempString);
	return i;
}

void CRandomAnimationSlot::SetPercentageProbability( float probability )
{
	CString tempString;
	tempString.Format("Selection Probability: %.2f%%", probability * 100.0f);
	GetDlgItem(IDC_STATIC_SELECTION_PROBABILITY)->SetWindowText(tempString);
}

bool CRandomAnimationSlot::IsValidSlot()
{
	CString tempString = "";
	m_animationName.GetWindowText(tempString);

	return !tempString.IsEmpty();
}

//////////////////////////////////////////////////////////////////////////
// Random Animation Set Manager
//////////////////////////////////////////////////////////////////////////


CRandomAnimationSetManager::CRandomAnimationSetManager()
{
	Reset();
}

CRandomAnimationSetManager::~CRandomAnimationSetManager()
{
	Reset();
}

void CRandomAnimationSetManager::Reset()
{
	m_nextNumber = 0;

	TRandomizerByName::iterator it = m_randomAnimSets.begin();
	TRandomizerByName::iterator endIt = m_randomAnimSets.end();
	for ( ; it != endIt; ++it)
		SAFE_DELETE((*it));

	m_randomAnimSets.clear();
}

CRandomizerData* CRandomAnimationSetManager::GetRandomizerData( const char* name )
{
	CRandomizerData* randData = DoesRandomizerDataExist(name);
	if (randData)
		return randData;

	// Create a new one with an unused generic name and use it
	CString newName;
	do
	{
		newName.Format("<RandomSet:Random_%i>", m_nextNumber);
		++m_nextNumber;
	} 
	while (DoesRandomizerDataExist(newName));
	
	CRandomizerData* newSet;
	string firstAnimName = name;
	// in case the "name" is an animation name, take it as the first animation into the set
	if (firstAnimName.find("<") == string::npos && !firstAnimName.empty())
		newSet = new CRandomizerData(name);
	else
		newSet = new CRandomizerData();

	newSet->m_name = newName;
	newSet->m_id = m_nextNumber;
	
	m_randomAnimSets.push_back(newSet);
	return newSet;
}

void CRandomAnimationSetManager::RemoveRandomizerData( const char* name )
{
	TRandomizerByName::iterator it = m_randomAnimSets.begin();
	TRandomizerByName::iterator endIt = m_randomAnimSets.end();
	for ( ; it != endIt; ++it)
	{
		if ((*it)->m_name.CompareNoCase(name) == 0)
		{
			SAFE_DELETE(*it);
			m_randomAnimSets.erase(it);
			--m_nextNumber;
			return;
		}
	}
}

void CRandomAnimationSetManager::SaveToXML( XmlNodeRef node )
{
	XmlNodeRef block = node->createNode("RandomizationSets");

	TRandomizerByName::iterator it = m_randomAnimSets.begin();
	TRandomizerByName::iterator endIt = m_randomAnimSets.end();
	for ( ; it != endIt; ++it)
	{
		//CRandomizerData* randData = *it;
		XmlNodeRef node = (*it)->ToXml();
		block->addChild(node);
	}

	node->addChild(block);

}

void CRandomAnimationSetManager::LoadFromXML( XmlNodeRef node )
{
	// Delete all data first (regardless of whether there will be new data
	Reset();

	XmlNodeRef block = node->findChild("RandomizationSets");
	if (!block)
		return;

	// Go through all children and load
	int childCount = block->getChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef child = block->getChild(i);
		CRandomizerData* newSet = GetRandomizerData("<none>");
		if (!newSet->LoadFromXml(child))
			RemoveRandomizerData("<none>");
	}
	
}

CRandomizerData* CRandomAnimationSetManager::DoesRandomizerDataExist( const char* name )
{
	TRandomizerByName::iterator it = m_randomAnimSets.begin();
	TRandomizerByName::iterator endIt = m_randomAnimSets.end();
	for ( ; it != endIt; ++it)
		if ((*it)->m_name.CompareNoCase(name) == 0)
			return *it;

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
// Random Animation Set Data
//////////////////////////////////////////////////////////////////////////


CRandomizerData::CRandomizerData()
{
	m_name.Empty(); 
	m_id = -1;
}

CRandomizerData::CRandomizerData( const char* firstAnimName )
{
	m_name.Empty(); 
	m_id = -1;

	CRandomAnim* newAnim = new CRandomAnim();
	newAnim->animName = firstAnimName;
	m_randomAnims.push_back(newAnim);
}

CRandomizerData::CRandomizerData( CRandomizerData& copyClass )
{
	*this = copyClass;
}

CRandomizerData::~CRandomizerData()
{
	m_randomAnims.clear();
}

CRandomizerData& CRandomizerData::operator=( const CRandomizerData& copyClass )
{
	m_randomParams = copyClass.m_randomParams;
	int animCount = copyClass.m_randomAnims.size();
	m_randomAnims.reserve(animCount);
	for (int i = 0; i < animCount; ++i)
		m_randomAnims.push_back(copyClass.m_randomAnims[i]);

	return *this;
}

bool CRandomizerData::operator==( const CRandomizerData& b ) const
{
	if (m_randomParams != b.m_randomParams)
		return false;

	int animCount = b.m_randomAnims.size();
	if (animCount != m_randomAnims.size())
		return false;

	for (int i = 0; i < animCount; ++i)
	{
		if (m_randomAnims[i] != b.m_randomAnims[i])
			return false;
	}

	return true;
}

XmlNodeRef CRandomizerData::ToXml()
{
	// For safety use a generic node name
	// instead of the class name (it could contain whitespaces after all!)
	XmlNodeRef newNode = XmlHelpers::CreateXmlNode( "RandomizerSet" );

	// write name and id of this set
	newNode->setAttr("id", m_id);
	newNode->setAttr("name", m_name);
	
	// write the general parameters
	m_randomParams.ToXml(newNode);

	// write all individual animations and their parameters of this set
	std::vector<CRandomAnim*>::iterator it = m_randomAnims.begin();
	std::vector<CRandomAnim*>::iterator endIt = m_randomAnims.end();
	for ( ; it != endIt; ++it)
	{
		(*it)->ToXml(newNode);
	}

	return newNode;
	
}

bool CRandomizerData::LoadFromXml( XmlNodeRef node )
{
	if (strcmpi(node->getTag(), "RandomizerSet") == 0)
	{
		m_name = node->getAttr("name");

		// go through all children and try to load them
		int childCount = node->getChildCount();
		for (int i = 0; i < childCount; ++i)
		{
			XmlNodeRef child = node->getChild(i);

			// Could be a parameter's node
			if (m_randomParams.LoadFromXml(child))
				continue;

			// Try to load a random Animation Asset
			CRandomAnim* newAnim = new CRandomAnim();
			if (newAnim->LoadFromXml(child))
				m_randomAnims.push_back(newAnim);
			else
				SAFE_DELETE(newAnim);
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool CRandomizerData::IsDifferent( CString& animName )
{
	if (m_randomAnims.size() > 1)
		return true;

	if (m_randomAnims.size() == 0)
		return false;

	if (m_randomParams.initialDelay > 0.0f)
		return true;

	if (m_randomAnims[0]->animName != animName && !m_randomAnims[0]->animName.empty())
		return true;

	return false;
}

bool CRandomizerData::IsValid()
{
	// the data is valid if there is at least one animation
	// so it can return true as soon as one non-empty animation name is found
	std::vector<CRandomAnim*>::iterator it = m_randomAnims.begin();
	std::vector<CRandomAnim*>::iterator endIt = m_randomAnims.end();
	for ( ; it != endIt; ++it)
	{
		if (!(*it)->animName.empty())
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CAnimNameEdit::OpenRandomizer(CAnimationGraph2* pGraph)
{
	if(pGraph == NULL)
		return;

	// Open the Randomizer Interface as a  *modal* window
	CString currentName = "";
	GetWindowText(currentName);

	// Feed actual Randomizer Data from Animation State Parameterization instead of NULL Pointer
	CAnimationGraph_RandomizerDialog rdDialog;

	CRandomizerData* randData = pGraph->GetRandomAnimSetManager()->GetRandomizerData(currentName);
	rdDialog.SetRandomizerData(randData);

	int ret = rdDialog.DoModal();

	// if the set is invalid, don't use it, don't set it as a name, have the random anim set manager discard it instead
	if (randData->IsDifferent(currentName) && randData->IsValid())
	{
		SetWindowText(randData->m_name);
	}
	else
	{
		pGraph->GetRandomAnimSetManager()->RemoveRandomizerData(randData->m_name);
	}
}