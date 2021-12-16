// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "IAudioSystemEditor.h"
#include "AudioConnectionInspectorPanel_wwise.h"
#include "IAudioConnection.h"

namespace AudioControls
{
	class CAudioConnectionInspectorPanel_wwise;

	class CWwiseConnection : public IAudioConnection
	{
	public:
		CWwiseConnection()
			: fMult(1.0f)
			, fShift(0.0f)
			, fValue(0.0f)
		{ }

		virtual ~CWwiseConnection() {}

		float fMult;
		float fShift;
		float fValue;
	};

	class CAudioSystemEditor_wwise : public IAudioSystemEditor
	{
	public:
		CAudioSystemEditor_wwise();
		virtual ~CAudioSystemEditor_wwise();

		virtual IAudioSystemControl* CreateControl(const string& name, TImplControlType type);

		// Access controls
		virtual int ControlCount() const;
		virtual IAudioSystemControl* GetControlByID(CID id) const;
		virtual IAudioSystemControl* GetControlByIndex(unsigned int index) const;
		virtual EACBControlType ImplTypeToATLType(TImplControlType type) const;
		virtual TImplControlTypeMask GetCompatibleTypes(EACBControlType eATLControlType) const;

		// Connections
		virtual IAudioConnection* CreateConnectionToControl(IAudioSystemControl* pControl);
		virtual IAudioConnection* CreateConnectionFromXMLNode(XmlNodeRef pNode);
		virtual void DestroyConnection(IAudioConnection* pConnection);
		virtual void WriteConnectionToXMLNode(XmlNodeRef pNode, const IAudioConnection* pConnection, const EACBControlType eATLControlType);

		// UI
		virtual IAudioConnectionInspectorPanel* NewConnectionInspectorPanel() const;
		virtual QIcon GetTypeIcon(TImplControlType type) const;

	private:
		IAudioSystemControl* GetControlByName(TImplControlType type, const string& name, const string& path) const;
		CID GenerateUniqueId() { return m_nextId++; }

		std::vector<IAudioSystemControl*> m_controls;
		CID m_nextId;
	};
}