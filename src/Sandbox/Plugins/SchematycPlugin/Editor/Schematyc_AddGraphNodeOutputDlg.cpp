/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add graph node output dialog.
-------------------------------------------------------------------------
History:
- 22:05:2014: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddGraphNodeOutputDlg.h"

#include <Schematyc/Schematyc_DocUtils.h>

#include "Resource.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CAddGraphNodeOutputDialog::CAddGraphNodeOutputDialog(CWnd* pParent, CPoint pos, IDocGraphNode& docGraphNode)
		: CDialog(IDD_SCHEMATYC_ADD_GRAPH_NODE_OUTPUT, pParent)
		, m_pos(pos)
		, m_docGraphNode(docGraphNode)
	{}

	//////////////////////////////////////////////////////////////////////////
	CAddGraphNodeOutputDialog::~CAddGraphNodeOutputDialog() {}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddGraphNodeOutputDialog::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		m_docGraphNode.EnumerateOptionalOutputs(MAKE_MEMBER_DELEGATE(CAddGraphNodeOutputDialog::EnumerateOptionalOutput, *this));
		for(TOutputVector::const_iterator iOutput = m_outputs.begin(), iEndOutput = m_outputs.end(); iOutput != iEndOutput; ++ iOutput)
		{
			m_outputsCtrl.AddString(iOutput->fullName.c_str());
		}
		m_outputsCtrl.SetCurSel(0);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddGraphNodeOutputDialog::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_GRAPH_NODE_OUTPUTS, m_outputsCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddGraphNodeOutputDialog::OnOK()
	{
		const SOutput*	pOutput = GetSelection();
		if(pOutput != NULL)
		{
			m_docGraphNode.AddOptionalOutput(pOutput->name.c_str(), pOutput->typeId, pOutput->flags);
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	CAddGraphNodeOutputDialog::SOutput::SOutput(const char* _name, const char* _fullName, const CTypeId& _typeId, DocGraphNodePortFlags::EValue _flags)
		: name(_name)
		, fullName(_fullName)
		, typeId(_typeId)
		, flags(_flags)
	{}

	//////////////////////////////////////////////////////////////////////////
	void CAddGraphNodeOutputDialog::EnumerateOptionalOutput(const char* name, const char* fullName, const CTypeId& typeId, DocGraphNodePortFlags::EValue flags)
	{
		m_outputs.push_back(SOutput(name, fullName, typeId, flags));
	}

	//////////////////////////////////////////////////////////////////////////
	const CAddGraphNodeOutputDialog::SOutput* CAddGraphNodeOutputDialog::GetSelection() const
	{
		const int	curSel = m_outputsCtrl.GetCurSel();
		return (curSel != LB_ERR) && (curSel >= 0) ? &m_outputs[curSel] : NULL;
	}
}