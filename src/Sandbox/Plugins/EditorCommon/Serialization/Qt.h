#pragma once

#include "EditorCommonAPI.h"

class QByteArray;
class QColor;
class QPalette;
class QSplitter;
class QString;
class QTreeView;

bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QSplitter* splitter, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QByteArray& value, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QString& value, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QTreeView* treeViewState, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QPalette& palette, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QColor& color, const char* name, const char* label);
