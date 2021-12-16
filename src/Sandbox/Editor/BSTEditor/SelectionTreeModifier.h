#ifndef __SelectionTreeModifier_H__
#define __SelectionTreeModifier_H__

#include "Util/IXmlHistoryManager.h"
#include "SelectionTreeManager.h"

class CSelectionTreeModifier
{
public:
	CSelectionTreeModifier(CSelectionTreeManager* pManager);
	~CSelectionTreeModifier();

	void ShowErrorMsg(const char* title, const char* textFormat, ...);

	enum EMsgBoxResult
	{
		eMBR_Cancel,
		eMBR_UserBtn1,
		eMBR_UserBtn2,
	};

	EMsgBoxResult ShowMsgBox(const char* title, const char* userButton1, const char* userButton2, const char* textFormat, ...);

	void CreateNewTree();
	
	void EditTree(const char* tree);
private:
	CSelectionTreeManager* m_pManager;
};

#endif // __SelectionTreeModifier_H__