/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
Description:  This is the header file for importing background objects into Mannequin.
              The recomended way to call this dialog is through DoModal() method.

-------------------------------------------------------------------------
History:
- 13:01:2014   11:05: Created by Tom Berry
*************************************************************************/

#ifndef MannImportBackgroundDialogDialog_h__
#define MannImportBackgroundDialogDialog_h__

class CMannImportBackgroundDialog : public CDialog
{
	//////////////////////////////////////////////////////////////////////////
	// Methods
	public:
		CMannImportBackgroundDialog(std::vector<CString> &loadedObjects);

		void DoDataExchange(CDataExchange* pDX);
		BOOL OnInitDialog();
		void OnOK();

		int GetCurrentRoot() const
		{
			return m_selectedRoot;
		}

	protected:
		const std::vector<CString> &m_loadedObjects;
		CComboBox m_comboRoot;
		int m_selectedRoot;
};

#endif // StringInputDialog_h__
