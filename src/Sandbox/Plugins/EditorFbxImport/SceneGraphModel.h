// Copyright (c) 1999-2014 Crytek.

#pragma once
#include "SceneGraph.h"
#include <QAbstractItemModel>

// Describes options on how the scene is viewed in the UI
struct SSceneViewOptions
{
	// The index of the column in which the checkbox is shown
	int checkboxColumn;

	// The index of the column in which the icon is shown
	int iconColumn;

	// For each type, whether or not that type of item should be hidden
	bool bHideType[eNC_COUNT];

	// For each type, whether or not that type of item should be checkable
	bool bCheckableType[eNC_COUNT];

	// If set, non-root bones are hidden in the UI
	// This setting can be changed at runtime
	bool bHideNonRootBones;
};

// Wrap CSceneGraph for display in Qt
class CSceneGraphModel : public QAbstractItemModel
{
	// The node properties exposed by the model
	// This is also the order in which these properties are displayed (left to right)
	// Note: If you expand this, also update NodePropertyToString below
	enum ENodeProperty
	{
		eNP_Name,
		eNP_Type,
		eNP_Description,
		eNP_COUNT,
	};

	// Compute the view values for the current options
	void ComputeViewValues(bool bInit);

public:
	// Construct the model
	CSceneGraphModel(QObject *pParent = nullptr);

	// Swap scene implementations
	void SwapScene(CSceneGraph &other)
	{ 
		CSceneGraph::Swap(m_scene, other);
		ComputeViewValues(true);
	}

	// Get the current scene, or 0 if not loaded
	const CSceneGraph *GetScene() const
	{
		return m_scene.GetRootNode() ? &m_scene : nullptr;
	}

	// Get the SSceneNode instance matching a Qt index
	const SSceneNode *GetNode(const QModelIndex &idx) const
	{
		return idx.isValid() ? (const SSceneNode *)idx.internalPointer() : nullptr;
	}

	// Check if the FBX file was loaded successfully
	bool IsLoaded() const
	{ 
		return m_scene.GetRootNode() != nullptr;
	}

	// Adds the currently selected items to the list
	void GetSelectedItems(QList<const SSceneNode *> &list) const;

	// Begin changing options
	// Call this before calling several display options to delay re-layout until EndChangeOptions() is called
	void BeginChangeOptions();
	void EndChangeOptions();

	// Set if non-root bones should be hidden in the UI
	// Note: This is a display option
	void HideBones(bool bHide);

	// Set the specified options for a node type
	// Note: This is a display option
	void SetTypeOptions(ENodeClass type, bool bVisible, bool bCheckable);

	// Set the index of the column for display of icons, or -1 to disable
	// Note: This is a display option
	void SetIconColumnIndex(int index);

	// Set the index of the column for display of checkbox, or -1 to disable
	// Note: This is a display option
	void SetCheckboxColumnIndex(int index);

	// Get the number of columns in the model
	static int GetColumnCount()
	{ 
		return eNP_COUNT;
	}

private:
	// Get shared icon for a given node type
	static QPixmap GetIconForType(ENodeClass type);

	// Get the printable type of a node property
	static QString NodePropertyToString(ENodeProperty property);
	
	// Get a property of a node
	QVariant GetNodeProperty(const SSceneNode *pNode, ENodeProperty property) const;

	// Test if the specified node should have a checkbox
	bool HasCheckbox(const SSceneNode *pNode) const;

	// Test if any parent has a check-box checked
	static bool HasCheckedParent(const SSceneNode *pNode);

	// Get Qt index for an item
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;

	// Get parent of an Qt index
	// Note: The SceneGraph's root node can not be accessed from Qt
	QModelIndex parent(const QModelIndex &index) const override;

	// Get row count (= number of children) of an Qt index
	int rowCount(const QModelIndex &index) const override;

	// Get column count (= number of data items) of an Qt index
	int columnCount(const QModelIndex &index) const override { return eNP_COUNT; }

	// Retrieve flags for a node
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	// Retrieve some data for a node
	QVariant data(const QModelIndex &index, int role) const override;

	// Modify UI checked flag
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	// Retrieve header data for the model
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	// Called when an option has changed so layouting is required
	void RequireLayout();

	// No copy/assign
	CSceneGraphModel(const CSceneGraphModel &); 
	void operator =(const CSceneGraphModel &);

	// The loaded scene
	CSceneGraph m_scene;
	
	// The view options
	SSceneViewOptions m_options;

	// If set, layout is being suppressed
	bool m_bSuppressingLayout;

	// If set, layout should be performed after suppression ends
	bool m_bShouldLayout;

	// If non-zero, we are nesting calls to setData
	int m_nesting;
};