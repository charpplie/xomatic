////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   AudioControlsModel.cpp
//  Created:     02/07/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AudioControl.h"
#include "ATLControlsModel.h"
#include "AudioLibrary.h"
#include "AudioControlsBrowserUndo.h"
#include "IEditor.h"
#include "common/IAudioSystemControl.h"
#include "common/IAudioSystemEditor.h"
#include "AudioControlsBrowserPlugin.h"

namespace AudioControls
{

	const string g_sDefaultGroup = "";

	CATLControl::CATLControl()
		: m_name("")
		, m_id(ACB_INVALID_ID)
		, m_type(eACBT_RTPC)
		, m_flags(0)
		, m_scope("")
		, m_bAutoLoad(true)
		, m_bModified(false)
		, m_pLibrary(nullptr)
	{
	}

	CATLControl::CATLControl(const string& name, CID id, EACBControlType type)
		: m_name(name)
		, m_id(id)
		, m_type(type)
		, m_flags(0)
		, m_scope("")
		, m_bAutoLoad(true)
		, m_bModified(false)
		, m_pLibrary(nullptr)
	{
	}

	CATLControl::~CATLControl()
	{
		if (m_pLibrary)
		{
			m_pLibrary->RemoveControl(this);
		}

		IAudioSystemEditor* pAudioSystemImpl = CAudioControlsBrowserPlugin::GetAudioSystemEditorImpl();
		if (pAudioSystemImpl)
		{
			const size_t size = m_connectedControls.size();
			for (size_t i=0; i<size; ++i)
			{
				pAudioSystemImpl->DestroyConnection(m_connectedControls[i]);
			}
			m_connectedControls.clear();
		}
	}

	CID CATLControl::GetId() const
	{
		return m_id;
	}

	EACBControlType CATLControl::GetType() const
	{
		return (EACBControlType)m_type;
	}

	string CATLControl::GetName() const
	{
		return m_name;
	}

	string CATLControl::GetVirtualPath() const
	{
		return m_path;
	}

	CAudioLibrary* CATLControl::GetLibrary() const
	{
		return m_pLibrary;
	}

	string CATLControl::GetFilepath() const
	{
		return m_filepath;
	}

	uint CATLControl::GetFlags() const
	{
		return m_flags;
	}

	void CATLControl::SetFlags(uint flags)
	{
		m_flags = flags;
	}

	void CATLControl::SetId(CID id)
	{
		m_id = id;
	}

	void CATLControl::SetType(EACBControlType type)
	{
		m_type = type;
	}

	void CATLControl::SetName(const string& name)
	{
		if (name != m_name)
		{
			SetModified(true);
			m_name = name;
		}
	}

	void CATLControl::SetVirtualPath(const string& path)
	{
		if (path != m_path)
		{
			SetModified(true);
			m_path = path;
		}
	}

	void CATLControl::SetLibrary(CAudioLibrary* pLibrary)
	{
		if (pLibrary != m_pLibrary)
		{
			if (m_pLibrary)
			{
				m_pLibrary->RemoveControl(this);
			}
			m_pLibrary = pLibrary;
			if (m_pLibrary)
			{
				m_pLibrary->AddControl(this);
			}
			SetModified(true);
		}
	}

	void CATLControl::SetFilepath(const string& filepath)
	{
		if (filepath != m_filepath)
		{
			SetModified(true);
			m_filepath = filepath;
		}
	}

	string CATLControl::GetScope() const
	{
		return m_scope;
	}

	void CATLControl::SetScope(const string& level)
	{
		SetModified(true);
		m_scope = level;
	}

	bool CATLControl::HasScope() const
	{
		return !m_scope.empty();
	}

	bool CATLControl::IsAutoLoad() const
	{
		return m_bAutoLoad;
	}

	void CATLControl::SetAutoLoad(bool bAutoLoad)
	{
		if (bAutoLoad != m_bAutoLoad)
		{
			SetModified(true);
			m_bAutoLoad = bAutoLoad;
		}
	}

	bool CATLControl::IsModified() const
	{
		return m_bModified;
	}

	void CATLControl::SetModified(bool bModified)
	{
		if (!CUndo::IsSuspended())
		{
			CUndo::Record(new CUndoControlModified(GetId()));
		}

		m_bModified = bModified;
		if (bModified && m_pLibrary)
		{
			m_pLibrary->SetModified(true);
		}
	}

	int CATLControl::GetGroupForPlatform(const string& platform) const
	{
		auto it = m_groupPerPlatform.find(platform);
		if (it == m_groupPerPlatform.end())
		{
			return 0;
		}
		return it->second;
	}

	void CATLControl::SetGroupForPlatform(const string& platform, int connectionGroupId)
	{
		SetModified(true);
		m_groupPerPlatform[platform] = connectionGroupId;
	}

	size_t CATLControl::ConnectionCount()
	{
		return m_connectedControls.size();
	}

	IAudioConnection* CATLControl::GetConnectionAt(int index)
	{
		if (index < m_connectedControls.size())
		{
			return m_connectedControls[index];
		}
		return nullptr;
	}

	IAudioConnection* CATLControl::GetConnectionTo(CID id, const string& group)
	{
		if (id >= 0)
		{
			const size_t size = m_connectedControls.size();
			for (int i = 0; i < size; ++i)
			{
				if (m_connectedControls[i])
				{
					IAudioSystemControl* pAudioSystemControl = m_connectedControls[i]->GetControl();
					if (pAudioSystemControl)
					{
						if (pAudioSystemControl->GetId() == id && m_connectedControls[i]->GetGroup() == group)
						{
							return m_connectedControls[i];
						}
					}
				}
			}
		}
		return nullptr;
	}

	IAudioConnection* CATLControl::GetConnectionTo(IAudioSystemControl* m_pAudioSystemControl, const string& group /*= ""*/)
	{
		if (m_pAudioSystemControl)
		{
			auto it = m_connectedControls.begin();
			auto end = m_connectedControls.end();
			for (auto it = m_connectedControls.begin(); it != end; ++it)
			{
				if ((*it)->GetControl() == m_pAudioSystemControl)
				{
					return (*it);
				}
			}
		}
		return nullptr;
	}

	void CATLControl::AddConnection(IAudioConnection* pConnection)
	{
		if (pConnection)
		{
			SetModified(true);
			m_connectedControls.push_back(pConnection);
		}
	}

	void CATLControl::RemoveConnection(IAudioConnection* pConnection)
	{
		if (pConnection)
		{
			auto it = std::find(m_connectedControls.begin(), m_connectedControls.end(), pConnection);
			if (it != m_connectedControls.end())
			{
				IAudioSystemEditor* pAudioSystemImpl = CAudioControlsBrowserPlugin::GetAudioSystemEditorImpl();
				if (pAudioSystemImpl)
				{
					pAudioSystemImpl->DestroyConnection(*it);
					m_connectedControls.erase(it);
				}
			}
		}
	}

	void CATLControl::RemoveConnectionTo(IAudioSystemControl* m_pAudioSystemControl)
	{
		if (m_pAudioSystemControl)
		{
			auto it = m_connectedControls.begin();
			auto end = m_connectedControls.end();
			for (auto it = m_connectedControls.begin(); it != end; ++it)
			{
				if ((*it)->GetControl() == m_pAudioSystemControl)
				{
					IAudioSystemEditor* pAudioSystemImpl = CAudioControlsBrowserPlugin::GetAudioSystemEditorImpl();
					if (pAudioSystemImpl)
					{
						pAudioSystemImpl->DestroyConnection(*it);
						m_connectedControls.erase(it);
					}
				}
			}
		}
	}
}