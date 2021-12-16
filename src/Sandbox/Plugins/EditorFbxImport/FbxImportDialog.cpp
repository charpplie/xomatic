// Copyright (c) 1999-2014 Crytek.

#include "StdAfx.h"
#include "FbxImportDialog.h"
#include "FbxImportPlugin.h"
#include "SceneGraphModel.h"
#include "ResourceCompilerHelper.h"
#include "RCListener.h"
#include "FileDialog.h"
#include "OptionDialog.h"
#include "StaticMeshDialog.h"
#include <QFile>
#include <QMimeData>
#include <QTextStream>
#include <QDropEvent>
#include <QMessageBox>

// Helper for QDropEvent
static void AnalyzeDropEvent(QDropEvent *pEvent, bool &bIsFbx, QString &path)
{
	// Reset output parameters
	bIsFbx = false;
	path.clear();

	// Handle URL lists
	const QMimeData *pMime = pEvent->mimeData();
	if (pMime->hasUrls())
	{
		QList<QUrl> urls = pMime->urls();
		if (urls.size() == 1)
		{
			// If this is exactly one file
			QString fileName = urls.at(0).toLocalFile();
			QFileInfo fileInfo = QFileInfo(fileName);
			if (fileInfo.isDir()) return;
			
			// Check extension
			QString extension = fileInfo.suffix();
			bIsFbx = extension.compare(QStringLiteral("fbx"), Qt::CaseInsensitive) == 0;
			if (bIsFbx)
			{
				path = fileInfo.absoluteFilePath();
			}
		}
	}
}

// Helper for applying visibility
static void ApplyVisibility(QWidget *pWidget, bool bShown, int &numShown)
{
	if (bShown)
	{
		++numShown;
	}
	else
	{
		pWidget->setHidden(true);
	}
}

// Helper to apply default state
static void ApplyDefaultState(QRadioButton *pWidget, bool &bAnySelected, int numShown)
{
	if (!bAnySelected && !pWidget->isHidden())
	{
		pWidget->setChecked(true);
		bAnySelected = false;
	}
	if (numShown < 2)
	{
		pWidget->setEnabled(false);
	}
}

CFbxImportDialog::CFbxImportDialog(QWidget *pParent /*= nullptr*/) 
	: QMainWindow(pParent)
{
	setupUi(this);

	// These options don't make sense at this point, since RC doesn't return any of the information that would be affected by these options
	// Don't want to delete the functionality in case RC starts supporting this
	m_pViewLabel->setVisible(false);
	m_pShowAll->setVisible(false);
	m_pHideBones->setVisible(false);

	// Disable import types that are not (yet) supported
	// This way, the dialog won't show options that aren't supported
	int numShown = 0;
	ApplyVisibility(m_pTypeAnimation, IsImportSupported(eNC_Animation), numShown);
	ApplyVisibility(m_pTypeMaterial, IsImportSupported(eNC_Material), numShown);
	ApplyVisibility(m_pTypeSkeleton, IsImportSupported(eNC_Skeleton), numShown);
	ApplyVisibility(m_pTypeSkinnedMesh, IsImportSupported(eNC_Skeleton) && IsImportSupported(eNC_Geometry), numShown);
	ApplyVisibility(m_pTypeStaticMesh, IsImportSupported(eNC_Geometry), numShown);

	if (numShown == 0)
	{
		// No import option, just disable the whole dialog
		setEnabled(false);
	}
	else
	{
		// Note: Call these in order of appearance in the dialog
		bool bAnySelected = false;
		ApplyDefaultState(m_pTypeStaticMesh, bAnySelected, numShown);
		ApplyDefaultState(m_pTypeSkinnedMesh, bAnySelected, numShown);
		ApplyDefaultState(m_pTypeSkeleton, bAnySelected, numShown);
		ApplyDefaultState(m_pTypeAnimation, bAnySelected, numShown);
		ApplyDefaultState(m_pTypeMaterial, bAnySelected, numShown);
	}

	// Set the color for the background to the window background
	// Reason for this is that the checkbox widgets embedded in the tree are near invisible in dark themes otherwise
	m_pSceneTree->viewport()->setBackgroundRole(QPalette::Window);

	// Connect events
	connect(m_pSceneTree, &QTreeView::clicked, this, &CFbxImportDialog::SceneNodeSelected);
	connect(m_pImport, &QPushButton::clicked, this, &CFbxImportDialog::ImportClicked);
	connect(m_pFileInput, &QLineEdit::textChanged, this, &CFbxImportDialog::InputFileChanged);
	connect(m_pFileBrowse, &QPushButton::clicked, this, &CFbxImportDialog::BrowseFileClicked);
	connect(m_pHideBones, &QCheckBox::toggled, this, &CFbxImportDialog::ApplyViewOptions);
	connect(m_pShowAll, &QCheckBox::toggled, this, &CFbxImportDialog::ApplyViewOptions);
	connect(m_pTypeStaticMesh, &QRadioButton::toggled, this, &CFbxImportDialog::ApplyViewOptions);
	connect(m_pTypeSkinnedMesh, &QRadioButton::toggled, this, &CFbxImportDialog::ApplyViewOptions);
	connect(m_pTypeSkeleton, &QRadioButton::toggled, this, &CFbxImportDialog::ApplyViewOptions);
	connect(m_pTypeAnimation, &QRadioButton::toggled, this, &CFbxImportDialog::ApplyViewOptions);
}

void CFbxImportDialog::InputFileChanged(const QString &file)
{
	// Check if the file exists
	QFile handle(file);
	if (handle.exists())
	{
		SContextLoadFbx context = { file, nullptr };
		RunWithProgressDialog(GetIEditor(), this, context, &CFbxImportDialog::AsyncLoadFbx, &CFbxImportDialog::FinishLoadFbx, QStringLiteral("Loading FBX file ") + file, QStringLiteral("Cancel loading FBX"), this, true, false);
	}
}

bool CFbxImportDialog::AsyncLoadFbx(ITaskContext *pContext, SContextLoadFbx &job)
{
	// Load FBX with callback
	job.pModel = new(std::nothrow) CSceneGraph(QtUtil::ToString(job.file));

	// Check if load succeeded
	return job.pModel && job.pModel->GetRootNode();
}

void CFbxImportDialog::FinishLoadFbx(bool bResult, SContextLoadFbx &job)
{
	// Release old model
	m_pSceneTree->setModel(nullptr);
	m_model.reset();
	m_loadedPath.clear();
	if (bResult)
	{
		// Take the model from the job state
		m_model.reset(new CSceneGraphModel(this));
		m_model->SwapScene(*job.pModel);
		m_pSceneTree->setModel(m_model.data());
		qSwap(m_loadedPath, job.file);
		ApplyViewOptions();
	}
	else
	{
		// Show an error in bottom bar
		m_pNodePath->setText(tr("Cannot load '%1' as FBX file").arg(job.file));
	}

	// Clean up scene
	delete job.pModel;
}

void CFbxImportDialog::SceneNodeSelected(const QModelIndex &index)
{
	// Set the path to the node in the dialog
	if (m_model.isNull()) return;
	const SSceneNode *pNode = m_model->GetNode(index);

	// Displaying escaped for purpose of debugging escaping logic, should not be enabled in general
	const bool bDisplayEscaped = false;
	m_pNodePath->setText(tr("Selected node: %1").arg(pNode->GetFullPath(bDisplayEscaped)));
}

void CFbxImportDialog::ImportClicked()
{
	// If the model is not loaded, can't import
	if (m_model.isNull() || !m_model->IsLoaded())
	{
		QMessageBox::critical(this, tr("No FBX file loaded"), tr("You have not loaded an FBX file\nPlease load an XML file (see step 1) before importing"), QMessageBox::Ok);
		return;
	}

	// Get the nodes selected by the user
	QList<const SSceneNode *> nodes;
	m_model->GetSelectedItems(nodes);

	if (nodes.empty())
	{
		// Require at least one node to continue
		QMessageBox::critical(this, tr("No objects checked"), tr("You have checked no objects to import\nReview your selection of objects and try again"), QMessageBox::Ok);
		return;
	}

	// Convert to import items
	SImportRequest context;
	for (QList<const SSceneNode *>::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
	{
		const SSceneNode *pNode = *it;
		SImportItem item;
		item.pathInsideFbxToNode = pNode->GetFullPath(true);
		context.items.push_back(item);
	}

	// Import type from radio buttons
	if (m_pTypeStaticMesh->isChecked())
	{
		context.type = eIAT_StaticMesh;
	}
	else if (m_pTypeSkinnedMesh->isChecked())
	{
		context.type = eIAT_SkinnedMesh;
	}
	else if (m_pTypeSkeleton->isChecked())
	{
		context.type = eIAT_Skeleton;
	}
	else if (m_pTypeAnimation->isChecked())
	{
		context.type = eIAT_Animation;
	}
	else if (m_pTypeMaterial->isChecked())
	{
		context.type = eIAT_Material;
	}
	else
	{
		// Unknown type, shouldn't happen
		assert(false);
		return;
	}
	context.pathToFbx = m_loadedPath;
	switch (context.type)
	{
	case eIAT_StaticMesh:
		{
			// Static meshes have a proper dialog that should be used
			SStaticMeshContext staticMeshContext;
			const CSceneGraph *pScene = m_model->GetScene();
			if (pScene)
			{
				pScene->GetValidOriginNodes(staticMeshContext.origins);
				staticMeshContext.dUnitInCm = pScene->GetUnitInCm();
			}
			RunOptionDialog<CStaticMeshDialog>(this, &CFbxImportDialog::ImportToFile, context, staticMeshContext, this);
		}
		break;
	default:
		// For now, there are no actual options implemented for these types, so prompt for saving the RC target file directly (ie, CGF, ANM, CAF etc)
		// It's likely that future expansion of functionality will add more (option) dialogs and those can feature a proper "target" input box instead of this dialog
		CFbxImportPlugin *pPlugin = CFbxImportPlugin::GetInstance();
		QString directoryHint = pPlugin->GetGameFolder();
		QString extension = GetDefaultExtension(context.type);
		RunSaveFileDialog(this, &CFbxImportDialog::ImportToFile, context, this, tr("Save import result"), tr("%1 files (*.%2)").arg(extension.toUpper(), extension), directoryHint);
		break;
	}
}

// Handle import to a file after option dialog
void CFbxImportDialog::ImportToFile(const QString &targetFile, SImportRequest &context)
{
	if (targetFile.isEmpty())
	{
		// Nothing to do, dialog was cancelled
		return;
	}

	// Store the XML in a <targetFile>.xml file
	QString sourceFile = context.pathToFbx;
	QString xml = CreateImportXml(context);
	QString xmlFileName = targetFile + QStringLiteral("$import");
	QFile xmlFile(xmlFileName);
	if (xmlFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QTextStream stream(&xmlFile);
		stream << xml;
		xmlFile.close();
	}
	else
	{
		// No temporary file can be created
		QMessageBox::critical(this, tr("Cannot import FBX"), tr("Cannot write to file '%1'\nCheck that the location exists and is writable").arg(xmlFileName), QMessageBox::Ok);
		return;
	}

	// Run the RC in a background thread
	SContextRunRC ctx = { xmlFileName, sourceFile, targetFile, context.items.count() };
	RunWithProgressDialog(GetIEditor(), this, ctx, &CFbxImportDialog::AsyncRunRC, &CFbxImportDialog::FinishRunRC, tr("Importing to %1...").arg(targetFile), tr("Abort FBX import"), this, false, false);
}

// Run RC asynchronously
bool CFbxImportDialog::AsyncRunRC(ITaskContext *pContext, SContextRunRC &job)
{
	const char *pOptions = "/refresh /overwriteextension=fbx";
	const CResourceCompilerHelper::ERcExePath path = CResourceCompilerHelper::eRcExePath_currentFolder;
	CRcListener listener(IResourceCompilerListener::MessageSeverity_Warning);
	gEnv->pSystem->GetILog()->LogAlways(LOG_RC_FORMAT "%S", "Running RC conversion for ", &*job.xmlFileName.cbegin());
	bool bSuccess = CResourceCompilerHelper::CallResourceCompiler(job.xmlFileName.toUtf8(), pOptions, &listener, false, path, true, true) == CResourceCompilerHelper::eRcCallResult_success;
	return bSuccess;
}

// After running RC finished
void CFbxImportDialog::FinishRunRC(bool bSuccess, SContextRunRC &job)
{
	// Delete the XML file after processing
	QFile::remove(job.xmlFileName);

	// Show a dialog with information on the result
	if (bSuccess)
	{
		QString message = tr("Successfully imported %1 object(s)\nfrom '%2'\nto '%3'").arg(QString::number(job.numObjects), job.sourceFile, job.targetFile);
		m_pNodePath->setText(message);
	}
	else
	{
		QMessageBox::critical(this, tr("FBX import failed"), tr("Failed to import from '%1'\nTry different options or items and try again").arg(job.sourceFile), QMessageBox::Ok);
	}
}

void CFbxImportDialog::BrowseFileClicked()
{
	SContextSelectFile context;
	RunOpenFileDialog(this, &CFbxImportDialog::SelectInputFile, context, this, tr("Open FBX file to import"), tr("FBX files (*.fbx)"));
}

void CFbxImportDialog::SelectInputFile(const QString &fbxFile, SContextSelectFile &context)
{
	m_pFileInput->setText(fbxFile);
}

void CFbxImportDialog::dragEnterEvent(QDragEnterEvent *pEvent)
{
	bool bIsFbx;
	QString path;
	AnalyzeDropEvent(pEvent, bIsFbx, path);
	if (bIsFbx)
	{
		// Accept FBX file or folder
		pEvent->acceptProposedAction();
	}
	else
	{
		pEvent->ignore();
	}
}

void CFbxImportDialog::dropEvent(QDropEvent *pEvent)
{
	bool bIsFbx;
	QString path;
	AnalyzeDropEvent(pEvent, bIsFbx, path);
	if (bIsFbx)
	{
		// Set FBX file
		m_pFileInput->setText(path);
		pEvent->acceptProposedAction();
	}
	else
	{
		pEvent->ignore();
	}
}

void CFbxImportDialog::ApplyViewOptions()
{
	if (m_model.isNull() || !m_model->IsLoaded()) return;

	m_model->BeginChangeOptions();
	m_model->HideBones(m_pHideBones->isChecked());
	bool bShowAll = m_pShowAll->isChecked();
	ENodeClass nodeClass = eNC_Unknown;
	if (m_pTypeStaticMesh->isChecked() || m_pTypeSkinnedMesh->isChecked())
	{
		nodeClass = eNC_Geometry;
	}
	else if (m_pTypeSkeleton->isChecked())
	{
		nodeClass = eNC_Skeleton;
	}
	else if (m_pTypeAnimation->isChecked())
	{
		nodeClass = eNC_Animation;
	}
	else if  (m_pTypeMaterial->isChecked())
	{
		nodeClass = eNC_Material;
	}
	for (int i = 0; i < eNC_COUNT; ++i)
	{
		// Apply display options for all types
		ENodeClass type = (ENodeClass)i;
		bool bIsCompatibleClass = type == nodeClass;
		bool bSupported = IsImportSupported(type) && bIsCompatibleClass;
		bool bCheckDummy = CanSelectDummyNode(nodeClass) && (type == eNC_None);
		m_model->SetTypeOptions(type, bShowAll || bIsCompatibleClass, bSupported || bCheckDummy);
	}
	m_model->EndChangeOptions();

	// Disable the checkbox if it doesn't make sense to use it
	bool bHideBonesEnabled = bShowAll || m_pTypeSkeleton->isChecked();
	m_pHideBones->setEnabled(bHideBonesEnabled);

	// Expand the tree
	m_pSceneTree->expandAll();
	for (int i = 0; i < CSceneGraphModel::GetColumnCount(); i++)
	{
		m_pSceneTree->resizeColumnToContents(i);
	}
}