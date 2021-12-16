#pragma once
#include "GizmoSink.h"
#include <QObject>

class QPropertyTree;
namespace Serialization
{
class IArchive;
class CContextList;
}

namespace CharacterTool
{

struct System;

class CharacterGizmoManager : public QObject
{
	Q_OBJECT
public:
	typedef vector<const void*> SelectionHandles;
	CharacterGizmoManager(System* system);
	QPropertyTree* Tree(GizmoLayer layer);

	void SetSubselection(GizmoLayer layer, const SelectionHandles& handles);
	const SelectionHandles& Subselection(GizmoLayer layer) const;
	void ReadGizmos();

signals:
	void SignalSubselectionChanged(int layer);
	void SignalGizmoChanged();

private slots:
	void OnActiveCharacterChanged();
	void OnActiveAnimationSwitched();

	void OnTreeAboutToSerialize(Serialization::IArchive& ar);
	void OnTreeSerialized(Serialization::IArchive& ar);
	void OnExplorerEntryModified(ExplorerEntryModifyEvent& ev);
	void OnExplorerBeginRemoveEntry(ExplorerEntry* entry);
	void OnSceneChanged();

private:
	vector<unique_ptr<QPropertyTree>> m_trees;
	unique_ptr<Serialization::CContextList> m_contextList;
	vector<SelectionHandles> m_subselections;
	ExplorerEntry* m_attachedAnimationEntry;

	System* m_system;
};

}
