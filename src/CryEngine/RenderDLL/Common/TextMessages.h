#ifndef _TEXTMESSAGES_H_
#define _TEXTMESSAGES_H_

// compact buffer to store text messages for a frame and render them each frame
// (replacement for the former PodArray<text_info_struct> m_listMessages[2], cleaner/more cache friendly,less memory,faster,typesafe)
// todo: can release memory in case this is needed
class CTextMessages
{
public:
	class CTextMessageHeader;

	// iteration should not be started yet
	// Arguments
	//   vPos - WorldSpace position
	//   szText - must not be 0
	//   nDrawFlags - EDrawTextFlags
	void PushEntry_Text( const Vec3 &vPos, const ColorB col, const float fFontSize, const int nDrawFlags, const char *szText );

	// iteration should not be started yet
	// Arguments
	//   szText - must not be 0
	void PushEntry_Texture( const Vec3 &vPos, const int nTextureId, const float fImageSize );

	// usually called every frame
	// resets/ends iteration
	void Clear();

	// todo: improve interface
	// starts the iteration
	// Returns
	//   0 if there are no more entries
	const CTextMessageHeader *GetNextEntry();

	uint32 ComputeSizeInMemory() const;

	//
	bool empty() const { return m_TextMessageData.empty(); }

	// -------------------------------------------------------------

	struct SText;
	struct STexture;

	class CTextMessageHeader
	{ 
	public:
		const SText *CastTo_Text() const { return m_Type==0 ? (SText *)this : 0; }
		const STexture *CastTo_Texture() const { return m_Type==1 ? (STexture *)this : 0; }

		uint8 GetSize() const { return m_Size; }

	protected: // ---------------------------------------------
		uint8			m_Type;						// S:CTextMessageHeader_Text, 1:STextMessageHeader_Texture
		uint8			m_Size;						// including attached text
		uint8			m_Padding[2];			// used later
	};

	// ---------------------------------------------

	struct SText :public CTextMessageHeader
	{
		void Init( const uint32 paddedSize ) { m_Type=0; assert(paddedSize < 256); m_Size=paddedSize; }
		const char *GetText() const { return (char *)this+sizeof(*this); }

		Vec3			m_vPos;
		ColorB		m_Color;
		float			m_fFontSize;
		uint32		m_nDrawFlags;		// EDrawTextFlags
	};

	// ---------------------------------------------

	struct STexture :public CTextMessageHeader
	{
		void Init() { m_Type=1; m_Size=sizeof(*this); }

		Vec3			m_vPos;
		float			m_fImageSize;
		int				m_nTextureID;	
	};

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_TextMessageData);
	}
private: // ------------------------------------------------------

	// each call the former returned pointer might be invalid
	// Arguments
	//   dwBytes >0 and dividable by 4
	// Returns
	//   0 if there is not enough space
	uint8 *PushData( const uint32 dwBytes );
 
// ------------------------------------------------------

	std::vector<uint8>				m_TextMessageData;		// consists of many 4 byte aligned STextMessageHeader+ZeroTermintedText
	uint32										m_dwCurrentReadPos;		// in bytes, !=0 interation started

	CryCriticalSection				m_TextMessageLock;
};


#endif // #ifndef _TEXTMESSAGES_H_
