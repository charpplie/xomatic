#include "StdAfx.h"
#ifdef __SPU__
#include <spu_intrinsics.h>
#include <cell/atomic.h>
#include <cell/dma.h>
//#include <algorithm>
#include <stdint.h>
#include <edge/zlib/edgezlib_deflate.h>
#include <edge/zlib/edgezlib_inffast.h>
#include <SPU/SPU.h>
#include "zlib_spu.h"

inline void UnalignedLargeDmaPut( const void* ls, uint32 ea, uint32 size)
{
	uint32 lsAddress	= (uint32) ls;
	assert( (lsAddress & 0xF) == (ea & 0xF) );
	si_wrch( MFC_TagID, si_from_uint(MEM_TRANSFER_DMA_TAG_BASE) );
	while ( size > 0 )
	{
		//Calculate the size of the current DMA to transfer
		uint32 currSize =	(lsAddress & 1) ? 1 :
			((lsAddress & 2) && (size >= 2)) ? 2 :
			((lsAddress & 4) && (size >= 4)) ? 4 :
			((lsAddress & 8) && (size >= 8)) ? 8 :
			(size >= MFC_MAX_DMA_SIZE) ? MFC_MAX_DMA_SIZE :
			(size >= MFC_MIN_DMA_SIZE) ? (size & ~(MFC_MIN_DMA_SIZE - 1)) :
			(size >= 8) ? 8 :
			(size >= 4) ? 4 :
			(size >= 2) ? 2 :
			1;
		si_wrch( MFC_LSA, si_from_uint(lsAddress) );
		si_wrch( MFC_EAL, si_from_uint(ea) );
		si_wrch( MFC_Size, si_from_uint(currSize) );
		si_wrch( MFC_Cmd, si_from_uint(MFC_CMD_WORD(0,0,MFC_PUT_CMD))) ;
		lsAddress	+= currSize;
		ea			+= currSize;
		size		-= currSize;
	}
	memtransfer_sync(0);
}

inline int FetchAndInflateRawData(	uint32 eaUncompOutput, uint32 expectedUncompSize,
																	 uint32 eaCompressed, uint32 compressedSize,
																	 unsigned char* pLsInputTempBuffer,
																	 uint32 inputTempBuffSize,
																	 unsigned char* pLsOutputTempBuffer,
																	 uint32 outputTempBuffSize	)
{
	assert( ((uint32)eaCompressed) >= LS_RANGE );			//Double check the user hasn't passed in LS addresses by mistake
	assert( ((uint32)eaUncompOutput) >= LS_RANGE );
	assert( ((uint32)pLsInputTempBuffer) < LS_RANGE );		//Double check the user hasn't passed in effective addresses by mistake
	assert( ((uint32)pLsOutputTempBuffer) < LS_RANGE );

	assert( (((uint32)pLsInputTempBuffer) & 0x7F) == 0 );
	assert( (inputTempBuffSize & 0x7F) == 0 );
	assert( (((uint32)pLsOutputTempBuffer) & 0x7F) == 0 );
	assert( (outputTempBuffSize & 0x7F) == 0 );
	uint32 singleInputMaxSize				= inputTempBuffSize - OverAllocateInputBufferSize;
	assert( singleInputMaxSize > 0 );
	uint32 singleOutputMaxSize			= outputTempBuffSize - OverAllocateOutputBufferSize;
	assert( singleOutputMaxSize > 0 );

	uint32 currEaInput					= eaCompressed;
	uint32 currEaOutput					= eaUncompOutput;
	uint32 remainingInputData				= compressedSize;
	uint32 remainingOutputSpace			= expectedUncompSize;

	z_stream stream;

	stream.avail_in							= 0;
	stream.next_in							= Z_NULL;

	int ret = inflateInit2( &stream, -MAX_WBITS );
	if ( ret != Z_OK )
		return ret;

	uint32 totalOutputSize = 0;

	/* decompress until deflate stream ends or end of file */
	do
	{
		if ( remainingInputData == 0 )
			break;

		//Firstly, fetch up to "singleInputMaxSize" worth of input data
		uint32 thisInputSize				= (remainingInputData < singleInputMaxSize)?remainingInputData:singleInputMaxSize;

		{
			uint32 eaInputRoundDown		= currEaInput & ~0x7F;
			uint32 inputAlignOffset		= currEaInput & 0x7F;
			uint32 inputSizeRoundUp		= thisInputSize + inputAlignOffset;
			inputSizeRoundUp = (inputSizeRoundUp + 0x7F) & ~0x7F;
			assert( inputAlignOffset <= OverAllocateInputBufferSize );
			assert( (eaInputRoundDown + inputAlignOffset) == currEaInput );
			assert( inputSizeRoundUp <= inputTempBuffSize );

			memtransfer_from_main(pLsInputTempBuffer, SPU_MAIN_PTR((void*)eaInputRoundDown), inputSizeRoundUp, 1);
			memtransfer_sync(1);

			stream.next_in					= &pLsInputTempBuffer[inputAlignOffset];
			stream.avail_in					= thisInputSize;
		}

		//We have consumed "thisInputSize" worth of the input data
		currEaInput							+= thisInputSize;
		remainingInputData					-= thisInputSize;

		//Set up our output buffer
		uint32 thisOutputAlignOffset1		= currEaOutput & 0x7F;
		assert( thisOutputAlignOffset1 < OverAllocateOutputBufferSize );
		unsigned char* pThisOutputBase		= &pLsOutputTempBuffer[thisOutputAlignOffset1];
		stream.next_out						= pThisOutputBase;
		stream.avail_out					= singleOutputMaxSize;

		// Keep running inflate() on input and sending out data to main memory until we've consumed all input data
		while ( true )
		{
			ret = inflate( &stream, Z_NO_FLUSH );
			// Z_STREAM_ERROR is only for code error, not error in data itself
			assert( ret != Z_STREAM_ERROR );

			switch ( ret )
			{
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
				// fall through

			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				//This may be caused by bad data.
				//Either assert here, or feedback the error so the PPU can re-load and re-attempt the decompression
				assert(0);//inflate error
				inflateEnd( &stream );
				return ret;
			}

			uint32 thisOutputSize			= singleOutputMaxSize - stream.avail_out;
			assert( thisOutputSize <= singleOutputMaxSize );
			assert( thisOutputSize <= remainingOutputSpace );

			if ( thisOutputSize )
			{
				//Flush the LS buffer
				UnalignedLargeDmaPut( pThisOutputBase, currEaOutput, thisOutputSize);

				remainingOutputSpace		-= thisOutputSize;
				currEaOutput				+= thisOutputSize;

				totalOutputSize				+= thisOutputSize;
			}

			if ( stream.avail_out )
			{
				//If there was still output buffer space available
				//then we must have run out of input data, so break out
				//of the inner while loop and fetch more input data
				break;
			}

			if ( thisOutputSize )
			{
				//Start filling the output buffer form the start again
				uint32 thisOutputAlignOffset2	= currEaOutput & 0x7F;
				assert( thisOutputAlignOffset2 < OverAllocateOutputBufferSize );
				pThisOutputBase				= &pLsOutputTempBuffer[thisOutputAlignOffset2];

				stream.next_out				= pThisOutputBase;
				stream.avail_out			= singleOutputMaxSize;
			}
		}

		// Keep fetching more input data until inflate tells us we've reached the end of the stream
	} while ( ret != Z_STREAM_END );

	assert( totalOutputSize == expectedUncompSize );
	assert( remainingOutputSpace == 0 );

	// clean up and return
	inflateEnd( &stream );

	return ((ret == Z_STREAM_END) ? Z_OK : Z_DATA_ERROR);
}

inline int InflateRawData
(
	unsigned char* pUncompr,
	uint32 expectedUncompSize,
	const unsigned char* pComprData,
	uint32 comprDataSize )
{
	assert( ((uint32)pUncompr) < LS_RANGE );		//Double check the user hasn't passed in effective addresses by mistake
	assert( ((uint32)pComprData) < LS_RANGE );

	z_stream stream;
	stream.avail_in		= comprDataSize;
	stream.avail_out	= expectedUncompSize;

	stream.next_in		= pComprData;
	stream.next_out		= pUncompr;

	int err = inflateInit2( &stream, -MAX_WBITS );
	if ( err != Z_OK )
	{
		assert( false );
		return err;
	}

	err = inflate( &stream, Z_FINISH );
	if ( err != Z_STREAM_END )
	{
		//This may be caused by bad data.
		//Either assert here, or feedback the error so the PPU can re-load and re-attempt the decompression
		assert(0);//inflate error
		inflateEnd(&stream);
		if ( (err == Z_NEED_DICT) || (err == Z_BUF_ERROR && stream.avail_in == 0) )
			return Z_DATA_ERROR;
		return err;
	}

	err = inflateEnd( &stream );
	if ( err != Z_OK )
	{
		//This may be caused by bad data.
		//Either assert here, or feedback the error so the PPU can re-load and re-attempt the decompression
		assert(0);//inflateEnd error
		return err;
	}

	// check number of bytes uncompressed
	if ( stream.total_out != expectedUncompSize )
	{
		//This may be caused by bad data.
		//Either assert here, or feedback the error so the PPU can re-load and re-attempt the decompression
		assert(0);//Stream decompressed to size different from expected
		return Z_DATA_ERROR;
	}

	return Z_OK;
}

ILINE int DecompressInflateQueueElement
(
	unsigned char *const __restrict gInflateInputBuf,
	unsigned char *const __restrict gInflateOutputBuf,
	uint32	eaCompressed, 
	uint32	eaUncompressed, 
	uint32	compressedSize, 
	uint32	outputUncompPartialBuffSize
)
{
	uint32 isCompressed	= true;
	uint32 expectedUncompSize = outputUncompPartialBuffSize;
	if ( ( compressedSize > InflateInputBufferMaxSize )
		|| ( expectedUncompSize > InflateOutputBufferMaxSize ) )
	{
		assert( isCompressed == true );
		int err = FetchAndInflateRawData
		(	
			eaUncompressed, expectedUncompSize,
			eaCompressed, compressedSize,
			gInflateInputBuf,
			InflateInputBufferMaxSize+OverAllocateInputBufferSize,
			gInflateOutputBuf,
			InflateOutputBufferMaxSize+OverAllocateOutputBufferSize
		);
		if ( 0 != err )
		{
			assert(0);//FetchAndInflateLargeRawData failed
			return err;
		}
	}
	else
	{
		uint32 eaCompressedRoundDown	= eaCompressed & ~0x7F;
		uint32 inputAlignOffset		= eaCompressed & 0x7F;
		uint32 compressedSizeRoundUp	= compressedSize + inputAlignOffset;
		compressedSizeRoundUp = (compressedSizeRoundUp + 0x7F) & ~0x7F;
		assert( inputAlignOffset <= OverAllocateInputBufferSize );
		assert( (eaCompressedRoundDown + inputAlignOffset) == eaCompressed );
		assert( compressedSizeRoundUp <= InflateInputBufferMaxSize+OverAllocateInputBufferSize );
		memtransfer_from_main(gInflateInputBuf, SPU_MAIN_PTR((void*)eaCompressedRoundDown), compressedSizeRoundUp, 0);
		memtransfer_sync(0);
		const unsigned char* pInputBuffer = &gInflateInputBuf[inputAlignOffset];
		//Compute the decompressed output into the LS buffer at the same offset as the main mem buffer has
		uint32 outputAlignOffset		= (eaUncompressed) & 0x7F;
		assert( outputAlignOffset < OverAllocateOutputBufferSize );
		unsigned char* pLsOutputBuffer;
		if ( isCompressed )
		{
			pLsOutputBuffer							= &gInflateOutputBuf[outputAlignOffset];
			//Perform the decompression
			int err = InflateRawData
			(	
				pLsOutputBuffer,
				expectedUncompSize,
				pInputBuffer,
				compressedSize 
			);
			if ( 0 != err )
			{
				assert(0);//InflateRawData failed
				return 1;
			}
		}
		else
		{
			if ( compressedSize != expectedUncompSize )
			{
				assert( false );
				return 1;
			}
			if ( inputAlignOffset != outputAlignOffset )
			{
				//We will move the input so that when it is output
				//the low 7 bits will be the same as the destination
				const void* pLsBufferSource				= pInputBuffer;
				pLsOutputBuffer							= &gInflateOutputBuf[outputAlignOffset];
				memcpy( pLsOutputBuffer, pLsBufferSource, compressedSize );
			}
			else
			{
				//They're already both at the same alignment within 128 bytes
				pLsOutputBuffer		 		 		 	= &gInflateInputBuf[inputAlignOffset];
			}
		}

		//	Send the results out to the specified Effective Address
		UnalignedLargeDmaPut
		(	
			pLsOutputBuffer,
			eaUncompressed,
			expectedUncompSize
		);
	}
	return 0;
}

SPU_ENTRY(zlib_inflate)
void ProcessInflateQueueElement
(
	uint32 eaCompressed, 
	uint32 eaUncompressed, 
	uint32 compressedSize, 
	uint32 outputUncompPartialBuffSize, 
	uint32 eaEv
)
{
	unsigned char gInflateInputBuf[InflateInputBufferMaxSize + OverAllocateInputBufferSize]  __attribute__((aligned(128)));
	unsigned char gInflateOutputBuf[InflateOutputBufferMaxSize + OverAllocateOutputBufferSize] __attribute__((aligned(128)));
	int err = DecompressInflateQueueElement(gInflateInputBuf, gInflateOutputBuf, eaCompressed, eaUncompressed, compressedSize, outputUncompPartialBuffSize);
	assert( 0 != err );
	assert( (eaEv & 15) == 0 );
	IF(eaEv, true)
	{
		SPollEvent evLS;
		const uint32 cLSAddr = evLS.SetValue(eaEv, (err?0x80000000 : 0));
		si_wrch(MFC_TagID, si_from_uint(MEM_TRANSFER_DMA_TAG_BASE));
		si_wrch(MFC_LSA, si_from_uint(cLSAddr));
		si_wrch(MFC_EAL, si_from_uint(eaEv));
		si_wrch(MFC_Size, si_from_uint(4));
		si_wrch(MFC_Cmd, si_from_uint(MFC_CMD_WORD(0,0,MFC_PUT_CMD))) ;
		memtransfer_sync(0);
	}
}
#endif//__SPU__
