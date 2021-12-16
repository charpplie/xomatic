////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "IEditor.h"
#include "GameEngine.h"
#include "ViewManager.h"
#include "Util/BoostPythonHelpers.h"
#include "StringDlg.h"
#include "NumberDlg.h"
#include "GenericSelectItemDialog.h"
#include "Util/Ruler.h"
#include "Util/Mailer.h"
#include "UndoCVar.h"

namespace
{
	//////////////////////////////////////////////////////////////////////////
	const char * PyGetCVar(const char *pName)
	{
		ICVar *pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
		if(!pCVar)
		{
			Warning("PyGetCVar: Attempt to access non-existent CVar '%s'", pName ? pName : "(null)");
			throw std::logic_error(CString("\"")+pName+"\" is an invalid cvar.");
		}
		return pCVar->GetString();
	}
	//////////////////////////////////////////////////////////////////////////
	void PySetCVar(const char* pName, pSPyWrappedProperty pValue)
	{
		ICVar *pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
		if(!pCVar)
		{
			Warning("PySetCVar: Attempt to access non-existent CVar '%s'", pName ? pName : "(null)");
			throw std::logic_error(CString("\"")+pName+" is an invalid cvar.");
		}

		CUndo undo("Set CVar");
		if (CUndo::IsRecording())
		{
			CUndo::Record( new CUndoCVar(pName) );
		}
		
		if(pCVar->GetType() == CVAR_INT && pValue->type == SPyWrappedProperty::eType_Int)
		{
			pCVar->Set(pValue->property.intValue);
		}
		else if(pCVar->GetType() == CVAR_FLOAT && pValue->type == SPyWrappedProperty::eType_Float)
		{
			pCVar->Set(pValue->property.floatValue);
		}
		else if(pCVar->GetType() == CVAR_STRING && pValue->type == SPyWrappedProperty::eType_String)
		{
			pCVar->Set((LPCTSTR)pValue->stringValue);
		}
		else
		{
			Warning("PyGetCVar: Type mismatch while assigning CVar '%s'", pName ? pName : "(null)");
			throw std::logic_error("Invalid data type.");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void PyEnterGameMode()
	{
		if (GetIEditor()->GetGameEngine())
			GetIEditor()->GetGameEngine()->SetGameMode(true);
	}

	void PyExitGameMode()
	{
		if (GetIEditor()->GetGameEngine())
			GetIEditor()->GetGameEngine()->SetGameMode(false);
	}

	bool PyIsInGameMode()
	{
		return GetIEditor()->IsInGameMode();
	}

	//////////////////////////////////////////////////////////////////////////
	const char* PyNewObject(const char *typeName, const char *fileName, const char *name, float x, float y, float z)
	{
		CUndo undo("Create new object");
		GetIEditor()->SetModifiedFlag();
		GetIEditor()->SetModifiedModule(eModifiedBrushes);
		CBaseObject *pObject = GetIEditor()->GetObjectManager()->NewObject(typeName, 0, fileName);
		if(pObject == NULL)
			return "";
		if(name && strlen(name) > 0)
			pObject->SetName(name);
		pObject->SetPos(Vec3(x, y, z));
		return pObject->GetName().GetString();
	}

	pPyGameObject PyCreateObject(const char* typeName, const char* fileName, const char* name, float x, float y, float z)
	{
		CBaseObject* pObject = GetIEditor()->GetObjectManager()->NewObject(typeName, 0, fileName);

		if(pObject != NULL)
		{
			GetIEditor()->SetModifiedFlag();

			if (strcmp(typeName, "Brush") == 0)
				GetIEditor()->SetModifiedModule(eModifiedBrushes);
			else if (strcmp(typeName, "Entity") == 0)
				GetIEditor()->SetModifiedModule(eModifiedEntities);
			else
				GetIEditor()->SetModifiedModule(eModifiedAll);

			if (strcmp(name, "") != 0)
				pObject->SetUniqName(name);

			pObject->SetPos(Vec3(x, y, z));
		}

		return PyScript::CreatePyGameObject(pObject);
	}

	//////////////////////////////////////////////////////////////////////////
	const char * PyNewObjectAtCursor(const char *typeName, const char *fileName, const char *name)
	{
		CUndo undo("Create new object");

		Vec3 pos(0,0,0);

		CPoint p;
		GetCursorPos(&p);
		CViewport* viewport = GetIEditor()->GetViewManager()->GetViewportAtPoint(p);
		if(viewport)
		{
			viewport->ScreenToClient(&p);
			if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
			{
				pos = viewport->MapViewToCP(p);
			}
			else
			{
				// Snap to terrain.
				bool hitTerrain;
				pos = viewport->ViewToWorld(p,&hitTerrain);
				if(hitTerrain)
					pos.z = GetIEditor()->GetTerrainElevation(pos.x,pos.y) + 1.0f;
				pos = viewport->SnapToGrid(pos);
			}
		}

		return PyNewObject(typeName, fileName, name, pos.x, pos.y, pos.z);
	}

	//////////////////////////////////////////////////////////////////////////
	void PyStartObjectCreation(const char *typeName, const char *fileName)
	{
		CUndo undo("Create new object");
		GetIEditor()->StartObjectCreation(typeName, fileName);
	}

	//////////////////////////////////////////////////////////////////////////
	void PyRunConsole(const char *text)
	{
		GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(text);
	}

	//////////////////////////////////////////////////////////////////////////
	void PyRunLua(const char *text)
	{
		GetIEditor()->GetSystem()->GetIScriptSystem()->ExecuteBuffer(text, strlen(text));
	}

	//////////////////////////////////////////////////////////////////////////
	bool GetPythonScriptPath( const char *pFile, CString &path )
	{
		bool bRelativePath = true;
		char drive[_MAX_DRIVE];
		drive[0] = '\0';
		_splitpath(pFile, drive, 0, 0, 0);
		if(strlen(drive) != 0)
		{
			bRelativePath = false;
		}

		CString userSandboxFolder = Path::GetUserSandboxFolder();
		Path::ConvertBackSlashToSlash(userSandboxFolder);

		char workingDirectory[MAX_PATH];
		GetCurrentDirectory( MAX_PATH, workingDirectory );
		CString scriptFolder = workingDirectory + CString("/Editor/Scripts/");
		Path::ConvertBackSlashToSlash(scriptFolder);

		if (bRelativePath)
		{		
			// Try to open from user folder
			path = Path::GetUserSandboxFolder() + pFile;

			// If not found try editor folder
			if(!CFileUtil::FileExists(path))
			{
				path = scriptFolder + pFile;
			}
		}
		else
		{
			path = pFile;		
		}

		Path::ConvertBackSlashToSlash(path);

		if(!CFileUtil::FileExists(path))
		{
			CString error = CString("Could not find '") + pFile + "'\n in '" + userSandboxFolder + "'\n or '" + scriptFolder + "'\n";
			PyScript::PrintError(error);
			return false;	
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void GetPythonArgumentsVector(const char * pArguments, std::vector<CString> & inputArguments)
	{
		if ( pArguments == NULL )
			return;

		CString str(pArguments);
		int pos = 0;
		CString token = str.Tokenize(" ", pos);
		while(!token.IsEmpty())
		{
			inputArguments.push_back(token);
			token = str.Tokenize(" ", pos);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void PyRunFileWithParameters(const char *pFile, const char *pArguments)
	{
		CString path;
		std::vector<CString> inputArguments;
		GetPythonArgumentsVector(pArguments,inputArguments);
		if (GetPythonScriptPath(pFile, path))
		{
			PyObject *pyFileObject = PyFile_FromString(const_cast<char*>(path.GetString()), "r");

			if (pyFileObject) 
			{
				std::vector<const char*> argv;
				argv.reserve(inputArguments.size()+1);
				argv.push_back( path.GetString() );	

				for (auto iter = inputArguments.begin(); iter != inputArguments.end(); ++iter)
				{						
					argv.push_back(*iter);
				}

				PySys_SetArgv( argv.size(), const_cast<char**>(&argv[0]) );
				PyRun_SimpleFile(PyFile_AsFile(pyFileObject), path);
				PyErr_Print();
			}	
			Py_DECREF(pyFileObject);
		}
	}

	void PyRunFile(const char *pFile)
	{
		PyRunFileWithParameters(pFile, nullptr);
	}

	//////////////////////////////////////////////////////////////////////////
	void PyExecuteCommand(const char *cmdline)
	{
		GetIEditor()->GetCommandManager()->Execute(cmdline);
	}

	//////////////////////////////////////////////////////////////////////////
	void PyLog(const char* pMessage) 
	{
		if (strcmp(pMessage, "") != 0)
			CryLogAlways(pMessage);
	}

	//////////////////////////////////////////////////////////////////////////
	bool PyMessageBox(const char* pMessage)
	{
		return AfxMessageBox(pMessage, MB_OKCANCEL) == IDOK;
	}

	bool PyMessageBoxYesNo(const char* pMessage)
	{
		return AfxMessageBox(pMessage, MB_YESNO) == IDYES;
	}

	bool PyMessageBoxOK(const char* pMessage)
	{
		return AfxMessageBox(pMessage, MB_OK) == IDOK;
	}


	CString PyEditBox(const char* pTitle)
	{
		CStringDlg stringDialog(pTitle, AfxGetMainWnd());
		if(stringDialog.DoModal() == IDOK)
		{
			return stringDialog.GetString();
		}
		return "";
	}

	SPyWrappedProperty PyEditBoxAndCheckSPyWrappedProperty(const char* pTitle)
	{
		CStringDlg stringDialog(pTitle, AfxGetMainWnd());
		SPyWrappedProperty value;
		stringDialog.SetString("");
		bool isValidDataType(false);

		while(!isValidDataType && stringDialog.DoModal() == IDOK)
		{
			// detect data type
			CString tempString = stringDialog.GetString();
			int countComa = 0;
			int countOpenRoundBraket = 0;
			int countCloseRoundBraket = 0;
			int countDots = 0;

			int posDots = 0;
			int posComa = 0;
			int posOpenRoundBraket = 0;
			int posCloseRoundBraket = 0;

			for (int i=0; i<3; i++)
			{
				if(tempString.Find(".", posDots) > -1)
				{
					posDots = tempString.Find(".", posDots)+1;
					countDots++;
				}
				if(tempString.Find(",", posComa) > -1)
				{
					posComa = tempString.Find(",", posComa)+1;
					countComa++;
				}
				if(tempString.Find("(", posOpenRoundBraket) > -1)
				{
					posOpenRoundBraket = tempString.Find("(", posOpenRoundBraket)+1;
					countOpenRoundBraket++;
				}
				if(tempString.Find(")", posCloseRoundBraket) > -1)
				{
					posCloseRoundBraket = tempString.Find(")", posCloseRoundBraket)+1;
					countCloseRoundBraket++;
				}
			}

			if (countDots == 3 && countComa == 2 && countOpenRoundBraket == 1 && countCloseRoundBraket == 1)
			{
				value.type = SPyWrappedProperty::eType_Vec3;
			}
			else if (countDots == 0 && countComa == 2 && countOpenRoundBraket == 1 && countCloseRoundBraket == 1)
			{
				value.type = SPyWrappedProperty::eType_Color;
			}
			else if (countDots == 1 && countComa == 0 && countOpenRoundBraket == 0 && countCloseRoundBraket == 0)
			{
				value.type = SPyWrappedProperty::eType_Float;
			}
			else if(countDots == 0 && countComa == 0 && countOpenRoundBraket == 0 && countCloseRoundBraket == 0)
			{
				if(stringDialog.GetString() == "False" || stringDialog.GetString() == "True")
				{
					value.type = SPyWrappedProperty::eType_Bool;
				}
				else
				{
					bool isString(false);

					if(stringDialog.GetString().IsEmpty())
					{
						isString = true;
					}

					char tempString[255];
					strcpy(tempString, stringDialog.GetString());

					for(int i=0; i<stringDialog.GetString().GetLength(); i++)
					{
						if(!isdigit(tempString[i]))
						{
							isString = true;
						}
					}

					if(isString)
					{
						value.type = SPyWrappedProperty::eType_String;
					}
					else
					{
						value.type = SPyWrappedProperty::eType_Int;
					}
				}
			}

			// initialize value
			if(value.type == SPyWrappedProperty::eType_Vec3)
			{
				CString valueRed = stringDialog.GetString();
				int iStart = valueRed.Find("(");
				valueRed.Delete(0, iStart+1);
				int iEnd = valueRed.Find(",");
				valueRed.Delete(iEnd, valueRed.GetLength());
				float fValueRed = atof(valueRed);

				CString valueGreen = stringDialog.GetString();
				iStart = valueGreen.Find(",");
				valueGreen.Delete(0, iStart+1);
				iEnd = valueGreen.Find(",");
				valueGreen.Delete(iEnd,valueGreen.GetLength());
				float fValueGreen = atof(valueGreen);

				CString valueBlue = stringDialog.GetString();
				valueBlue.Delete(0, valueBlue.Find(",")+1);
				valueBlue.Delete(0, valueBlue.Find(",")+1);
				valueBlue.Delete(valueBlue.Find(")"), valueBlue.GetLength());
				float fValueBlue = atof(valueBlue);

				value.property.vecValue.x = fValueRed;
				value.property.vecValue.y = fValueGreen;
				value.property.vecValue.z = fValueBlue;
				isValidDataType = true;
			}
			else if (value.type == SPyWrappedProperty::eType_Color)
			{
				CString valueRed = stringDialog.GetString();
				int iStart = valueRed.Find("(");
				valueRed.Delete(0, iStart+1);
				int iEnd = valueRed.Find(",");
				valueRed.Delete(iEnd, valueRed.GetLength());
				int iValueRed = atoi(valueRed);

				CString valueGreen = stringDialog.GetString();
				iStart = valueGreen.Find(",");
				valueGreen.Delete(0, iStart+1);
				iEnd = valueGreen.Find(",");
				valueGreen.Delete(iEnd,valueGreen.GetLength());
				int iValueGreen = atoi(valueGreen);

				CString valueBlue = stringDialog.GetString();
				valueBlue.Delete(0, valueBlue.Find(",")+1);
				valueBlue.Delete(0, valueBlue.Find(",")+1);
				valueBlue.Delete(valueBlue.Find(")"), valueBlue.GetLength());
				int iValueBlue = atoi(valueBlue);

				value.property.colorValue.r = iValueRed;
				value.property.colorValue.g = iValueGreen;
				value.property.colorValue.b = iValueBlue;
				isValidDataType = true;
			}
			else if (value.type == SPyWrappedProperty::eType_Int)
			{
				value.property.intValue = atoi(stringDialog.GetString());
				isValidDataType = true;
			}
			else if (value.type == SPyWrappedProperty::eType_Float)
			{
				value.property.floatValue = atof(stringDialog.GetString());
				isValidDataType = true;
			}
			else if (value.type == SPyWrappedProperty::eType_String)
			{
				value.stringValue = stringDialog.GetString();
				isValidDataType = true;
			}
			else if (value.type == SPyWrappedProperty::eType_Bool)
			{
				if (stringDialog.GetString() == "True" || stringDialog.GetString() == "False")
				{
					value.property.boolValue == stringDialog.GetString();
					isValidDataType = true;
				} 
			}	
			else
			{
				AfxMessageBox("Invalid data type.");
				isValidDataType = false;
			}
		}
		return value;
	}

	CString PyOpenFileBox()
	{
		CFileDialog fileDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY, NULL);				
		CString path = "";
		if (fileDialog.DoModal() == IDOK)
		{
			path = fileDialog.GetPathName();
			Path::ConvertBackSlashToSlash(path);
		}
		return path;
	}

	CString PyComboBox(CString title, std::vector<CString> values, int selectedIdx=0)
	{
		CString result;

		if (title.IsEmpty())
		{
			throw std::runtime_error("Incorrect title argument passed in. ");
			return result;
		}

		if (values.size() == 0)
		{
			throw std::runtime_error("Empty value list passed in. ");
			return result;
		}
		
		CGenericSelectItemDialog pyDlg(AfxGetMainWnd());
		pyDlg.SetTitle(title);
		pyDlg.SetMode(CGenericSelectItemDialog::eMODE_LIST);
		pyDlg.SetItems(values);
		pyDlg.PreSelectItem(values[selectedIdx]);

		if (pyDlg.DoModal() == IDOK)
			result = pyDlg.GetSelectedItem();

		return result;
	}

	static void PyDrawLabel(int x, int y, float size, float r, float g, float b, float a, const char* pLabel) 
	{
		if (!pLabel)
		{
			throw std::logic_error("No label given.");
			return;
		}

		if (!r || !g || !b || !a)
		{
			throw std::logic_error("Invalid color parameters given.");
			return;
		}

		if (!x || !y || !size)
		{
			throw std::logic_error("Invalid position or size parameters given.");
		}
		else
		{
			float color[] = {r, g, b, a};
			gEnv->pRenderer->Draw2dLabel(x, y, size, color, false, pLabel); 
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Constrain
	//////////////////////////////////////////////////////////////////////////
	const char* PyGetAxisConstraint()
	{
		AxisConstrains actualConstrain = GetIEditor()->GetAxisConstrains();
		switch (actualConstrain)
		{
		case AXIS_X: 
			return "X";
		case AXIS_Y:
			return "Y";
		case AXIS_Z:
			return "Z";
		case AXIS_XY:
			return "XY";
		case AXIS_XZ:
			return "XZ";
		case AXIS_YZ:
			return "YZ";
		case AXIS_XYZ:
			return "XYZ";
		case AXIS_TERRAIN:
			return (GetIEditor()->IsTerrainAxisIgnoreObjects())?"TERRAIN":"TERRAINSNAP";
		default:
			throw std::logic_error("Invalid axes.");
		}
	}
	
	void PySetAxisConstraint(CString pConstrain)
	{
		if(pConstrain == "X")
		{
			GetIEditor()->SetAxisConstrains( AXIS_X );
		}
		else if(pConstrain == "Y")
		{
			GetIEditor()->SetAxisConstrains( AXIS_Y );
		}
		else if(pConstrain == "Z")
		{
			GetIEditor()->SetAxisConstrains( AXIS_Z );
		}
		else if(pConstrain == "XY")
		{
			GetIEditor()->SetAxisConstrains( AXIS_XY );
		}
		else if(pConstrain == "YZ")
		{
			GetIEditor()->SetAxisConstrains( AXIS_YZ);
		}
		else if(pConstrain == "XZ")
		{
			GetIEditor()->SetAxisConstrains( AXIS_XZ);
		}
		else if(pConstrain == "XYZ")
		{
			GetIEditor()->SetAxisConstrains( AXIS_XYZ);
		}
		else if(pConstrain == "TERRAIN")
		{
			GetIEditor()->SetAxisConstrains( AXIS_TERRAIN );
			GetIEditor()->SetTerrainAxisIgnoreObjects( true );
		}
		else if(pConstrain == "TERRAINSNAP")
		{
			GetIEditor()->SetAxisConstrains( AXIS_TERRAIN );
			GetIEditor()->SetTerrainAxisIgnoreObjects( false );
		}
		else
		{
			throw std::logic_error("Invalid axes.");
		}
	}
	//////////////////////////////////////////////////////////////////////////
	// Edit Mode
	//////////////////////////////////////////////////////////////////////////
	const char* PyGetEditMode()
	{
		int actualEditMode = GetIEditor()->GetEditMode();
		switch(actualEditMode)
		{
		case eEditModeSelect:
			return "SELECT";
		case eEditModeSelectArea:
			return "SELECTAREA";
		case eEditModeMove:
			return "MOVE";
		case eEditModeRotate:
			return "ROTATE";
		case eEditModeScale:
			return "SCALE";
		case eEditModeTool:
			return "TOOL";
		default:
			throw std::logic_error("Invalid edit mode.");
		}
	}

	void PySetEditMode(CString pEditMode)
	{
		if(pEditMode == "MOVE")
		{
			GetIEditor()->SetEditMode( eEditModeMove );
		}
		else if(pEditMode == "ROTATE")
		{
			GetIEditor()->SetEditMode( eEditModeRotate );
		}
		else if(pEditMode == "SCALE")
		{
			GetIEditor()->SetEditMode( eEditModeScale );
		}
		else if(pEditMode == "SELECT")
		{
			GetIEditor()->SetEditMode( eEditModeSelect );
		}
		else if(pEditMode == "SELECTAREA")
		{
			GetIEditor()->SetEditMode( eEditModeSelectArea );
		}
		else if(pEditMode == "TOOL")
		{
			GetIEditor()->SetEditMode( eEditModeTool );
		}
		else if(pEditMode == "RULER")
		{
			CRuler *pRuler = GetIEditor()->GetRuler();
			pRuler->SetActive( !pRuler->IsActive() );
		}
		else
		{
			throw std::logic_error("Invalid edit mode.");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	const char* PyGetPakFromFile(const char* filename)
	{
		ICryPak* pIPak = GetIEditor()->GetSystem()->GetIPak();
		FILE* pFile = pIPak->FOpen(filename, "rb");
		if (!pFile)
		{
			throw std::logic_error("Invalid file name.");
		}
		const char* pArchPath = pIPak->GetFileArchivePath(pFile);
		pIPak->FClose(pFile);
		return pArchPath;
	}

	//////////////////////////////////////////////////////////////////////////
	void PyUndo()
	{
		GetIEditor()->Undo();
	}

	//////////////////////////////////////////////////////////////////////////
	void PyRedo()
	{
		GetIEditor()->Redo();
	}
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetCVar, general, get_cvar,
	"Gets a cvar value as a string.",
	"general.get_cvar(str cvarName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetCVar, general, set_cvar,
	"Sets a cvar value from an integer, float or string.",
	"general.set_cvar(str cvarName, [int, float, string] cvarValue)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyEnterGameMode, general, enter_game_mode,
	"Enters the editor game mode.",
	"general.enter_game_mode()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyExitGameMode, general, exit_game_mode,
	"Exits the editor game mode.",
	"general.exit_game_mode()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyIsInGameMode, general, is_in_game_mode,
	"Queries if it's in the game mode or not.",
	"general.is_in_game_mode()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyNewObject, general, new_object, 
	"Creates a new object with given arguments and returns the name of the object.", 
	"general.new_object(str entityTypeName, str cgfName, str entityName, float xValue, float yValue, float zValue)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyNewObjectAtCursor, general, new_object_at_cursor, 
	"Creates a new object at a position targeted by cursor and returns the name of the object.", 
	"general.new_object_at_cursor(str entityTypeName, str cgfName, str entityName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyStartObjectCreation, general, start_object_creation,
	"Creates a new object, which is following the cursor.",
	"general.start_object_creation(str entityTypeName, str cgfName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRunConsole, general, run_console,
	"Runs a console command.",
	"general.run_console(str consoleCommand)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRunLua, general, run_lua,
	"Runs a lua script command.",
	"general.run_lua(str luaName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRunFile, general, run_file,
	"Runs a script file. A relative path from the editor user folder or an absolute path should be given as an argument",
	"general.run_file(str fileName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRunFileWithParameters, general, run_file_parameters,
	"Runs a script file with parameters. A relative path from the editor user folder or an absolute path should be given as an argument. The arguments should be separated by whitespace.",
	"general.run_file_parameters(str fileName, str arguments)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyExecuteCommand, general, execute_command,
	"Executes a given string as an editor command.",
	"general.execute_command(str editorCommand)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyMessageBox, general, message_box,
	"Shows a confirmation message box with ok|cancel and shows a custom message.",
	"general.message_box(str message)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyMessageBoxYesNo, general, message_box_yes_no,
	"Shows a confirmation message box with yes|no and shows a custom message.",
	"general.message_box_yes_no(str message)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyMessageBoxOK, general, message_box_ok,
	"Shows a confirmation message box with only ok and shows a custom message.",
	"general.message_box_ok(str message)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyEditBox, general, edit_box,
	"Shows an edit box and returns the value as string.",
	"general.edit_box(str title)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyEditBoxAndCheckSPyWrappedProperty, general, edit_box_check_data_type,
	"Shows an edit box and checks the custom value to use the return value with other functions correctly.",
	"general.edit_box_check_data_type(str title)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyOpenFileBox, general, open_file_box,
	"Shows an open file box and returns the selected file path and name.",
	"general.open_file_box()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetAxisConstraint, general, get_axis_constraint,
	"Gets axis.",
	"general.get_axis_constraint()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetAxisConstraint, general, set_axis_constraint,
	"Sets axis.",
	"general.set_axis_constraint(str axisName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetEditMode, general, get_edit_mode,
	"Gets edit mode",
	"general.get_edit_mode()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetEditMode, general, set_edit_mode,
	"Sets edit mode",
	"general.set_edit_mode(str editModeName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetPakFromFile, general, get_pak_from_file,
	"Finds a pak file name for a given file",
	"general.get_pak_from_file(str fileName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyLog, general, log,
	"Prints the message to the editor console window.",
	"general.log(str message)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyUndo, general, undo,
	"Undos the last operation",
	"general.undo()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRedo, general, redo,
	"Redoes the last undone operation",
	"general.redo()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyDrawLabel, general, draw_label,
	"Shows a 2d label on the screen at the given position and given color.",
	"general.draw_label(int x, int y, float r, float g, float b, float a, str label)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyCreateObject, general, create_object, 
	"Creates a new object with given arguments and returns the name of the object.", 
	"general.create_object(str objectClass, str objectFile, str objectName, (float, float, float) position");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyComboBox, general, combo_box,
	"Shows an combo box listing each value passed in, returns string value selected by the user.",
	"general.combo_box(str title, [str] values, int selectedIndex)");