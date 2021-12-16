#pragma once

class TypeSymbol
{
public:
	TypeSymbol()
	{
		m_name=NULL;
		m_typeName=NULL;
		m_offset=0;
		m_length=0;
		m_parent=NULL;
		m_count=1;
	}
	~TypeSymbol()
	{
		if (m_name)
			free(m_name);
		if (m_typeName)
			free(m_typeName);
	}
	void SetName(BSTR &name)
	{
		m_name=(s8*)malloc(SysStringLen(name)+1);
		sprintf(m_name, "%ls", name);
	}
	void SetTypeName(BSTR &name)
	{
		m_typeName=(s8*)malloc(SysStringLen(name)+1);
		sprintf(m_typeName, "%ls", name);
	}
	void SetOffset(LONG offset)
	{
		m_offset=offset;
	}
	void SetLength(ULONGLONG length)
	{
		m_length=(u32)length*8;
	}
	void SetLengthInBits(ULONGLONG length)
	{
		m_length=(u32)length;
	}
	void SetCount(DWORD count)
	{
		m_count=(u32)count;
	}
	void AddChild(TypeSymbol *sym)
	{
		m_children.push_back(sym);
		sym->m_parent=this;
	}
	s8 *m_name;
	s8 *m_typeName;
	int m_offset;
	u32 m_length;
	u32 m_count;
	TypeSymbol *m_parent;
	std::vector<TypeSymbol*> m_children;
};
