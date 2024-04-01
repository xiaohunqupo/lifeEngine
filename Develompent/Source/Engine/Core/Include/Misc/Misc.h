/**
 * @file
 * @addtogroup Core Core
 *
 * Copyright BSOD-Games, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef MISC_H
#define MISC_H

#include <string>
#include <vector>

#include "Misc/Types.h"
#include "Misc/CoreGlobals.h"
#include "Hashing/FastHash.h"
#include "Containers/String.h"
#include "Core.h"

/**
 * @ingroup Core
 * Flags controlling [de]compression
 */
enum ECompressionFlags
{
	CF_None = 0,		/**< No compression */
	CF_ZLIB = 1 << 0,	/**< Compress with ZLIB */
};

/**
 * @ingroup Core
 * Size of chunk for loading compression data
 */
#define LOADING_COMPRESSION_CHUNK_SIZE			131072

/**
 * @ingroup Core
 * Size of chunk for saving compression data
 */
#define SAVING_COMPRESSION_CHUNK_SIZE			LOADING_COMPRESSION_CHUNK_SIZE

/**
 * @ingroup Core
 * Is char is whitespace
 * 
 * @param[in] InChar Tested char
 * @return Return true if InChar is whitespace, else return false
 */
FORCEINLINE bool Sys_IsWhitespace( tchar InChar )
{
	return InChar == TEXT( ' ' ) || InChar == TEXT( '\t' );
}

/**
 * @ingroup Core
 * Normalize path separators
 * 
 * @param InOutFilename Path to file
 */
FORCEINLINE void Sys_NormalizePathSeparators( std::wstring& InOutFilename )
{
	bool		bIsNeedDeleteNextSeparator = false;
	for ( uint32 index = 0, count = InOutFilename.size(); index < count; ++index )
	{
		tchar&		ch = InOutFilename[ index ];
		if ( bIsNeedDeleteNextSeparator && ( ch == TEXT( '/' ) || ch == TEXT( '\\' ) ) )
		{
			InOutFilename.erase( index, 1 );
			count = InOutFilename.size();
			--index;
			continue;
		}

		if ( ch == TEXT( '/' ) || ch == TEXT( '\\' ) )
		{
			ch = PATH_SEPARATOR[ 0 ];
			bIsNeedDeleteNextSeparator = true;
		}
		else
		{
			bIsNeedDeleteNextSeparator = false;
		}
	}
}

/**
 * @ingroup Core
 * Return base directory of the game
 * @return Return base directory of the game
 */
FORCEINLINE std::wstring Sys_BaseDir()
{
	return TEXT( ".." ) PATH_SEPARATOR TEXT( ".." ) PATH_SEPARATOR;
}

/**
 * @ingroup Core
 * Return directory of the game
 * @return Return directory of the game
 */
FORCEINLINE std::wstring Sys_GameDir()
{
	return CString::Format( TEXT( ".." ) PATH_SEPARATOR TEXT( ".." ) PATH_SEPARATOR TEXT( "%s" ) PATH_SEPARATOR, g_GameName.c_str() );
}

/**
 * @ingroup Core
 * Return shader directory
 * @return Return shader directory
 */
FORCEINLINE std::wstring Sys_ShaderDir()
{
	return TEXT( ".." ) PATH_SEPARATOR TEXT( ".." ) PATH_SEPARATOR TEXT( "Engine" ) PATH_SEPARATOR TEXT( "Shaders" ) PATH_SEPARATOR;
}

/**
 * @ingroup Core
 * @brief Get computer name
 * @return Return computer name
 */
std::wstring Sys_ComputerName();

/**
 * @ingroup Core
 * @brief Get user name
 * @return Return user name
 */
std::wstring Sys_UserName();

/**
 * @ingroup Core
 * Calculate hash from name
 *
 * @param[in] InName Name
 * @param[in] InHash Start hash
 * @return Return hash
 */
FORCEINLINE uint64 Sys_CalcHash( const std::wstring& InName, uint64 InHash = 0 )
{
	return FastHash( InName.data(), ( uint64 )InName.size() * sizeof( std::wstring::value_type ), InHash );		// TODO BG yehor.pohuliaka - Need change to one format without dependency from platform
}

/**
 * @ingroup Core
 * Thread-safe abstract compression routine. Compresses memory from uncompressed buffer and writes it to compressed
 * buffer. Updates CompressedSize with size of compressed data. Compression controlled by the passed in flags.
 *
 * @param[in] InFlags Flags to control what method to use and optionally control memory vs speed
 * @param[in] InCompressedBuffer Buffer compressed data is going to be written to
 * @param[in/out] OutCompressedSize Size of CompressedBuffer, at exit will be size of compressed data
 * @param[in] InUncompressedBuffer Buffer containing uncompressed data
 * @param[in] InUncompressedSize Size of uncompressed data in bytes
 * @return true if compression succeeds, false if it fails because CompressedBuffer was too small or other reasons
 */
bool Sys_CompressMemory( ECompressionFlags InFlags, void* InCompressedBuffer, uint32& InOutCompressedSize, const void* InUncompressedBuffer, uint32 InUncompressedSize );

/**
 * @ingroup Core
 * Thread-safe abstract decompression routine. Uncompresses memory from compressed buffer and writes it to uncompressed
 * buffer. UncompressedSize is expected to be the exact size of the data after decompression.
 *
 * @param[in] InFlags Flags to control what method to use to decompress
 * @param[in] InUncompressedBuffer Buffer containing uncompressed data
 * @param[in] InUncompressedSize Size of uncompressed data in bytes
 * @param[in] InCompressedBuffer Buffer compressed data is going to be read from
 * @param[in] InCompressedSize Size of CompressedBuffer data in bytes
 * @return true if compression succeeds, false if it fails because CompressedBuffer was too small or other reasons
 */
bool Sys_UncompressMemory( ECompressionFlags InFlags, void* InUncompressedBuffer, uint32 InUncompressedSize, const void* InCompressedBuffer, uint32 InCompressedSize );

/**
 * @ingroup Core
 * Does per platform initialization of timing information and returns the current time
 * @return Return current time
 */
double Sys_InitTiming();

// Platform specific functions
#if PLATFORM_WINDOWS
#include "WindowsMisc.h"
#else
#error Unknown platform
#endif // PLATFORM_WINDOWS

#endif // !MISC_H
