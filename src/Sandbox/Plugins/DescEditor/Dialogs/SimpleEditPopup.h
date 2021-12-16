#pragma once


// CSimpleEditPopup dialog
namespace CryGame
{
	class CSimpleEditPopup : public CXTResizeDialog
	{
		DECLARE_DYNAMIC(CSimpleEditPopup)

	public:
		CSimpleEditPopup(CWnd* pParent = NULL);   // standard constructor
		virtual ~CSimpleEditPopup();

		// Dialog Data
		enum { IDD = IDD_SIMPLE_EDIT_POPUP };

		void SetEditString(string& pString) { m_editString = &pString; }
		void SetEditWidth(int nWidth) { m_nEditWidth = nWidth; }
		void SetEditHeight(int nHeight) { m_nEditHeight = nHeight; }
		void SetDialogTitle(const char* pValue) { m_title = pValue; }
		void SetDialogLabel(const char* pValue) { m_label = pValue; }

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		DECLARE_MESSAGE_MAP()
		virtual void OnOK();

	public:
		virtual BOOL OnInitDialog();

	private:
		string* m_editString;
		int m_nEditWidth;
		int m_nEditHeight;
		const char* m_title;
		const char* m_label;
	};
}
