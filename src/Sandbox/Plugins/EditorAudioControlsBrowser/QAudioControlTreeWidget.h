////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   QAudioControlTreeWidget.h
//  Version:     v1.00
//  Created:     04/06/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <QTreeWidget.h>
#include "QtUtil.h"
#include "AudioControlMimeData.h"
#include "ATLControlsModel.h"

namespace AudioControls
{
	class CATLControl;
	class CATLControlsModel;
}

class QAudioControlTreeWidget : public QTreeWidget, public AudioControls::IATLControlModelListener
{
	Q_OBJECT
public:
	QAudioControlTreeWidget(QWidget* parent = 0);
	~QAudioControlTreeWidget();
	void SetModel(AudioControls::CATLControlsModel* pModel);
	void Refresh(bool reload = true);

	QTreeWidgetItem* InsertControl(AudioControls::CATLControl* pControl);
	QTreeWidgetItem* UpdateControl(AudioControls::CATLControl* pControl);
	void RemoveControl(AudioControls::CATLControl* pControl);
	QTreeWidgetItem* AddFolder(string name, QTreeWidgetItem* pParent);

	string GenerateUniqueName(const string& nameRoot, QTreeWidgetItem* pParent);

	//
	QTreeWidgetItem* GetItem(AudioControls::CID id);
	EItemType GetItemType(QTreeWidgetItem* item);
	AudioControls::EACBControlType GetControlType(QTreeWidgetItem* item);
	AudioControls::CID GetItemId(QTreeWidgetItem* item);
	std::vector<AudioControls::CID> GetSelectedIds();
	bool IsConnected(QTreeWidgetItem* item);
	void EnableEditing(bool bEnable);

protected:
	void keyPressEvent(QKeyEvent* pEvent);
	void dropEvent(QDropEvent* pEvent);
	void dragEnterEvent(QDragEnterEvent* pEvent);

	// IATLControlModelListener
	virtual void OnControlAdded(AudioControls::CATLControl* pControl);
	virtual void OnControlModified(AudioControls::CATLControl* pControl);
	virtual void OnControlRemoved(AudioControls::CATLControl* pControl);

private:
	void ClearControls();
	void LoadControls();
	QTreeWidgetItem* CreateFolderPath(const string& path, EItemType leafType, QTreeWidgetItem* pRoot);
	QTreeWidgetItem* CreateFolderPath(AudioControls::CATLControl* pControl);

	void InitItemFromControl(QTreeWidgetItem* pItem, AudioControls::CATLControl* pControl);
	void InitItemFromType(QTreeWidgetItem* pItem, const string& name, EItemType type);
	void InitItem(QTreeWidgetItem* pItem);

signals:
	void DeleteItem(QAudioControlTreeWidget* tree);
	void ParentChanged(QTreeWidgetItem* pParent);

private:
	// Model
	AudioControls::CATLControlsModel* m_pModel;

	// Icons and colours
	QColor m_connectedColor;
	QColor m_disconnectedColor;
};