
#ifndef EXPORT_FORMAT_H
#define EXPORT_FORMAT_H

#include <map>

class CCryFile;

namespace LayeredNavMesh {
namespace ExportFormat {

// NOTE Aug 1, 2008: <pvl> it's struct mostly just to get proper
// symbolic constant scoping
struct ChunkType {
	uint16 m_type;
	ChunkType (uint16 type) : m_type (type) { }
	operator uint16 () const { return m_type; }
	// ATTN Aug 1, 2008: <pvl> mustn't ever change or clients are broken
	static const uint16 FIRST_AVAILABLE = 1000;
};

namespace SystemChunkType {
	static const uint16 INVALID_CHUNK = 0;
	static const uint16 INFO = 4;
	// TODO Aug 1, 2008: <pvl> consider adding a FileType chunk that would need
	// to come before any user-defined chunks (if at all) and if present would
	// make it possible to basically switch user-defined chunk type namespaces
	// (e.g. chunk type 1000 might mean vertex list in an LNM file but area
	// definitions in a BAI-like file).
}


/**
 * A very thin wrapper around std::map.  Maps uint16 which is meant to be
 * chunk version number, to IoOperation which is expected to be essentially
 * either ChunkReader or ChunkWriter.
 *
 * Owns the pointers it holds.
 */
template <typename IoOperation>
class ChunkVersionMgr {
	typedef std::map <uint16, IoOperation*> OperationTable;
	OperationTable m_operations;
public:
	~ChunkVersionMgr ();
	void AddOperation (uint16 version, IoOperation * );
	IoOperation * RemoveOperation (uint16 version);
	IoOperation * FindOperation (uint16 version) const;
};

template <typename IoOperation>
inline ChunkVersionMgr<IoOperation>::~ChunkVersionMgr ()
{
	typename OperationTable::iterator it = m_operations.begin ();
	typename OperationTable::iterator end = m_operations.end ();
	for ( ; it != end; ++it)
		delete it->second;
}

template <typename IoOperation>
inline void ChunkVersionMgr<IoOperation>::AddOperation (uint16 version, IoOperation * operation)
{
	m_operations.insert (std::make_pair (version, operation));
}

template <typename IoOperation>
inline IoOperation * ChunkVersionMgr<IoOperation>::RemoveOperation (uint16 version)
{
	typename OperationTable::iterator operationIt = m_operations.find (version);
	if (operationIt == m_operations.end ())
		return 0;

	IoOperation * op = operationIt->second;
	m_operations.erase (operationIt);
	return op;
}

template <typename IoOperation>
inline IoOperation * ChunkVersionMgr<IoOperation>::FindOperation (uint16 version) const
{
	typename OperationTable::const_iterator operationIt = m_operations.find (version);
	if (operationIt == m_operations.end ())
		return 0;
	return operationIt->second;
}

// ---

struct VersionChunkHeader;

class ChunkReader {
public:
	virtual ~ChunkReader () { }

	virtual bool ReadChunk (CCryFile & ) = 0;
};

// NOTE Aug 1, 2008: <pvl> not inherited from ChunkReader since 1) ReadChunk()
// signature is different here, and 2) strangely, there's no need :-) .
class VersionedChunkReader {
public:
	virtual ~VersionedChunkReader () { }
	/**
	* Takes VersionChunkHeader as well to enable a single implementation
	* to handle several versions if differences between them are minor enough.
	*/
	virtual bool ReadChunk (const VersionChunkHeader & , CCryFile & ) = 0;
};

// NOTE Aug 1, 2008: <pvl> implements ChunkReader interface so it can
// masquerade as one.  Maps chunk version numbers to corresponding readers.
// Invokes the readers to do the actual reading.
class VersionSwitchChunkReader : public ChunkReader {
	typedef ChunkVersionMgr<VersionedChunkReader> ReaderVersionMgr;
	ReaderVersionMgr m_readers;
public:
	virtual bool ReadChunk (CCryFile & );

	void RegisterReader (uint16 version, VersionedChunkReader * );
	VersionedChunkReader * UnregisterReader (uint16 version);
};

// ---

class ChunkWriter {
protected:
	ChunkType m_chunkType;

	/// Override this supplying an implementation that writes m_chunkType's header and data.
	virtual bool WriteChunkData (CCryFile & ) const = 0;
public:
	ChunkWriter (ChunkType );
	virtual ~ChunkWriter () { }

	/// Override this if you'd like to add writing another generic header.
	virtual bool WriteChunk (CCryFile & ) const;
};

class VersionedChunkWriter : public ChunkWriter {
protected:
	uint16 m_version;

	virtual bool WriteChunkData (CCryFile & ) const = 0;
public:
	VersionedChunkWriter (ChunkType , uint16 version);

	virtual bool WriteChunk (CCryFile & ) const;
};

// ---

/**
 * Owns the ChunkReader pointers it holds.  Any chunk reader pointers still held
 * by this class at the moment of its destruction will be deleted.  If you want
 * chunk reader instances to survive unregister them before ~Reader() runs.
 */
class FileReader {
	typedef std::map <uint16/*chunk type*/, ChunkReader*> ReadersMap;

	ReadersMap m_readers;
public:
	~FileReader ();

	bool Read (const char * fname);

	bool RegisterChunkReader (ChunkType , ChunkReader * );
	ChunkReader * UnregisterChunkReader (ChunkType );
};


/**
 * There's no function for unregistering since the Writer assumes ownership
 * of ChunkWriter*'s and deletes them in its dtor.  That way application doesn't
 * need to keep track of them and delete them itself.
 */
class FileWriter {
	typedef std::vector<ChunkWriter*> ChunkWriters;
	ChunkWriters m_writers;
public:
	FileWriter ();
	~FileWriter ();

	void Write (const char * fname) const;

	bool RegisterChunkWriter (ChunkWriter * );
};

}	// namespace ExportFormat
}	// namespace LayeredNavMesh

#endif // #define PORT_FORMAT_H
