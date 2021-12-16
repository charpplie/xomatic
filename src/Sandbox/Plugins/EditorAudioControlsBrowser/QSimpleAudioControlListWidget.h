// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once
#include <QTreeWidget.h>
#include "AudioControl.h"
#include "ATLControlsModel.h"
#include "QtUtil.h"
#include "AudioControlMimeData.h"
#include "common/IAudioSystemEditor.h"

class QSimpleAudioControlListWidget : public QTreeWidget
{
	Q_OBJECT
public:
	QSimpleAudioControlListWidget(QWidget* parent = 0);
	void SetModel(AudioControls::IAudioSystemEditor* pAudioSystemImpl);
	void Refresh(bool reload = true);

	void UpdateControl(const AudioControls::IAudioSystemControl& control);

	//
	QTreeWidgetItem* GetItem(AudioControls::CID id);
	EItemType GetItemType(QTreeWidgetItem* item);
	AudioControls::TImplControlType GetControlType(QTreeWidgetItem* item);
	AudioControls::CID GetItemId(QTreeWidgetItem* item);
	std::vector<AudioControls::CID> GetSelectedIds();
	bool IsConnected(QTreeWidgetItem* item);

private:
	void ClearControls();
	void LoadControls();
	QTreeWidgetItem* InsertControl(AudioControls::IAudioSystemControl* pControl, QTreeWidgetItem* pRoot);

	void InitItem(QTreeWidgetItem* pItem);
	void InitItemData(QTreeWidgetItem* pItem, EItemType type, AudioControls::CID id = 0);

	AudioControls::IAudioSystemControl* GetControlFromId(AudioControls::CID id);

private:
	// Model
	AudioControls::IAudioSystemEditor* m_pAudioSystemImpl;

	// Icons and colours
	QColor m_connectedColor;
	QColor m_disconnectedColor;
	QColor m_localisedColor;
};