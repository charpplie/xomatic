//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "TestSuite.h"
#include "PathUtil.h"
#include "FileSystemUtil.h"
#include "Utility.h"
#include <sstream>
#include <functional>

TestSuite::TestSuite(CSExportUtility* pExportUtility, void (* pfnHandleError)(const std::string& sString, CSExportUtility* pExportUtility), const std::string& sTestSuiteDirectory)
:
	pExportUtility(pExportUtility),
	pfnHandleError(pfnHandleError),
	sTestSuiteDirectory(sTestSuiteDirectory)
{
}

void TestSuite::Run()
{
	// Check whether the test suite directories exist.
	if (!this->CheckDirectories(this->sTestSuiteDirectory))
		return;

	// Clean up the generated files in the test directory.
	if (!this->DeleteGeneratedFiles(PathUtil::Make(this->sTestSuiteDirectory, "test")))
		return;

	// Export everything in the 'test' directory.
	if (!this->ExportDirectory(PathUtil::Make(this->sTestSuiteDirectory, "test")))
		return;

	// Check for differences between the 'test' and 'reference' directories.
	if (!this->CheckForDifferences(PathUtil::Make(this->sTestSuiteDirectory, "reference"), PathUtil::Make(this->sTestSuiteDirectory, "test")))
		return;
}

bool TestSuite::CheckDirectories(const std::string& sDirectory)
{
	if (!FileSystemUtil::IsDirectory(sDirectory))
	{
		this->pfnHandleError("Cannot find test suite directory \"" + sDirectory + "\", specified in CryExport.ini.", this->pExportUtility);
		return false;
	}
	else if (!FileSystemUtil::IsDirectory(PathUtil::Make(sDirectory, "test")) ||
		!FileSystemUtil::IsDirectory(PathUtil::Make(sDirectory, "reference")))
	{
		this->pfnHandleError("Test suite directory \"" + sDirectory + "\", needs to have two sub-directories called 'test' and 'reference'.", this->pExportUtility);
		return false;
	}

	return true;
}

TestSuite::ExportFileHelper::ExportFileHelper(TestSuite* pTestSuite, std::vector<ExportError>& errors)
:	pTestSuite(pTestSuite),
	errors(errors)
{
}

void TestSuite::ExportFileHelper::ExportFile(std::string sFile)
{
	this->pTestSuite->ExportFile(sFile, this->errors);
}

bool TestSuite::ExportDirectory(const std::string& sDirectory)
{
	bool bSuccess = true;

	// Export all the files in the directory.
	std::set<std::string> extensions;
	extensions.insert("max");
	std::vector<ExportError> errors;
	ExportFileHelper helper(this, errors);
	FileSystemUtil::FindFiles(sDirectory, extensions, std::bind1st(std::mem_fun<void, ExportFileHelper, std::string>(&ExportFileHelper::ExportFile), &helper));

	// Report any errors that we encountered.
	if (!errors.empty())
	{
		int nNumOmitted = 0;
		if (errors.size() > 10)
		{
			nNumOmitted = errors.size() - 10;
			errors.resize(10);
		}
		std::ostringstream message;
		message << "The following errors were encountered when exporting:\n";
		for (std::vector<ExportError>::iterator itError = errors.begin(); itError != errors.end(); ++itError)
			message << "- " << (*itError).sFileName << ": " << (*itError).sDescription << "\n";
		if (nNumOmitted > 0)
			message << "<" << nNumOmitted << " further errors omitted>\n";
		this->pfnHandleError(message.str(), this->pExportUtility);

		bSuccess = false;
	}

	return bSuccess;
}

void TestSuite::ExportFile(const std::string& sDirectory, std::vector<ExportError>& errors)
{
	extern bool bDontShowMessageBoxes;

	if(this->pExportUtility->ip->IsMaxFile(sDirectory.c_str()))
	{
		this->pExportUtility->ip->LoadFromFile(sDirectory.c_str());

		this->pExportUtility->LoadConfigFromMaxFile();

		bool bPrev = bDontShowMessageBoxes;
		bDontShowMessageBoxes = true;
		ExportButton(this->pExportUtility, 0, false);
		if (this->pExportUtility->CanExportBones())
			this->pExportUtility->SaveBoneAnimFile("");
		bDontShowMessageBoxes = bPrev;
	}
}

bool TestSuite::DeleteGeneratedFiles(const std::string& sDirectory)
{
	std::set<std::string> extensions;
	extensions.insert("cgf");
	extensions.insert("cga");
	extensions.insert("caf");
	extensions.insert("chr");
	FileSystemUtil::DeleteFiles(sDirectory, extensions);

	return true;
}

bool TestSuite::CheckForDifferences(const std::string& sDirectory1, const std::string& sDirectory2)
{
	// Compare the directory trees.
	DirectoryDifferenceChecker checker;
	FileSystemUtil::CompareDirectories(sDirectory1, sDirectory2,
		std::bind1st(std::mem_fun(&DirectoryDifferenceChecker::HandleDifference), &checker));

	// Print out any extra files that were generated.
	this->ReportFiles("The following EXTRA files are in the test directory:", checker.extra);

	// Print out any missing files.
	this->ReportFiles("The following files are missing from the test directory:", checker.missing);

	// Print out any different files.
	this->ReportFiles("The following files are different from the reference directory:", checker.different);

	if (checker.extra.size() + checker.missing.size() + checker.different.size() == 0)
		this->pfnHandleError("All tests passed.", this->pExportUtility);

	return true;
}

void TestSuite::DirectoryDifferenceChecker::HandleDifference(FileSystemUtil::DirectoryDifference diff)
{
	switch (diff.uExistenceFlags)
	{
		// File exists only in reference directory.
	case 1:
		this->missing.push_back(diff.sRelativePath);
		break;

		// File exists only in test directory.
	case 2:
		this->extra.push_back(diff.sRelativePath);
		break;

		// File exists in both directories.
	case 3:
		this->different.push_back(diff.sRelativePath);
		break;
	}
}

void TestSuite::ReportFiles(const std::string& sMessage, const std::vector<std::string>& files)
{
	// Print out any extra files that were generated.
	if (!files.empty())
	{
		std::ostringstream msg;
		msg << sMessage << "\n";
		int nNumFilesPrinted = 0;
		for (std::vector<std::string>::const_iterator itFile = files.begin(); itFile != files.end(); ++itFile)
		{
			msg << "- " << *itFile << "\n";
			++nNumFilesPrinted;
			if (nNumFilesPrinted >= 30)
				break;
		}
		if (nNumFilesPrinted < files.size())
			msg << "<" << (files.size() - nNumFilesPrinted) << " more files>\n";
		this->pfnHandleError(msg.str(), this->pExportUtility);
	}
}
