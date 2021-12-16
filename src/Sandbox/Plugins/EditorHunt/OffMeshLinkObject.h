#ifndef __OFFMESHLINKSHAPE_H__
#define __OFFMESHLINKSHAPE_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include <Objects/ShapeObject.h>

namespace CryGame
{
	class COffMeshLinkObject : public CGameShapeObject
	{
		DECLARE_DYNCREATE(COffMeshLinkObject)

	public:
		COffMeshLinkObject();
		~COffMeshLinkObject();

		virtual void InitVariables() OVERRIDE;
		virtual void BeginEditParams(IEditor* ie, int flags) OVERRIDE;
		virtual void PostLoad(CObjectArchive& ar) OVERRIDE;

		virtual void Display(DisplayContext& dc) OVERRIDE;

		virtual void SetPoint(int index, const Vec3& pos) OVERRIDE;

	private:
		bool GetProperties(SmartScriptTable& out);

		virtual void EndCreation() OVERRIDE;

		///> Callbacks
		void OnPropertyChange(IVariable* var);

	private:
		CVariableEnum<int> m_dir;
		CVariable<bool> m_trimExcess;

		// Points
		CVariableEnum<CString> m_desc[2];
		CVariable<bool> m_enabled[2];
	};

	class COffMeshLinkObjectClassDesc : public CObjectClassDesc
	{
	public:
		REFGUID ClassID()
		{
			// NOTE: This has been generate in MSVC via "Tools->Create GUID" with option 3. "static const struct GUID = {...}"
			//       selected and choosing "Copy".

			// {CCDC7ED7-24E7-4F84-9715-A20827A84C09}
			static const GUID guid = { 0xccdc7ed7, 0x24e7, 0x4f84, { 0x97, 0x15, 0xa2, 0x8, 0x27, 0xa8, 0x4c, 0x9 } };
			return guid;
		}
		ObjectType GetObjectType() { return OBJTYPE_SHAPE; };
		const char* ClassName() { return "OffMeshLinkObject"; };
		const char* Category() { return "Hunt"; };
		CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(COffMeshLinkObject); };
		int GameCreationOrder() { return 201; };  // After most entities have been created
	};
}

#endif  // __OFFMESHLINKSHAPE_H__