#ifndef CRYTEXTURECOMPRESSOR_H
#define CRYTEXTURECOMPRESSOR_H

class CryTextureCompressor
{
public:
	struct CompressorParameters
	{
		const void* srcBGRA8;
		unsigned int width;
		unsigned int height;
		unsigned int pitch;

		bool highQuality;

		float rWeight;
		float gWeight;
		float bWeight;

		void* userPtr;
		int userInt;

		void (*userOutputFunction)(const CompressorParameters& compress, const void* compressedData, unsigned int compressedSize);
	};

public:
	static void CompressDXT1(const CompressorParameters& compress);
	static void CompressDXT1ForceOpaque(const CompressorParameters& compress);
	static void CompressDXT3(const CompressorParameters& compress);
	static void CompressDXT5(const CompressorParameters& compress);
	static void CompressDXT5n(const CompressorParameters& compress);
};


#endif
