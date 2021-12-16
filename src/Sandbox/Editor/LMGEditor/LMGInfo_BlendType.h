#ifndef __LMG_Info__Blend_Type__h__
#define __LMG_Info__Blend_Type__h__

class CLMGInfo_BlendType
{
public:
	CLMGInfo_BlendType();
	virtual ~CLMGInfo_BlendType();

	const CString& GetCode() const;
	const CString& GetName() const;
	const CString& GetDescription() const;

	size_t GetCapabilityIdCount() const;
	const CString& GetCapabilityId( size_t i ) const;

	size_t GetSlotIdCount() const;
	const CString& GetSlotId( size_t i ) const;

	size_t GetTestIdCount() const;
	const CString& GetTestId( size_t i ) const;

	size_t GetCapsCodeCount() const;
	const CString& GetCapsCodeId( size_t i ) const;

	bool ParseFromXmlNode( XmlNodeRef node );

protected:

	bool ParseSlotIdsFromXmlNode( XmlNodeRef node );
	bool ParseCapabilityIdsFromXmlNode( XmlNodeRef node );
	bool ParseCapsCodeIdsFromXmlNode( XmlNodeRef node );
	bool ParseTestIdsFromXmlNode( XmlNodeRef node );

private:
	CString m_code;
	CString m_name;
	CString m_description;
	std::vector< CString > m_capabilityIds;
	std::vector< CString > m_slotIds;
	std::vector< CString > m_testIds;
	std::vector< CString > m_capsCodeIds;
};

#endif
