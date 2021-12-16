// SetVectorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ViewManager.h"
#include "SetVectorDlg.h"
#include "Objects/EntityObject.h"
#include "Include\ITransformManipulator.h"

/////////////////////////////////////////////////////////////////////////////
// CSetVectorDlg dialog


CSetVectorDlg::CSetVectorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSetVectorDlg::IDD, pParent)
{}


void CSetVectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSetVectorDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetVectorDlg message handlers


BOOL CSetVectorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CString editModeString;
	int emode = GetIEditor()->GetEditMode();

	if(emode==eEditModeMove)
	{
		editModeString = "Position";
	}
	else if (emode==eEditModeRotate)
	{
		editModeString = "Rotation";
	}
	else if (emode==eEditModeScale)
	{
		editModeString = "Scale";
	}

	char str[1024];
	sprintf(str, "Enter %s here:", editModeString);
	GetDlgItem(IDC_STATIC1)->SetWindowText(str);
	
	currentVec = GetVectorFromEditor();
	sprintf(str, "%.2f, %.2f, %.2f", currentVec.x, currentVec.y, currentVec.z);
	GetDlgItem(IDC_EDIT1)->SetWindowText(str);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSetVectorDlg::OnBnClickedOk()
{
	Vec3 newVec = GetVectorFromText();
	SetVector(newVec);

	if (GetIEditor()->GetEditMode() == eEditModeMove && currentVec.GetDistance(newVec)> 10.0f)
	{
		AfxGetMainWnd()->SendMessage(WM_COMMAND, MAKEWPARAM(ID_GOTO_SELECTED,0), 0);
	}
	OnOK();
}

Vec3 CSetVectorDlg::GetVectorFromEditor()
{
	Vec3 v;
	int emode = GetIEditor()->GetEditMode();
	CBaseObject *obj = GetIEditor()->GetSelectedObject();
	RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
	bool bWorldSpace = GetIEditor()->GetReferenceCoordSys() == COORDS_WORLD;

	if (obj)
	{
		v = obj->GetWorldPos();
	}

	if (emode == eEditModeMove)
	{
		if (obj)
		{
			if (bWorldSpace)
				v = obj->GetWorldTM().GetTranslation();
			else
				v = obj->GetPos();
		}
	}
	if (emode == eEditModeRotate)
	{
		if (obj)
		{
			if (bWorldSpace)
			{
				AffineParts ap;
				ap.SpectralDecompose(obj->GetWorldTM());
				v = Vec3(RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(ap.rot))));
			}
			else
			{
				v = Vec3(RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(obj->GetRotation()))));
			}
		}
	}
	if (emode == eEditModeScale)
	{
		if (obj)
		{
			if (bWorldSpace)
			{
				AffineParts ap;
				ap.SpectralDecompose(obj->GetWorldTM());
				v = ap.scale;
			}
			else
				v = obj->GetScale();
		}
	}
	return v;
}

Vec3 CSetVectorDlg::GetVectorFromText()
{
	float vec[3] = {0, 0, 0};
	CWnd * pWnd = GetDlgItem(IDC_EDIT1);
	if(pWnd)
	{
		CString m_sPos;
		pWnd->GetWindowText(m_sPos);
		if(m_sPos.GetLength()>0)
		{
			char str[1024];
			int len = m_sPos.GetLength();
			strcpy(str, m_sPos);
			str[1023] = 0;
			int i;
			for(i=0; i<len; i++)
				if(str[i] == ' ' || str[i] == ',' || str[i] == ';' || str[i] == '\t')
					str[i] = 0;

			int k;
			for(i=0, k=0; i<len && k<3; i++)
			{
				int ln = strlen(&str[i]);
				if(ln>0)
				{
					sscanf( &str[i],"%f",&vec[k]);
					i+=ln-1;
					k++;
				}
			}
		}
	}
	return Vec3(vec[0],vec[1],vec[2]);
}

void CSetVectorDlg::SetVector(const Vec3 &v)
{
	int emode = GetIEditor()->GetEditMode();
	if (emode != eEditModeMove && emode != eEditModeRotate && emode != eEditModeScale)
		return;

	int referenceCoordSys = GetIEditor()->GetReferenceCoordSys();

	CBaseObject *obj = GetIEditor()->GetSelectedObject();

	Matrix34 tm;
	AffineParts ap;
	if (obj)
	{
		tm = obj->GetWorldTM();
		ap.SpectralDecompose(tm);
	}

	if (emode == eEditModeMove)
	{
		if (obj)
		{
			if (referenceCoordSys == COORDS_WORLD)
			{
				tm.SetTranslation(v);
				obj->SetWorldTM(tm);
			}
			else
				obj->SetPos( v );
		} 
	}
	if (emode == eEditModeRotate)
	{
		if (obj)
		{
			Quat qrot( Quat::CreateRotationXYZ( DEG2RAD((Ang3)v)) );
			if (referenceCoordSys == COORDS_WORLD)
			{
				tm = Matrix34::Create(ap.scale,qrot,ap.pos);
				obj->SetWorldTM(tm);
			}
			else
				obj->SetRotation( qrot );
		} 
		else 
		{
			GetIEditor()->GetSelection()->Rotate( (Ang3)v,referenceCoordSys );
		}
	}
	if (emode == eEditModeScale)
	{
		if (v.x == 0 || v.y == 0 || v.z == 0)
			return;

		if (obj)
		{
			if (referenceCoordSys == COORDS_WORLD)
			{
				tm = Matrix34::Create(v,ap.rot,ap.pos);
				obj->SetWorldTM(tm);
			}
			else
				obj->SetScale( v );
		} 
		else 
		{
			GetIEditor()->GetSelection()->Scale( v,referenceCoordSys );
		}
	}
}