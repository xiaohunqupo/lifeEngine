/**
 * @file
 * @addtogroup Engine Engine
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef SCENERENDERTARGETS_H
#define SCENERENDERTARGETS_H

#include "RenderResource.h"
#include "RenderUtils.h"
#include "Render/RenderTarget.h"
#include "RHI/BaseSurfaceRHI.h"
#include "RHI/TypesRHI.h"
#include "LEBuild.h"

/**
 * @ingroup Engine
 * Enumeration of scene render target types
 */
enum ESceneRenderTargetTypes
{
	SRTT_SceneColorHDR,						/**< Render target for scene colors in HDR mode */
	SRTT_SceneColorLDR,						/**< Render target for scene colors in LDR mode */
	SRTT_SceneDepthZ,						/**< Render target for scene depths */
	SRTT_Bloom,								/**< Render target for bloom */

#if ENABLE_HITPROXY
	SRTT_HitProxies,						/**< Render target for hitProxiesy */
#endif // ENABLE_HITPROXY

	SRTT_Albedo_Roughness_GBuffer,			/**< Render target for albedo and roughness GBuffer */
	SRTT_Normal_Metal_GBuffer,				/**< Render target for normal and metal GBuffer */
	SRTT_Emission_AO_GBuffer,				/**< Render target for emission and AO GBuffer */
	SRTT_LightPassDepthZ,					/**< Render target for light pass' depths */

	SRTT_MaxSceneRenderTargets				/**< Max scene RTs available */
};

/**
 * @ingroup Engine
 * Render targets for scene rendering
 */
class CSceneRenderTargets : public CRenderResource
{
public:
	/**
	 * @brief Constructor
	 */
	CSceneRenderTargets();

	/**
	 * @brief Allocate render targets for new size
	 * 
	 * @param InNewSizeX New size by X
	 * @param InNewSizeY New size by Y
	 */
	void Allocate( uint32 InNewSizeX, uint32 InNewSizeY );

	/**
	 * @brief Begin rendering scene color in HDR mode
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void BeginRenderingSceneColorHDR( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Finish rendering scene color in HDR mode
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void FinishRenderingSceneColorHDR( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Begin rendering scene color in LDR mode
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void BeginRenderingSceneColorLDR( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Finish rendering scene color in LDR mode
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void FinishRenderingSceneColorLDR( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Begin rendering pre pass
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void BeginRenderingPrePass( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Finish rendering pre pass
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void FinishRenderingPrePass( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Begin rendering GBuffer
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void BeginRenderingGBuffer( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Finish rendering GBuffer
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void FinishRenderingGBuffer( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Begin rendering bloom
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void BeginRenderingBloom( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Finish rendering bloom
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void FinishRenderingBloom( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Clear rendering GBuffer
	 * @param InDeviceContextRHI	Device context RHI
	 */
	void ClearGBufferTargets( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Resolve light pass depth
	 */
	void ResolveLightPassDepth( class CBaseDeviceContextRHI* InDeviceContextRHI ) const;

	/**
	 * @brief Get texture of scene color in HDR mode
	 * @return Return texture of scene color in HDR mode
	 */
	FORCEINLINE Texture2DRHIRef_t GetSceneColorHDRTexture() const
	{
		return renderTargets[SRTT_SceneColorHDR].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of scene color in HDR mode
	 * @return Return surface of scene color in HDR mode
	 */
	FORCEINLINE SurfaceRHIRef_t GetSceneColorHDRSurface() const
	{
		return renderTargets[SRTT_SceneColorHDR].GetSurfaceRHI();
	}

	/**
	 * @brief Get texture of scene color in LDR mode
	 * @return Return texture of scene color in LDR mode
	 */
	FORCEINLINE Texture2DRHIRef_t GetSceneColorLDRTexture() const
	{
		return renderTargets[SRTT_SceneColorLDR].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of scene color in LDR mode
	 * @return Return surface of scene color in LDR mode
	 */
	FORCEINLINE SurfaceRHIRef_t GetSceneColorLDRSurface() const
	{
		return renderTargets[SRTT_SceneColorLDR].GetSurfaceRHI();
	}

	/**
	 * @brief Get texture of scene depth Z
	 * @return Return texture of scene depth Z
	 */
	FORCEINLINE Texture2DRHIRef_t GetSceneDepthZTexture() const
	{
		return renderTargets[SRTT_SceneDepthZ].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of scene depth Z
	 * @return Return surface of scene depth Z
	 */
	FORCEINLINE SurfaceRHIRef_t GetSceneDepthZSurface() const
	{
		return renderTargets[SRTT_SceneDepthZ].GetSurfaceRHI();
	}

#if ENABLE_HITPROXY
	/**
	 * @brief Get texture of hit proxy
	 * @return Return texture of hit proxy
	 */
	FORCEINLINE Texture2DRHIRef_t GetHitProxyTexture() const
	{
		return renderTargets[SRTT_HitProxies].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of hit proxy
	 * @return Return surface of hit proxy
	 */
	FORCEINLINE SurfaceRHIRef_t GetHitProxySurface() const
	{
		return renderTargets[SRTT_HitProxies].GetSurfaceRHI();
	}
#endif // ENABLE_HITPROXY

	/**
	 * @brief Get texture of Albedo/Roughness GBuffer
	 * @return Return texture of Albedo/Roughness GBuffer
	 */
	FORCEINLINE Texture2DRHIRef_t GetAlbedo_Roughness_GBufferTexture() const
	{
		return renderTargets[SRTT_Albedo_Roughness_GBuffer].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of Albedo/Roughness GBuffer
	 * @return Return surface of Albedo/Roughness GBuffer
	 */
	FORCEINLINE SurfaceRHIRef_t GetAlbedo_Roughness_GBufferSurface() const
	{
		return renderTargets[SRTT_Albedo_Roughness_GBuffer].GetSurfaceRHI();
	}

	/**
	 * @brief Get texture of Normal/Metal GBuffer
	 * @return Return texture of Normal/Metal GBuffer
	 */
	FORCEINLINE Texture2DRHIRef_t GetNormal_Metal_GBufferTexture() const
	{
		return renderTargets[SRTT_Normal_Metal_GBuffer].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of Normal/Metal GBuffer
	 * @return Return surface of Normal/Metal GBuffer
	 */
	FORCEINLINE SurfaceRHIRef_t GetNormal_Metal_GBufferSurface() const
	{
		return renderTargets[SRTT_Normal_Metal_GBuffer].GetSurfaceRHI();
	}

	/**
	 * @brief Get texture of Emission GBuffer
	 * @return Return texture of Emission GBuffer
	 */
	FORCEINLINE Texture2DRHIRef_t GetEmission_AO_GBufferTexture() const
	{
		return renderTargets[SRTT_Emission_AO_GBuffer].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of Emission GBuffer
	 * @return Return surface of Emission GBuffer
	 */
	FORCEINLINE SurfaceRHIRef_t GetEmission_AO_GBufferSurface() const
	{
		return renderTargets[SRTT_Emission_AO_GBuffer].GetSurfaceRHI();
	}

	/**
	 * @brief Get texture of light pass depth
	 * @return Return texture of light pass depth
	 */
	FORCEINLINE Texture2DRHIRef_t GetLightPassDepthZTexture() const
	{
		return renderTargets[SRTT_LightPassDepthZ].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of light pass depth
	 * @return Return surface of light pass depth
	 */
	FORCEINLINE SurfaceRHIRef_t GetLightPassDepthZSurface() const
	{
		return renderTargets[SRTT_LightPassDepthZ].GetSurfaceRHI();
	}

	/**
	 * @brief Get texture of bloom
	 * @return Return texture of bloom
	 */
	FORCEINLINE Texture2DRHIRef_t GetBloomTexture() const
	{
		return renderTargets[SRTT_Bloom].GetTexture2DRHI();
	}

	/**
	 * @brief Get surface of bloom
	 * @return Return surface of bloom
	 */
	FORCEINLINE SurfaceRHIRef_t GetBloomSurface() const
	{
		return renderTargets[SRTT_Bloom].GetSurfaceRHI();
	}

	/**
	 * @brief Get buffer size
	 * @return Return buffer size
	 */
	FORCEINLINE Vector2D GetBufferSize() const
	{
		return Vector2D( bufferSizeX, bufferSizeY );
	}

	/**
	 * @brief Get buffer width
	 * @return Return buffer width
	 */
	FORCEINLINE float GetBufferWidth() const
	{
		return bufferSizeX;
	}

	/**
	 * @brief Get buffer height
	 * @return Return buffer height
	 */
	FORCEINLINE float GetBufferHeight() const
	{
		return bufferSizeY;
	}

protected:
	/**
	 * @brief Initializes the RHI resources used by this resource.
	 * Called when the resource is initialized.
	 * This is only called by the rendering thread.
	 */
	virtual void InitRHI() override;

	/**
	 * @brief Releases the RHI resources used by this resource.
	 * Called when the resource is released.
	 * This is only called by the rendering thread.
	 */
	virtual void ReleaseRHI() override;

private:
	uint32				bufferSizeX;										/**< Size of buffer by X */
	uint32				bufferSizeY;										/**< Size of buffer by Y */
	CRenderTarget		renderTargets[ SRTT_MaxSceneRenderTargets ];		/**< Static array of all the scene render targets */
};

extern TGlobalResource<CSceneRenderTargets>		g_SceneRenderTargets;		/**< The global render targets used for scene rendering */

#endif // !SCENERENDERTARGETS_H