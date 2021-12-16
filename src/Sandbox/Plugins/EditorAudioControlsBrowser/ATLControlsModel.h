////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   AudioControlsModel.h
//  Created:     11/04/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "CryString.h"
#include "AudioControl.h"
#include "common/IAudioConnection.h"

namespace AudioControls
{
	// available levels where the controls can be stored
	struct SControlScope
	{
		SControlScope() {}
		SControlScope(const string& _name, bool _bOnlyLocal) : name(_name), bOnlyLocal(_bOnlyLocal) {}
		string name;

		// if true, there is a level in the game audio
		// data that doesn't exist in the global list
		// of levels for your project
		bool bOnlyLocal;
	};

	struct IATLControlModelListener
	{
		virtual void OnControlAdded(CATLControl* pControl) {}
		virtual void OnControlModified(CATLControl* pControl) {}
		virtual void OnControlRemoved(CATLControl* pControl) {}
	};

	class CATLControlsModel
	{
		friend class CAudioControlsLoader;
		friend class IUndoControlOperation;
		friend class CUndoControlModified;

	public:
		CATLControlsModel();
		~CATLControlsModel();

		void Clear();
		CATLControl* CreateControlInLibrary(const string& controlName, EACBControlType type, const string& libraryName, const string& libraryPath, const string& virtualPath);
		void RemoveControl(CID id);

		int ControlCount() const;
		CATLControl* GetControlByID(CID id) const;
		CATLControl* GetControlByName(const string& name) const;
		CATLControl* GetControlByIndex(unsigned int index) const;

		// Libraries
		CAudioLibrary* AddLibrary(const string& name);
		int GetLibraryCount() const;
		CAudioLibrary* GetLibrary(int index);

		// Platforms
		string GetPlatformAt(uint index);
		void AddPlatform(const string& name);
		uint GetPlatformCount();

		// Connection Groups
		void AddConnectionGroup(const string& name);
		int GetConnectionGroupId(const string& name);
		int GetConnectionGroupCount() const;
		string GetConnectionGroupAt(int index) const;

		// Scope
		void AddScope(const string& name, bool bLocalOnly = false);
		void ClearScopes();
		int GetScopeCount() const;
		SControlScope GetScopeAt(int index) const;
		bool ScopeExists(const string& name) const;

		// Helper functions
		virtual bool IsNameValid(const string& name, EACBControlType type, const string& scope, const string& path) const;
		virtual string GenerateUniqueName(const string& root, EACBControlType type, const string& scope, const string& path) const;

		void AddListener(IATLControlModelListener* pListener);
		void RemoveListener(IATLControlModelListener* pListener);
		void SetSuppressMessages(bool bSuppressMessages);

	public:
		void OnControlAdded(CATLControl* pControl);
		void OnControlModified(CATLControl* pControl);
		void OnControlRemoved(CATLControl* pControl);

	private:
		CID GenerateUniqueId() { return m_nextId++; }

	private:
		static CID m_nextId;

		std::vector<CAudioLibrary*> m_libraries;
		std::vector<std::unique_ptr<CATLControl> > m_controls;

		std::vector<string> m_platforms;
		std::vector<SControlScope> m_scopes;
		std::vector<string> m_connectionGroups;

		std::vector<IATLControlModelListener*> m_listeners;
		bool m_bSuppressMessages;
	};
}