#ifndef __ACTORENTITY_H__
#define __ACTORENTITY_H__

#include "../Editor/Objects/EntityObject.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Actor entity.  This is mainly just a way to let us add entities that
	// use an ActorDesc without having to drop in a lua file then type in the
	// name of the desc to use.
	//////////////////////////////////////////////////////////////////////////
	class CActorEntityObject : public CEntityObject
	{
	public:
		DECLARE_DYNCREATE(CActorEntityObject)

		//////////////////////////////////////////////////////////////////////////
		// From CEntity
		//////////////////////////////////////////////////////////////////////////
		virtual bool Init(IEditor* ie, CBaseObject* prev, const CString& file) override;
		virtual void OnEvent(ObjectEvent event) override;
		//////////////////////////////////////////////////////////////////////////
	};

	/*!
	* Class Description of Entity
	*/
	class CActorEntityObjectClassDesc : public CObjectClassDesc
	{
	public:
		REFGUID ClassID()
		{
			// {83DF841D-E595-498F-9572-54B9882BBA67}
			static const GUID guid = { 0x83df841d, 0xe595, 0x498f, { 0x95, 0x72, 0x54, 0xb9, 0x88, 0x2b, 0xba, 0x67 } };
			return guid;
		}
		virtual ObjectType GetObjectType() { return OBJTYPE_ENTITY; };
		virtual const char* ClassName() { return "ActorEntity"; };
		virtual const char* Category() { return "Hunt"; };
		virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CActorEntityObject); };
		virtual const char* GetFileSpec() { return "Data/CActorDesc/*.xml"; };
		virtual int GameCreationOrder() { return 201; };
	};
}

#endif // __ACTORENTITY_H__
