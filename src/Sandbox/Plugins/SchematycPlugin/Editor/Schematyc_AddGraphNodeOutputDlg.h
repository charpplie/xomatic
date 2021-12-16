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

#ifndef __SCHEMATYC_ADDGRAPHNODEOUTPUTDLG_H__
#define __SCHEMATYC_ADDGRAPHNODEOUTPUTDLG_H__

namespace Schematyc
{
	class CAddGraphNodeOutputDialog : public CDialog
	{
	public:

		CAddGraphNodeOutputDialog(CWnd* pParent, CPoint pos, IDocGraphNode& docGraphNode);
		
		virtual ~CAddGraphNodeOutputDialog();

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();


	private:

		struct SOutput
		{
			SOutput(const char* _name, const char* _fullName, const CTypeId& _typeId, DocGraphNodePortFlags::EValue _flags);

			string												name;
			string												fullName;
			CTypeId												typeId;
			DocGraphNodePortFlags::EValue	flags;
		};

		typedef std::vector<SOutput> TOutputVector;

		void EnumerateOptionalOutput(const char* name, const char* fullName, const CTypeId& typeId, DocGraphNodePortFlags::EValue flags);
		const SOutput* GetSelection() const;

		CPoint					m_pos;
		IDocGraphNode&	m_docGraphNode;
		TOutputVector		m_outputs;
		CListBox				m_outputsCtrl;
	};
}

#endif __SCHEMATYC_ADDGRAPHNODEOUTPUTDLG_H__