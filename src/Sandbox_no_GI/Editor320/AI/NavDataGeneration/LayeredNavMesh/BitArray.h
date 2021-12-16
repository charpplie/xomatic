
#ifndef LNM_BITARRAY_H
#define LNM_BITARRAY_H

namespace LayeredNavMesh {

class BitArray {
	unsigned m_memSize;
	unsigned char * m_bits;

	// NOTE Jan 28, 2010: <pvl> disallow copying
	BitArray (const BitArray & );
	BitArray & operator= (const BitArray & );
public:
	BitArray (unsigned size);
	~BitArray ();
	void Mark (unsigned index);
	void Unmark (unsigned index);
	bool IsMarked (unsigned index) const;
	void Clear ();
};

inline BitArray::BitArray (unsigned size) :
		m_memSize (size / 8 + 1), m_bits (new unsigned char [m_memSize])
{
	Clear ();
}

inline BitArray::~BitArray ()
{
	delete m_bits;
}

inline void BitArray::Mark (unsigned index)
{
	m_bits[index/8] |= (1 << (index%8));
}

inline void BitArray::Unmark (unsigned index)
{
	m_bits[index/8] &= ~(1 << (index%8));
}

inline bool BitArray::IsMarked (unsigned index) const
{
	return ( m_bits[index/8] & (1 << (index%8)) ) != 0;
}

inline void BitArray::Clear ()
{
	memset (m_bits, 0, m_memSize);
}

} // namespace LayeredNavMesh

#endif // LNM_BITARRAY_H
