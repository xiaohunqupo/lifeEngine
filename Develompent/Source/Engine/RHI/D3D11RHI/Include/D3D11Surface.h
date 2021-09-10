/**
 * @file
 * @addtogroup D3D11RHI D3D11RHI
 *
 * Copyright BSOD-Games, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef D3D11SURFACE_H
#define D3D11SURFACE_H

#include <d3d11.h>

#include "Core.h"
#include "RHI/BaseSurfaceRHI.h"

/**
 * @ingroup D3D11RHI
 * @brief Class for work with DirectX 11 surface
 */
class FD3D11Surface : public FBaseSurfaceRHI
{
public:
	/**
	 * @brief Constructor
	 * 
	 * @param[in] InRenderTargetView Render target view
	 */
												FD3D11Surface( ID3D11RenderTargetView* InRenderTargetView );

	/**
	 * @brief Destructor
	 */
												~FD3D11Surface();

	/**
	 * @brief Get render target view
	 */
	FORCEINLINE ID3D11RenderTargetView*			GetD3D11RenderTargetView() const
	{
		return d3d11RenderTargetView;
	}

private:
	ID3D11RenderTargetView*				d3d11RenderTargetView;		/**< A view of the surface as a render target */
};

/**
 * @ingroup D3D11RHI
 * Class of base texture for DirectX 11
 */
class FD3D11TextureRHI : public FBaseTextureRHI
{
public:
	/**
	 * Constructor
	 * 
	 * @param[in] InSizeX Width of texture
	 * @param[in] InSizeY Height of texture
	 * @param[in] InNumMips Number of mip-maps in texture
	 * @param[in] InFormat Pixel format in texture
	 * @param[in] InFlags Texture create flags (use ETextureCreateFlags)
	 */
	FD3D11TextureRHI( uint32 InSizeX, uint32 InSizeY, uint32 InNumMips, EPixelFormat InFormat, uint32 InFlags );

	/**
	 * Destructor
	 */
	virtual ~FD3D11TextureRHI();

	/**
	 * Get shader resource view
	 * @return Return pointer to shader resource view
	 */
	FORCEINLINE ID3D11ShaderResourceView* GetShaderResourceView()
	{
		return d3d11ShaderResourceView;
	}

protected:
	ID3D11ShaderResourceView*				d3d11ShaderResourceView;		/**< The view that is used to access the texture from a shader */
};

/**
 * @ingroup D3D11RHI
 * Class for work with texture 2D in DirectX 11
 */
class FD3D11Texture2DRHI : public FD3D11TextureRHI
{
public:
	/**
	 * Constructor
	 * 
	 * @param[in] InDebugName Debug name of texture
	 * @param[in] InSizeX Width of texture
	 * @param[in] InSizeY Height of texture
	 * @param[in] InNumMips Number of mip-maps in texture
	 * @param[in] InFormat Pixel format in texture
	 * @param[in] InFlags Texture create flags (use ETextureCreateFlags)
	 * @param[in] InData Pointer to data texture
	 */
	FD3D11Texture2DRHI( const tchar* InDebugName, uint32 InSizeX, uint32 InSizeY, uint32 InNumMips, EPixelFormat InFormat, uint32 InFlags, void* InData = nullptr );

	/**
	 * Destructor
	 */
	~FD3D11Texture2DRHI();

	/**
	 * Lock texture for read/write
	 * 
	 * @param[in] InDeviceContext Device context
	 * @param[in] InMipIndex Mip index
	 * @param[in] InIsDataWrite Is begin written to texture
	 * @param[in] InIsUseCPUShadow Is use CPU shadow
	 * @param[out] OutLockedData Locked data in texture
	 */
	void Lock( class FBaseDeviceContextRHI* InDeviceContext, uint32 InMipIndex, bool InIsDataWrite, bool InIsUseCPUShadow, struct FLockedData& OutLockedData );

	/**
	 * Unlock texture for apply changes to texture
	 * 
	 * @param[in] InDeviceContext Device context
	 * @param[in] InMipIndex Mip index
	 * @param[in] InLockedData Locked data
	 * @param[in] InDiscardUpdate Disczrd update
	 */
	void Unlock( class FBaseDeviceContextRHI* InDeviceContext, uint32 InMipIndex, struct FLockedData& InLockedData, bool InDiscardUpdate = false );

	/**
	 * Get DirectX resource
	 * @return Return pointer to DirectX texture 2D
	 */
	FORCEINLINE ID3D11Texture2D* GetResource()
	{
		return d3d11Texture2D;
	}

private:
	/**
	 * Constructor for copy already created DirectX texture
	 * 
	 * @param[in] InD3D11Texture2D Pointer to DirectX texture 2D
	 * @param[in] InSizeX Width of texture
	 * @param[in] InSizeY Height of texture
	 * @param[in] InNumMips Number of mip-maps in texture
	 * @param[in] InFormat Pixel format in texture
	 * @param[in] InFlags Texture create flags (use ETextureCreateFlags)
	 */
	FD3D11Texture2DRHI( ID3D11Texture2D* InD3D11Texture2D, uint32 InSizeX, uint32 InSizeY, uint32 InNumMips, EPixelFormat InFormat, uint32 InFlags );

	/**
	 * Create staging resource for lock
	 * 
	 * @param[in] InDeviceContext Device context
	 * @param[in] InMipSizeX Mip-map width
	 * @param[in] InMipSizeY Mip-map height
	 * @param[in] InSubresource Index of subresource in texture
	 * @param[in] InIsDataWrite Is begin written to texture
	 * @param[in] InIsUseCPUShadow Is use CPU shadow
	 * @return Pointer to created staging texture 2D
	 */
	FD3D11Texture2DRHI* CreateStagingResource( class FBaseDeviceContextRHI* InDeviceContext, uint32 InMipSizeX, uint32 InMipSizeY, uint32 InSubresource, bool InIsDataWrite, bool InIsUseCPUShadow );

	ID3D11Texture2D*			d3d11Texture2D;			/**< Pointer to DirectX texture 2D */
};

#endif // D3D11SURFACE_H