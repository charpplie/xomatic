#!/bin/python

import re
import sys
import os

class Config:
	def __init__(self):
		options = self.__parseOptions()
	
		self.projectFileName = options.project
		self.sourcesPerFile = options.number
		self.mkFileName = options.makefile
		self.mkProjectFileName = options.mkproject
		self.destinationFolder = os.path.basename(os.getcwd())
		
		self.excludeFileNames = ['OrganicMotion/OrganicMotionClient.cpp']

	def __parseOptions(self):
		from optparse import OptionParser
		
		optionParser = OptionParser()

		optionParser.add_option('-p', '--project', help='Project file name', default='../GameDllCrysis2.vcproj', type='string')
		optionParser.add_option('-k', '--mkproject', help='Project file name', default='../Project.mk', type='string')
		optionParser.add_option('-n', '--number', help='Number of sources per file', default='25', type='int')
		optionParser.add_option('-m', '--makefile', help='Makefile name', default='Project.mk', type='string')
		(options, args) = optionParser.parse_args()
		
		return options
		
	def __str__(self):
		return 'projectFileName="%s" destinationFolder="%s" sourcesPerFile="%d" mkFileName="%s"' % (self.projectFileName, self.destinationFolder, self.sourcesPerFile, self.mkFileName)

class Parser:
	def __init__(self, config):
		self.config = config
		self.reFileName = re.compile(r'<File\s*RelativePath\s*=\s*"(.+?)"\s*>', re.DOTALL)
		
	def parseFileNames(self):
		fileNames = []
		projectFileContent = open(self.config.projectFileName).read()
		for match in self.reFileName.findall(projectFileContent):
			fileName = match.replace('\\', '/').strip('./')
			isSourceFile = fileName.endswith('.cpp') or fileName.endswith('.c')
			if isSourceFile and not fileName in fileNames and not fileName in self.config.excludeFileNames:
				fileNames.append(fileName)
		return fileNames

	def createUnityFileName(self, unityFileID):
		return 'Game03_%d_uber.cpp' % unityFileID
		
	def splitFileNames(self, list):
		splittedFilesDict = {}
		subList = []
		
		for elem in list:
			if len(subList) == self.config.sourcesPerFile:
				unityFileName = self.createUnityFileName(len(splittedFilesDict))
				splittedFilesDict[unityFileName] = subList
				subList = []

			subList.append(elem)
				
		if len(subList) > 0:
			unityFileName = self.createUnityFileName(len(splittedFilesDict))
			splittedFilesDict[unityFileName] = subList
			
		return splittedFilesDict

class Generator:
	def __init__(self, config):
		self.config = config
		self.unityBuildFirstLine = 'ifeq ($(MKOPTION_UNITYBUILD),1)'
		self.unityBuildLastLine = 'endif'

	def __writeRemovedSourceFiles(self, splittedFileNames, mkFile):
		print >>mkFile, self.unityBuildFirstLine
		print >>mkFile, 'PROJECT_SOURCES_CPP_REMOVE += \\'
		
		removedCounter = 0
		for (unityFileName, codeFileNames) in splittedFileNames.iteritems():
			for codeFileName in codeFileNames:
				removedCounter = removedCounter + 1
				print >>mkFile, '\t%s \\' % codeFileName
			
		print >>mkFile, ''
		
		print 'Writing removed sources in "%s" - %d' % (self.config.mkFileName, removedCounter)

	def __writeUnityFileNames(self, splittedFileNames, mkFile):
		print >>mkFile, 'PROJECT_SOURCES_CPP_ADD += \\'
		
		for (unityFileName, codeFileNames) in splittedFileNames.iteritems():
			print >>mkFile, '\t%s/%s \\' % (config.destinationFolder, unityFileName)

		print >>mkFile, ''	
		print >>mkFile, self.unityBuildLastLine
		
		print 'Writing unity file names to be compiled in "%s" - %d' % (self.config.mkFileName, len(splittedFileNames))

	def __writeUnityFiles(self, splittedFileNames):
		unityFileNamesWritten = []
		for (unityFileName, codeFileNames) in splittedFileNames.iteritems():
			print 'Generating unity file: %s - %d' % (unityFileName, len(codeFileNames))
			unityFile = open(unityFileName, 'w')
			for codeFileName in codeFileNames:
				print >>unityFile, '#include "../%s"' % codeFileName
			
			unityFile.flush()
			unityFileNamesWritten.append(unityFileName)
			
		for fileName in os.listdir('./'):
			if fileName not in unityFileNamesWritten and fileName.endswith('_uber.cpp'):
				print 'Clearing:', fileName
				file = open(fileName, 'w')
				file.close()
		
	def __writeProjectFile(self):
		mkFile = open(self.config.mkFileName, 'w')
		mkPrjFile = open(self.config.mkProjectFileName)
		
		copyCurrentLine = True
		for line in mkPrjFile:

			if line.startswith(self.unityBuildFirstLine):
				copyCurrentLine = False
				self.__writeRemovedSourceFiles(splittedFileNames, mkFile)
				self.__writeUnityFileNames(splittedFileNames, mkFile)

			if copyCurrentLine:
				print >>mkFile, line.rstrip('\n')
				
			if line.startswith(self.unityBuildLastLine):
				copyCurrentLine = True
			
		mkFile.flush()
					
	def writeFiles(self, splittedFileNames):
		try:	
			self.__writeProjectFile()
			self.__writeUnityFiles(splittedFileNames)

		except IOError as (_, errorMessage):
			print 'IO error: %s' % errorMessage
			

config = Config()

parser = Parser(config)

fileNames = parser.parseFileNames()
splittedFileNames = parser.splitFileNames(fileNames)

generator = Generator(config)
generator.writeFiles(splittedFileNames)
