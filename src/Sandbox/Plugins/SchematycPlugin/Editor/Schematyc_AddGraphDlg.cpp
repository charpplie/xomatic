/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add graph dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddGraphDlg.h"

#include <Schematyc/Schematyc_DocUtils.h>

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CAddGraphDlg, CDialog)
		ON_EN_CHANGE(IDC_SCHEMATYC_GRAPH_NAME, OnNameCtrlSelChange)
		ON_LBN_SELCHANGE(IDD_SCHEMATYC_GRAPH_CONTEXTS, OnContextsCtrlSelChange)
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	CAddGraphDlg::CAddGraphDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID, DocGraphType::EValue type)
		: CDialog(IDD_SCHEMATYC_ADD_GRAPH, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
		, m_type(type)
		, m_nameModified(false)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddGraphDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		switch(m_type)
		{
		case DocGraphType::CONSTRUCTOR:
		case DocGraphType::DESTRUCTOR:
			{
				m_contexts.push_back(TGUIDStringPair(SGUID(), "Any"));
			}
		case DocGraphType::SIGNAL_RECEIVER:
			{
				GetSchematycFramework().GetEnvRegistry().VisitSignals(MAKE_MEMBER_DELEGATE(CAddGraphDlg::VisitEnvSignal, *this));
				m_doc.VisitSignals(MAKE_MEMBER_DELEGATE(CAddGraphDlg::VisitDocSignal, *this));
				m_doc.VisitTimers(MAKE_MEMBER_DELEGATE(CAddGraphDlg::VisitDocTimer, *this));
				for(TGUIDStringPairVector::const_iterator iContext = m_contexts.begin(), iEndContext = m_contexts.end(); iContext != iEndContext; ++ iContext)
				{
					m_contextsCtrl.AddString(iContext->second.c_str());
				}
				break;
			}
		default:
			{
				m_contextsCtrl.EnableWindow(FALSE);
			}
		}
		m_contextsCtrl.SetCurSel(0);
		OnContextsCtrlSelChange();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddGraphDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_GRAPH_NAME, m_nameCtrl);
		DDX_Control(pDX, IDD_SCHEMATYC_GRAPH_CONTEXTS, m_contextsCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddGraphDlg::OnNameCtrlSelChange()
	{
		m_nameModified = true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddGraphDlg::OnContextsCtrlSelChange()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if((m_nameModified == false) || (name.IsEmpty() == true))
		{
			stack_string	autoName;
			switch(m_type)
			{
			case DocGraphType::CONSTRUCTOR:
				{
					autoName = "Constructor";
					break;
				}
			case DocGraphType::DESTRUCTOR:
				{
					autoName = "Destructor";
					break;
				}
			case DocGraphType::SIGNAL_RECEIVER:
				{
					autoName = "On";
					autoName.append(GetSelectionString());
					autoName.replace(":", "");
					break;
				}
			}
			if(autoName.empty() == false)
			{
				m_nameCtrl.SetWindowText(autoName.c_str());
				m_nameModified = false;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddGraphDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			switch(m_type)
			{
			case DocGraphType::FUNCTION:
				{
					IDocGraphPtr	pGraph = m_doc.AddGraph(m_scopeGUID, name.GetString(), m_type, SGUID());
					CRY_ASSERT(pGraph != NULL);
					if(pGraph != NULL)
					{
						// TODO : Should really do this somewhere in the doc graph code!!!
						pGraph->AddNode(DocGraphNodeType::BEGIN, SGUID(), Vec2(0.0f, 0.0f));
					}
					break;
				}
			case DocGraphType::CONDITION:
				{
					IDocGraphPtr	pGraph = m_doc.AddGraph(m_scopeGUID, name.GetString(), m_type, SGUID());
					CRY_ASSERT(pGraph != NULL);
					if(pGraph != NULL)
					{
						// TODO : Should really do this somewhere in the doc graph code!!!
						pGraph->AddNode(DocGraphNodeType::BEGIN, SGUID(), Vec2(0.0f, 0.0f));
					}
					break;
				}
			case DocGraphType::CONSTRUCTOR:
				{
					const SGUID		contextGUID = GetSelectionGUID();
					IDocGraphPtr	pGraph = m_doc.AddGraph( m_scopeGUID, name.GetString(), m_type, contextGUID);
					CRY_ASSERT(pGraph != NULL);
					if(pGraph != NULL)
					{
						// TODO : Should really do this somewhere in the doc graph code!!!
						pGraph->AddNode(DocGraphNodeType::BEGIN_CONSTRUCTOR, SGUID(), Vec2(0.0f, 0.0f));
					}
					break;
				}
			case DocGraphType::DESTRUCTOR:
				{
					const SGUID		contextGUID = GetSelectionGUID();
					IDocGraphPtr	pGraph = m_doc.AddGraph( m_scopeGUID, name.GetString(), m_type, contextGUID);
					CRY_ASSERT(pGraph != NULL);
					if(pGraph != NULL)
					{
						// TODO : Should really do this somewhere in the doc graph code!!!
						pGraph->AddNode(DocGraphNodeType::BEGIN_DESTRUCTOR, SGUID(), Vec2(0.0f, 0.0f));
					}
					break;
				}
			case DocGraphType::SIGNAL_RECEIVER:
				{
					const SGUID		contextGUID = GetSelectionGUID();
					IDocGraphPtr	pGraph = m_doc.AddGraph(m_scopeGUID, name.GetString(), m_type, contextGUID);
					CRY_ASSERT(pGraph != NULL);
					if(pGraph != NULL)
					{
						// TODO : Should really do this somewhere in the doc graph code!!!
						pGraph->AddNode(DocGraphNodeType::BEGIN_SIGNAL_RECEIVER, SGUID(), Vec2(0.0f, 0.0f));
					}
					break;
				}
			case DocGraphType::TRANSITION:
				{
					m_doc.AddGraph(m_scopeGUID, name.GetString(), m_type, SGUID());
					break;
				}
			}
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddGraphDlg::VisitEnvSignal(const ISignalConstPtr& pSignal)
	{
		if(DocUtils::IsSignalAvailableInScope(m_doc, m_scopeGUID, pSignal->GetSenderGUID(), pSignal->GetScope()))
		{
			stack_string	name;
			EnvRegistryUtils::GetFullName(pSignal->GetName(), pSignal->GetScope(), pSignal->GetSenderGUID(), name);
			m_contexts.push_back(TGUIDStringPair(pSignal->GetGUID(), name.c_str()));
		}
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddGraphDlg::VisitDocSignal(const IDocSignalConstPtr& pSignal)
	{
		stack_string	name;
		DocUtils::GetFullElementName(m_doc, *pSignal, name, ElementType::DOC_STATE);
		m_contexts.push_back(TGUIDStringPair(pSignal->GetGUID(), name.c_str()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddGraphDlg::VisitDocTimer(const IDocTimerConstPtr& pTimer)
	{
		stack_string	name;
		DocUtils::GetFullElementName(m_doc, *pTimer, name, ElementType::DOC_STATE);
		m_contexts.push_back(TGUIDStringPair(pTimer->GetGUID(), name.c_str()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CAddGraphDlg::ValidateName(const stack_string& name)
	{
		stack_string	errorMessage;
		if(PluginUtils::IsValidName(name, errorMessage) == true)
		{
			return true;
		}
		else
		{
			MessageBox(errorMessage.c_str(), "Invalid Name", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	SGUID CAddGraphDlg::GetSelectionGUID() const
	{
		const int	curSel = m_contextsCtrl.GetCurSel();
		return (curSel != LB_ERR) && (curSel >= 0) ? m_contexts[curSel].first : SGUID();
	}

	//////////////////////////////////////////////////////////////////////////
	const char* CAddGraphDlg::GetSelectionString() const
	{
		const int	curSel = m_contextsCtrl.GetCurSel();
		return (curSel != LB_ERR) && (curSel >= 0) ? m_contexts[curSel].second.c_str() : "";
	}
}