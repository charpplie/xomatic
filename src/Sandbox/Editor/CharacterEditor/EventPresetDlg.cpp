#include "stdafx.h"
#include "EventPresetDlg.h"

IMPLEMENT_DYNAMIC(CEventPresetDlg, CDialog)

BEGIN_MESSAGE_MAP(CEventPresetDlg, CDialog)
END_MESSAGE_MAP()

CEventPresetDlg::CEventPresetDlg(CAnimEventData* pAnimEventData, SButtonItem* pButtonItem, const std::vector<string>& boneNameList, CWnd* pParent)
: CDialog(CEventPresetDlg::IDD, pParent)
{
	assert( pAnimEventData && pButtonItem );
	m_pEventData = pAnimEventData;
	m_pButtonItem = pButtonItem;

	int nBoneNameListSize = boneNameList.size();
	m_BoneNameList.reserve(nBoneNameListSize);
	for( int i = 0; i < nBoneNameListSize; ++i )
		m_BoneNameList.push_back(boneNameList[i].c_str());
}

CEventPresetDlg::~CEventPresetDlg()
{
}

void CEventPresetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CEventPresetDlg::OnInitDialog()
{
	CWnd* pWnd = GetDlgItem(IDC_EVENTPRESET_LISTBOX);
	CRect clientRect;
	pWnd->GetClientRect(&clientRect);

	m_PropertyCtrl.Create( WS_CHILD|WS_VISIBLE, &clientRect, this );

	CVarBlock* pBlock = new CVarBlock; 	

	m_pPresetNameVar = new CVariable<CString>;
	m_pPresetNameVar->Set(m_pButtonItem->m_Name);
	pBlock->AddVariable(m_pPresetNameVar, "Preset Name" );

	m_pButtonColorVar = new CVariable<Vec3>;
	m_pButtonColorVar->Set(m_pButtonItem->m_Color);
	pBlock->AddVariable( m_pButtonColorVar, "Button Color", IVariable::DT_COLOR );

	m_pNameVar = new CVariable<CString>;
	m_pNameVar->Set(m_pEventData->GetName());
	pBlock->AddVariable( m_pNameVar, "Name" );

	m_pParameterVar = new CVariable<CString>;
	m_pParameterVar->Set(m_pEventData->GetCustomParameter());
	pBlock->AddVariable( m_pParameterVar, "Parameter" );

	CVarEnumList<CString>* boneEnumList = new CVarEnumList<CString>;
	for( int i = 0, iSize(m_BoneNameList.size()); i < iSize; ++i )
		boneEnumList->AddItem(m_BoneNameList[i],m_BoneNameList[i]);
	m_BoneListVar->SetEnumList( boneEnumList );
	if( m_pEventData->GetBoneName() && strlen(m_pEventData->GetBoneName()) > 0 )
		m_BoneListVar->Set( m_pEventData->GetBoneName() );
	else
		m_BoneListVar->Set( "<none>" );
	pBlock->AddVariable(m_BoneListVar, "Bone");

	m_pOffsetVar = new CVariable<Vec3>;
	m_pOffsetVar->Set(m_pEventData->GetOffset());
	pBlock->AddVariable( m_pOffsetVar, "Offset" );

	m_pDirVar = new CVariable<Vec3>;
	m_pDirVar->Set(m_pEventData->GetDirection());
	pBlock->AddVariable( m_pDirVar, "Dir" );

	m_pModelVar = new CVariable<CString>;
	m_pModelVar->Set(m_pEventData->GetModelName());
	pBlock->AddVariable( m_pModelVar, "Model" );

	m_PropertyCtrl.AddVarBlock(pBlock);

	CRect rect;
	GetClientRect(&rect);	

	CRect parentWndRect;
	GetParent()->GetWindowRect(parentWndRect);

	MoveWindow( parentWndRect.CenterPoint().x, parentWndRect.CenterPoint().y, rect.Width(), rect.Height() );

	return TRUE;
}

void CEventPresetDlg::OnOK()
{
	CString name;
	m_pNameVar->Get(name);
	m_pEventData->SetName(name);

	CString parameter;
	m_pParameterVar->Get(parameter);
	m_pEventData->SetCustomParameter(parameter);

	CString bone;
	m_BoneListVar->Get(bone);
	m_pEventData->SetBoneName(bone);

	Vec3 offset;
	m_pOffsetVar->Get(offset);
	m_pEventData->SetOffset(offset);

	Vec3 dir;
	m_pDirVar->Get(dir);
	m_pEventData->SetDirection(dir);

	CString model;
	m_pModelVar->Get(model);
	m_pEventData->SetModelName(model);

	m_pPresetNameVar->Get(m_pButtonItem->m_Name);
	m_pButtonColorVar->Get(m_pButtonItem->m_Color);

	__super::OnOK();
}