// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once
#include "QTreeWidgetFilter.h"
#include <QString>
#include "AudioControl.h"

namespace AudioControls
{
	class CATLControlsModel;
}

//
struct SNameFilter : public ITreeWidgetItemFilter
{
	SNameFilter(const QString& filter = "");
	virtual bool IsItemValid(QTreeWidgetItem* pItem);
	void SetFilter(QString filter);
	bool IsNameValid(const QString& name);

private:
	QString m_filter;
};

struct SImplNameFilter : public SNameFilter
{
	virtual bool IsItemValid(QTreeWidgetItem* pItem);
};

//
struct STypeFilter : public ITreeWidgetItemFilter
{
	STypeFilter();
	virtual bool IsItemValid(QTreeWidgetItem* pItem);
	void SetControlTypeHidden(AudioControls::EACBControlType type, bool bHidden);
	bool IsControlTypeHidden(AudioControls::EACBControlType type);

protected:
	bool m_hiddenTypes[AudioControls::EACBControlType::eACBT_NUM_TYPES];
};

struct SImplTypeFilter : public ITreeWidgetItemFilter
{
	virtual bool IsItemValid(QTreeWidgetItem* pItem);
	void SetAllowedControlsMask(uint nAllowedControlsMask) { m_nAllowedControlsMask = nAllowedControlsMask; }
	uint m_nAllowedControlsMask;
};

//
struct SHideConnectedFilter : public ITreeWidgetItemFilter
{
	SHideConnectedFilter();
	virtual bool IsItemValid(QTreeWidgetItem* pItem);
	void SetHideConnected(bool bHideConnected);

private:
	bool m_bHideConnected;
};

struct SScopeFilter : public ITreeWidgetItemFilter
{
	virtual bool IsItemValid(QTreeWidgetItem* pItem);
	void SetModel(AudioControls::CATLControlsModel* pModel) { m_pModel = pModel; }
	void SetScope(const string& sScope) { m_sScope = sScope; }
	string m_sScope;
	AudioControls::CATLControlsModel* m_pModel;
};