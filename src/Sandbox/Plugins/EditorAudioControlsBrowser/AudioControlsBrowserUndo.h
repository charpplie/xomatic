// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "AudioControl.h"
#include "Undo/IUndoObject.h"

namespace AudioControls
{
	//-----------------------------------------
	class IUndoControlOperation : public IUndoObject
	{
	protected:
		IUndoControlOperation() {}
		void AddStoredControls();
		void RemoveControls();

		CID m_id;
		string m_sLibraryName;
		std::unique_ptr<CATLControl> m_storedControl;
	};

	//-----------------------------------------
	class CUndoControlAdd : public IUndoControlOperation
	{
	public:
		CUndoControlAdd(CID id);
	protected:
		virtual int GetSize() override { return sizeof(*this); };
		virtual const char* GetDescription() override { return "Undo Control Add"; };

		virtual void Undo(bool bUndo) override;
		virtual void Redo() override;
	};

	//-----------------------------------------
	class CUndoControlRemove : public IUndoControlOperation
	{
	public:
		CUndoControlRemove(std::unique_ptr<CATLControl>& pControl);
	protected:
		virtual int GetSize() override { return sizeof(*this); };
		virtual const char* GetDescription() override { return "Undo Control Remove"; };

		virtual void Undo(bool bUndo) override;
		virtual void Redo() override;
	};

	//-----------------------------------------
	class CUndoControlModified : public IUndoObject
	{
	public:
		CUndoControlModified(CID id);
	protected:
		virtual int GetSize() override { return sizeof(*this); };
		virtual const char* GetDescription() override { return "Undo Control Changed"; };

		void SwapData();
		virtual void Undo(bool bUndo) override;
		virtual void Redo() override;

		CID m_id;
		string m_name;
		string m_scope;
		string m_filepath;
		string m_virtualPath;
		CAudioLibrary* m_library;
		bool m_bAutoLoad;
		std::map<string, int> m_groupPerPlatform;
		std::vector<IAudioConnection*> m_connectedControls;
	};
}