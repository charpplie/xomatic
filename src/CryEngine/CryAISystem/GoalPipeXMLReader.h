#ifndef __GoalPipeXMLReader_H__
#define __GoalPipeXMLReader_H__


class CGoalPipeXMLReader
{
public:
	CGoalPipeXMLReader(const char* szFileName);
	
	bool Success() { return m_bSuccess; }
	
private:
	void ParseGoalPipes(const XmlNodeRef& root);
	void ParseGoalOps(IGoalPipe* pGoalPipe, const XmlNodeRef& root,
		string sIfLabel = string(), bool b_IfElseEnd_Halves_ReverseOrder = true);
	void ParseGoalOp(IGoalPipe* pGoalPipe, const XmlNodeRef& goalOpNode);

	string GetLabel(int i);

private:
	class CXMLAttrReader
	{
		typedef std::pair<const char*, int> TRecord;
		std::vector<TRecord> m_dictionary;

	public:
		void Add(const char* szKey, int nValue) { m_dictionary.push_back(TRecord(szKey, nValue)); }
		bool Get(const char* szKey, int& nValue);
	};

	CXMLAttrReader m_dictBranchType;

	int m_goalopCounter;
	
	IGoalPipe::EGroupType m_currentGrouping;

	bool m_bSuccess;
};


#endif	// #ifndef __GoalPipeXMLReader_H__
