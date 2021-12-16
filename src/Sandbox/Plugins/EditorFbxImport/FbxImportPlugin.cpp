// Copyright (c) 1999-2014 Crytek.

#include "StdAfx.h"
#include "FbxImportPlugin.h"
#include "FbxImportDialog.h"
#include <ICryPak.h>
#include <QtViewPane.h>
#include <QFileInfo>
#include <QPixmap>

// Pull in compiled sources
#include "rcc_Resources.h"

CFbxImportPlugin *CFbxImportPlugin::s_pInstance;

CFbxImportPlugin::CFbxImportPlugin(IEditor* pEditor) : m_pEditor(pEditor)
{
	s_pInstance = this;

	QString gameFolder = QtUtil::ToQString(PathUtil::GetGameFolder());
	QFileInfo info(gameFolder);
	if (info.isAbsolute())
	{
		// Use path as-is
		m_gameFolder = gameFolder;
	}
	else
	{
		// Resolve absolute path
		m_gameFolder = info.absoluteFilePath();
	}

	// Load icons
	m_icons[eNC_Skeleton] = QPixmap(QStringLiteral(":/bone.png"));
	m_icons[eNC_Geometry] = QPixmap(QStringLiteral(":/geom.png"));
	m_icons[eNC_Animation] = QPixmap(QStringLiteral(":/anim.png"));
	m_icons[eNC_Material] = QPixmap(QStringLiteral(":/mtl.png"));

	// Init done
	QString toolName = QObject::tr("Import geometry from FBX");
	m_translatedToolName = QtUtil::ToString(toolName); // Note: RegisterQtViewPane doesn't deep-copy the string, so we need to keep the buffer
	RegisterQtViewPane<CFbxImportDialog>(pEditor, m_translatedToolName.c_str(), "Tools");
}

void CFbxImportPlugin::Release()
{
	// Clean up
	UnregisterQtViewPane<CFbxImportDialog>();
}