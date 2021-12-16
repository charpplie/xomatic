////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   LMGEditor.cpp
//  Version:     v1.00
//  Created:     07/07/2009 by Pau Novau
//  Description: Locomotion Group Editor main controller.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "LMGEditor/LMGEditor.h"

#include "LMGEditor/LMGInfo_BlendType.h"

#include "ICryAnimation.h"

#define DEFAULT_CHARACTER "objects/characters/neutral_male/sdk_character_male.cdf"
#define LMG_EDITOR_ANIMATION_NAME "_LMGEDITOR_TMP"

IMPLEMENT_DYNCREATE( CLMGEditor, CBaseFrameWnd )

BEGIN_MESSAGE_MAP( CLMGEditor, CBaseFrameWnd )
	ON_COMMAND( ID_UNDO, OnUndo )
	ON_COMMAND( ID_REDO, OnRedo )
	ON_COMMAND( ID_FILE_LOADLMG, OnFileLoadLmg )
	ON_COMMAND( ID_FILE_SAVELMG, OnFileSaveLmg )
	ON_COMMAND( ID_FILE_LOADCHARACTER, OnFileLoadCharacter )
	ON_COMMAND( ID_OPTIONS_OVERLAYPARAMETERVALUES, OnOverlayParametersValues )
	ON_UPDATE_COMMAND_UI( ID_OPTIONS_OVERLAYPARAMETERVALUES, OnUpdateOverlayParametersValues )
	ON_COMMAND( ID_OPTIONS_OVERLAYANIMATIONWEIGHTS, OnOverlayAnimationWeights )
	ON_UPDATE_COMMAND_UI( ID_OPTIONS_OVERLAYANIMATIONWEIGHTS, OnUpdateOverlayAnimationWeights )
END_MESSAGE_MAP()


const int ID_LMG_EDITOR_PANEL = IDD_DATABASE;

enum
{
	ID_LMG_EDITOR_BASE = 62000,
	ID_LOCOMOTION_PREVIEW_VIEW,
	ID_ANIMATION_LIST_VIEW,
	ID_SLOTS_VIEW,
	ID_BLENDCODE_SELECTION_VIEW,
	ID_ANIMATION_PREVIEW_VIEW,
	ID_CAPSCODE_SELECTION_VIEW,
	ID_MOTION_COMBINATIONS_VIEW
};

CLMGEditor::CLMGEditor()
: m_pSelectedBlendType( NULL )
, m_pSelectedCapsCode( NULL )
, m_overlayParameterValues( true )
, m_overlayAnimationWeights( true )
{
	CRect rc( 0, 0, 0, 0 );
	Create( WS_CHILD | WS_VISIBLE, rc, AfxGetMainWnd() );
}

CLMGEditor::~CLMGEditor()
{
}

BOOL CLMGEditor::OnInitDialog()
{
	LoadAccelTable( MAKEINTRESOURCE( IDR_LMG_EDITOR_ACCELERATORS ) );

	GetDockingPaneManager()->HideClient( TRUE );
	GetDockingPaneManager()->SetTheme( CMainFrame::GetDockingPaneTheme() );

	m_previewLmgView.Create( ID_LMG_EDITOR_PANEL, this );
	m_previewAnimationView.Create( ID_LMG_EDITOR_PANEL, this );
	m_animationListView.Create( IDD_LMG_ANIMATION_LIST_VIEW, this );
	m_slotsView.Create( IDD_ANIMATION_SLOT_CONTAINER, this );
	m_testView.Create( IDD_LMG_TEST, this );
	m_blendCodeSelectionView.Create( IDD_LMG_BLENDCODE_SELECTION, this );
	m_capsCodeSelectionView.Create( IDD_LMG_BLENDCODE_SELECTION, this );
	m_motionCombinationSlots.Create( IDD_LMG_MOTION_COMBINATION_VIEW, this );

	{
		CXTPDockingPane* pDockingPanePreview = AttachDockingWnd( ID_LOCOMOTION_PREVIEW_VIEW, &m_previewLmgView, "Locomotion Preview" );
		pDockingPanePreview->SetMinTrackSize( CSize( 256, 256 ) );
		
		CXTPDockingPane* pDockingPaneAnimationList = AttachDockingWnd( ID_ANIMATION_LIST_VIEW, &m_animationListView, "Animations", xtpPaneDockLeft, pDockingPanePreview );
		pDockingPaneAnimationList->SetMinTrackSize( CSize( 128, 25 ) );
		pDockingPaneAnimationList->SetMaxTrackSize( CSize( 300, 10000 ) );
		
		CXTPDockingPane* pDockingPaneTest = AttachDockingWnd( IDD_LMG_TEST, &m_testView, "Test", xtpPaneDockRight, pDockingPanePreview );
		pDockingPaneTest->SetMinTrackSize( CSize( 256, 256 ) );

		CXTPDockingPane* pDockingPaneAnimationPreview = AttachDockingWnd( ID_ANIMATION_PREVIEW_VIEW, &m_previewAnimationView, "Animation Preview", xtpPaneDockBottom, pDockingPaneAnimationList );
		GetDockingPaneManager()->AttachPane( pDockingPaneAnimationPreview, pDockingPanePreview );
		GetDockingPaneManager()->ShowPane( pDockingPanePreview );
		pDockingPaneAnimationPreview->SetMinTrackSize( CSize( 128, 128 ) );

		CXTPDockingPane* pDockingPaneSlots = AttachDockingWnd( IDD_ANIMATION_SLOT_CONTAINER, &m_slotsView, "Slots", xtpPaneDockBottom, pDockingPanePreview );
		pDockingPaneSlots->SetMinTrackSize( CSize( 256, 30 ) );

		CXTPDockingPane* pDockingPaneSelectBlendCode = AttachDockingWnd( ID_BLENDCODE_SELECTION_VIEW, &m_blendCodeSelectionView, "Blend Code", xtpPaneDockTop, pDockingPaneAnimationList );
		pDockingPaneSelectBlendCode->SetMinTrackSize( CSize( 128, 40 ) );
		pDockingPaneSelectBlendCode->SetMaxTrackSize( CSize( 300, 40 ) );

		CXTPDockingPane* pDockingPaneSelectCapsCode = AttachDockingWnd( ID_CAPSCODE_SELECTION_VIEW, &m_capsCodeSelectionView, "Caps Code", xtpPaneDockBottom, pDockingPaneSelectBlendCode );
		pDockingPaneSelectCapsCode->SetMinTrackSize( CSize( 128, 40 ) );
		pDockingPaneSelectCapsCode->SetMaxTrackSize( CSize( 300, 40 ) );

		CXTPDockingPane* pDockingPaneMotionCombinations = AttachDockingWnd( ID_MOTION_COMBINATIONS_VIEW, &m_motionCombinationSlots, "Motion Combinations", xtpPaneDockBottom, pDockingPaneTest );
		pDockingPaneMotionCombinations->SetMinTrackSize( CSize( 128, 40 ) );
		pDockingPaneMotionCombinations->SetMaxTrackSize( CSize( 300, 300 ) );
	}

	m_slotsView.SetListener( this );

	m_motionCombinationSlots.SetListener( this );
	m_previewLmgView.SetListener( this );

	m_blendCodeSelectionView.SetBlendCodeChangedListener( this );
	m_capsCodeSelectionView.SetCapsCodeChangedListener( this );

	m_animationListView.AddDragDropListener( &m_slotsView );
	m_animationListView.AddDragDropListener( &m_motionCombinationSlots );
	m_animationListView.SetSelectionChangeListener( this );

	for ( size_t i = 0; i < m_lmgInfo.GetBlendTypeCount(); i++ )
	{
		CLMGInfo_BlendType* pBlendTypeInfo = m_lmgInfo.GetBlendType( i );
		const CString& blendTypeName = pBlendTypeInfo->GetName();
		const CString& blendTypeCode = pBlendTypeInfo->GetCode();
		m_blendCodeSelectionView.AddNameCodePair( blendTypeName, blendTypeCode );
	}

	CXTPCommandBar* pMenuBar = GetCommandBars()->SetMenu( _T("Menu Bar"), IDR_MENU_LMGEDITOR );
	pMenuBar->SetFlags( xtpFlagStretched | xtpFlagNoMovable );
	pMenuBar->EnableCustomization( FALSE );

	LoadCharacter( DEFAULT_CHARACTER );

	return TRUE;
}


void CLMGEditor::OnUndo()
{
	GetIEditor()->Undo();
}


void CLMGEditor::OnRedo()
{
	GetIEditor()->Redo();
}


void CLMGEditor::OnFileLoadLmg()
{
	const char* filters = "Lmg Files (*.lmg)|*.lmg|";

	CString file;
	bool fileSelected = CFileUtil::SelectFile( filters, "", file );
	if ( ! fileSelected )
	{
		return;
	}

	XmlNodeRef root = GetISystem()->LoadXmlFile( file );
	if ( ! root )
	{
		return;
	}

	CString blendTypeCode;
	std::vector< CString > animationNames;
	std::vector< CString > motionCombinationNames;

	CString capsCode;
	for ( int i = 0; i < root->getChildCount(); i++ )
	{
		XmlNodeRef childNode = root->getChild( i );
		const char* childNodeName = childNode->getTag();
		if ( strcmpi( childNodeName, "BLENDTYPE" ) == 0 )
		{
			childNode->getAttr( "type", blendTypeCode );
		}
		else if ( strcmpi( childNodeName, "CAPS") == 0 )
		{
			childNode->getAttr( "code", capsCode );
		}
		else if ( strcmpi( childNodeName, "ExampleList" ) == 0 )
		{
			for ( int j = 0; j < childNode->getChildCount(); j++ )
			{
				XmlNodeRef exampleNode = childNode->getChild( j );
				CString animationName;
				exampleNode->getAttr( "AName", animationName );

				animationNames.push_back( animationName );
			}
		}
		else if ( strcmpi( childNodeName, "MotionCombination" ) == 0 )
		{
			for ( int j = 0; j < childNode->getChildCount(); j++ )
			{
				XmlNodeRef newStyleNode = childNode->getChild( j );
				CString motionCombinationName;
				newStyleNode->getAttr( "Style", motionCombinationName );

				motionCombinationNames.push_back( motionCombinationName );
			}
		}
	}

	SetBlendCode( blendTypeCode );
	SetCapsCode( capsCode );

	for ( size_t i = 0; i < animationNames.size() && i < m_slotsView.GetSlotCount(); i++ )
	{
		const CString& animationName = animationNames[ i ];
		m_slotsView.SetAnimationName( i, animationName );
	}

	m_motionCombinationSlots.RemoveAllSlots();
	for ( size_t i = 0; i < motionCombinationNames.size(); i++ )
	{
		const CString& motionCombinationName = motionCombinationNames[ i ];
		m_motionCombinationSlots.CreateMotionCombinationSlot( motionCombinationName );
	}

	TestStuff();
}

void CLMGEditor::OnFileSaveLmg()
{
	const char* filters = "Lmg Files (*.lmg)|*.lmg|";

	CString filePath;
	bool fileChosen = CFileUtil::SelectSaveFile( filters, "lmg", "", filePath );

	if ( ! fileChosen )
	{
		return;
	}

	CString lmgXml = GenerateLmgXml();

	FILE* file = gEnv->pCryPak->FOpen( filePath, "wt" );
	if ( file == NULL )
	{
		// TODO: display error.
		return;
	}
	
	const char* lmgData = ( const char* )( lmgXml );
	gEnv->pCryPak->FWrite( lmgData, lmgXml.GetLength(), file );
	gEnv->pCryPak->FClose( file );
}

void CLMGEditor::OnFileLoadCharacter()
{
	CString file;
	bool fileSelected = CFileUtil::SelectSingleFile( EFILE_TYPE_GEOMETRY, file );
	if ( ! fileSelected )
	{
		return;
	}

	LoadCharacter( file );
}


CXTPDockingPane* CLMGEditor::AttachDockingWnd( UINT wndId, CWnd* pWnd, const CString& dockingPaneTitle, XTPDockingPaneDirection direction, CXTPDockingPaneBase* pNeighbour )
{
	assert( pWnd != NULL );
	if ( pWnd == NULL )
	{
		return NULL;
	}

	CRect rc( 0, 0, 400, 400 );

	CXTPDockingPane* pDockPane = GetDockingPaneManager()->CreatePane( wndId, rc, direction, pNeighbour );
	pDockPane->SetOptions( xtpPaneNoCloseable );
	pDockPane->SetTitle( dockingPaneTitle );
	pDockPane->Attach( pWnd );

	pWnd->SetOwner( this );

	return pDockPane;
}

void CLMGEditor::LoadCharacter( const CString& filename )
{
	CWaitCursor waitForLoadingToFinish;

	m_pCharacterInstance = NULL;
	m_pAnimationSet = NULL;

	m_animationListView.Clear();

	m_pCharacterInstance = m_previewLmgView.LoadCharacter( filename );
	if ( m_pCharacterInstance == NULL )
	{
		return;
	}

	m_pAnimationSet = m_pCharacterInstance->GetIAnimationSet();
	if ( m_pAnimationSet == NULL )
	{
		return;
	}

	m_animationListView.UpdateAnimations( m_pAnimationSet );

	ICharacterInstance* pAnimationPreviewCharacter = m_previewAnimationView.LoadCharacter( filename );
	assert( pAnimationPreviewCharacter != NULL );

	TestStuff();
}

bool CLMGEditor::ValidateSlots()
{
	if ( m_pAnimationSet == NULL )
	{
		return false;
	}

	if ( m_pSelectedBlendType == NULL )
	{
		return false;
	}

	const CString& selectedBlendCode = m_pSelectedBlendType->GetCode();

	bool validateSuccess = true;

	for ( size_t i = 0; i < m_slotsView.GetSlotCount(); i++ )
	{
		CString animationName = m_slotsView.GetAnimationName( i );

		LmgAnimationStatus lmgAnimationStatus = m_pAnimationSet->IsAnimationValidForLMG( selectedBlendCode, animationName );
		if ( lmgAnimationStatus != LMGAS_OK )
		{
			validateSuccess = false;

			const char* lmgStatusToErrorString[] =
			{
				"", //LMGAS_OK
				"Animation not in CAL file.", //LMGAS_ANIMATION_NOT_IN_CAL_FILE
				"Asset doesn't exist.", //LMGAS_ASSET_DOESNT_EXIST
				"Animation has no speed.", //LMGAS_ANIMATION_HAS_NO_SPEED
				"Locomotion Blend Code invalid" //LMGAS_INVALID_BLEND_CODE
			};

			const char* errorString = lmgStatusToErrorString[ lmgAnimationStatus ];
			
			m_slotsView.SetSlotStatus( i, CSlotControl::SS_ERROR, errorString );
		}
		else
		{
			m_slotsView.SetSlotStatus( i, CSlotControl::SS_OK );
		}
	}

	return validateSuccess;
}

void CLMGEditor::TestStuff()
{
	if ( m_pAnimationSet == NULL )
	{
		return;
	}

	if ( m_pSelectedBlendType == NULL )
	{
		return;
	}

	m_previewLmgView.StopAnimation();

	bool slotsValidateSuccess = ValidateSlots();
	if ( ! slotsValidateSuccess )
	{
		return;
	}

	CString lmgXml = GenerateLmgXml();	

	m_pAnimationSet->LoadLMGFromMemory( "_test_resource_", LMG_EDITOR_ANIMATION_NAME, lmgXml );

	int animationId = m_pAnimationSet->GetAnimIDByName( LMG_EDITOR_ANIMATION_NAME );
	uint32 animationFlags = m_pAnimationSet->GetAnimationFlags( animationId );
	bool isValidLmg = ( animationFlags & CA_ASSET_LMG_VALID );

	if ( ! isValidLmg )
	{
		return;
	}

	m_previewLmgView.PlayAnimation( LMG_EDITOR_ANIMATION_NAME );
}

CString CLMGEditor::GenerateLmgXml()
{
	CString lmgXml = "<LocomotionGroup>\n";
	CString blendTypeCode = m_pSelectedBlendType->GetCode();
	lmgXml.AppendFormat( "\t<BLENDTYPE type=\"%s\" />\n", blendTypeCode );

	if ( m_pSelectedCapsCode != NULL )
	{
		CString capsCode = m_pSelectedCapsCode->GetCode();
		lmgXml.AppendFormat( "\t<CAPS code=\"%s\" />\n", capsCode );
	}
	lmgXml.Append( "\t<ExampleList>\n" );

	assert( m_pSelectedBlendType->GetSlotIdCount() == m_slotsView.GetSlotCount() );
	for ( size_t i = 0; i < m_pSelectedBlendType->GetSlotIdCount(); i++ )
	{
		CString example;
		CString animationName = m_slotsView.GetAnimationName( i );
		CString position = "0,1,0";

		const CString& slotId = m_pSelectedBlendType->GetSlotId( i );
		CLMGInfo_Slot* pSlot = m_lmgInfo.GetSlotById( slotId );
		if ( pSlot != NULL )
		{
			position = pSlot->GetPosition();
		}

		lmgXml.AppendFormat( "\t\t<Example Position=\"%s\" AName=\"%s\" />\n", position, animationName );
	}

	lmgXml.Append( "\t</ExampleList>\n");
	if ( m_motionCombinationSlots.GetSlotCount() != 0 )
	{
		lmgXml.Append( "\t<MotionCombination>" );
		for ( size_t i = 0; i < m_motionCombinationSlots.GetSlotCount(); i++ )
		{
			CString motionCombinationName = m_motionCombinationSlots.GetMotionCombinationName( i );
			lmgXml.AppendFormat( "\t\t<NewStyle Style=\"%s\" />", motionCombinationName );
		}
		lmgXml.Append( "\t</MotionCombination>" );
	}
	lmgXml.Append( "</LocomotionGroup>\n" );

	return lmgXml;
}

void CLMGEditor::OnBlendCodeChanged( const CString& blendCode )
{
	SetBlendCode( blendCode );
}

void CLMGEditor::SetBlendCode( const CString& code )
{
	m_blendCodeSelectionView.SetCurrentNameByCode( code );

	HoldCurrentSlotValues();
	FillSlotsView( code );
	FetchHeldSlotValues();

	FillCapsCodeView();

	FillTestView();

	TestStuff();
}

void CLMGEditor::OnCapsCodeChanged( const CString& capsCode )
{
	SetCapsCode( capsCode );
}

void CLMGEditor::SetCapsCode( const CString& capsCode )
{
	m_pSelectedCapsCode = m_lmgInfo.GetCapsCodeByCode( capsCode );
	m_capsCodeSelectionView.SetCurrentNameByCode( capsCode );
}

void CLMGEditor::OnMotionCombinationSlotNameChanged()
{
	TestStuff();
}

void CLMGEditor::OnMotionCombinationSlotRemoved()
{
	TestStuff();
}

void CLMGEditor::OnSlotAnimationNameChanged()
{
	TestStuff();
}

void CLMGEditor::FillSlotsView( const CString& code )
{
	m_slotsView.RemoveAllSlots();

	m_pSelectedBlendType = m_lmgInfo.GetBlendTypeByCode( code );
	if ( m_pSelectedBlendType == NULL )
	{
		return;	
	}

	m_slotsView.BatchSlotCreationStart();
	for ( size_t i = 0; i < m_pSelectedBlendType->GetSlotIdCount(); i++ )
	{
		const CString& slotId = m_pSelectedBlendType->GetSlotId( i );

		CLMGInfo_Slot* pSlot = m_lmgInfo.GetSlotById( slotId );
		if ( pSlot == NULL )
		{
			m_slotsView.CreateSlot( slotId, "" );
		}
		else
		{
			m_slotsView.CreateSlot( pSlot->GetName(), pSlot->GetDescription() );
		}
	}
	m_slotsView.BatchSlotCreationEnd();
}

void CLMGEditor::FillCapsCodeView()
{
	m_capsCodeSelectionView.Clear();
	m_capsCodeSelectionView.AddNameCodePair( "<none>", "" );
	m_capsCodeSelectionView.SetCurrentNameByCode( "" );

	if ( m_pSelectedBlendType == NULL )
	{
		return;
	}

	for ( size_t i = 0; i < m_pSelectedBlendType->GetCapsCodeCount(); i++ )
	{
		const CString& capsCodeId = m_pSelectedBlendType->GetCapsCodeId( i );
		CLMGInfo_CapsCode* capsCodeInfo = m_lmgInfo.GetCapsCodeById( capsCodeId );
		if ( capsCodeInfo != NULL )
		{
			m_capsCodeSelectionView.AddNameCodePair( capsCodeInfo->GetName(), capsCodeInfo->GetCode() );
		}
	}

	if ( m_pSelectedCapsCode != NULL )
	{
		// Try to keep old caps code if possible
		m_capsCodeSelectionView.SetCurrentNameByCode( m_pSelectedCapsCode->GetCode() );
	}
}

void CLMGEditor::FillTestView()
{
	m_uiParamMappings.ClearMappings();

	m_testView.ClearTestView();

	if ( m_pSelectedBlendType == NULL )
	{
		return;
	}

	bool hasTest = ( 0 < m_pSelectedBlendType->GetTestIdCount() );
	if ( ! hasTest )
	{
		return;
	}
	
	const CString& testId = m_pSelectedBlendType->GetTestId( 0 );
	CLMGInfo_Test* test = m_lmgInfo.GetTestById( testId );
	if ( test == NULL )
	{
		return;
	}

	for ( size_t i = 0; i < test->GetJoystickCount(); i++ )
	{
		CLMGInfo_Test_Joystick* pJoystickInfo = test->GetJoystick( i );
		CTestJoystickControl* pJoystickControl = m_testView.CreateJoystickControl();
		pJoystickControl->MoveWindow( pJoystickInfo->x, pJoystickInfo->y, pJoystickInfo->width, pJoystickInfo->height );
		pJoystickControl->SetValue( Vec2( pJoystickInfo->defaultX, pJoystickInfo->defaultY ) );
		m_testView.CreateNameControl( pJoystickInfo->x, pJoystickInfo->y, pJoystickInfo->width, pJoystickInfo->name );

		for ( size_t j = 0; j < pJoystickInfo->mappings.size(); j++ )
		{
			const CLMGInfo_Test_Control::MappingInfo& mappingInfo = pJoystickInfo->mappings[ j ];
			IUiMotionParamMapping::Mapping mapping = mappingInfo.first;
			EMotionParamID paramId = mappingInfo.second;
			m_uiParamMappings.CreateMapping( paramId, mapping, pJoystickControl, pJoystickInfo->lengthMin, pJoystickInfo->lengthMax );
		}
	}

	for ( size_t i = 0; i < test->GetSliderCount(); i++ )
	{
		CLMGInfo_Test_Slider* pSliderInfo = test->GetSlider( i );
		CTestSliderControl* pSlider = m_testView.CreateSliderControl();

		pSlider->MoveWindow( pSliderInfo->x, pSliderInfo->y, pSliderInfo->width, pSliderInfo->height );
		pSlider->SetRange( Vec2( pSliderInfo->minX, pSliderInfo->minY ), Vec2( pSliderInfo->maxX, pSliderInfo->maxY ) );
		pSlider->SetValue( Vec2( pSliderInfo->defaultX, pSliderInfo->defaultY ) );
		m_testView.CreateNameControl( pSliderInfo->x, pSliderInfo->y, pSliderInfo->width, pSliderInfo->name );

		for ( size_t j = 0; j < pSliderInfo->mappings.size(); j++ )
		{
			const CLMGInfo_Test_Control::MappingInfo& mappingInfo = pSliderInfo->mappings[ j ];
			IUiMotionParamMapping::Mapping mapping = mappingInfo.first;
			EMotionParamID paramId = mappingInfo.second;
			m_uiParamMappings.CreateMapping( paramId, mapping, pSlider );
		}
	}

	m_testView.RecalculateCurrentLayoutSize();
}


void CLMGEditor::ClearHeldSlotValues()
{
	m_heldSlotIdToAnimationNames.clear();
}

void CLMGEditor::HoldCurrentSlotValues()
{
	if ( m_pSelectedBlendType == NULL )
	{
		return;
	}

	if ( m_slotsView.GetSlotCount() != m_pSelectedBlendType->GetSlotIdCount() )
	{
		return;
	}

	for ( size_t i = 0; i < m_pSelectedBlendType->GetSlotIdCount(); i++ )
	{
		const CString& slotId = m_pSelectedBlendType->GetSlotId( i );
		const CString& animationName = m_slotsView.GetAnimationName( i );
		
		m_heldSlotIdToAnimationNames[ slotId ] = animationName;
	}
}

void CLMGEditor::FetchHeldSlotValues()
{
	if ( m_pSelectedBlendType == NULL )
	{
		return;
	}

	if ( m_slotsView.GetSlotCount() != m_pSelectedBlendType->GetSlotIdCount() )
	{
		return;
	}

	for ( size_t i = 0; i < m_pSelectedBlendType->GetSlotIdCount(); i++ )
	{
		const CString& slotId = m_pSelectedBlendType->GetSlotId( i );
		bool animationNameHeld = ( m_heldSlotIdToAnimationNames.find( slotId ) != m_heldSlotIdToAnimationNames.end() );
		if ( animationNameHeld )
		{
			const CString& animationName = m_heldSlotIdToAnimationNames[ slotId ];
			m_slotsView.SetAnimationName( i, animationName );
		}
	}
}

void CLMGEditor::AnimationNameSelectionChanged( const CString& currentSelectedAnimationName )
{
	m_previewAnimationView.StopAnimation();
	if ( ! currentSelectedAnimationName.IsEmpty() )
	{
		m_previewAnimationView.PlayAnimation( currentSelectedAnimationName );
	}
}

void CLMGEditor::PreDrawCharacter( ICharacterInstance* pCharacter, f32 frameTime )
{
	IAnimationSet* pAnimationSet = pCharacter->GetIAnimationSet();

	ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim();
	if ( pSkeletonAnim == NULL )
	{
		return;
	}

	int numAnimations = pSkeletonAnim->GetNumAnimsInFIFO( 0 );
	if ( numAnimations <= 0 )
	{
		return;
	}

	CAnimation& animation = pSkeletonAnim->GetAnimFromFIFO( 0, 0 );

	SParametric& lmg = animation.m_Parametric;

	for ( int i = 0; i < eMotionParamID_COUNT; i++ )
	{
		IUiMotionParamMapping* pUiMotionParamMapping = m_uiParamMappings.GetMapping( ( EMotionParamID )( i ) );
		if ( pUiMotionParamMapping != NULL )
		{
			float minParamValue = lmg.m_params[ i ].desc.m_fMin;
			float maxParamValue = lmg.m_params[ i ].desc.m_fMax;
			float minAssetValue = lmg.m_params[ i ].desc.m_fMinAsset;
			float maxAssetValue = lmg.m_params[ i ].desc.m_fMaxAsset;

			float value = pUiMotionParamMapping->GetValue( minParamValue, maxParamValue, minAssetValue, maxAssetValue );
	
			if ( lmg.m_params[ i ].desc.m_fMaxChangeRate == 0.f )
			{
				lmg.m_params[ i ].desc.m_fMaxChangeRate = 1;
				lmg.m_params[ i ].desc.m_bLocked = false;
			}
		
			pSkeletonAnim->SetDesiredMotionParam( ( EMotionParamID )( i ), value, frameTime );
		}
	}


	//////////////////////////////////////////////////////////////////////////

	IRenderer* pRenderer = GetISystem()->GetIRenderer();
	if ( pRenderer == NULL )
	{
		return;
	}

	char* motionParamNames[] =
	{
		"TravelAngle",
		"TravelDistScale",
		"TravelSpeed",
		"TravelDist",
		"TravelSlope",
		"WeightShift",
		"TurnSpeed",
		"TurnAngle",
		"Duration",
		"Curving",
		"Height",
		"Scale"
	};




	float paramTextColor[] = { 0.9f, 0.9f, 0.9f, 1 };
	f32 textY = 10;

	if ( m_overlayParameterValues )
	{
		for ( int i = 0; i < eMotionParamID_COUNT; i++ )
		{
			MotionParam& motionParam = lmg.m_params[ i ];
			f32 paramValue = motionParam.value;

			pRenderer->Draw2dLabel( 10, textY, 1.2f, paramTextColor, false, "%s: %f", motionParamNames[ i ], paramValue );
			textY += 12;
		}
	}

	if ( m_overlayAnimationWeights )
	{
		textY += 10;
		for ( int i = 0; i < lmg.m_numAnims && i < m_slotsView.GetSlotCount(); i++ )
		{
			f32 weight = lmg.m_fBlendWeight[ i ];

			float textWeightColor[] = { 1, 1, weight, 1 };

			pRenderer->Draw2dLabel( 10, textY, 1.2f, textWeightColor, false, "%s: %f", m_slotsView.GetAnimationName( i ), weight );
			textY += 12;
		}
	}

}


void CLMGEditor::OnOverlayParametersValues()
{
	m_overlayParameterValues = ! m_overlayParameterValues;
}


void CLMGEditor::OnOverlayAnimationWeights()
{
	m_overlayAnimationWeights = ! m_overlayAnimationWeights;
}

void CLMGEditor::OnUpdateOverlayParametersValues( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_overlayParameterValues );
}

void CLMGEditor::OnUpdateOverlayAnimationWeights( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_overlayAnimationWeights );
}