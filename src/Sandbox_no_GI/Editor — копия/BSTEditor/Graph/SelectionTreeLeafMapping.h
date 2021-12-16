#ifndef __SelectionTreeLeafMapping_H__
#define __SelectionTreeLeafMapping_H__

#include "Nodes/SelectionTree_BaseNode.h"
#include "Util/IXmlHistoryManager.h"

class CSelectionTree_TreeNode;
class CSelectionTreeGraph;

struct SLeafTranslation
{
	bool operator==(const SLeafTranslation& other) const { return !(*this != other); }
	bool operator!=(const SLeafTranslation& other) const { return Target != other.Target || NodeStack != other.NodeStack;	}
	bool LoadFromXml( const XmlNodeRef& xmlNode );
	bool SaveToXml( const XmlNodeRef& xmlNode );

	bool CheckNode(CSelectionTree_TreeNode* pNode, string& translatedBehavior, CSelectionTree_TreeNode** pEndNode = NULL) const;

	string Target;
	typedef std::vector<string> TNodeNameStack;
	TNodeNameStack NodeStack;

private:
	bool SLeafTranslation::CheckNodeInternal(CSelectionTree_TreeNode* pNode, int index, CSelectionTree_TreeNode** pEndNode) const;
};

class CLeafMappingEditor;

class CLeafMappingManager
	: public IXmlUndoEventHandler
{
public:
	CLeafMappingManager(CSelectionTreeGraph* pGraph);
	~CLeafMappingManager();

	void ClearAll();
	int CreateTranslation();
	void DeleteLeafTranslation(int idx);

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );

	bool CheckNode(CSelectionTree_TreeNode* pNode, string& translatedBehavior, int& translationId ) const;
	SLeafTranslation& GetTranslation(int idx);

	CLeafMappingEditor* GetEditor() const { return m_pEditor; }
private:
	CSelectionTreeGraph* m_pGraph;
	CLeafMappingEditor* m_pEditor;

	typedef std::vector<SLeafTranslation> TMappings;
	TMappings m_Mappings;
};


class CLeafMappingEditor
{
public:
	CLeafMappingEditor(CSelectionTreeGraph* pGraph);
	~CLeafMappingEditor();

	void StartEdit(CSelectionTree_TreeNode* pNode);
	void StopEdit(bool abord = false);
	void ChangeEndNode(CSelectionTree_BaseNode* pNode);

	bool IsInEditMode() const;

private:
	void CommitChanges(CSelectionTree_TreeNode* pEndNode);
	void MarkTranslation();
	void ResetMarkings();
	void LockTree();
	void UnlockTree();

private:
	CSelectionTreeGraph* m_pGraph;

	CSelectionTree_TreeNode* m_pMappingEditNode;
	CSelectionTree_TreeNode* m_pMappingEndEditNode;

	SLeafTranslation m_LeafMappingBackup;
	int m_CurrentLeafMappingIdx;
	bool m_bIsCurrentLeafMappingNew;
};


#endif //__SelectionTreeLeafMapping_H__