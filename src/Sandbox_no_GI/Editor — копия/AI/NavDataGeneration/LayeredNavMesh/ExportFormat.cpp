
#include "StdAfx.h"

#include "ExportFormat.h"
#include "env.h"
#include "GameUtils.h"	// NOTE Feb 19, 2009: <pvl> only needed for timestamp in info chunk

namespace LayeredNavMesh {
namespace ExportFormat {

struct CommonChunkHeader {
	static const uint32 s_magic = 0xfeeddead;

	uint32 m_magic;
	uint16 m_type;
	uint32 m_length;

	CommonChunkHeader (ChunkType type, unsigned int len) :
			m_magic (s_magic), m_type (type), m_length (len)
	{ }

	CommonChunkHeader (CCryFile & );

	void Write (CCryFile & ) const;

	bool IsValid () const { return m_magic == s_magic; }
	static size_t Size ();
	//AUTO_STRUCT_INFO;
};

CommonChunkHeader::CommonChunkHeader (CCryFile & file) :
		m_type (SystemChunkType::INVALID_CHUNK), m_length (0)
{
	// TODO Jul 3, 2008: <pvl> switch to auto types once I figure out how they work?
	//file.ReadType (this);
	file.ReadType ( & m_magic);
	file.ReadType ( & m_type);
	file.ReadType ( & m_length);
}

void CommonChunkHeader::Write (CCryFile & file) const
{
	file.Write ( & m_magic, sizeof (m_magic));
	file.Write ( & m_type, sizeof (m_type));
	// NOTE Jul 3, 2008: <pvl> if you add a data member, make sure to keep m_length last!
	file.Write ( & m_length, sizeof (m_length));
}

size_t CommonChunkHeader::Size ()
{
	// FIXME Jul 17, 2008: <pvl> REALLY bad to hardcode types here!!!!!!!!!!!!!!!!!
	return sizeof (uint32) + sizeof (uint16) + sizeof (uint32);
}

/**
 * Automates writing common chunk header to a file.
 *
 * Common chunk header includes a length field that allows a reader that doesn't
 * understand a chunk to skip it and continue reading the file at the beginning
 * of the next chunk.  To avoid having to compute the amount of data to be written
 * for a chunk ahead of time the header is written only after the data has been
 * written.  This is implemented by setting aside enough space at the beginning
 * of a chunk for common chunk header, then writing the chunk data itself and
 * then going back in the file and writing the header.
 *
 * This class automates this by performing in its ctor anything that needs
 * to be done before writing the actual chunk data, and then doing in its dtor
 * what needs to be done after chunk data has been written.  It follows that
 * it should be used in the RAII manner - just instantiated at the top of
 * the writer function.  The writer function should perform no writes into
 * the file that aren't that particular chunk's data.
 *
 * Use of this class is completely optional (everything it does can easily be
 * done by hand) but keep in mind that it has some advantages, for instance in
 * case of more complex writers with possible premature returns this class
 * ensures that the chunk is always properly fixed up (so even if the chunk is
 * incomplete/corrupted the file as a whole is still OK).
 */
class CommonHeaderAutowriter {
	CCryFile & m_file;
	ChunkType m_chunkType;
	size_t m_chunkBeginPos;
public:
	CommonHeaderAutowriter (CCryFile & file, ChunkType chunkType) :
			m_file(file), m_chunkType(chunkType)
	{
		m_chunkBeginPos = file.GetPosition ();

		file.Seek (CommonChunkHeader::Size (), SEEK_CUR);
	}

	~CommonHeaderAutowriter ()
	{
		unsigned int sizeofData = m_file.GetPosition () - m_chunkBeginPos - CommonChunkHeader::Size();
		CommonChunkHeader commonHdr (m_chunkType, sizeofData);
		m_file.Seek (m_chunkBeginPos, SEEK_SET);
		commonHdr.Write (m_file);
		m_file.Seek (0, SEEK_END);
	}
};

// ---

struct VersionChunkHeader {
	static const uint16 INVALID_VERSION = 0;
	uint16 m_version;

	VersionChunkHeader (uint16 version) : m_version (version) { }
	VersionChunkHeader (CCryFile & );
};

// ---

VersionChunkHeader::VersionChunkHeader (CCryFile & file) :
		m_version (INVALID_VERSION)
{
	// TODO Jul 3, 2008: <pvl> switch to auto types once I figure out how they work?
	//file.ReadType (this);
	file.ReadType ( & m_version);
}

/**
 * Same deal as with CommnonChunkHeaderAutowriter but much simpler since this
 * time we can just write the header right away in the ctor.
 */
class VersionHeaderAutowriter {
public:
	VersionHeaderAutowriter (CCryFile & file, uint16 versionToWrite)
	{
		// TODO Jul 4, 2008: <pvl> add "current version number" management?
		VersionChunkHeader versionHdr (versionToWrite);
		file.Write ( & versionHdr, sizeof (versionHdr));
	}
};

// ---

bool VersionSwitchChunkReader::ReadChunk (CCryFile & file)
{
	VersionChunkHeader versionHdr (file);

	VersionedChunkReader * reader = m_readers.FindOperation (versionHdr.m_version);
	if (reader == 0)
	{
		// TODO Jul 4, 2008: <pvl> warn about not having a reader for this version?
		return false;
	}

	return reader->ReadChunk (versionHdr, file);
}

void VersionSwitchChunkReader::RegisterReader (uint16 version, VersionedChunkReader * reader)
{
	m_readers.AddOperation (version, reader);
}

VersionedChunkReader * VersionSwitchChunkReader::UnregisterReader (uint16 version)
{
	return m_readers.RemoveOperation (version);
}

// ---

ChunkWriter::ChunkWriter (ChunkType type) : m_chunkType (type)
{
}

bool ChunkWriter::WriteChunk (CCryFile & file) const
{
	CommonHeaderAutowriter commonHdrAutowriter (file, m_chunkType);
	return WriteChunkData (file);
}

VersionedChunkWriter::VersionedChunkWriter (ChunkType type, uint16 version) :
		ChunkWriter (type), m_version (version)
{
}

bool VersionedChunkWriter::WriteChunk (CCryFile & file) const
{
	CommonHeaderAutowriter commonHdrAutowriter (file, m_chunkType);
	VersionHeaderAutowriter versionHdrAutowriter (file, m_version);
	return WriteChunkData (file);
}

// ---

// ---

class InfoChunkWriter : public ChunkWriter {
	virtual bool WriteChunkData (CCryFile & ) const;
public:
	InfoChunkWriter () : ChunkWriter (SystemChunkType::INFO) { }
};

bool InfoChunkWriter::WriteChunkData (CCryFile & file) const
{
#ifndef _RELEASE
	// NOTE Aug 1, 2008: <pvl> GetComputerName() doesn't compile for some reason
	// within the context of CryAISystem - but it's OK since this is not supposed
	// to be run outside the editor
	// FIXME Aug 1, 2008: <pvl> testing CRYAISYSTEM_EXPORTS is a wacky
	// way of knowing if we're being compiled in CryAISystem - find another way
#ifndef CRYAISYSTEM_EXPORTS
	ISystem * pSystem = gEnv->pSystem;
	const SPlatformInfo & platformInfo = pSystem->GetGlobalEnvironment()->pi;

	string info;

	// NOTE Feb 16, 2009: <pvl> don't include user or machine name as it seems
	// that we don't have a reliable way of preventing them from leaking to
	// the public (noone seem to know exactly what _RELEASE is supposed to mean
	// so relying on it to prevent leaking doesn't look like a good idea)














	string osStr;
	switch (platformInfo.winVer)
	{
	case SPlatformInfo::Win2000:		osStr.append ("Windows 2000");				break;
	case SPlatformInfo::WinXP:			osStr.append ("Windows XP");					break;
	case SPlatformInfo::WinSrv2003:	osStr.append ("Windows Server 2003"); break;
	case SPlatformInfo::WinVista:		osStr.append ("Windows Vista");				break;
	case SPlatformInfo::Win7:				osStr.append ("Windows 7");						break;
	default:												osStr.append ("Windows");							break;
	}
	if (platformInfo.win64Bit)
		osStr.append (" 64bit");
	else
		osStr.append (" 32bit");

	info.append ("(" + osStr + "); ");

	// NOTE Feb 19, 2009: <pvl> include time when this build was made (to be precise,
	// time when this file was compiled - shouldn't make much difference in standard
	// builds but can be confusing in a programmer's build)
	info.append (string (__TIMESTAMP__) + "; ");

	// NOTE Feb 19, 2009: <pvl> time when nav data was generated (AKA now)
	string timestamp;
	GameUtils::timeToString (time (NULL), timestamp);
	info.append (timestamp);

	file.Write (info.c_str (), info.length ());
#endif

#endif // _RELEASE

	return true;
}

// ---

FileWriter::FileWriter ()
{
#ifndef _RELEASE
	RegisterChunkWriter (new InfoChunkWriter);
#endif
}

FileWriter::~FileWriter ()
{
	ChunkWriters::const_iterator it = m_writers.begin ();
	ChunkWriters::const_iterator end = m_writers.end ();
	for ( ; it != end; ++it)
		delete *it;
}

void FileWriter::Write (const char * fname) const
{
	CCryFile file;
	// NOTE Jul 3, 2008: <pvl> one would assume an error report would be in order
	// in case of failure, however it's not customary in these parts to produce one
	// so let's follow suit, at least for now.
	if (false == file.Open (fname, "wb" ))
		return;

	ChunkWriters::const_iterator it = m_writers.begin ();
	ChunkWriters::const_iterator end = m_writers.end ();
	for ( ; it != end; ++it)
		// TODO Jul 30, 2008: <pvl> check return values
		(*it)->WriteChunk (file);

	file.Close();
}

bool FileWriter::RegisterChunkWriter (ChunkWriter * writer)
{
	m_writers.push_back (writer);
	return true;
}

// ---

FileReader::~FileReader ()
{
	ReadersMap::const_iterator it = m_readers.begin ();
	ReadersMap::const_iterator end = m_readers.end ();
	for ( ; it != end; ++it)
		delete it->second;
}

/**
 * As for the return value - encountering an invalid chunk header means failure
 * since it means that the file is corrupted.  Not having a chunk reader is
 * not an error but failure reading a chunk is.
 */
bool FileReader::Read (const char * fname)
{
	CCryFile file;
	// NOTE Jul 3, 2008: <pvl> one would assume an error report would be in order
	// in case of failure, however it's not customary in these parts to produce one
	// so let's follow suit, at least for now.
	if (false == file.Open (fname, "rb" ))
		return false;

	bool success = false;

	// NOTE Jul 28, 2008: <pvl> can't put the EOF test here since a file isn't
	// flagged as being at EOF until a read has been attempted that failed
	while (true)
	{
		CommonChunkHeader commonChunkHdr (file);

		if (file.IsEof ())
			break;

		if ( ! commonChunkHdr.IsValid ())
			break;

		// will come handy when chunk reading fails and we'd like to skip it
		const size_t startOfChunkData = file.GetPosition ();

		ReadersMap::iterator it = m_readers.find (commonChunkHdr.m_type);
		if (it == m_readers.end ())
		{
			// TODO Jul 3, 2008: <pvl> add warning?
			file.Seek (commonChunkHdr.m_length, SEEK_CUR);
			continue;
		}

		ChunkReader * reader = it->second;
		if ( ! reader->ReadChunk (file))
		{
			// TODO Jul 31, 2008: <pvl> add warning?
			file.Seek (startOfChunkData + commonChunkHdr.m_length, SEEK_SET);
		}
		else
			success = true;
	}

	// FIXME Jul 3, 2008: <pvl> add check to see if all chunks were read

	file.Close();
	return success;
}

bool FileReader::RegisterChunkReader (ChunkType type, ChunkReader * reader)
{
	m_readers[type] = reader;
	return true;
}

ChunkReader * FileReader::UnregisterChunkReader (ChunkType type)
{
	ReadersMap::iterator it = m_readers.find (type);
	if (it == m_readers.end ())
		return 0;

	ChunkReader * reader = it->second;
	m_readers.erase (it);
	return reader;
}

} // namespace ExportFormat
} // namespace LayeredNavMesh