// Copyright (c) 1999-2014 Crytek.

#include "StdAfx.h"
#include "StaticMeshDialog.h"
#include "FileDialog.h"
#include "FbxImportPlugin.h"
#include <QMessageBox>
#include <cmath>

// Last used save folder
static QString s_lastSaveFolder;

// Set last save folder
// Note: Provide path to a file, not a folder
static void SetLastSaveFolder(const QString &filePath)
{
	// Update last save folder
	QFileInfo fileInfo(filePath);
	s_lastSaveFolder = fileInfo.absolutePath();
}

// Known units
static KnownUnit s_units[] = 
{
	{ 0.1,	    QWidget::tr("mm"),   QWidget::tr("millimeters") },
	{ 1.0,	    QWidget::tr("cm"),   QWidget::tr("centimeters") },
	{ 2.54,	    QWidget::tr("in"),   QWidget::tr("inches") },
	{ 10.0,     QWidget::tr("dm"),   QWidget::tr("decimeters") },
	{ 30.48,    QWidget::tr("ft"),   QWidget::tr("feet") },
	{ 91.44,    QWidget::tr("yd"),   QWidget::tr("yards") },
	{ 100.0,    QWidget::tr("m"),    QWidget::tr("meters") },
	{ 100000.0, QWidget::tr("km"),   QWidget::tr("kilometers") },
};
static const int s_numUnits = sizeof(s_units) / sizeof(KnownUnit);

// Scale limits used by slider
static const double dMinScale = 0.001;
static const double dMaxScale = 1000.0;

// Default scale
static const double dDefaultScale = 1.0;

// Slider will use logarithmic scale
static double dLinearMin, dLinearMax;
void CalcLinearRange()
{
	dLinearMin = std::log10(dMinScale);
	dLinearMax = std::log10(dMaxScale);
}

// From linear [0, 1] to scale value
static double MapLogarithmic(double dVal)
{
	// [0, 1] to [linearMin, linearMax]
	dVal = dVal * (dLinearMax - dLinearMin) + dLinearMin;

	// linear -> logarithmic
	return std::pow(10.0, dVal);
}

// From scale to [0, 1]
static double UnmapLogarithmic(double dVal)
{
	// logarithmic -> linear
	dVal = std::log10(dVal);

	// [linearMin, linearMax] to [0, 1]
	return (dVal - dLinearMin) / (dLinearMax - dLinearMin);
}

// Constructor
CStaticMeshDialog::CStaticMeshDialog(QWidget *pParent) : QDialog(pParent), m_bHandleScaleChanged(true), m_bWarnedFileExists(false)
{
	setupUi(this);

	// We want to have the f(double) argument list instead of f(QString), so we need this type to resolve the function overloading
	typedef void (QDoubleSpinBox::*TResolveOverload)(double);

	// Connect events
	connect(m_pCGFBrowseButton, &QPushButton::pressed, this, &CStaticMeshDialog::OnCGFBrowse);
	connect(m_pScaleSlider, &QSlider::valueChanged, this, &CStaticMeshDialog::OnScaleChanged);
	connect(m_pScaleInput, (TResolveOverload)&QDoubleSpinBox::valueChanged, this, &CStaticMeshDialog::OnScaleSpin);
	connect(m_pImportButton, &QPushButton::pressed, this, &CStaticMeshDialog::OnImport);
	connect(m_pCancelButton, &QPushButton::pressed, this, &QDialog::close);

	// Make sure our constants are set, then set default
	CalcLinearRange();
	m_pScaleInput->setValue(dDefaultScale);
}

// Populate dialog
void CStaticMeshDialog::Populate(SStaticMeshContext &context)
{
	if (s_lastSaveFolder.isEmpty())
	{
		// Set the default file for this import in the default folder
		QString suggestedFile = SuggestSaveFilePath(m_pRequest->pathToFbx, m_pRequest->type, true);
		m_pCGFText->setText(suggestedFile);
	}
	else
	{
		// Set the default file inside the last used folder
		QString suggestedFile = SuggestSaveFileName(m_pRequest->pathToFbx, m_pRequest->type);
		m_pCGFText->setText(QDir(s_lastSaveFolder).absoluteFilePath(suggestedFile));
	}

	// Add origins to the combo box
	// Steal the list of items so we can reference them by index
	qSwap(m_origins, context.origins);
	int index = 0;
	for (QList<CSceneGraph::SOriginNode>::const_iterator i = m_origins.begin(); i != m_origins.end(); ++i, ++index)
	{
		m_pOriginCombo->insertItem(index, i->displayPath);
	}

	// Try to match to known unit
	const double dMatchMargin = 0.001;
	const KnownUnit *pUnit = nullptr;
	for (int i = 0; i < s_numUnits; ++i)
	{
		const KnownUnit &known = s_units[i];
		double dMatch = known.dUnitInCm / context.dUnitInCm;
		bool bMatch = (1.0 - dMatchMargin) <= dMatch && dMatch <= (1.0 + dMatchMargin);
		if (bMatch)
		{
			pUnit = &known;
		}
	}
	m_fileUnit.dUnitInCm = context.dUnitInCm;
	m_fileUnit.rcName = QStringLiteral("file");
	if (pUnit)
	{
		// Set up file with known unit
		m_fileUnit.uiName = QStringLiteral("file (") + pUnit->uiName + QStringLiteral(")");
	}
	else
	{
		// Set up file with unknown unit
		m_fileUnit.uiName = QStringLiteral("file (") + QString::number(context.dUnitInCm, 'f', 2) + QStringLiteral(" cm)");
	}

	 // Add file unit first (default)
	m_pUnitCombo->addItem(m_fileUnit.uiName, qVariantFromValue((void *)&m_fileUnit));

	// Add known units
	for (int i = 0; i < s_numUnits; ++i)
	{
		const KnownUnit *pItem = s_units + i;
		m_pUnitCombo->addItem(pItem->uiName, qVariantFromValue((void *)pItem));
	}
}

// Browse for CGF target
void CStaticMeshDialog::OnCGFBrowse()
{
	// Set hint folder to current specified path
	QFileInfo fileInfo(m_pCGFText->text());
	QString hintDirectory = fileInfo.absolutePath();
	QString hintFile = fileInfo.fileName();

	// Show save dialog
	SContextSelectFile context;
	RunSaveFileDialog(this, &CStaticMeshDialog::OnCGFSelected, context, this, tr("Select target static mesh file"), tr("CGF files (*.cgf)"), hintDirectory, hintFile);
}

// CGF file was selected
void CStaticMeshDialog::OnCGFSelected(const QString &targetFile, SContextSelectFile &context)
{
	if (!targetFile.isEmpty())
	{
		m_pCGFText->setText(targetFile);
		m_bWarnedFileExists = true;
		SetLastSaveFolder(targetFile);
	}
}

// Scale changed from the slider
void CStaticMeshDialog::OnScaleChanged(int iVal)
{
	if (m_bHandleScaleChanged)
	{
		double dVal = (double)iVal / (double)m_pScaleSlider->maximum();
		m_bHandleScaleChanged = false;
		m_pScaleInput->setValue(MapLogarithmic(dVal));
		m_bHandleScaleChanged = true;
	}
}

// Scale changed from the spinbox
void CStaticMeshDialog::OnScaleSpin(double dVal)
{
	if (m_bHandleScaleChanged)
	{
		m_bHandleScaleChanged = false;
		m_pScaleSlider->setValue((int)(UnmapLogarithmic(dVal) * (double)m_pScaleSlider->maximum() + 0.5));
		m_bHandleScaleChanged = true;
	}
}

// Handle actual import
void CStaticMeshDialog::OnImport()
{
	// The origin node to use
	int originIndex = m_pOriginCombo->currentIndex();
	if (originIndex < 0 || originIndex >= m_origins.size())
	{
		// Warn about no origin selected
		QMessageBox::critical(this, tr("No origin selected"), tr("You need to specify a origin for the import, please review your selection"));
		return;
	}

	// The unit that was selected
	int unitIndex = m_pUnitCombo->currentIndex();
	const KnownUnit *pUnit = (const KnownUnit *)m_pUnitCombo->itemData(unitIndex).value<void *>();

	// The target file that was selected
	QString targetFile = m_pCGFText->text();
	if (!m_bWarnedFileExists && QFile::exists(targetFile))
	{
		if (QMessageBox::Yes == QMessageBox::question(this, tr("Overwrite file"), tr("This file already exists:\n%1\n\nDo you wish to overwrite it?").arg(targetFile)))
		{
			m_bWarnedFileExists = true;
		}
		else
		{
			// Do nothing
			return;
		}
	}

	// Apply origin option
	QString origin = m_origins[originIndex].escapedPath;
	m_pRequest->AddOriginOption(origin);

	// Apply scale option
	double scale = m_pScaleInput->value();
	m_pRequest->AddScaleOption(scale);

	// Apply source unit option
	m_pRequest->AddSourceUnitOption(pUnit->rcName);

	// Save folder
	SetLastSaveFolder(targetFile);

	// Success
	Finish(true);
}