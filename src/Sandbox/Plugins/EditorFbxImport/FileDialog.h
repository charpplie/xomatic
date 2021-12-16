// Copyright (c) 1999-2014 Crytek.

#pragma once
#include <QString>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include "FbxImportPlugin.h"
#include "RCCapabilities.h"

// Helper to run a modal file dialog
// The continuation will receive a copy of the context and the selected file (or empty, if no file was selected/dialog was canceled)
// TContext must be copy-constructible
template<typename TClass, typename TContext>
inline void RunFileDialog(
	TClass *pClass,                                             // The class instance containing the callback functions
	void (TClass::*pContinuation)(const QString &, TContext &), // The function called on the main thread after file dialog completed
	const TContext &context,                                    // The context for the call
	QWidget *pWidget,                                           // The parent widget (typically some kind of parent dialog)
	const QString &caption,                                     // The caption of the file dialog
	const QString &filter,                                      // The file filter of the dialog
	const QString &directory,                                   // The directory to look inside of
	const QString &defaultFile,                                 // The default filename (useful for save)
	QFileDialog::AcceptMode acceptMode,                         // The dialog accept mode
	QFileDialog::FileMode fileMode                              // The file accept mode
	)
{
	struct SContext : QFileDialog
	{
		// Continuation information
		struct SContinuation
		{
			TClass *pClass;
			void (TClass::*pContinuation)(const QString &, TContext &);
			TContext context;
			QString file;

			SContinuation(const TContext &context) : context(context) {}

			void Invoke()
			{
				(pClass->*pContinuation)(file, context);
			}
		} continuation;

		SContext(const TContext &context, QWidget *pWidget, const QString &caption, const QString &directory, const QString &defaultFile, const QString &filter, QFileDialog::AcceptMode accept, QFileDialog::FileMode fileMode)
			: QFileDialog(pWidget, caption, directory, filter), continuation(context)
		{
			setAcceptMode(accept);
			setFileMode(fileMode);
			selectFile(defaultFile);
			setWindowModality(Qt::WindowModal);
			setAttribute(Qt::WA_DeleteOnClose);
			connect(this, &QFileDialog::finished, this, &SContext::Finish);
		}

		void Finish(int result)
		{
			// Get the selected file
			if (result != 0)
			{
				QStringList files = selectedFiles();
				if (files.count() == 1)
				{
					continuation.file = files.at(0);
				}
			}
			continuation.Invoke();
			close();
		}
	} *pContext;

	pContext = new SContext(context, pWidget, caption, directory, defaultFile, filter, acceptMode, fileMode);
	
	// Store continuation information
	SContext::SContinuation *pBlock = &pContext->continuation;
	pBlock->pClass = pClass;
	pBlock->pContinuation = pContinuation;

	// Show the file dialog
	pContext->showNormal();
}

// Helper to run a modal file dialog to open a file
template<typename TClass, typename TContext>
inline void RunOpenFileDialog(
	TClass *pClass,                                             // The class instance containing the callback functions
	void (TClass::*pContinuation)(const QString &, TContext &), // The function called on the main thread after file dialog completed
	const TContext &context,                                    // The context for the call
	QWidget *pWidget,                                           // The parent widget (typically some kind of parent dialog)
	const QString &caption,                                     // The caption of the file dialog
	const QString &filter,                                      // The file filter of the dialog
	const QString &directory = QString()                        // The directory to look inside of
	)
{
	RunFileDialog(pClass, pContinuation, context, pWidget, caption, filter, directory, QString(), QFileDialog::AcceptOpen, QFileDialog::ExistingFile);
}

// Helper to run a modal file dialog to save a file
template<typename TClass, typename TContext>
inline void RunSaveFileDialog(
	TClass *pClass,                                             // The class instance containing the callback functions
	void (TClass::*pContinuation)(const QString &, TContext &), // The function called on the main thread after file dialog completed
	const TContext &context,                                    // The context for the call
	QWidget *pWidget,                                           // The parent widget (typically some kind of parent dialog)
	const QString &caption,                                     // The caption of the file dialog
	const QString &filter,                                      // The file filter of the dialog
	const QString &directory = QString(),                       // The directory to look inside of
	const QString &defaultFile = QString()                      // The default filename
	)
{
	RunFileDialog(pClass, pContinuation, context, pWidget, caption, filter, directory, defaultFile, QFileDialog::AcceptSave, QFileDialog::AnyFile);
}

// Helper to set up for a save file dialog
inline QString SuggestSaveFileName(const QString &fbxFile, const QString &extension)
{
	return QFileInfo(fbxFile).baseName() + QStringLiteral(".") + extension;
}
inline QString SuggestSaveFileName(const QString &fbxFile, EImportAssetType type)
{
	QString extension = GetDefaultExtension(type);
	return SuggestSaveFileName(fbxFile, extension);
}
inline QString SuggestSaveFilePath(const QString &fbxFile, const QString &extension, const QString &gameSubFolder, bool bCreateFolder)
{
	// Get the target directory given the current game folder and sub folder for the file
	QString baseDirectory = CFbxImportPlugin::GetInstance()->GetGameFolder();
	QDir targetDirectory(baseDirectory + QStringLiteral("\\") + gameSubFolder);
	if (bCreateFolder && !targetDirectory.exists())
	{
		QDir(baseDirectory).mkdir(gameSubFolder);
	}

	// Suggest a file in this directory from the FBX file path
	return targetDirectory.absoluteFilePath(SuggestSaveFileName(fbxFile, extension));
}
inline QString SuggestSaveFilePath(const QString &fbxFile, EImportAssetType type, bool bCreateFolder)
{
	QString extension = GetDefaultExtension(type);
	QString subFolder = GetDefaultSubfolder(type);
	return SuggestSaveFilePath(fbxFile, extension, subFolder, bCreateFolder);
}