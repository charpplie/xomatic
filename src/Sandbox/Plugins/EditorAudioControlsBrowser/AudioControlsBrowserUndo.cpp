// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "StdAfx.h"
#include "AudioControlsBrowserUndo.h"
#include "ATLControlsModel.h"
#include "AudioControlsBrowserPlugin.h"
#include "AudioLibrary.h"

namespace AudioControls
{
	void IUndoControlOperation::AddStoredControls()
	{
		CATLControlsModel* pModel = CAudioControlsBrowserPlugin::GetATLModel();
		
		if (pModel != NULL)
		{
			CATLControl* pControl = m_storedControl.release();

			if (pControl != NULL)
			{
				pModel->m_controls.push_back(std::unique_ptr<CATLControl>(pControl));

				// add to library
				CAudioLibrary* pLibrary = pModel->AddLibrary(m_sLibraryName);

				if (pLibrary != NULL)
				{
					pControl->SetLibrary(pLibrary);
				}

				pModel->OnControlAdded(pControl);
				m_id = pControl->GetId();
			}
		}
	}

	void IUndoControlOperation::RemoveControls()
	{
		CUndoSuspend suspendUndo;
		CATLControlsModel* pModel = CAudioControlsBrowserPlugin::GetATLModel();
		if (pModel)
		{
			for (auto it = pModel->m_controls.begin(); it != pModel->m_controls.end(); ++it)
			{
				std::unique_ptr<CATLControl>& pControl = *it;
				if (pControl->GetId() == m_id)
				{
					CATLControl* pStoredControl = pControl.release();
					m_storedControl = std::unique_ptr<CATLControl>(pStoredControl);

					pModel->OnControlRemoved(m_storedControl.get());

					pModel->m_controls.erase(it, it + 1);

					// remove from library
					CAudioLibrary* pLibrary = m_storedControl->GetLibrary();
					if (pLibrary)
					{
						pLibrary->RemoveControl(m_storedControl.get());
						m_sLibraryName = pLibrary->GetName();
						m_storedControl->SetLibrary(nullptr);
					}
					break;
				}
			}
		}
	}

	CUndoControlAdd::CUndoControlAdd(CID id)
	{
		m_id = id;
	}

	void CUndoControlAdd::Undo(bool bUndo)
	{
		RemoveControls();
	}

	void CUndoControlAdd::Redo()
	{
		AddStoredControls();
	}

	CUndoControlRemove::CUndoControlRemove(std::unique_ptr<CATLControl>& pControl)
	{
		CUndoSuspend suspendUndo;

		CATLControl* pStoredControl = pControl.release();
		m_storedControl = std::unique_ptr<CATLControl>(pStoredControl);
		
		// remove from library
		CAudioLibrary* pLibrary = m_storedControl->GetLibrary();
		if (pLibrary)
		{
			m_sLibraryName = pLibrary->GetName();
			m_storedControl->SetLibrary(nullptr);
		}
	}

	void CUndoControlRemove::Undo(bool bUndo)
	{
		AddStoredControls();
	}

	void CUndoControlRemove::Redo()
	{
		RemoveControls();
	}

	CUndoControlModified::CUndoControlModified(CID id) : m_id(id)
	{
		CATLControlsModel* pModel = CAudioControlsBrowserPlugin::GetATLModel();
		if (pModel)
		{
			CATLControl* pControl = pModel->GetControlByID(m_id);
			if (pControl)
			{
				m_name = pControl->GetName();
				m_scope = pControl->GetScope();
				m_filepath = pControl->GetFilepath();
				m_virtualPath = pControl->GetVirtualPath();
				m_library = pControl->GetLibrary();
				m_bAutoLoad = pControl->IsAutoLoad();
				m_groupPerPlatform = pControl->m_groupPerPlatform;
				m_connectedControls = pControl->m_connectedControls;
			}
		}
	}

	void CUndoControlModified::Undo(bool bUndo)
	{
		CUndoSuspend suspendUndo;
		SwapData();
	}

	void CUndoControlModified::Redo()
	{
		CUndoSuspend suspendUndo;
		SwapData();
	}

	void CUndoControlModified::SwapData()
	{
		CATLControlsModel* pModel = CAudioControlsBrowserPlugin::GetATLModel();
		if (pModel)
		{
			CATLControl* pControl = pModel->GetControlByID(m_id);
			if (pControl)
			{
				string name = pControl->GetName();
				string scope = pControl->GetScope();
				string filepath = pControl->GetFilepath();
				string virtualPath = pControl->GetVirtualPath();
				CAudioLibrary* library = pControl->GetLibrary();
				bool bAutoLoad = pControl->IsAutoLoad();
				std::map<string, int> groupPerPlatform = pControl->m_groupPerPlatform;
				std::vector<IAudioConnection*> connectedControls = pControl->m_connectedControls;

				pControl->SetName(m_name);
				pControl->SetScope(m_scope);
				pControl->SetFilepath(m_filepath);
				pControl->SetVirtualPath(m_virtualPath);
				pControl->SetLibrary(m_library);
				pControl->SetAutoLoad(m_bAutoLoad);
				pControl->m_groupPerPlatform = m_groupPerPlatform;
				pControl->m_connectedControls = m_connectedControls;
				pModel->OnControlModified(pControl);

				m_name = name;
				m_scope = scope;
				m_filepath = filepath;
				m_virtualPath = virtualPath;
				m_library = library;
				m_bAutoLoad = bAutoLoad;
				m_groupPerPlatform = groupPerPlatform;
				m_connectedControls = connectedControls;

			}
		}
	}
}