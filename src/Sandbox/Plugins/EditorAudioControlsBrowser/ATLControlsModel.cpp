// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "ATLControlsModel.h"
#include <StringUtils.h>
#include "AudioLibrary.h"
#include "AudioControlsBrowserUndo.h"
#include <IEditor.h>

namespace AudioControls
{
	CID CATLControlsModel::m_nextId = 1;

	CATLControlsModel::CATLControlsModel() : m_bSuppressMessages(false) {}

	CATLControlsModel::~CATLControlsModel()
	{
		Clear();
	}

	CATLControl* CATLControlsModel::CreateControlInLibrary(const string& controlName, EACBControlType type, const string& libraryName, const string& libraryPath, const string& virtualPath)
	{
		CATLControl* pControl = new CATLControl(controlName, GenerateUniqueId(), type);
		m_controls.push_back(std::unique_ptr<CATLControl>(pControl));

		if (!CUndo::IsSuspended())
		{
			CUndo undo("Audio Control Created");
			CUndo::Record(new CUndoControlAdd(pControl->GetId()));
		}

		pControl->SetFilepath(libraryPath);
		CAudioLibrary* pLibrary = AddLibrary(libraryName);
		if (pLibrary)
		{
			pControl->SetLibrary(pLibrary);
		}
		pControl->SetVirtualPath(virtualPath);

		OnControlAdded(pControl);

		return pControl;
	}

	void CATLControlsModel::RemoveControl(CID id)
	{
		if (id >= 0)
		{
			size_t size = m_controls.size();
			for (auto it = m_controls.begin(); it != m_controls.end(); ++it)
			{
				std::unique_ptr<CATLControl>& pControl = *it;
				if (pControl && pControl->GetId() == id)
				{
					OnControlRemoved(pControl.get());

					CAudioLibrary* pLibrary = pControl->GetLibrary();
					if (pLibrary)
					{
						pLibrary->RemoveControl(pControl.get());
					}

					if (!CUndo::IsSuspended())
					{
						CUndo::Record(new CUndoControlRemove(pControl));
					}

					m_controls.erase(it, it + 1);

					break;
				}
			}
		}
	}

	CATLControl* CATLControlsModel::GetControlByID(CID id) const
	{
		if (id >= 0)
		{
			size_t size = m_controls.size();
			for (size_t i = 0; i < size; ++i)
			{
				if (m_controls[i]->GetId() == id)
				{
					return m_controls[i].get();
				}
			}
		}
		return nullptr;
	}

	CATLControl* CATLControlsModel::GetControlByName(const string& name) const
	{
		size_t size = m_controls.size();
		for (size_t i = 0; i < size; ++i)
		{
			if (m_controls[i]->GetName() == name)
			{
				return m_controls[i].get();
			}
		}
		return nullptr;
	}

	CATLControl* CATLControlsModel::GetControlByIndex(unsigned int index)  const
	{
		if (index < m_controls.size())
		{
			return m_controls[index].get();
		}
		return nullptr;
	}

	int CATLControlsModel::ControlCount() const
	{
		return m_controls.size();
	}

	bool CATLControlsModel::IsNameValid(const string& name, EACBControlType type, const string& scope, const string& path) const
	{
		string newSwitchName = "";
		if (type == eACBT_SWITCH)
		{
			newSwitchName = path;
			string::size_type pos = path.find_last_of("/");
			if (pos != string::npos)
			{
				newSwitchName = path.substr(pos);
			}
		}

		const size_t size = m_controls.size();
		for (size_t i = 0; i < size; ++i)
		{
			if ((m_controls[i]->GetType() == type && (m_controls[i]->GetName().compareNoCase(name) == 0)) &&
			    (m_controls[i]->GetScope() == "" || m_controls[i]->GetScope() == scope))
			{
				if (type == eACBT_SWITCH)
				{
					string controlPath = m_controls[i]->GetVirtualPath();
					string::size_type controlPos = controlPath.find_last_of("/");

					string switchName = controlPath;
					if (controlPos != string::npos)
					{
						switchName = controlPath.substr(controlPos);
					}

					if (newSwitchName != switchName)
					{
						// if the switch name is different then controls
						// can have the same name
						continue;
					}
				}
				return false;
			}
		}
		return true;
	}

	string CATLControlsModel::GenerateUniqueName(const string& root, EACBControlType type, const string& scope, const string& path) const
	{
		string finalName = root;
		int number = 1;
		while (!IsNameValid(finalName, type, scope, path))
		{
			finalName = root + "_" + CryStringUtils::toString(number);
			++number;
		}

		return finalName;
	}

	CAudioLibrary* CATLControlsModel::AddLibrary(const string& name)
	{
		size_t size = m_libraries.size();
		for (int i = 0; i < size; ++i)
		{
			if (m_libraries[i]->GetName() == name)
			{
				return m_libraries[i];
			}
		}

		CAudioLibrary* pLibrary = new CAudioLibrary(name);
		m_libraries.push_back(pLibrary);
		return pLibrary;
	}

	int CATLControlsModel::GetLibraryCount() const
	{
		return m_libraries.size();
	}

	CAudioLibrary* CATLControlsModel::GetLibrary(int index)
	{
		if (index < m_libraries.size())
		{
			return m_libraries[index];
		}
		return NULL;
	}

	void CATLControlsModel::AddScope(const string& name, bool bLocalOnly)
	{
		string scopeName = name;
		scopeName.MakeLower();
		const size_t size = m_scopes.size();
		for (int i = 0; i < size; ++i)
		{
			if (m_scopes[i].name == scopeName)
			{
				return;
			}
		}
		m_scopes.push_back(SControlScope(scopeName, bLocalOnly));
	}

	void CATLControlsModel::ClearScopes()
	{
		m_scopes.clear();
	}

	bool CATLControlsModel::ScopeExists(const string& name) const
	{
		string scopeName = name;
		scopeName.MakeLower();
		const size_t size = m_scopes.size();
		for (int i = 0; i < size; ++i)
		{
			if (m_scopes[i].name == name)
			{
				return true;
			}
		}
		return false;
	}

	int CATLControlsModel::GetScopeCount() const
	{
		return m_scopes.size();
	}

	SControlScope CATLControlsModel::GetScopeAt(int index) const
	{
		if (index < m_scopes.size())
		{
			return m_scopes[index];
		}
		return SControlScope();
	}

	string CATLControlsModel::GetPlatformAt(uint index)
	{
		if (index < m_platforms.size())
		{
			return m_platforms[index];
		}
		return "";
	}

	uint CATLControlsModel::GetPlatformCount()
	{
		return m_platforms.size();
	}

	void CATLControlsModel::AddPlatform(const string& name)
	{
		string lowercaseName = CryStringUtils::toLower(name);
		if (std::find(m_platforms.begin(), m_platforms.end(), lowercaseName) == m_platforms.end())
		{
			m_platforms.push_back(lowercaseName);
		}
	}

	void CATLControlsModel::Clear()
	{
		m_controls.clear();

		size_t size = m_libraries.size();
		for (size_t i = 0; i < size; ++i)
		{
			SAFE_DELETE(m_libraries[i]);
		}
		m_libraries.clear();

		m_scopes.clear();
		m_platforms.clear();
		m_connectionGroups.clear();
	}

	void CATLControlsModel::AddConnectionGroup(const string& name)
	{
		if (std::find(m_connectionGroups.begin(), m_connectionGroups.end(), name) == m_connectionGroups.end())
		{
			m_connectionGroups.push_back(name);
		}
	}

	int CATLControlsModel::GetConnectionGroupId(const string& name)
	{
		size_t size = m_connectionGroups.size();
		for (int i = 0; i < size; ++i)
		{
			if (m_connectionGroups[i].compare(name) == 0)
			{
				return i;
			}
		}
		return -1;
	}

	int CATLControlsModel::GetConnectionGroupCount() const
	{
		return m_connectionGroups.size();
	}

	string CATLControlsModel::GetConnectionGroupAt(int index) const
	{
		if (index < m_connectionGroups.size())
		{
			return m_connectionGroups[index];
		}
		return "";
	}

	void CATLControlsModel::AddListener(IATLControlModelListener* pListener)
	{
		stl::push_back_unique(m_listeners, pListener);
	}

	void CATLControlsModel::RemoveListener(IATLControlModelListener* pListener)
	{
		stl::find_and_erase(m_listeners, pListener);
	}

	void CATLControlsModel::OnControlAdded(CATLControl* pControl)
	{
		if (!m_bSuppressMessages)
		{
			for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
			{
				(*iter)->OnControlAdded(pControl);
			}
		}
	}

	void CATLControlsModel::OnControlRemoved(CATLControl* pControl)
	{
		if (!m_bSuppressMessages)
		{
			for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
			{
				(*iter)->OnControlRemoved(pControl);
			}
		}
	}

	void CATLControlsModel::OnControlModified(CATLControl* pControl)
	{
		if (!m_bSuppressMessages)
		{
			for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
			{
				(*iter)->OnControlModified(pControl);
			}
		}
	}

	void CATLControlsModel::SetSuppressMessages(bool bSuppressMessages)
	{
		m_bSuppressMessages = bSuppressMessages;
	}
}