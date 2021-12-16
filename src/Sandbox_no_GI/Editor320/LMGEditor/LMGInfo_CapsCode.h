#ifndef __LMG_Info__Caps_Code__h__
#define __LMG_Info__Caps_Code__h__

class CLMGInfo_CapsCode
{
public:
	CLMGInfo_CapsCode();
	virtual ~CLMGInfo_CapsCode();

	const CString& GetId() const;
	const CString& GetName() const;
	const CString& GetCode() const;

	bool ParseFromXmlNode( XmlNodeRef node );

private:
	CString m_id;
	CString m_name;
	CString m_code;
};

#endif
