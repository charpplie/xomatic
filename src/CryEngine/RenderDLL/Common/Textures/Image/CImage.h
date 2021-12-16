
#ifndef CIMAGE_H
#define CIMAGE_H

#define CHK(x) x

#define SH_LITTLE_ENDIAN

// The mask for extracting just R/G/B from an ulong or SRGBPixel
#ifdef SH_BIG_ENDIAN
#  define RGB_MASK 0xffffff00
#else
#  define RGB_MASK 0x00ffffff
#endif

/**
 * An RGB pixel.
 */
struct SRGBPixel
{
  uint8 blue, green, red, alpha;
  SRGBPixel () /* : red(0), green(0), blue(0), alpha(255) {} */
  { *(unsigned int *)this = (unsigned int)~RGB_MASK; }
  SRGBPixel (int r, int g, int b) : red (r), green (g), blue (b), alpha (255) {}
  //bool eq (const SRGBPixel& p) const { return ((*(unsigned int *)this) & RGB_MASK) == ((*(unsigned int *)&p) & RGB_MASK); }
};

/**
 * Possible errors for CImageFile::mfGet_error.
 */
enum EImFileError { eIFE_OK = 0, eIFE_IOerror, eIFE_OutOfMemory, eIFE_BadFormat, eIFE_ChunkNotFound };

#define FIM_NORMALMAP						0x0001
#define FIM_NOTSUPPORTS_MIPS		0x0004
#define FIM_ALPHA								0x0008	// even if not requested when loading 3dc we store the info if ALPHA would be available so we know for later
#define FIM_DECAL								0x0010
#define FIM_GREYSCALE						0x0020	// hint this texture is greyscale (could be DXT1 with colored artifacts)
#define FIM_STREAM_PREPARE			0x0080
#define FIM_FILESINGLE					0x0100	// info from rc: no need to search for other files (e.g. DDNDIF)
#define FIM_BIG_ENDIANNESS			0x0400	// for textures converted to big endianness format
#define FIM_SPLITTED						0x0800	// for dds textures stored in splitted files
#define FIM_SRGB_READ						0x1000
#define FIM_X360_NOT_PRETILED		0x2000	// for dds textures that cannot be pretiled

class CImageFile
{
friend class CTexMan;
protected:
  int m_Width;	// Width of image.
  int m_Height;	// Height of image.
	int m_Depth;	// Depth of image.
	int m_Sides;	// Depth of image.

  int m_ImgSize;

  int m_NumMips;
  int m_Flags;					// e.g. FIM_GREYSCALE|FIM_ALPHA
  ColorF m_AvgColor;		// average texture color (r,g,b,a independently) loaded from image file
  int m_nStartSeek;

  union	// The image data.
  {
		byte*				m_pByteImage[6];
		SRGBPixel*	m_pPixImage[6];
  };

  EImFileError	m_eError;		// Last error code.
	string				m_FileName;	// file name

  ETEX_Format m_eFormat;
  ETEX_Format m_eSrcFormat;
protected:
	CImageFile (const string& filename);

  void mfSet_error(const EImFileError error, const char* detail = NULL);
  void mfSet_dimensions (const int w, const int h);
public:
  virtual ~CImageFile ();

	const string& mfGet_filename () const  { return m_FileName; }

  int mfGet_width () const { return m_Width; }
  int mfGet_height () const { return m_Height; }
	int mfGet_depth () const { return m_Depth; }
	int mfGet_NumSides () const { return m_Sides; }

	EImFileError mfGet_error () const  { return m_eError; }

  byte* mfGet_image (const int nSide);
	void mfFree_image(const int nSide);
  bool mfIs_image (const int nSide) const { return m_pByteImage[nSide] != NULL; }

  const ColorF& mfGet_AvgColor () const { return m_AvgColor; }
  int mfGet_StartSeek () const { return m_nStartSeek; }

	int mfGet_ImageSize () const {return m_ImgSize;}
  void mfSet_ImageSize (int Size) {m_ImgSize = Size;}
  
  ETEX_Format mfGetFormat() const { return m_eFormat; }
  ETEX_Format mfGetSrcFormat() const { return m_eSrcFormat; }

  void mfSet_numMips (const int num) { m_NumMips = num; }
  int  mfGet_numMips () const { return m_NumMips; }

  void mfSet_Flags (const int Flags) { m_Flags |= Flags; }
  int mfGet_Flags () const { return m_Flags; }

public:
	static CImageFile* mfLoad_file(const string& filename, const bool bReload, const uint32 nFlags);
};

#endif

