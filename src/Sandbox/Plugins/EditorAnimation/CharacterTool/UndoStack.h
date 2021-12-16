#pragma once

#include <vector>
#include "Strings.h"

namespace CharacterTool
{

using std::vector;

struct SUndoState
{
	string description;
	vector<char> state;
	unsigned long long sequentialIndex;
};

class CUndoStack
{
public:
	void PushUndo(vector<char>* previousContentToMove, const char* description, unsigned long long sequentialIndex);
	bool Undo(vector<char>* newState, const vector<char>& currentState, int count, unsigned long long sequentialIndex);
	bool HasUndo() const { return !m_undos.empty(); }
	bool Redo(vector<char>* newState, const vector<char>& currentState, unsigned long long sequentialIndex);
	bool HasRedo() const{ return !m_redos.empty(); }
	unsigned long long NewestUndoIndex() const { return m_undos.empty() ? 0 : m_undos.back().sequentialIndex; }
	unsigned long long NewestRedoIndex() const { return m_redos.empty() ? 0 : m_redos.back().sequentialIndex; }

	void GetUndoActions(vector<string>* actionNames, int maxActionCount);
private:
	vector<SUndoState> m_undos;
	vector<SUndoState> m_redos;
};

}
