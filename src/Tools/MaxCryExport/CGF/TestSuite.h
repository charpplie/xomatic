//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __TESTSUITE_H__
#define __TESTSUITE_H__

#include "FileSystemUtil.h"
#include <string>
#include <vector>
#include <set>

class CSExportUtility;

class TestSuite
{
public:
	TestSuite(CSExportUtility* pExportUtility, void (* pfnHandleError)(const std::string& sString, CSExportUtility* pExportUtility), const std::string& sTestSuiteDirectory);

	void Run();

private:
	class ExportError
	{
	public:
		std::string sFileName;
		std::string sDescription;
	};

	class ExportFileHelper
	{
	public:
		ExportFileHelper(TestSuite* pTestSuite, std::vector<ExportError>& errors);
		void ExportFile(std::string sFile);

	private:
		TestSuite* pTestSuite;
		std::vector<ExportError>& errors;
	};

	class DirectoryDifferenceChecker
	{
	public:
		// Because we are using STLPort 4.6 (ie < 5.0) we cannot pass this value by reference.
		void HandleDifference(FileSystemUtil::DirectoryDifference diff);

		std::vector<std::string> missing;
		std::vector<std::string> extra;
		std::vector<std::string> different;
	};

	bool CheckDirectories(const std::string& sDirectory);
	bool ExportDirectory(const std::string& sDirectory);
	void ExportDirectoryRecurse(const std::string& sDirectory, std::vector<ExportError>& errors);
	bool DeleteGeneratedFiles(const std::string& sDirectory);
	void ExportFile(const std::string& sPath, std::vector<ExportError>& errors);
	bool CheckForDifferences(const std::string& sDirectory1, const std::string& sDirectory2);
	void ReportFiles(const std::string& sMessage, const std::vector<std::string>& files);

	CSExportUtility* pExportUtility;
	void (* pfnHandleError)(const std::string& sString, CSExportUtility* pExportUtility);
	std::string sTestSuiteDirectory;
};

#endif //__TESTSUITE_H__
