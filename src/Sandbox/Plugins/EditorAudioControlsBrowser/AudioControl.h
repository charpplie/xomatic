// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "CryString.h"
#include "common/IAudioConnection.h"
#include "common/ACBTypes.h"
#include <IXml.h>

namespace AudioControls
{
	class CAudioLibrary;

	extern const string g_sDefaultGroup;

	class CATLControl
	{
		friend class CAudioControlsLoader;
		friend class CAudioControlsWriter;
		friend class CUndoControlModified;

	public:
		CATLControl();
		CATLControl(const string& name, CID id, EACBControlType type);
		~CATLControl();

		// unique id for this control
		CID GetId() const;
		void SetId(CID id);

		EACBControlType GetType() const;
		void SetType(EACBControlType type);

		// Real path to the filename in disk
		string GetFilepath() const;
		void SetFilepath(const string& filepath);

		CAudioLibrary* GetLibrary() const;
		void SetLibrary(CAudioLibrary* pLibrary);

		// Virtual paths are used for grouping
		// control them within a file without/
		// having to mimic the structure in disk
		string GetVirtualPath() const;
		void SetVirtualPath(const string& path);

		string GetName() const;
		void SetName(const string& name);

		bool HasScope() const;
		string GetScope() const;
		void SetScope(const string& level);

		uint GetFlags() const;
		void SetFlags(uint flags);

		bool IsAutoLoad() const;
		void SetAutoLoad(bool bAutoLoad);

		bool IsModified() const;
		void SetModified(bool bModified);

		// Controls can group connection according to a platform
		// This is used primarily for the Preload Requests
		int GetGroupForPlatform(const string& platform) const;
		void SetGroupForPlatform(const string& platform, int connectionGroupId);

		size_t ConnectionCount();

		void AddConnection(IAudioConnection* pConnection);
		void RemoveConnection(IAudioConnection* pConnection);
		void RemoveConnectionTo(IAudioSystemControl* m_pAudioSystemControl);
		IAudioConnection* GetConnectionAt(int index);
		IAudioConnection* GetConnectionTo(CID id, const string& group = g_sDefaultGroup);
		IAudioConnection* GetConnectionTo(IAudioSystemControl* m_pAudioSystemControl, const string& group = g_sDefaultGroup);

	private:
		CID m_id;
		int m_type;
		string m_name;
		string m_path;
		string m_filepath;
		string m_scope;
		uint m_flags;
		std::map<string, int> m_groupPerPlatform;
		std::vector<IAudioConnection*> m_connectedControls;
		CAudioLibrary* m_pLibrary;
		bool m_bAutoLoad;
		bool m_bModified;

		// connection nodes unrecognized by the selected middleware (i.e. connections to other middleware implementations)
		// these need to be stored to not destroy that data if the node is re-saved to disk
		typedef std::map<string, std::vector<XmlNodeRef> > TConnectionPerGroup;
		TConnectionPerGroup m_unknownConnectionNodes;

	};
}