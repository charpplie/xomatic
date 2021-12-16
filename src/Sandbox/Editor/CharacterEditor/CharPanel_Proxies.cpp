// CharEditAttachmentsDlg.cpp : implementation file
//

#include "stdafx.h"
#include <I3DEngine.h>
#include <ICryAnimation.h>

#include "StringDlg.h"
#include "CharPanel_Proxies.h"
#include "CharacterEditor.h"
#include "ModelViewportCE.h"
#include ".\charpanel_attachments.h"
#include "Material/MaterialManager.h"

IMPLEMENT_DYNAMIC(CProxiesDlg, CDialog)

class CJointList
{
public:
	CJointList()
	{
	}

	void Clear()
	{
		m_indexNameMap.clear();
		m_nameIndexMap.clear();
		m_nameOrigIndexMap.clear();
	}

	void InitFromNamesList( const std::vector< CString >& names )
	{
		m_nameOrigIndexMap.clear();
		m_nameIndexMap.clear();

		m_indexNameMap.clear();
		m_indexNameMap.reserve( names.size() );

		for ( int i = 0; i < names.size(); ++i )
		{
			const CString& name = names[ i ];
			m_nameOrigIndexMap[ name ] = i;
			m_nameIndexMap[name] = -1;
		}

		for ( std::map<CString, int>::iterator cit = m_nameIndexMap.begin(); cit != m_nameIndexMap.end(); ++cit )
		{
			const CString& name = cit->first;
			cit->second = m_indexNameMap.size();
			m_indexNameMap.push_back( name );
		}
	}

	const CString& NameFromIndex(int index)
	{
		return m_indexNameMap[index];
	}

	int IndexFromName(const CString& name)
	{
		return m_nameIndexMap[name];
	}

	int OrigIndexFromName(const CString& name)
	{
		return m_nameOrigIndexMap[name];
	}

	// Get the size of names
	const int GetSize() const
	{
		return m_nameIndexMap.size();
	}

private:
	std::vector<CString> m_indexNameMap;
	std::map<CString, int> m_nameIndexMap; // Sorted Name index
	std::map<CString, int> m_nameOrigIndexMap; //The original order, not sorted by Name

};

class CJointComboBoxManager
{
public:
	CJointComboBoxManager(CComboBox& comboBox) :	m_comboBox(comboBox), 	m_selectionIndex(-1)
	{
	}

	void Clear()
	{
		m_jointList.Clear();
		//for (int i = m_comboBox.GetCount()-1; i >= 0; i--)
		//	 m_comboBox->DeleteString( i );
		m_comboBox.ResetContent();
	}


	void InitFromBoneNameList( const std::vector< CString >& bonesList )
	{
		m_jointList.InitFromNamesList( bonesList );
		
		m_comboBox.ResetContent();
		CString text = NULL;
		text.Format("Total bones: %.2d", m_jointList.GetSize());
		m_comboBox.AddString(text.GetString());

		for(int i=0; i<m_jointList.GetSize(); ++i)
		{
			const CString name = m_jointList.NameFromIndex(i);
			text.Format("%.2d - %s ", m_jointList.OrigIndexFromName(name), name);
			m_comboBox.AddString(text.GetString());
		}
	}

	void SelectBone(const CString& name)
	{
		m_selectionIndex = m_jointList.IndexFromName(name)+1;
		m_comboBox.SetCurSel(m_selectionIndex);
	}

	void SelectBone(int index)
	{
		if(index == 0) // This is the total amount of joint
			return;

		m_selectionIndex = index; // Substract the fist line: the total amount of joint
		m_comboBox.SetCurSel(m_selectionIndex);
	}

	const CString& GetSelectedBone()
	{
		if (m_selectionIndex ==0 || m_selectionIndex == -1)
		{
			static CString empty("");
			return empty;
		}
		return m_jointList.NameFromIndex(m_selectionIndex-1);
	}

private:
	CJointList m_jointList;
	CComboBox& m_comboBox;
	int m_selectionIndex; // The actual selected index, after adding the total amount of bones on the top.
};









BOOL CProxiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_pJointComboBoxManager = new CJointComboBoxManager(m_strJointName);

	CCustomButton* pdrawDynamicProxies = (CCustomButton*)GetDlgItem(IDC_DRAW_DYNAMIC_PROXIES);
	pdrawDynamicProxies->SetCheck(0);
	pdrawDynamicProxies->EnableWindow(TRUE);

	CCustomButton* pdrawAuxiliaryProxies = (CCustomButton*)GetDlgItem(IDC_DRAW_AUXILIARY_PROXIES);
	pdrawAuxiliaryProxies->SetCheck(0);
	pdrawAuxiliaryProxies->EnableWindow(TRUE);

	CCustomButton* pdrawClothProxies = (CCustomButton*)GetDlgItem(IDC_DRAW_CLOTH_PROXIES);
	pdrawClothProxies->SetCheck(0);
	pdrawClothProxies->EnableWindow(TRUE);

	CCustomButton* pdrawRagdollProxies = (CCustomButton*)GetDlgItem(IDC_DRAW_RAGDOLL_PROXIES);
	pdrawRagdollProxies->SetCheck(0);
	pdrawRagdollProxies->EnableWindow(TRUE);

	CCustomButton* pHideProxy = (CCustomButton*)GetDlgItem(IDC_HIDE_PROXY);
	pHideProxy->SetCheck(0);
	pHideProxy->EnableWindow(TRUE);

	m_Proxy_Radius.Create(this,IDC_PROXPARA_R,CNumberCtrl::LEFTALIGN );
	m_Proxy_Radius.SetRange(0.0f,10.0f);
	m_Proxy_Radius.SetInteger(false);
	m_Proxy_Radius.SetInternalPrecision( 3 );
	m_Proxy_Radius.SetValue(0.0f);
	m_Proxy_Radius.EnableWindow(false);

	m_Proxy_XAxis.Create(this,IDC_PROXPARA_X,CNumberCtrl::LEFTALIGN );
	m_Proxy_XAxis.SetRange(0.0f,10.0f);
	m_Proxy_XAxis.SetInteger(false);
	m_Proxy_XAxis.SetInternalPrecision( 3 );
	m_Proxy_XAxis.SetValue(0.0f);
	m_Proxy_XAxis.EnableWindow(false);

	m_Proxy_YAxis.Create(this,IDC_PROXPARA_Y,CNumberCtrl::LEFTALIGN );
	m_Proxy_YAxis.SetRange(0.0f,10.0f);
	m_Proxy_YAxis.SetInteger(false);
	m_Proxy_YAxis.SetInternalPrecision( 3 );
	m_Proxy_YAxis.SetValue(0.0f);
	m_Proxy_YAxis.EnableWindow(false);

	m_Proxy_ZAxis.Create(this,IDC_PROXPARA_Z,CNumberCtrl::LEFTALIGN );
	m_Proxy_ZAxis.SetRange(0.0f,10.0f);
	m_Proxy_ZAxis.SetInteger(false);
	m_Proxy_ZAxis.SetInternalPrecision( 3 );
	m_Proxy_ZAxis.SetValue(0.0f);
	m_Proxy_ZAxis.EnableWindow(false);

	m_Proxy_Purpose.ResetContent();
	m_Proxy_Purpose.AddString("Auxiliary Proxy");
	m_Proxy_Purpose.AddString("Cloth Proxy");
	m_Proxy_Purpose.AddString("Ragdoll Proxy");

	return TRUE;
}


void CProxiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BONE, m_strJointName);
	DDX_Control(pDX, IDC_BUTTON_ALIGNBONEATTACHMENT,m_ButtonAlignBoneAttachment );

	DDX_Control(pDX, IDC_PROXBUTTON_RENAME, m_ButtonRENAME);
	DDX_Control(pDX, IDC_PROXBUTTON_REMOVE, m_ButtonREMOVE);
	DDX_Control(pDX, IDC_PROXBUTTON_EXPORT, m_ButtonEXPORT);
	DDX_Control(pDX, IDC_PROXIES,m_ProxyList );

	DDX_Control(pDX, IDC_PROXY_PURPOSE, m_Proxy_Purpose);
}


BEGIN_MESSAGE_MAP(CProxiesDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ALIGNBONEATTACHMENT, OnClicked_AlignJointWithProxy)
	ON_CBN_SELCHANGE(IDC_BONE, OnJointSelect)

	ON_BN_CLICKED(IDC_PROXBUTTON_NEW,    OnBnClicked_NEW)
	ON_BN_CLICKED(IDC_PROXBUTTON_RENAME, OnBnClicked_RENAME)
	ON_BN_CLICKED(IDC_PROXBUTTON_REMOVE, OnBnClicked_REMOVE)
	ON_BN_CLICKED(IDC_PROXBUTTON_IMPORT, OnBnClicked_IMPORT)
	ON_BN_CLICKED(IDC_PROXBUTTON_EXPORT, OnBnClicked_EXPORT)

	ON_LBN_SELCHANGE(IDC_PROXIES,OnProxySelect)

	ON_EN_UPDATE(IDC_PROXPARA_R, OnChange_ProxyParameters)
	ON_EN_UPDATE(IDC_PROXPARA_X, OnChange_ProxyParameters)
	ON_EN_UPDATE(IDC_PROXPARA_Y, OnChange_ProxyParameters)
	ON_EN_UPDATE(IDC_PROXPARA_Z, OnChange_ProxyParameters)

	ON_BN_CLICKED(IDC_DRAW_DYNAMIC_PROXIES, OnChange_ProxyParameters)
	ON_BN_CLICKED(IDC_DRAW_AUXILIARY_PROXIES, OnChange_ProxyParameters)
	ON_BN_CLICKED(IDC_DRAW_CLOTH_PROXIES, OnChange_ProxyParameters)
	ON_BN_CLICKED(IDC_DRAW_RAGDOLL_PROXIES, OnChange_ProxyParameters)
	ON_BN_CLICKED(IDC_HIDE_PROXY, OnChange_ProxyParameters)

	ON_CBN_SELCHANGE(IDC_PROXY_PURPOSE,OnChange_ProxyParameters)

END_MESSAGE_MAP()




//////////////////////////////////////////////////////////////////////////
void CProxiesDlg::InitFromJointNameList( const std::vector< CString >& jointNames )
{
	m_pJointComboBoxManager->InitFromBoneNameList( jointNames );
}

//////////////////////////////////////////////////////////////////////////
void CProxiesDlg::ClearBones()
{
	m_pJointComboBoxManager->Clear();
}

//////////////////////////////////////////////////////////////////////////
void CProxiesDlg::SelectBone( const CString &bone )
{
	m_pJointComboBoxManager->SelectBone(bone);
}

CString CProxiesDlg::GetBonenameFromWindow()
{
	return m_pJointComboBoxManager->GetSelectedBone();
}


void CProxiesDlg::OnClicked_AlignJointWithProxy()
{
	IProxy* pIProxy = GetSelectedIProxy();	
	if (!pIProxy)
		return;

	m_pModelViewportCE->m_SelectedProxy = 0;
	pIProxy->AlignProxyWithJoint();
	OnProxySelect();

}


void CProxiesDlg::OnProxySelect()
{
	IProxy* pIProxy = GetSelectedIProxy();
	if (pIProxy==0) 
		return;

	CCustomButton* pButton=(CCustomButton*)GetDlgItem(IDC_BUTTON_HIDEATTACH);

	CCustomButton* pAlignButton=(CCustomButton*)GetDlgItem(IDC_BUTTON_ALIGNBONEATTACHMENT);

	m_strJointName.EnableWindow(TRUE);
	//set joint-name into the window
	uint32 nJointID = pIProxy->GetJointID();
	ICharacterInstance* pCharacter = m_pModelViewportCE->GetCharacterBase();
	const char* strJointName = pCharacter->GetIDefaultSkeleton().GetJointNameByID(nJointID);
	m_pJointComboBoxManager->SelectBone( strJointName );

	//initialized dialog with physical properties
	Vec4 para = pIProxy->GetProxyParams();
	m_Proxy_Radius.SetValue(para.w);
	m_Proxy_Radius.EnableWindow(1);
	m_Proxy_XAxis.SetValue(para.x);
	m_Proxy_XAxis.EnableWindow(1);
	m_Proxy_YAxis.SetValue(para.y);
	m_Proxy_YAxis.EnableWindow(1);
	m_Proxy_ZAxis.SetValue(para.z);
	m_Proxy_ZAxis.EnableWindow(1);

	int8 purpose = pIProxy->GetProxyPurpose();
	m_Proxy_Purpose.SetCurSel(purpose);

	UpdateData( FALSE );
}



IProxy* CProxiesDlg::GetSelectedIProxy()
{
	m_ButtonRENAME.EnableWindow(FALSE);
	m_ButtonREMOVE.EnableWindow(FALSE);

	int nSel = m_ProxyList.GetCurSel();
	if (nSel == LB_ERR)
		return 0;

	m_ButtonRENAME.EnableWindow(TRUE);
	m_ButtonREMOVE.EnableWindow(TRUE);

	CString name;
	m_ProxyList.GetText(nSel,name);

	ICharacterInstance *pCharacter = m_pModelViewportCE->GetCharacterBase();
	if (!pCharacter)
		return 0;

	IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
	IProxy* pIProxy = pAttachmentManager->GetProxyInterfaceByName(name);
	if (pIProxy)
	{
		m_pModelViewportCE->m_SelectedAttachment			=	pAttachmentManager->GetProxyIndexByName(name)|PROXYINDICATTION;
		m_pModelViewportCE->m_ArcBall.DragRotation.SetIdentity();
		m_pModelViewportCE->m_ArcBall.ObjectRotation	=	pIProxy->GetProxyAbsoluteDefault().q;
		m_pModelViewportCE->m_ArcBall.sphere.center		= pIProxy->GetProxyAbsoluteDefault().t; 
	}
	return pIProxy;
}


void CProxiesDlg::OnJointSelect()
{
	m_pJointComboBoxManager->SelectBone(m_strJointName.GetCurSel());
}



void CProxiesDlg::UpdateList() 
{
  ICharacterInstance* pCharacter = m_pModelViewportCE->GetCharacterBase();
  if (pCharacter) 
	{
    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    if (pCharacter) 
    {
      //update selection window
      m_ProxyList.ResetContent();
      int anum = pIAttachmentManager->GetProxyCount();
      if (anum) 
      {
        CString aname; 
        for (int i=0; i<anum; i++)
        {
          IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByIndex(i);
           aname = pIProxy->GetName();
           m_ProxyList.AddString( aname );
        }
        IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByIndex(0);
        aname = pIProxy->GetName();
        int32 idx = m_ProxyList.FindString(-1, aname);
        m_ProxyList.SetCurSel( idx );
      }

      if (anum) 
			{ 
        m_ButtonRENAME.EnableWindow(TRUE); 
        m_ButtonREMOVE.EnableWindow(TRUE); 
        m_ButtonEXPORT.EnableWindow(TRUE); 
      }	
			else 
			{  
        m_ButtonRENAME.EnableWindow(FALSE); 
        m_ButtonREMOVE.EnableWindow(FALSE); 
        m_ButtonEXPORT.EnableWindow(FALSE); 
      }
    }

		OnProxySelect();
		CCustomButton* pdrawDyanmicProxies  = (CCustomButton*)GetDlgItem(IDC_DRAW_DYNAMIC_PROXIES);
		uint32 drawProxies0 = pdrawDyanmicProxies->GetCheck();
		if (drawProxies0)
			pCharacter->GetIAttachmentManager()->DrawProxies( drawProxies0<<0 );
		else
			pCharacter->GetIAttachmentManager()->DrawProxies( 1^-1 );

		CCustomButton* pdrawAuxiliaryProxies  = (CCustomButton*)GetDlgItem(IDC_DRAW_AUXILIARY_PROXIES);
		uint32 drawProxies1 = pdrawAuxiliaryProxies->GetCheck();
		if (drawProxies1)
			pCharacter->GetIAttachmentManager()->DrawProxies( drawProxies1<<1 );
		else
			pCharacter->GetIAttachmentManager()->DrawProxies( 2^-1 );

		CCustomButton* pdrawClothProxies  = (CCustomButton*)GetDlgItem(IDC_DRAW_CLOTH_PROXIES);
		uint32 drawProxies2 = pdrawClothProxies->GetCheck();
		if (drawProxies2)
			pCharacter->GetIAttachmentManager()->DrawProxies( drawProxies2<<2 );
		else
			pCharacter->GetIAttachmentManager()->DrawProxies( 4^-1 );

		CCustomButton* pdrawRagdollProxies  = (CCustomButton*)GetDlgItem(IDC_DRAW_RAGDOLL_PROXIES);
		uint32 drawProxies3 = pdrawRagdollProxies->GetCheck();
		if (drawProxies3)
			pCharacter->GetIAttachmentManager()->DrawProxies( drawProxies3<<3 );
		else
			pCharacter->GetIAttachmentManager()->DrawProxies( 8^-1 );
  }
}


void CProxiesDlg::OnBnClicked_NEW()
{
	//	CString relFileName;
	CStringDlg dlg( _T( "Enter Attachment Name" ),this );
	dlg.SetString( "Default" );

	if (dlg.DoModal() == IDOK)
	{
		CString strProxyName = dlg.GetString();
		ICharacterInstance *pCharacter = m_pModelViewportCE->GetCharacterBase();
		if (!pCharacter)
			return;
		IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
		IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByName(strProxyName);
		if (pIProxy) 
			return;  //if name exists, don't do anything
		CString strJointName = m_pJointComboBoxManager->GetSelectedBone();
		pIAttachmentManager->CreateProxy( strProxyName, strJointName );
		uint32 num = pIAttachmentManager->GetProxyCount();
		if (num==1) 
		{
			IProxy* p = pIAttachmentManager->GetProxyInterfaceByIndex(0);
			m_pModelViewportCE->m_ArcBall.DragRotation.SetIdentity();
			m_pModelViewportCE->m_ArcBall.ObjectRotation	=	p->GetProxyAbsoluteDefault().q;
			m_pModelViewportCE->m_ArcBall.sphere.center		= p->GetProxyAbsoluteDefault().t; 
		}

		//rebuild the UI-proxy list
		m_ProxyList.ResetContent();
		num = pIAttachmentManager->GetProxyCount();
		for (int i = 0; i < num; i++)
		{
			IProxy* pprox = pIAttachmentManager->GetProxyInterfaceByIndex(i);
		  m_ProxyList.AddString( pprox->GetName() ) ;
		}
		int32 idx = m_ProxyList.FindString(-1, strProxyName);
		m_ProxyList.SetCurSel( idx );
		
		m_ButtonRENAME.EnableWindow(TRUE);
		m_ButtonREMOVE.EnableWindow(TRUE);
		m_ButtonEXPORT.EnableWindow(TRUE);
	}
}



void CProxiesDlg::OnBnClicked_RENAME()
{
	int nSel = m_ProxyList.GetCurSel();
	if (nSel == LB_ERR)
		return;

	CString oldname;
	m_ProxyList.GetText(nSel,oldname);

	CString relFileName;
	CStringDlg dlg( _T( "Enter New Name" ),this );
	dlg.SetString( oldname );

	if (dlg.DoModal() == IDOK)
	{
		CString newname = dlg.GetString();

		ICharacterInstance *pCharacter = m_pModelViewportCE->GetCharacterBase();
		if (!pCharacter)
			return;
		IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
		IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByName(newname);
		if (pIProxy)
		{
			CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "Proxy name '%s' is already in use, attachment will not be renamed", newname);
			return;
		}
		uint32 nameCRC = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(newname);
		pIProxy = pIAttachmentManager->GetProxyInterfaceByCRC(nameCRC);
		if (pIProxy)
		{
			CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "Proxy name crc for '%s' clashes with attachment name '%s' (crc's are created using lower case only), attachment will not be renamed", newname, pIProxy->GetName());
			return;
		}

		pIProxy = pIAttachmentManager->GetProxyInterfaceByName(oldname);
	  pIProxy->ReName( newname, nameCRC );

		//rebuild the UI-proxy list
		m_ProxyList.ResetContent();
		int num = pIAttachmentManager->GetProxyCount();
		for (int i = 0; i < num; i++)
		{
			IProxy* pProxy = pIAttachmentManager->GetProxyInterfaceByIndex(i);
			const char* strProxyName = pProxy->GetName();
			m_ProxyList.AddString( strProxyName ) ;
		}
		int32 idx = m_ProxyList.FindString(-1, newname);
		m_ProxyList.SetCurSel( idx );
	}
}


void CProxiesDlg::OnBnClicked_REMOVE()
{
	int nSel = m_ProxyList.GetCurSel();
	if (nSel == LB_ERR)
		return;

	CString name;
	m_ProxyList.GetText(nSel,name);

	ICharacterInstance *pCharacter = m_pModelViewportCE->GetCharacterBase();
	if (pCharacter==0)
		return;

	uint32 nSelectedProxy=m_ProxyList.GetCurSel( );
	IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
	IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByName(name);
	int32 result = pIAttachmentManager->RemoveProxyByInterface(pIProxy);

	//rebuild the UI-proxy list
	m_ProxyList.ResetContent();
	int num = pIAttachmentManager->GetProxyCount();
	for (int i = 0; i < num; i++)
	{
		IProxy* pProxy = pIAttachmentManager->GetProxyInterfaceByIndex(i);
		const char* strProxyName = pProxy->GetName();
		m_ProxyList.AddString( strProxyName ) ;
	}
	if (num)
	{
		if (nSelectedProxy>=num)
			nSelectedProxy=num;
		m_ProxyList.SetCurSel( nSelectedProxy );
		OnProxySelect();
	}

	//initialize selection
	uint32 numProxy = pIAttachmentManager->GetProxyCount();
	if (numProxy) 
	{ 
		m_ButtonEXPORT.EnableWindow(TRUE); 
		m_ButtonRENAME.EnableWindow(FALSE);
		m_ButtonREMOVE.EnableWindow(FALSE);

		IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByIndex(m_pModelViewportCE->m_SelectedAttachment&0xfff);  
		m_pModelViewportCE->m_ArcBall.DragRotation.SetIdentity();
		m_pModelViewportCE->m_ArcBall.ObjectRotation	=	pIProxy->GetProxyAbsoluteDefault().q;
		m_pModelViewportCE->m_ArcBall.sphere.center		= pIProxy->GetProxyAbsoluteDefault().t; 
	} 
	else 
	{ 
		m_ButtonRENAME.EnableWindow(FALSE);
		m_ButtonREMOVE.EnableWindow(FALSE);
		m_ButtonEXPORT.EnableWindow(FALSE); 
	}
}




void CProxiesDlg::OnBnClicked_IMPORT()
{

	char szFilters[] = "Attachment List Files|*.atl; | Attachment List Files (*.atl)|*.atl | All files (*.*)|*.*| |";
	CAutoDirectoryRestoreFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR, szFilters);

	if (dlg.DoModal() == IDOK) 
	{
		char ext[_MAX_EXT];
		_splitpath( dlg.GetPathName(),NULL,NULL,NULL,ext );
		if (stricmp(ext,".atl") == 0)
		{
			CLogFile::WriteLine("Importing Attachment List...");
			CString ATL_FileName = dlg.GetPathName();
			ICharacterInstance* pCharacter = m_pModelViewportCE->GetCharacterBase();
			if (pCharacter) {
				IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
				pIAttachmentManager->LoadAttachmentList( ATL_FileName );

				//initialize selection
				uint32 numAttachment = pIAttachmentManager->GetAttachmentCount();
				if (numAttachment) { 
					m_ButtonEXPORT.EnableWindow(TRUE); 
					m_ButtonRENAME.EnableWindow(FALSE);
					m_ButtonREMOVE.EnableWindow(FALSE);

					IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(0);  
					uint32 type = pIAttachment->GetType();
					if (type==CA_BONE || type==CA_FACE) 
					{
						m_pModelViewportCE->m_ArcBall.DragRotation.SetIdentity();
						m_pModelViewportCE->m_ArcBall.ObjectRotation	=	pIAttachment->GetAttAbsoluteDefault().q;
						m_pModelViewportCE->m_ArcBall.sphere.center		= pIAttachment->GetAttAbsoluteDefault().t; 
					}
					m_pModelViewportCE->m_pAttachmentsDlg->UpdateList();
					string name = pIAttachment->GetName();
					uint32 n = m_pModelViewportCE->m_pAttachmentsDlg->m_attachmentsList.FindString(-1,name);
					m_pModelViewportCE->m_pAttachmentsDlg->m_attachmentsList.SetCurSel(n);
					m_pModelViewportCE->m_pAttachmentsDlg->OnAttachmentSelect();
				} 
				else 
				{ 
					m_ButtonRENAME.EnableWindow(FALSE);
					m_ButtonREMOVE.EnableWindow(FALSE);
					m_ButtonEXPORT.EnableWindow(FALSE); 
				}
			}
		}
		BeginWaitCursor();
	}

	//-------------------------------------------------------------------------------
	UpdateList();

}

void CProxiesDlg::OnBnClicked_EXPORT()
{
	char szFilters[] = "Attachment List Files (*.atl)|*.atl| ";
	CAutoDirectoryRestoreFileDialog dlg(FALSE, "atl", NULL, OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR, szFilters);

	// Show the dialog
	if (dlg.DoModal() == IDOK) 
	{
		BeginWaitCursor();
		char ext[_MAX_EXT];
		_splitpath( dlg.GetPathName(),NULL,NULL,NULL,ext );
		if (stricmp(ext,".atl") == 0)
		{
			CLogFile::WriteLine("Exporting Attachment List ...");
			CString ATL_FileName = dlg.GetPathName();
			ICharacterInstance* pCharacter = m_pModelViewportCE->GetCharacterBase();
			if (pCharacter) {
				pCharacter->GetIAttachmentManager()->SaveAttachmentList( ATL_FileName );
			}
		}
		EndWaitCursor();
	}
}


void CProxiesDlg::OnChange_ProxyParameters()
{
	ICharacterInstance* pCharacter = m_pModelViewportCE->GetCharacterBase();
	if (pCharacter==0)
		return;

	CCustomButton* pdrawDyanmicProxies  = (CCustomButton*)GetDlgItem(IDC_DRAW_DYNAMIC_PROXIES);
	uint32 drawProxies0 = pdrawDyanmicProxies->GetCheck();
	if (drawProxies0)
		pCharacter->GetIAttachmentManager()->DrawProxies( drawProxies0<<0 );
	else
		pCharacter->GetIAttachmentManager()->DrawProxies( 1^-1 );

	CCustomButton* pdrawAuxiliaryProxies  = (CCustomButton*)GetDlgItem(IDC_DRAW_AUXILIARY_PROXIES);
	uint32 drawProxies1 = pdrawAuxiliaryProxies->GetCheck();
	if (drawProxies1)
		pCharacter->GetIAttachmentManager()->DrawProxies( drawProxies1<<1 );
	else
		pCharacter->GetIAttachmentManager()->DrawProxies( 2^-1 );

	CCustomButton* pdrawClothProxies  = (CCustomButton*)GetDlgItem(IDC_DRAW_CLOTH_PROXIES);
	uint32 drawProxies2 = pdrawClothProxies->GetCheck();
	if (drawProxies2)
		pCharacter->GetIAttachmentManager()->DrawProxies( drawProxies2<<2 );
	else
		pCharacter->GetIAttachmentManager()->DrawProxies( 4^-1 );

	CCustomButton* pdrawRagdollProxies  = (CCustomButton*)GetDlgItem(IDC_DRAW_RAGDOLL_PROXIES);
	uint32 drawProxies3 = pdrawRagdollProxies->GetCheck();
	if (drawProxies3)
		pCharacter->GetIAttachmentManager()->DrawProxies( drawProxies3<<3 );
	else
		pCharacter->GetIAttachmentManager()->DrawProxies( 8^-1 );

	IProxy* pIProxy = GetSelectedIProxy();	
	if (pIProxy==0)
		return;

	Vec4 params;
	params.x	= m_Proxy_XAxis.GetValue();
	params.y	= m_Proxy_YAxis.GetValue();
	params.z	= m_Proxy_ZAxis.GetValue();
	params.w	= m_Proxy_Radius.GetValue();
	pIProxy->SetProxyParams(params);

	CCustomButton* pHideProxy  = (CCustomButton*)GetDlgItem(IDC_HIDE_PROXY);
	uint32 nHideProxy = pHideProxy->GetCheck();
	pIProxy->SetHideProxy(nHideProxy);

	int8 purpose = m_Proxy_Purpose.GetCurSel();
	pIProxy->SetProxyPurpose(purpose);
}


CProxiesDlg::~CProxiesDlg()
{
	if (m_pJointComboBoxManager)
		delete m_pJointComboBoxManager;
}
