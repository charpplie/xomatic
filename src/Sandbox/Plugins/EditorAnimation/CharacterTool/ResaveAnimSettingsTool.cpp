#include "../../EditorCommon/BatchFileDialog.h"
#include "Serialization/StringList.h"
#include <QMessageBox>
#include <QApplication>
#include "AnimationList.h"
#include <Windows.h>

namespace CharacterTool
{

void ShowResaveAnimSettingsTool(AnimationList* animationList, QWidget* parent)
{
	Serialization::StringList filenames;
	SBatchFileSettings settings;
	settings.useCryPak = true;
	settings.extension = "animsettings";
	settings.title = "Resave AnimSettings";
	settings.stateFilename = "resaveAnimSettings.state";
	settings.listLabel = "AnimSettings files";
	settings.descriptionText = "Files marked below will be automatically resaved (converted to new format if needed).";

	if (ShowBatchFileDialog(&filenames, settings, parent))
	{
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		QApplication::processEvents();

		int numFailed = 0;
		for (size_t i = 0; i < filenames.size(); ++i)
		{
			if (!animationList->ResaveAnimSettings(filenames[i].c_str())) 
			{
				CryLogAlways("Failed to resave animsettings: \"%s\"", filenames[i].c_str());
				++numFailed;
			}
		}
		if (numFailed > 0)
		{
			QString message;
			message.sprintf("Failed to resave %d files. See Sandbox log for details.", numFailed);
			QMessageBox::warning(parent, "Error", message);
		}
		QApplication::restoreOverrideCursor();
	}
}

}
