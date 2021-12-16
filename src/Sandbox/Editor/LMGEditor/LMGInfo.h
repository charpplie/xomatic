#ifndef __LMG_Info__h__
#define __LMG_Info__h__

#include "..\LMGEditor/LMGInfo_BlendType.h"
#include "..\LMGEditor/LMGInfo_Slot.h"
#include "..\LMGEditor/LMGInfo_CapsCode.h"
#include "..\LMGEditor/LMGInfo_Test.h"

class CLMGInfo
{
public:
	CLMGInfo();
	virtual ~CLMGInfo();

	size_t GetBlendTypeCount() const;
	CLMGInfo_BlendType* GetBlendType( size_t index );
	size_t GetBlendTypeIndexByCode( const CString& code ) const;
	CLMGInfo_BlendType* GetBlendTypeByCode( const CString& code );

	CLMGInfo_Slot* GetSlotById( const CString& id );

	CLMGInfo_CapsCode* GetCapsCodeById( const CString& id );
	CLMGInfo_CapsCode* GetCapsCodeByCode( const CString& code );

	CLMGInfo_Test* GetTestById( const CString& id );

protected:
	void InsertBlendType( CLMGInfo_BlendType* pBlendType );
	void InsertSlot( CLMGInfo_Slot* pSlot );
	void InsertTest( CLMGInfo_Test* pTest );
	void InsertCapsCode( CLMGInfo_CapsCode* pCapsCode );

	void ParseSlotsXml( XmlNodeRef node );
	void ParseBlendTypesXml( XmlNodeRef node );
	void ParseTestsXml( XmlNodeRef node );
	void ParseCapsCodesXml( XmlNodeRef node );

private:
	std::map< CString, size_t > m_blendTypeCodeToIndex;
	std::vector< CLMGInfo_BlendType* > m_blendTypes;

	std::map< CString, size_t > m_slotIdToIndex;
	std::vector< CLMGInfo_Slot* > m_slots;

	std::map< CString, size_t > m_capsCodeIdToIndex;
	std::vector< CLMGInfo_CapsCode* > m_capsCodes;

	std::map< CString, size_t > m_testIdToIndex;
	std::vector< CLMGInfo_Test* > m_tests;
};

#endif