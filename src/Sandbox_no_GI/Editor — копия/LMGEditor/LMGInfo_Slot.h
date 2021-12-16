#ifndef __LMG_Info__Slot__h__
#define __LMG_Info__Slot__h__

class CLMGInfo_Slot
{
public:
	CLMGInfo_Slot();
	virtual ~CLMGInfo_Slot();

	const CString& GetId() const;
	const CString& GetName() const;
	const CString& GetDescription() const;
	const CString& GetPosition() const;

	bool ParseFromXmlNode( XmlNodeRef node );

private:
	CString m_id;
	CString m_name;
	CString m_description;
	CString m_position;
};

#endif