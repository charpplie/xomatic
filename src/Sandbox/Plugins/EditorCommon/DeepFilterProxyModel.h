#pragma once

#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QStringList>
#include "EditorCommonAPI.h"

class EDITOR_COMMON_API DeepFilterProxyModel : public QSortFilterProxyModel
{
public:
	DeepFilterProxyModel(QObject* parent);

	void setFilterString(const QString& filter);
	void invalidate();
	
	QVariant data(const QModelIndex& index, int role) const override;

  void setFilterWildcard(const QString &pattern);

	bool matchFilter(int source_row, const QModelIndex& source_parent) const; 
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
	bool hasAcceptedChildrenCached(int source_row, const QModelIndex& source_parent) const;
	bool hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const;

	QModelIndex findFirstMatchingIndex(const QModelIndex& root);
private:
	QString m_filter;
	QStringList m_filterParts;
	typedef std::map<std::pair<QModelIndex, int>, bool> TAcceptCache;
	mutable TAcceptCache m_acceptCache;
};
