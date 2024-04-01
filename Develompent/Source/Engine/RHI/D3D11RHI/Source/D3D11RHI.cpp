#include <d3d9.h>

#include "Core.h"
#include "Logger/LoggerMacros.h"
#include "Render/RenderResource.h"
#include "Render/RenderUtils.h"
#include "Render/GlobalConstantsHelper.h"
#include "Render/SceneUtils.h"
#include "Render/Shaders/Shader.h"
#include "Render/Shaders/ShaderManager.h"
#include "Render/VertexFactory/SimpleElementVertexFactory.h"
#include "Render/SceneRenderTargets.h"
#include "RHI/StaticStatesRHI.h"
#include "D3D11RHI.h"
#include "D3D11Viewport.h"
#include "D3D11DeviceContext.h"
#include "D3D11Surface.h"
#include "D3D11Shader.h"
#include "D3D11Buffer.h"
#include "D3D11ImGUI.h"
#include "D3D11State.h"

/*
==================
GetVertexCountForPrimitiveCount
==================
*/
static FORCEINLINE uint32 GetVertexCountForPrimitiveCount( uint32 InNumPrimitives, EPrimitiveType InPrimitiveType )
{
	uint32		vertexCount = 0;
	switch ( InPrimitiveType )
	{
	case PT_PointList:			vertexCount = InNumPrimitives;		break;
	case PT_TriangleList:		vertexCount = InNumPrimitives * 3;	break;
	case PT_TriangleStrip:		vertexCount = InNumPrimitives + 2;	break;
	case PT_LineList:			vertexCount = InNumPrimitives * 2;	break;

	default:
		Sys_Errorf( TEXT( "Unknown primitive type: %u" ), ( uint32 )InPrimitiveType );
	}

	return vertexCount;
}

/*
==================
GetD3D11PrimitiveType
==================
*/
static FORCEINLINE D3D11_PRIMITIVE_TOPOLOGY GetD3D11PrimitiveType( uint32 InPrimitiveType, bool InIsUsingTessellation )
{
	AssertMsg( !InIsUsingTessellation, TEXT( "Tessellation not supported!" ) );
	switch ( InPrimitiveType )
	{
	case PT_PointList:			return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case PT_TriangleList:		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case PT_TriangleStrip:		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case PT_LineList:			return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	default: 
		Sys_Errorf( TEXT( "Unknown primitive type: %u" ), ( uint32 )InPrimitiveType );
	};

	return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

/*
==================
InitD3D11TextureFormat
==================
*/
static FORCEINLINE void InitD3D11TextureFormat( EPixelFormat InPixelFormat, DXGI_FORMAT InD3D11Format, int32 InBlockBytes = -1 )
{
	// Set up basic info
	g_PixelFormats[InPixelFormat].platformFormat = InD3D11Format;
	g_PixelFormats[InPixelFormat].supported		= InD3D11Format != DXGI_FORMAT_UNKNOWN;
	if ( InBlockBytes != -1 )
	{
		g_PixelFormats[InPixelFormat].blockBytes = InBlockBytes;
	}
}

/**
 * @ingroup D3D11RHI
 * @brief Macro for init engine pixel format
 */
#define INIT_FORMAT_FULL( InRHIFormat, InD3D11Format, InBlockBytes )				InitD3D11TextureFormat( InRHIFormat, InD3D11Format, InBlockBytes )

/**
 * @ingroup D3D11RHI
 * @brief Macro for init engine pixel format
 */
#define INIT_FORMAT( InRHIFormat, InD3D11Format )									InitD3D11TextureFormat( InRHIFormat, InD3D11Format )

 /**
  * @ingroup D3D11RHI
  * @brief Macro for init unsupported engine pixel format
  */
#define INIT_UNSUPPORTED_FORMAT( InRHIFormat )										InitD3D11TextureFormat( InRHIFormat, DXGI_FORMAT_UNKNOWN )

/**
 * @ingroup D3D11RHI
 * @brief A vertex shader for rendering a textured screen element 
 */
class CResolveVertexShader : public CShader
{
	DECLARE_SHADER_TYPE( CResolveVertexShader )

public:
#if WITH_EDITOR
	/**
	 * @brief Is need compile shader for platform
	 *
	 * @param InShaderPlatform Shader platform
	 * @param InVFMetaType Vertex factory meta type. If him is nullptr - return general Assert
	 * @return Return true if need compile shader, else returning false
	 */
	static bool ShouldCache( EShaderPlatform InShaderPlatform, class CVertexFactoryMetaType* InVFMetaType = nullptr )
	{
		if ( InShaderPlatform != SP_PCD3D_SM5 )
		{
			return false;
		}

		return !InVFMetaType || InVFMetaType->GetHash() == CSimpleElementVertexFactory::staticType.GetHash();
	}
#endif // WITH_EDITOR
};

/**
 * @ingroup D3D11RHI
 * @brief A pixel shader for resolving depth
 */
class CResolveDepthPixelShader : public CShader
{
	DECLARE_SHADER_TYPE( CResolveDepthPixelShader )

public:
	/**
	 * @brief Initialize shader
	 * @param[in] InShaderCacheItem Cache of shader
	 */
	virtual void Init( const CShaderCache::ShaderCacheItem& InShaderCacheItem ) override
	{
		CShader::Init( InShaderCacheItem );
		unresolvedSurfaceParameter.Bind( InShaderCacheItem.parameterMap, TEXT( "unresolvedSurface" ) );
	}

#if WITH_EDITOR
	/**
	 * @brief Is need compile shader for platform
	 *
	 * @param InShaderPlatform Shader platform
	 * @param InVFMetaType Vertex factory meta type. If him is nullptr - return general Assert
	 * @return Return true if need compile shader, else returning false
	 */
	static bool ShouldCache( EShaderPlatform InShaderPlatform, class CVertexFactoryMetaType* InVFMetaType = nullptr )
	{
		if ( InShaderPlatform != SP_PCD3D_SM5 )
		{
			return false;
		}

		return !InVFMetaType || InVFMetaType->GetHash() == CSimpleElementVertexFactory::staticType.GetHash();
	}
#endif // WITH_EDITOR

	/**
	 * @brief Set unresolved surface
	 *
	 * @param InDeviceContextRHI	DirectX 11 device context
	 * @param InSourceSurfaceRHI	DirectX 11 source surface
	 */
	FORCEINLINE void SetUnresolvedSurface( class CD3D11DeviceContext* InDeviceContextRHI, const CD3D11Surface* InSourceSurfaceRHI ) const
	{
		uint32							textureIndex			= unresolvedSurfaceParameter.GetBaseIndex();
		ID3D11ShaderResourceView*		d3d11ShaderResourceView = InSourceSurfaceRHI->GetShaderResourceView();
		InDeviceContextRHI->GetD3D11DeviceContext()->PSSetShaderResources( textureIndex, 1, &d3d11ShaderResourceView );
	}

private:
	CShaderResourceParameter		unresolvedSurfaceParameter;			/**< Unresolved surface parameter */
};

IMPLEMENT_SHADER_TYPE(, CResolveVertexShader, TEXT( "RHI/D3D11RHI/ResolveVertexShader.hlsl" ), TEXT( "MainVS" ), SF_Vertex, true );
IMPLEMENT_SHADER_TYPE(, CResolveDepthPixelShader, TEXT( "RHI/D3D11RHI/ResolvePixelShader.hlsl" ), TEXT( "MainDepth" ), SF_Pixel, true );

/*
==================
ResolveSurfaceUsingShader
==================
*/
void ResolveSurfaceUsingShader( CD3D11DeviceContext* InDeviceContextRHI, CD3D11Surface* InSourceSurfaceRHI, CD3D11Texture2DRHI* InDestTexture2D, const D3D11_TEXTURE2D_DESC& InD3D11ResolveTargetDesc, const ResolveRect& InSourceRect, const ResolveRect& InDestRect )
{
	ID3D11DeviceContext*		d3d11DeviceContext		= InDeviceContextRHI->GetD3D11DeviceContext();
	ID3D11Texture2D*			d3d11DestTexture2D		= InDestTexture2D->GetResource();
	ID3D11RenderTargetView*		d3d11RenderTargetView	= InDestTexture2D->GetRenderTargetView();
	ID3D11DepthStencilView*		d3d11DepthStencilView	= InDestTexture2D->GetDepthStencilView();

	// Save the current viewport so that it can be restored
	D3D11_VIEWPORT				d3d11SavedViewport;
	uint32						numSavedViewports = 1;
	d3d11DeviceContext->RSGetViewports( &numSavedViewports, &d3d11SavedViewport );

	// No alpha blending, no depth tests or writes, no stencil tests or writes, no backface culling.
	g_RHI->SetBlendState( InDeviceContextRHI, TStaticBlendStateRHI<>::GetRHI() );
	g_RHI->SetStencilState( InDeviceContextRHI, TStaticStencilStateRHI<>::GetRHI() );
	g_RHI->SetRasterizerState( InDeviceContextRHI, TStaticRasterizerStateRHI<FM_Solid, CM_None>::GetRHI() );

	// Determine if the entire destination surface is being resolved to.
	// If the entire surface is being resolved to, then it means we can clear it and signal the driver that it can discard
	// the surface's previous contents, which breaks dependencies between frames when using alternate-frame SLI.
	const bool					bClearDestSurface = InDestRect.x1 == 0 && InDestRect.y1 == 0 && InDestRect.x2 == InD3D11ResolveTargetDesc.Width && InDestRect.y2 == InD3D11ResolveTargetDesc.Height;

	if ( InD3D11ResolveTargetDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL )
	{
		// Clear the dest surface.
		if ( bClearDestSurface )
		{
			d3d11DeviceContext->ClearDepthStencilView( d3d11DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0, 0 );
		}

		g_RHI->SetDepthState( InDeviceContextRHI, TStaticDepthStateRHI<true, CF_Always>::GetRHI() );

		// Write to the dest surface as a depth-stencil target
		ID3D11RenderTargetView*			nullRTV = nullptr;
		d3d11DeviceContext->OMSetRenderTargets( 1, &nullRTV, d3d11DepthStencilView );
	}
	else
	{
		// Clear the dest surface.
		if ( bClearDestSurface )
		{
			CColor	clearColor = CColor::black;
			d3d11DeviceContext->ClearRenderTargetView( d3d11RenderTargetView, ( float* ) &clearColor.ToNormalizedVector4D() );
		}

		g_RHI->SetDepthState( InDeviceContextRHI, TStaticDepthStateRHI<false, CF_Always>::GetRHI() );

		// Write to the dest surface as a render target.
		d3d11DeviceContext->OMSetRenderTargets( 1, &d3d11RenderTargetView, nullptr );
	}

	D3D11_VIEWPORT		d3d11TempViewport;
	d3d11TempViewport.MinDepth	= 0.0f;
	d3d11TempViewport.MaxDepth	= 1.0f;
	d3d11TempViewport.TopLeftX	= 0;
	d3d11TempViewport.TopLeftY	= 0;
	d3d11TempViewport.Width		= InD3D11ResolveTargetDesc.Width;
	d3d11TempViewport.Height	= InD3D11ResolveTargetDesc.Height;
	d3d11DeviceContext->RSSetViewports( 1, &d3d11TempViewport );

	// Generate the vertices used to copy from the source surface to the destination surface.
	const float minU				= InSourceRect.x1;
	const float minV				= InSourceRect.y1;
	const float maxU				= InSourceRect.x2;
	const float maxV				= InSourceRect.y2;
	const float minX				= -1.f + InDestRect.x1 / ( ( float )InD3D11ResolveTargetDesc.Width	* 0.5f );
	const float minY				= +1.f - InDestRect.y1 / ( ( float )InD3D11ResolveTargetDesc.Height	* 0.5f );
	const float maxX				= -1.f + InDestRect.x2 / ( ( float )InD3D11ResolveTargetDesc.Width	* 0.5f );
	const float maxY				= +1.f - InDestRect.y2 / ( ( float )InD3D11ResolveTargetDesc.Height	* 0.5f );

	static BoundShaderStateRHIRef_t	resolveBoundShaderState;

	// Set the vertex and pixel shader
	CResolveVertexShader*			resolveVertexShader = g_ShaderManager->FindInstance<CResolveVertexShader, CSimpleElementVertexFactory>();
	CResolveDepthPixelShader*		resolvePixelShader	= g_ShaderManager->FindInstance<CResolveDepthPixelShader, CSimpleElementVertexFactory>();
	resolvePixelShader->SetUnresolvedSurface( InDeviceContextRHI, InSourceSurfaceRHI );
	
	if ( !resolveBoundShaderState )
	{
		resolveBoundShaderState = g_RHI->CreateBoundShaderState( TEXT( "Resolve" ), g_SimpleElementVertexDeclaration.GetVertexDeclarationRHI(), resolveVertexShader->GetVertexShader(), resolvePixelShader->GetPixelShader() );
		Assert( resolveBoundShaderState );
	}
	g_RHI->SetBoundShaderState( InDeviceContextRHI, resolveBoundShaderState );

	// Generate the vertices used
	SimpleElementVertexType	vertices[4];
	Memory::Memzero( vertices, sizeof( SimpleElementVertexType ) * 4 );

	vertices[0].position.x = maxX;
	vertices[0].position.y = minY;
	vertices[0].texCoord.x = maxU;
	vertices[0].texCoord.y = minV;

	vertices[1].position.x = maxX;
	vertices[1].position.y = maxY;
	vertices[1].texCoord.x = maxU;
	vertices[1].texCoord.y = maxV;

	vertices[2].position.x = minX;
	vertices[2].position.y = minY;
	vertices[2].texCoord.x = minU;
	vertices[2].texCoord.y = minV;

	vertices[3].position.x = minX;
	vertices[3].position.y = maxY;
	vertices[3].texCoord.x = minU;
	vertices[3].texCoord.y = maxV;

	g_RHI->DrawPrimitiveUP( InDeviceContextRHI, PT_TriangleStrip, 0, 2, vertices, sizeof( vertices[0] ) );

	// Reset saved render targets
	{
		const D3D11StateCache&		d3d11StateCache = ( ( CD3D11RHI* )g_RHI )->GetStateCache();
		d3d11DeviceContext->OMSetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, d3d11StateCache.renderTargetViews, d3d11StateCache.depthStencilView );
	}

	// Reset saved viewport
	d3d11DeviceContext->RSSetViewports( numSavedViewports, &d3d11SavedViewport );
}

/*
==================
GetDefaultRect
==================
*/
static FORCEINLINE ResolveRect GetDefaultRect( const ResolveRect& InRect, uint32 InDefaultWidth, uint32 InDefaultHeight )
{
	if ( InRect.x1 >= 0 && InRect.x2 >= 0 && InRect.y1 >= 0 && InRect.y2 >= 0 )
	{
		return InRect;
	}
	else
	{
		return ResolveRect( 0, 0, InDefaultWidth, InDefaultHeight );
	}
}

/*
==================
CD3D11RHI::CD3D11RHI
==================
*/
CD3D11RHI::CD3D11RHI() 
	: isInitialize( false )
	, immediateContext( nullptr )
	, globalConstantBuffer( nullptr )
	, psConstantBuffer( nullptr )
	, d3d11Device( nullptr )
{
	Memory::Memzero( vsConstantBuffers, sizeof( vsConstantBuffers ) );
}

/*
==================
CD3D11RHI::~CD3D11RHI
==================
*/
CD3D11RHI::~CD3D11RHI()
{
	Destroy();
}

/*
==================
CD3D11RHI::Init
==================
*/
void CD3D11RHI::Init( bool InIsEditor )
{
	if ( IsInitialize() )			return;

	uint32					deviceFlags = 0;

	// In Direct3D 11, if you are trying to create a hardware or a software device, set pAdapter != NULL which constrains the other inputs to be:
	//		DriverType must be D3D_DRIVER_TYPE_UNKNOWN 
	//		Software must be NULL. 
	D3D_DRIVER_TYPE			driverType = D3D_DRIVER_TYPE_UNKNOWN;

#if DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // DEBUG

	// Create DXGI factory and adapter
	HRESULT		result = CreateDXGIFactory( __uuidof( IDXGIFactory ), ( void** ) &dxgiFactory );
	Assert( result == S_OK );

	uint32			currentAdapter = 0;
	while ( dxgiFactory->EnumAdapters( currentAdapter, &dxgiAdapter ) == DXGI_ERROR_NOT_FOUND )
	{
		++currentAdapter;
	}
	AssertMsg( dxgiAdapter, TEXT( "GPU adapter not found" ) );

	D3D_FEATURE_LEVEL				maxFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	ID3D11DeviceContext*			d3d11DeviceContext = nullptr;
	result = D3D11CreateDevice( dxgiAdapter, driverType, nullptr, deviceFlags, &maxFeatureLevel, 1, D3D11_SDK_VERSION, &d3d11Device, &d3dFeatureLevel, &d3d11DeviceContext );
	Assert( result == S_OK );

	immediateContext = new CD3D11DeviceContext( d3d11DeviceContext );
	
	// Create constant buffers for shaders
	// Global constant buffer
	globalConstantBuffer = new class CD3D11ConstantBuffer( GConstantBufferSizes[ SOB_GlobalConstants ], TEXT( "GlobalConstantBuffer" ) );
	{
		ID3D11Buffer*		d3d11GlobalConstantBuffer = globalConstantBuffer->GetD3D11Buffer();
		d3d11DeviceContext->VSSetConstantBuffers( SOB_GlobalConstants, 1, &d3d11GlobalConstantBuffer );
		d3d11DeviceContext->PSSetConstantBuffers( SOB_GlobalConstants, 1, &d3d11GlobalConstantBuffer );
		d3d11DeviceContext->GSSetConstantBuffers( SOB_GlobalConstants, 1, &d3d11GlobalConstantBuffer );
		d3d11DeviceContext->CSSetConstantBuffers( SOB_GlobalConstants, 1, &d3d11GlobalConstantBuffer );
	}

	// Vertex constant buffer
	vsConstantBuffers[ SOB_ShaderConstants ] = new CD3D11ConstantBuffer( GConstantBufferSizes[ SOB_ShaderConstants ], TEXT( "ShaderConstantBuffer" ) );
	vsConstantBuffers[ SOB_GlobalConstants ] = nullptr;
	{
		for ( uint32 index = 0, num = ARRAY_COUNT( vsConstantBuffers ); index < num; ++index )
		{
			CD3D11ConstantBuffer*		constantBuffer = vsConstantBuffers[ index ];
			if ( constantBuffer )
			{
				ID3D11Buffer*		d3d11ConstantBuffer = constantBuffer->GetD3D11Buffer();
				d3d11DeviceContext->VSSetConstantBuffers( SOB_ShaderConstants, 1, &d3d11ConstantBuffer );
			}
		}
	}

	// Pixel constant buffer
	psConstantBuffer = new CD3D11ConstantBuffer( GConstantBufferSizes[ SOB_ShaderConstants ], TEXT( "ShaderConstantBuffer" ) );
	{
		ID3D11Buffer*		d3d11ConstantBuffer = psConstantBuffer->GetD3D11Buffer();
		d3d11DeviceContext->PSSetConstantBuffers( SOB_ShaderConstants, 1, &d3d11ConstantBuffer );
	}

	// Print info adapter
	DXGI_ADAPTER_DESC				adapterDesc;
	dxgiAdapter->GetDesc( &adapterDesc );

	Logf( TEXT( "Found D3D11 adapter: %s\n" ), adapterDesc.Description );
	Logf( TEXT( "Adapter has %uMB of dedicated video memory, %uMB of dedicated system memory, and %uMB of shared system memory\n" ),
			adapterDesc.DedicatedVideoMemory / ( 1024 * 1024 ),
			adapterDesc.DedicatedSystemMemory / ( 1024 * 1024 ),
			adapterDesc.SharedSystemMemory / ( 1024 * 1024 ) );

	g_PixelCenterOffset = 0.f;		// Note that in D3D11, there is no half-texel offset (ala DX9)

	// Initialize the platform pixel format map
	INIT_FORMAT( PF_A8R8G8B8,				DXGI_FORMAT_R8G8B8A8_UNORM );	
	INIT_FORMAT_FULL( PF_DepthStencil,		DXGI_FORMAT_R32G8X24_TYPELESS, 8 );
	INIT_FORMAT( PF_ShadowDepth,			DXGI_FORMAT_R16_TYPELESS );
	INIT_FORMAT( PF_FilteredShadowDepth,	DXGI_FORMAT_D32_FLOAT );
	INIT_FORMAT( PF_D32,					DXGI_FORMAT_R32_TYPELESS );
	INIT_FORMAT_FULL( PF_FloatRGB,			DXGI_FORMAT_R16G16B16A16_FLOAT, 8 );
	INIT_FORMAT_FULL( PF_FloatRGBA,			DXGI_FORMAT_R16G16B16A16_FLOAT, 8 );
	INIT_FORMAT( PF_R32F,					DXGI_FORMAT_R32_FLOAT );
	INIT_FORMAT( PF_BC1,					DXGI_FORMAT_BC1_UNORM );
	INIT_FORMAT( PF_BC2,					DXGI_FORMAT_BC2_UNORM );
	INIT_FORMAT( PF_BC3,					DXGI_FORMAT_BC3_UNORM );
	INIT_FORMAT( PF_BC5,					DXGI_FORMAT_BC5_UNORM );
	INIT_FORMAT( PF_BC6H,					DXGI_FORMAT_BC6H_UF16 );
	INIT_FORMAT( PF_BC7,					DXGI_FORMAT_BC7_UNORM );

	INIT_UNSUPPORTED_FORMAT( PF_Unknown );
	isInitialize = true;

	// Initialize all global render resources
	std::set< CRenderResource* >&			globalResourceList = CRenderResource::GetResourceList();
	for ( auto it = globalResourceList.begin(), itEnd = globalResourceList.end(); it != itEnd; ++it )
	{
		( *it )->InitResource();
	}
}

/*
==================
CD3D11RHI::AcquireThreadOwnership
==================
*/
void CD3D11RHI::AcquireThreadOwnership()
{}

/*
==================
CD3D11RHI::ReleaseThreadOwnership
==================
*/
void CD3D11RHI::ReleaseThreadOwnership()
{
}

/*
==================
CD3D11RHI::IsInitialize
==================
*/
bool CD3D11RHI::IsInitialize() const
{
	return isInitialize;
}

/*
==================
CD3D11RHI::Destroy
==================
*/
void CD3D11RHI::Destroy()
{
	if ( !isInitialize )		return;

	// Release all global render resources
	std::set<CRenderResource*>		globalResourceList = CRenderResource::GetResourceList();
	for ( auto it = globalResourceList.begin(), itEnd = globalResourceList.end(); it != itEnd; ++it )
	{
		( *it )->ReleaseResource();
	}

	for ( uint32 index = 0, num = ARRAY_COUNT( vsConstantBuffers ); index < num; ++index )
	{
		CD3D11ConstantBuffer*		constantBuffer = vsConstantBuffers[ index ];
		if ( constantBuffer )
		{
			delete constantBuffer;
		}
	}

	delete globalConstantBuffer;
	delete psConstantBuffer;
	delete immediateContext;
	d3d11Device->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();

	isInitialize = false;
	globalConstantBuffer = nullptr;
	psConstantBuffer = nullptr;
	immediateContext = nullptr;
	d3d11Device = nullptr;
	dxgiAdapter = nullptr;
	dxgiFactory = nullptr;

	stateCache.Reset();
	Memory::Memzero( vsConstantBuffers, sizeof( vsConstantBuffers ) );
}

/*
==================
CD3D11RHI::CreateViewport
==================
*/
ViewportRHIRef_t CD3D11RHI::CreateViewport( WindowHandle_t InWindowHandle, uint32 InWidth, uint32 InHeight )
{
	return new CD3D11Viewport( InWindowHandle, InWidth, InHeight );
}

/*
==================
CD3D11RHI::CreateViewport
==================
*/
ViewportRHIRef_t CD3D11RHI::CreateViewport( SurfaceRHIParamRef_t InSurfaceRHI, uint32 InWidth, uint32 InHeight )
{
	return new CD3D11Viewport( InSurfaceRHI, InWidth, InHeight );
}

/*
==================
CD3D11RHI::CreateVertexShader
==================
*/
VertexShaderRHIRef_t CD3D11RHI::CreateVertexShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11VertexShaderRHI( InData, InSize, InShaderName );
}

/*
==================
CD3D11RHI::CreateHullShader
==================
*/
HullShaderRHIRef_t CD3D11RHI::CreateHullShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11HullShaderRHI( InData, InSize, InShaderName );
}

/*
==================
CD3D11RHI::CreateDomainShader
==================
*/
DomainShaderRHIRef_t CD3D11RHI::CreateDomainShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11DomainShaderRHI( InData, InSize, InShaderName );
}

/*
==================
CD3D11RHI::CreatePixelShader
==================
*/
PixelShaderRHIRef_t CD3D11RHI::CreatePixelShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11PixelShaderRHI( InData, InSize, InShaderName );
}

/*
==================
CD3D11RHI::CreateGeometryShader
==================
*/
GeometryShaderRHIRef_t CD3D11RHI::CreateGeometryShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11GeometryShaderRHI( InData, InSize, InShaderName );
}

/*
==================
CD3D11RHI::CreateVertexBuffer
==================
*/
VertexBufferRHIRef_t CD3D11RHI::CreateVertexBuffer( const tchar* InBufferName, uint32 InSize, const byte* InData, uint32 InUsage )
{
	return new CD3D11VertexBufferRHI( InUsage, InSize, InData, InBufferName );
}

/*
==================
CD3D11RHI::CreateIndexBuffer
==================
*/
IndexBufferRHIRef_t CD3D11RHI::CreateIndexBuffer( const tchar* InBufferName, uint32 InStride, uint32 InSize, const byte* InData, uint32 InUsage )
{
	return new CD3D11IndexBufferRHI( InUsage, InStride, InSize, InData, InBufferName );
}

/*
==================
CD3D11RHI::CreateVertexDeclaration
==================
*/
VertexDeclarationRHIRef_t CD3D11RHI::CreateVertexDeclaration( const VertexDeclarationElementList_t& InElementList )
{
	return new CD3D11VertexDeclarationRHI( InElementList );
}

/*
==================
CD3D11RHI::CreateBoundShaderState
==================
*/
BoundShaderStateRHIRef_t CD3D11RHI::CreateBoundShaderState( const tchar* InBoundShaderStateName, VertexDeclarationRHIRef_t InVertexDeclaration, VertexShaderRHIRef_t InVertexShader, PixelShaderRHIRef_t InPixelShader, HullShaderRHIRef_t InHullShader /*= nullptr*/, DomainShaderRHIRef_t InDomainShader /*= nullptr*/, GeometryShaderRHIRef_t InGeometryShader /*= nullptr*/ )
{
	CBoundShaderStateKey		key( InVertexDeclaration, InVertexShader, InPixelShader, InHullShader, InDomainShader, InGeometryShader );
	BoundShaderStateRHIRef_t		boundShaderStateRHI = boundShaderStateHistory.Find( key );
	if ( !boundShaderStateRHI )
	{
		boundShaderStateRHI = new CD3D11BoundShaderStateRHI( InBoundShaderStateName, key, InVertexDeclaration, InVertexShader, InPixelShader, InHullShader, InDomainShader, InGeometryShader );
		boundShaderStateHistory.Add( key, boundShaderStateRHI );
	}

	return boundShaderStateRHI;
}

/*
==================
CD3D11RHI::CreateRasterizerState
==================
*/
RasterizerStateRHIRef_t CD3D11RHI::CreateRasterizerState( const RasterizerStateInitializerRHI& InInitializer )
{
	return new CD3D11RasterizerStateRHI( InInitializer );
}

/*
==================
CD3D11RHI::CreateSamplerState
==================
*/
SamplerStateRHIRef_t CD3D11RHI::CreateSamplerState( const SSamplerStateInitializerRHI& InInitializer )
{
	return new CD3D11SamplerStateRHI( InInitializer );
}

/*
==================
CD3D11RHI::CreateDepthState
==================
*/
DepthStateRHIRef_t CD3D11RHI::CreateDepthState( const DepthStateInitializerRHI& InInitializer )
{
	return new CD3D11DepthStateRHI( InInitializer );
}

/*
==================
CD3D11RHI::CreateBlendState
==================
*/
BlendStateRHIRef_t CD3D11RHI::CreateBlendState( const BlendStateInitializerRHI& InInitializer )
{
	return new CD3D11BlendStateRHI( InInitializer );
}

/*
==================
CD3D11RHI::CreateStencilState
==================
*/
StencilStateRHIRef_t CD3D11RHI::CreateStencilState( const StencilStateInitializerRHI& InInitializer )
{
	return new CD3D11StencilStateRHI( InInitializer );
}

/*
==================
CD3D11RHI::CreateTexture2D
==================
*/
Texture2DRHIRef_t CD3D11RHI::CreateTexture2D( const tchar* InDebugName, uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InFlags, void* InData /*= nullptr*/ )
{
	return new CD3D11Texture2DRHI( InDebugName, InSizeX, InSizeY, InNumMips, InFormat, InFlags, InData );
}

/*
==================
CD3D11RHI::CreateTargetableSurface
==================
*/
SurfaceRHIRef_t CD3D11RHI::CreateTargetableSurface( const tchar* InDebugName, uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, Texture2DRHIParamRef_t InResolveTargetTexture, uint32 InFlags )
{
	return new CD3D11Surface( InResolveTargetTexture );
}

/*
==================
CD3D11RHI::GetImmediateContext
==================
*/
class CBaseDeviceContextRHI* CD3D11RHI::GetImmediateContext() const
{
	return immediateContext;
}

/*
==================
CD3D11RHI::GetViewport
==================
*/
void CD3D11RHI::GetViewport( uint32& OutMinX, uint32& OutMinY, float& OutMinZ, uint32& OutMaxX, uint32& OutMaxY, float& OutMaxZ ) const
{
	OutMinX = stateCache.viewport.TopLeftX;
	OutMinY = stateCache.viewport.TopLeftY;
	OutMaxX = stateCache.viewport.Width + OutMinX;
	OutMaxY = stateCache.viewport.Height + OutMinY;
	OutMinZ = stateCache.viewport.MinDepth;
	OutMaxZ = stateCache.viewport.MaxDepth;
}

/*
==================
CD3D11RHI::SetupInstancing
==================
*/
void CD3D11RHI::SetupInstancing( class CBaseDeviceContextRHI* InDeviceContext, uint32 InStreamIndex, void* InInstanceData, uint32 InInstanceStride, uint32 InInstanceSize, uint32 InNumInstances )
{
	if ( !instanceBuffer || instanceBuffer->GetSize() < InInstanceSize )
	{
		instanceBuffer = new CD3D11VertexBufferRHI( RUF_Dynamic, InInstanceSize, ( byte* )InInstanceData, TEXT( "Instance" ) );
	}
	else
	{
		LockedData		lockedData;
		LockVertexBuffer( InDeviceContext, instanceBuffer, InInstanceSize, 0, lockedData );
		Memory::Memcpy( lockedData.data, InInstanceData, InInstanceSize );
		UnlockVertexBuffer( InDeviceContext, instanceBuffer, lockedData );
	}

	SetStreamSource( InDeviceContext, InStreamIndex, instanceBuffer, InInstanceStride, 0 );
}

/*
==================
CD3D11RHI::SetViewport
==================
*/
void CD3D11RHI::SetViewport( class CBaseDeviceContextRHI* InDeviceContext, uint32 InMinX, uint32 InMinY, float InMinZ, uint32 InMaxX, uint32 InMaxY, float InMaxZ )
{
	D3D11_VIEWPORT			d3d11Viewport = { ( float )InMinX, ( float )InMinY, ( float )InMaxX - InMinX, ( float )InMaxY - InMinY, ( float )InMinZ, InMaxZ };
	if ( d3d11Viewport.Width > 0 && d3d11Viewport.Height > 0 && d3d11Viewport != stateCache.viewport )
	{
		static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->RSSetViewports( 1, &d3d11Viewport );
		stateCache.viewport = d3d11Viewport;
	}
}

/*
==================
CD3D11RHI::SetBoundShaderState
==================
*/
void CD3D11RHI::SetBoundShaderState( class CBaseDeviceContextRHI* InDeviceContext, BoundShaderStateRHIParamRef_t InBoundShaderState )
{
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	CD3D11BoundShaderStateRHI*		boundShaderState = ( CD3D11BoundShaderStateRHI* )InBoundShaderState;

	// Bind input layout
	{
		ID3D11InputLayout*			d3d11InputLayout = boundShaderState ? boundShaderState->GetD3D11InputLayout() : nullptr;
		if ( d3d11InputLayout != stateCache.inputLayout )
		{
			d3d11DeviceContext->IASetInputLayout( d3d11InputLayout );
			stateCache.inputLayout = d3d11InputLayout;
		}
	}

	// Bind vertex shader
	{
		CD3D11VertexShaderRHI*		vertexShader		= boundShaderState ? ( CD3D11VertexShaderRHI* )boundShaderState->GetVertexShader().GetPtr() : nullptr;
		ID3D11VertexShader*			d3d11VertexShader	= vertexShader ? vertexShader->GetResource() : nullptr;
		if ( d3d11VertexShader != stateCache.vertexShader )
		{
			d3d11DeviceContext->VSSetShader( d3d11VertexShader, nullptr, 0 );
			stateCache.vertexShader = d3d11VertexShader;
		}
	}

	// Bind pixel shader
	{
		CD3D11PixelShaderRHI*		pixelShader			= boundShaderState ? ( CD3D11PixelShaderRHI* )boundShaderState->GetPixelShader().GetPtr() : nullptr;
		ID3D11PixelShader*			d3d11PixelShader	= pixelShader ? pixelShader->GetResource() : nullptr;
		if ( d3d11PixelShader != stateCache.pixelShader )
		{
			d3d11DeviceContext->PSSetShader( d3d11PixelShader, nullptr, 0 );
			stateCache.pixelShader = d3d11PixelShader;
		}
	}

	// Bind geometry shader
	{
		CD3D11GeometryShaderRHI*	geometryShader		= boundShaderState ? ( CD3D11GeometryShaderRHI* )boundShaderState->GetGeometryShader().GetPtr() : nullptr;
		ID3D11GeometryShader*		d3d11GeometryShader = geometryShader ? geometryShader->GetResource() : nullptr;
		if ( d3d11GeometryShader != stateCache.geometryShader )
		{
			d3d11DeviceContext->GSSetShader( d3d11GeometryShader, nullptr, 0 );
			stateCache.geometryShader = d3d11GeometryShader;
		}
	}

	// Bind hull shader
	{
		CD3D11HullShaderRHI*		hullShader			= boundShaderState ? ( CD3D11HullShaderRHI* )boundShaderState->GetHullShader().GetPtr() : nullptr;
		ID3D11HullShader*			d3d11HullShader		= hullShader ? hullShader->GetResource() : nullptr;
		if ( d3d11HullShader != stateCache.hullShader )
		{
			d3d11DeviceContext->HSSetShader( d3d11HullShader, nullptr, 0 );
			stateCache.hullShader = d3d11HullShader;
		}
	}

	// Bind domain shader
	{
		CD3D11DomainShaderRHI*		domainShader		= boundShaderState ? ( CD3D11DomainShaderRHI* )boundShaderState->GetDomainShader().GetPtr() : nullptr;
		ID3D11DomainShader*			d3d11DomainShader	= domainShader ? domainShader->GetResource() : nullptr;
		if ( d3d11DomainShader != stateCache.domainShader )
		{
			d3d11DeviceContext->DSSetShader( d3d11DomainShader, nullptr, 0 );
			stateCache.domainShader = d3d11DomainShader;
		}
	}
}

/*
==================
CD3D11RHI::SetStreamSource
==================
*/
void CD3D11RHI::SetStreamSource( class CBaseDeviceContextRHI* InDeviceContext, uint32 InStreamIndex, VertexBufferRHIParamRef_t InVertexBuffer, uint32 InStride, uint32 InOffset )
{
	ID3D11DeviceContext*			d3d11DeviceContext		= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	ID3D11Buffer*					d3d11Buffer				= InVertexBuffer ? ( ( CD3D11VertexBufferRHI* )InVertexBuffer )->GetD3D11Buffer() : nullptr;
	CD3D11StateVertexBuffer			stateVertexBuffer		= { d3d11Buffer, InStride, InOffset };

	if ( stateVertexBuffer != stateCache.vertexBuffers[ InStreamIndex ] )
	{
		d3d11DeviceContext->IASetVertexBuffers( InStreamIndex, 1, &d3d11Buffer, &InStride, &InOffset );
		stateCache.vertexBuffers[ InStreamIndex ] = stateVertexBuffer;
	}
}

/*
==================
CD3D11RHI::SetRasterizerState
==================
*/
void CD3D11RHI::SetRasterizerState( class CBaseDeviceContextRHI* InDeviceContext, RasterizerStateRHIParamRef_t InNewState )
{
	ID3D11DeviceContext*			d3d11DeviceContext		= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	ID3D11RasterizerState*			d3d11RasterizerState	= InNewState ? ( ( CD3D11RasterizerStateRHI* )InNewState )->GetResource() : nullptr;

	if ( d3d11RasterizerState != stateCache.rasterizerState )
	{
		d3d11DeviceContext->RSSetState( d3d11RasterizerState );
		stateCache.rasterizerState = d3d11RasterizerState;
	}
}

/*
==================
CD3D11RHI::SetSamplerState
==================
*/
void CD3D11RHI::SetSamplerState( class CBaseDeviceContextRHI* InDeviceContext, PixelShaderRHIParamRef_t InPixelShader, SamplerStateRHIParamRef_t InNewState, uint32 InStateIndex )
{
	ID3D11DeviceContext*			d3d11DeviceContext		= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	ID3D11SamplerState*				d3d11SamplerState		= InNewState ? ( ( CD3D11SamplerStateRHI* )InNewState )->GetResource() : nullptr;
	
	if ( d3d11SamplerState != stateCache.psSamplerStates[ InStateIndex ] )
	{
		d3d11DeviceContext->PSSetSamplers( InStateIndex, 1, &d3d11SamplerState );
		stateCache.psSamplerStates[ InStateIndex ] = d3d11SamplerState;
	}
}

/*
==================
CD3D11RHI::SetTextureParameter
==================
*/
void CD3D11RHI::SetTextureParameter( class CBaseDeviceContextRHI* InDeviceContext, PixelShaderRHIParamRef_t InPixelShader, TextureRHIParamRef_t InTexture, uint32 InTextureIndex )
{
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* ) InDeviceContext )->GetD3D11DeviceContext();
	ID3D11ShaderResourceView*		d3d11ShaderResourceView = InTexture ? ( ( CD3D11TextureRHI* )InTexture )->GetShaderResourceView() : nullptr;		// BS yehor.pohuliaka - Nullptr shader resource view is probably a code mistake

	if ( d3d11ShaderResourceView != stateCache.psShaderResourceViews[ InTextureIndex ] )
	{
		d3d11DeviceContext->PSSetShaderResources( InTextureIndex, 1, &d3d11ShaderResourceView );
		stateCache.psShaderResourceViews[ InTextureIndex ] = d3d11ShaderResourceView;
	}
}

/*
==================
CD3D11RHI::SetViewParameters
==================
*/
void CD3D11RHI::SetViewParameters( class CBaseDeviceContextRHI* InDeviceContext, class CSceneView& InSceneView )
{
	Assert( InDeviceContext );

	SGlobalConstantBufferContents			globalContents;
	SetGlobalConstants( globalContents, InSceneView, Vector4D( InSceneView.GetSizeX(), InSceneView.GetSizeY(), g_SceneRenderTargets.GetBufferWidth(), g_SceneRenderTargets.GetBufferHeight() ) );

	globalConstantBuffer->Update( ( byte* ) &globalContents, 0, sizeof( globalContents ) );
	globalConstantBuffer->CommitConstantsToDevice( ( CD3D11DeviceContext* )InDeviceContext );
}

/*
==================
CD3D11RHI::SetVertexShaderParameter
==================
*/
void CD3D11RHI::SetVertexShaderParameter( class CBaseDeviceContextRHI* InDeviceContext, uint32 InBufferIndex, uint32 InBaseIndex, uint32 InNumBytes, const void* InNewValue )
{
	vsConstantBuffers[ InBufferIndex ]->Update( ( const byte* )InNewValue, InBaseIndex, InNumBytes );
}

/*
==================
CD3D11RHI::SetPixelShaderParameter
==================
*/
void CD3D11RHI::SetPixelShaderParameter( class CBaseDeviceContextRHI* InDeviceContext, uint32 InBufferIndex, uint32 InBaseIndex, uint32 InNumBytes, const void* InNewValue )
{
	psConstantBuffer->Update( ( const byte* )InNewValue, InBaseIndex, InNumBytes );
}

/*
==================
CD3D11RHI::SetDepthState
==================
*/
void CD3D11RHI::SetDepthState( class CBaseDeviceContextRHI* InDeviceContext, DepthStateRHIParamRef_t InNewState )
{
	ID3D11DeviceContext*			d3d11DeviceContext	= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	D3D11_DEPTH_STENCIL_DESC		d3d11DepthStateDesc;
	
	if ( InNewState )
	{
		d3d11DepthStateDesc = ( ( CD3D11DepthStateRHI* )InNewState )->GetInfo();
	}
	else
	{
		Memory::Memzero( &d3d11DepthStateDesc, sizeof( D3D11_DEPTH_STENCIL_DESC ) );
	}

	if ( Memory::Memcmp( &d3d11DepthStateDesc, &stateCache.depthState, sizeof( D3D11_DEPTH_STENCIL_DESC ) ) )
	{
		d3d11DeviceContext->OMSetDepthStencilState( GetCachedDepthStencilState( d3d11DepthStateDesc, stateCache.stencilState ), stateCache.stencilRef );
		stateCache.depthState = d3d11DepthStateDesc;
	}
}

/*
==================
CD3D11RHI::SetBlendState
==================
*/
void CD3D11RHI::SetBlendState( class CBaseDeviceContextRHI* InDeviceContext, BlendStateRHIParamRef_t InNewState )
{
	ID3D11DeviceContext*			d3d11DeviceContext	= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	D3D11_BLEND_DESC				d3d11BlendStateDesc;

	if ( InNewState )
	{
		d3d11BlendStateDesc = ( ( CD3D11BlendStateRHI* )InNewState )->GetInfo();
	}
	else
	{
		Memory::Memzero( &d3d11BlendStateDesc, sizeof( D3D11_BLEND_DESC ) );
	}

	if ( Memory::Memcmp( &d3d11BlendStateDesc, &stateCache.blendState, sizeof( D3D11_BLEND_DESC ) ) )
	{
		float blendFactor[4]	= { 0.f, 0.f, 0.f, 0.f };
		d3d11DeviceContext->OMSetBlendState( GetCachedBlendState( d3d11BlendStateDesc, stateCache.colorWriteMasks ), blendFactor, 0xFFFFFFFF );
		stateCache.blendState = d3d11BlendStateDesc;
	}
}

/*
==================
CD3D11RHI::SetColorWriteEnable
==================
*/
void CD3D11RHI::SetColorWriteEnable( class CBaseDeviceContextRHI* InDeviceContext, bool InIsEnable )
{
	ID3D11DeviceContext*	d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	uint8					d3d11ColorWriteMask = InIsEnable ? D3D11_COLOR_WRITE_ENABLE_ALL : 0;
	
	if ( d3d11ColorWriteMask != stateCache.colorWriteMasks[0] )
	{
		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
		
		stateCache.colorWriteMasks[0] = d3d11ColorWriteMask;
		d3d11DeviceContext->OMSetBlendState( GetCachedBlendState( stateCache.blendState, stateCache.colorWriteMasks ), blendFactor, 0xFFFFFFFF );
	}
}

/*
==================
CD3D11RHI::SetMRTColorWriteEnable
==================
*/
void CD3D11RHI::SetMRTColorWriteEnable( class CBaseDeviceContextRHI* InDeviceContext, bool InIsEnable, uint32 InTargetIndex )
{
	Assert( InTargetIndex < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT );
	ID3D11DeviceContext*	d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	uint8					d3d11ColorWriteMask = InIsEnable ? D3D11_COLOR_WRITE_ENABLE_ALL : 0;
	
	if ( d3d11ColorWriteMask != stateCache.colorWriteMasks[InTargetIndex] )
	{
		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
		
		stateCache.colorWriteMasks[InTargetIndex] = d3d11ColorWriteMask;
		d3d11DeviceContext->OMSetBlendState( GetCachedBlendState( stateCache.blendState, stateCache.colorWriteMasks ), blendFactor, 0xFFFFFFFF );
	}
}

/*
==================
CD3D11RHI::SetColorWriteMask
==================
*/
void CD3D11RHI::SetColorWriteMask( class CBaseDeviceContextRHI* InDeviceContext, uint8 InColorWriteMask )
{
	ID3D11DeviceContext*	d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	uint8					d3d11ColorWriteMask = 0;
	d3d11ColorWriteMask		= ( InColorWriteMask & CW_Red ) ? D3D11_COLOR_WRITE_ENABLE_RED : 0;
	d3d11ColorWriteMask		|= ( InColorWriteMask & CW_Green ) ? D3D11_COLOR_WRITE_ENABLE_GREEN : 0;
	d3d11ColorWriteMask		|= ( InColorWriteMask & CW_Blue ) ? D3D11_COLOR_WRITE_ENABLE_BLUE : 0;
	d3d11ColorWriteMask		|= ( InColorWriteMask & CW_Alpha ) ? D3D11_COLOR_WRITE_ENABLE_ALPHA : 0;

	if ( d3d11ColorWriteMask != stateCache.colorWriteMasks[0] )
	{
		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };

		stateCache.colorWriteMasks[0] = d3d11ColorWriteMask;
		d3d11DeviceContext->OMSetBlendState( GetCachedBlendState( stateCache.blendState, stateCache.colorWriteMasks ), blendFactor, 0xFFFFFFFF );
	}
}

/*
==================
CD3D11RHI::SetMRTColorWriteMask
==================
*/
void CD3D11RHI::SetMRTColorWriteMask( class CBaseDeviceContextRHI* InDeviceContext, uint8 InColorWriteMask, uint32 InTargetIndex )
{
	Assert( InTargetIndex < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT );
	ID3D11DeviceContext*	d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	uint8					d3d11ColorWriteMask = 0;
	d3d11ColorWriteMask		= ( InColorWriteMask & CW_Red ) ? D3D11_COLOR_WRITE_ENABLE_RED : 0;
	d3d11ColorWriteMask		|= ( InColorWriteMask & CW_Green ) ? D3D11_COLOR_WRITE_ENABLE_GREEN : 0;
	d3d11ColorWriteMask		|= ( InColorWriteMask & CW_Blue ) ? D3D11_COLOR_WRITE_ENABLE_BLUE : 0;
	d3d11ColorWriteMask		|= ( InColorWriteMask & CW_Alpha ) ? D3D11_COLOR_WRITE_ENABLE_ALPHA : 0;

	if ( d3d11ColorWriteMask != stateCache.colorWriteMasks[InTargetIndex] )
	{
		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };

		stateCache.colorWriteMasks[InTargetIndex] = d3d11ColorWriteMask;
		d3d11DeviceContext->OMSetBlendState( GetCachedBlendState( stateCache.blendState, stateCache.colorWriteMasks ), blendFactor, 0xFFFFFFFF );
	}
}

/*
==================
CD3D11RHI::SetStencilState
==================
*/
void CD3D11RHI::SetStencilState( class CBaseDeviceContextRHI* InDeviceContext, StencilStateRHIParamRef_t InNewState )
{
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	D3D11_DEPTH_STENCIL_DESC		d3d1StencilStateDesc;
	uint32							stencilRef = 0;

	if ( InNewState )
	{
		d3d1StencilStateDesc	= ( ( CD3D11StencilStateRHI* )InNewState )->GetInfo();
		stencilRef				= ( ( CD3D11StencilStateRHI* )InNewState )->GetStencilRef();
	}
	else
	{
		Memory::Memzero( &d3d1StencilStateDesc, sizeof( D3D11_DEPTH_STENCIL_DESC ) );
	}

	if ( Memory::Memcmp( &d3d1StencilStateDesc, &stateCache.stencilState, sizeof( D3D11_DEPTH_STENCIL_DESC ) ) || stencilRef != stateCache.stencilRef )
	{
		d3d11DeviceContext->OMSetDepthStencilState( GetCachedDepthStencilState( stateCache.depthState, d3d1StencilStateDesc ), stencilRef );	
		stateCache.stencilState = d3d1StencilStateDesc;
		stateCache.stencilRef	= stencilRef;
	}
}

/*
==================
CD3D11RHI::CommitConstants
==================
*/
void CD3D11RHI::CommitConstants( class CBaseDeviceContextRHI* InDeviceContext )
{
	// Commit vertex shader constants
	for ( uint32 index = 0, num = ARRAY_COUNT( vsConstantBuffers ); index < num; ++index )
	{
		CD3D11ConstantBuffer*		constantBuffer = vsConstantBuffers[ index ];
		if ( constantBuffer )
		{
			constantBuffer->CommitConstantsToDevice( ( CD3D11DeviceContext* )InDeviceContext );
		}
	}

	// Commit pixel shader constants
	psConstantBuffer->CommitConstantsToDevice( ( CD3D11DeviceContext* )InDeviceContext );
}

/*
==================
CD3D11RHI::LockVertexBuffer
==================
*/
void CD3D11RHI::LockVertexBuffer( class CBaseDeviceContextRHI* InDeviceContext, const VertexBufferRHIRef_t InVertexBuffer, uint32 InSize, uint32 InOffset, LockedData& OutLockedData )
{
	Assert( OutLockedData.data == nullptr && InOffset < InSize );

	D3D11_MAP						writeMode = D3D11_MAP_WRITE_DISCARD;
	D3D11_MAPPED_SUBRESOURCE		mappedSubresource;
	
	static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->Map( static_cast< CD3D11VertexBufferRHI* >( InVertexBuffer.GetPtr() )->GetD3D11Buffer(), 0, writeMode, 0, &mappedSubresource );
	OutLockedData.data			= ( byte* )mappedSubresource.pData + InOffset;
	OutLockedData.pitch			= mappedSubresource.RowPitch;
	OutLockedData.isNeedFree	= false;
}

/*
==================
CD3D11RHI::UnlockVertexBuffer
==================
*/
void CD3D11RHI::UnlockVertexBuffer( class CBaseDeviceContextRHI* InDeviceContext, const VertexBufferRHIRef_t InVertexBuffer, LockedData& InLockedData )
{
	Assert( InLockedData.data );

	static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->Unmap( static_cast< CD3D11VertexBufferRHI* >( InVertexBuffer.GetPtr() )->GetD3D11Buffer(), 0 );
	InLockedData.data = nullptr;
}

/*
==================
CD3D11RHI::LockIndexBuffer
==================
*/
void CD3D11RHI::LockIndexBuffer( class CBaseDeviceContextRHI* InDeviceContext, const IndexBufferRHIRef_t InIndexBuffer, uint32 InSize, uint32 InOffset, LockedData& OutLockedData )
{
	Assert( OutLockedData.data == nullptr && InOffset < InSize );

	D3D11_MAP						writeMode = D3D11_MAP_WRITE_DISCARD;
	D3D11_MAPPED_SUBRESOURCE		mappedSubresource;

	static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->Map( static_cast< CD3D11IndexBufferRHI* >( InIndexBuffer.GetPtr() )->GetD3D11Buffer(), 0, writeMode, 0, &mappedSubresource );
	OutLockedData.data = ( byte* )mappedSubresource.pData + InOffset;
	OutLockedData.pitch = mappedSubresource.RowPitch;
	OutLockedData.isNeedFree = false;
}

/*
==================
CD3D11RHI::UnlockIndexBuffer
==================
*/
void CD3D11RHI::UnlockIndexBuffer( class CBaseDeviceContextRHI* InDeviceContext, const IndexBufferRHIRef_t InIndexBuffer, LockedData& InLockedData )
{
	Assert( InLockedData.data );

	static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->Unmap( static_cast< CD3D11IndexBufferRHI* >( InIndexBuffer.GetPtr() )->GetD3D11Buffer(), 0 );
	InLockedData.data = nullptr;
}

/*
==================
CD3D11RHI::LockTexture2D
==================
*/
void CD3D11RHI::LockTexture2D( class CBaseDeviceContextRHI* InDeviceContext, Texture2DRHIParamRef_t InTexture, uint32 InMipIndex, bool InIsDataWrite, LockedData& OutLockedData, bool InIsUseCPUShadow /*= false*/ )
{
	( ( CD3D11Texture2DRHI* )InTexture )->Lock( InDeviceContext, InMipIndex, InIsDataWrite, InIsUseCPUShadow, OutLockedData );
}

/*
==================
CD3D11RHI::UnlockTexture2D
==================
*/
void CD3D11RHI::UnlockTexture2D( class CBaseDeviceContextRHI* InDeviceContext, Texture2DRHIParamRef_t InTexture, uint32 InMipIndex, LockedData& InLockedData )
{
	( ( CD3D11Texture2DRHI* )InTexture )->Unlock( InDeviceContext, InMipIndex, InLockedData );
}

/*
==================
CD3D11RHI::DrawPrimitive
==================
*/
void CD3D11RHI::DrawPrimitive( class CBaseDeviceContextRHI* InDeviceContext, EPrimitiveType InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances /* = 1 */ )
{
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	uint32							vertexCount = GetVertexCountForPrimitiveCount( InNumPrimitives, InPrimitiveType );

	// Set primitive topology
	{
		D3D11_PRIMITIVE_TOPOLOGY		d3d11PrimitiveTopology = GetD3D11PrimitiveType( InPrimitiveType, false );
		if ( d3d11PrimitiveTopology != stateCache.primitiveTopology )
		{
			d3d11DeviceContext->IASetPrimitiveTopology( d3d11PrimitiveTopology );
			stateCache.primitiveTopology = d3d11PrimitiveTopology;
		}
	}

	// Draw primitive
	if ( InNumInstances > 1 )
	{
		d3d11DeviceContext->DrawInstanced( vertexCount, InNumInstances, InBaseVertexIndex, 0 );
	}
	else
	{
		d3d11DeviceContext->Draw( vertexCount, InBaseVertexIndex );
	}
}

/*
==================
CD3D11RHI::DrawIndexedPrimitive
==================
*/
void CD3D11RHI::DrawIndexedPrimitive( class CBaseDeviceContextRHI* InDeviceContext, class CBaseIndexBufferRHI* InIndexBuffer, EPrimitiveType InPrimitiveType, uint32 InBaseVertexIndex, uint32 InStartIndex, uint32 InNumPrimitives, uint32 InNumInstances /* = 1 */ )
{
	Assert( InIndexBuffer );
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	CD3D11IndexBufferRHI*			indexBuffer = ( CD3D11IndexBufferRHI* )InIndexBuffer;

	// Bind index buffer
	{
		const DXGI_FORMAT				format = ( indexBuffer->GetStride() == sizeof( uint16 ) ) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		ID3D11Buffer*					d3d11IndexBuffer = indexBuffer->GetD3D11Buffer();
		CD3D11StateIndexBuffer			stateIndexBuffer = { d3d11IndexBuffer, format, 0 };
		
		if ( stateIndexBuffer != stateCache.indexBuffer )
		{
			d3d11DeviceContext->IASetIndexBuffer( d3d11IndexBuffer, format, 0 );
			stateCache.indexBuffer = stateIndexBuffer;
		}
	}

	// Set primitive topology
	{
		D3D11_PRIMITIVE_TOPOLOGY		d3d11PrimitiveTopology = GetD3D11PrimitiveType( InPrimitiveType, false );
		if ( d3d11PrimitiveTopology != stateCache.primitiveTopology )
		{
			d3d11DeviceContext->IASetPrimitiveTopology( d3d11PrimitiveTopology );
			stateCache.primitiveTopology = d3d11PrimitiveTopology;
		}
	}

	// Draw indexed primitive	
	uint32							indexCount = GetVertexCountForPrimitiveCount( InNumPrimitives, InPrimitiveType );
	if ( InNumInstances > 1 )
	{
		d3d11DeviceContext->DrawIndexedInstanced( indexCount, InNumInstances, InStartIndex, InBaseVertexIndex, 0 );
	}
	else
	{
		d3d11DeviceContext->DrawIndexed( indexCount, InStartIndex, InBaseVertexIndex );
	}
}

/*
==================
CD3D11RHI::DrawPrimitiveUP
==================
*/
void CD3D11RHI::DrawPrimitiveUP( class CBaseDeviceContextRHI* InDeviceContext, EPrimitiveType InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, const void* InVertexData, uint32 InVertexDataStride, uint32 InNumInstances /* = 1 */ )
{
	uint32										vertexCount		= GetVertexCountForPrimitiveCount( InNumPrimitives, InPrimitiveType );
	TRefCountPtr< CD3D11VertexBufferRHI >		vertexBuffer	= CreateVertexBuffer( TEXT( "DrawPrimitiveUP" ), InVertexDataStride * vertexCount, ( const byte* )InVertexData, RUF_Static );
	
	SetStreamSource( InDeviceContext, 0, vertexBuffer, InVertexDataStride, 0 );
	DrawPrimitive( InDeviceContext, InPrimitiveType, InBaseVertexIndex, InNumPrimitives, InNumInstances );
}

/*
==================
CD3D11RHI::DrawIndexedPrimitiveUP
==================
*/
void CD3D11RHI::DrawIndexedPrimitiveUP( class CBaseDeviceContextRHI* InDeviceContext, EPrimitiveType InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumVertices, const void* InIndexData, uint32 InIndexDataStride, const void* InVertexData, uint32 InVertexDataStride, uint32 InNumInstances /* = 1 */ )
{
	uint32										indexCount		= GetVertexCountForPrimitiveCount( InNumPrimitives, InPrimitiveType );
	TRefCountPtr< CD3D11VertexBufferRHI >		vertexBuffer	= CreateVertexBuffer( TEXT( "DrawIndexedPrimitiveUP" ), InVertexDataStride * InNumVertices, ( const byte* )InVertexData, RUF_Static );
	TRefCountPtr< CD3D11IndexBufferRHI >		indexBuffer		= CreateIndexBuffer( TEXT( "DrawIndexedPrimitiveUP" ), InIndexDataStride, InIndexDataStride * indexCount, ( const byte* )InIndexData, RUF_Static );

	SetStreamSource( InDeviceContext, 0, vertexBuffer, InVertexDataStride, 0 );
	DrawIndexedPrimitive( InDeviceContext, indexBuffer, InPrimitiveType, InBaseVertexIndex, 0, InNumPrimitives, InNumInstances );
}

/*
==================
CD3D11RHI::CopyToResolveTarget
==================
*/
void CD3D11RHI::CopyToResolveTarget( class CBaseDeviceContextRHI* InDeviceContext, SurfaceRHIParamRef_t InSourceSurface, const ResolveParams& InResolveParams )
{
	ID3D11DeviceContext*	d3d11DeviceContext		= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	CD3D11Surface*			resolveTargetSurface	= InResolveParams.resolveTargetSurface ? ( CD3D11Surface* )InResolveParams.resolveTargetSurface : nullptr;
	CD3D11Texture2DRHI*		resolveTarget2D			= InResolveParams.resolveTarget ? ( CD3D11Texture2DRHI* )InResolveParams.resolveTarget : nullptr;
	Assert( resolveTargetSurface || resolveTarget2D );

	ID3D11Texture2D*		d3d11SourceSurface		= ( ( CD3D11Surface* )InSourceSurface )->GetResource();
	ID3D11Texture2D*		d3d11ResolveTarget		= resolveTargetSurface ? resolveTargetSurface->GetResource() : resolveTarget2D->GetResource();
	if ( d3d11SourceSurface && d3d11ResolveTarget && d3d11SourceSurface != d3d11ResolveTarget )
	{
		D3D11_TEXTURE2D_DESC	d3d11ResolveTargetDesc;
		d3d11ResolveTarget->GetDesc( &d3d11ResolveTargetDesc );

		if ( d3dFeatureLevel == D3D_FEATURE_LEVEL_11_0 && d3d11ResolveTargetDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL )
		{
			// If we're resolving part of the depth buffer, we need to do a copy using a shader. Otherwise we can do a CopySubresourceRegion
			ResolveRect		resolveRect = InResolveParams.rect;
			if ( resolveRect.x1 >= 0 && resolveRect.x2 < ( int32 )d3d11ResolveTargetDesc.Width && resolveRect.y1 >= 0 && resolveRect.y2 < ( int32 )d3d11ResolveTargetDesc.Height )
			{
				ResolveSurfaceUsingShader( ( CD3D11DeviceContext* )InDeviceContext, ( CD3D11Surface* )InSourceSurface, resolveTarget2D, d3d11ResolveTargetDesc, GetDefaultRect( InResolveParams.rect, d3d11ResolveTargetDesc.Width, d3d11ResolveTargetDesc.Height ), GetDefaultRect( InResolveParams.rect, d3d11ResolveTargetDesc.Width, d3d11ResolveTargetDesc.Height ) );
			}
			else
			{
				d3d11DeviceContext->CopySubresourceRegion( d3d11ResolveTarget, 0, 0, 0, 0, d3d11SourceSurface, 0, nullptr );
			}
		}
		else
		{
			uint32					subresource = 0;
			D3D11_TEXTURE2D_DESC	d3d11SourceSurfaceDesc;
			d3d11SourceSurface->GetDesc( &d3d11SourceSurfaceDesc );

			if ( d3d11SourceSurfaceDesc.SampleDesc.Count != 1 )
			{
				d3d11DeviceContext->ResolveSubresource( d3d11ResolveTarget, subresource, d3d11SourceSurface, subresource, d3d11ResolveTargetDesc.Format );
			}
			else
			{
				bool		bZeroArea		= false;
				D3D11_BOX	d3d11SrcBox;
				d3d11SrcBox.left			= 0;
				d3d11SrcBox.top				= 0;
				d3d11SrcBox.front			= 0;
				D3D11_BOX*	pD3D11SrcBox	= nullptr;
				ResolveRect resolveRect	= InResolveParams.rect;
				
				if ( resolveRect.x1 >= 0 && resolveRect.x2 >= 0 && resolveRect.y1 >= 0 && resolveRect.y2 >= 0 && ( resolveRect.x1 > 0 || resolveRect.y1 > 0 || resolveRect.x2 < ( int32 )d3d11ResolveTargetDesc.Width || resolveRect.y2 < ( int32 )d3d11ResolveTargetDesc.Height || resolveRect.x2 < ( int32 )d3d11SourceSurfaceDesc.Width || resolveRect.y2 < ( int32 )d3d11SourceSurfaceDesc.Height ) )
				{
					if ( resolveRect.x1 == resolveRect.x2 || resolveRect.y1 == resolveRect.y2 )
					{
						// Zero area - skip
						bZeroArea			= true;
					}
					else
					{
						d3d11SrcBox.left	= resolveRect.x1;
						d3d11SrcBox.right	= resolveRect.x2;
						d3d11SrcBox.top		= resolveRect.y1;
						d3d11SrcBox.bottom	= resolveRect.y2;
						d3d11SrcBox.front	= 0;
						d3d11SrcBox.back	= 1;
						pD3D11SrcBox		= &d3d11SrcBox;
					}
				}

				if ( !bZeroArea )
				{
					d3d11DeviceContext->CopySubresourceRegion( d3d11ResolveTarget, subresource, d3d11SrcBox.left, d3d11SrcBox.top, d3d11SrcBox.front, d3d11SourceSurface, subresource, pD3D11SrcBox );
				}
			}
		}
	}
}

/*
==================
CD3D11RHI::BeginDrawingViewport
==================
*/
void CD3D11RHI::BeginDrawingViewport( class CBaseDeviceContextRHI* InDeviceContext, class CBaseViewportRHI* InViewport )
{
	Assert( InDeviceContext && InViewport );
	CD3D11DeviceContext*			deviceContext = ( CD3D11DeviceContext* )InDeviceContext;
	CD3D11Viewport*					viewport = ( CD3D11Viewport* )InViewport;
	Assert( viewport );

#if FRAME_CAPTURE_MARKERS
	BeginDrawEvent( InDeviceContext, DEC_SCENE_ITEMS, TEXT( "Viewport" ) );
#endif // FRAME_CAPTURE_MARKERS

	// Clear state cache
	stateCache.Reset();

	SetRenderTarget( InDeviceContext, viewport->GetSurface(), nullptr );
	SetViewport( InDeviceContext, 0, 0, 0.f, viewport->GetWidth(), viewport->GetHeight(), 1.f );
}

/*
==================
CD3D11RHI::SetRenderTarget
==================
*/
void CD3D11RHI::SetRenderTarget( class CBaseDeviceContextRHI* InDeviceContext, SurfaceRHIParamRef_t InNewRenderTarget, SurfaceRHIParamRef_t InNewDepthStencilTarget )
{
	Assert( InDeviceContext );
	CD3D11DeviceContext*			deviceContext				= ( CD3D11DeviceContext* )InDeviceContext;
	CD3D11Surface*					renderTarget				= ( CD3D11Surface* )InNewRenderTarget;
	CD3D11Surface*					depthStencilTarget			= ( CD3D11Surface* )InNewDepthStencilTarget;

	// Set render target
	{
		ID3D11RenderTargetView*		d3d11RenderTargetView		= renderTarget ? renderTarget->GetRenderTargetView() : nullptr;
		ID3D11DepthStencilView*		d3d11DepthStencilView		= depthStencilTarget ? depthStencilTarget->GetDepthStencilView() : nullptr;
		
		if ( d3d11RenderTargetView != stateCache.renderTargetViews[ 0 ] || d3d11DepthStencilView != stateCache.depthStencilView )
		{
			deviceContext->GetD3D11DeviceContext()->OMSetRenderTargets( 1, &d3d11RenderTargetView, d3d11DepthStencilView );
			stateCache.renderTargetViews[ 0 ] = d3d11RenderTargetView;
			stateCache.depthStencilView = d3d11DepthStencilView;
		}
	}
}

/*
==================
CD3D11RHI::SetMRTRenderTarget
==================
*/
void CD3D11RHI::SetMRTRenderTarget( class CBaseDeviceContextRHI* InDeviceContext, SurfaceRHIParamRef_t InNewRenderTarget, uint32 InTargetIndex )
{
	Assert( InDeviceContext && InTargetIndex < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT );
	CD3D11DeviceContext*	deviceContext	= ( CD3D11DeviceContext* )InDeviceContext;
	CD3D11Surface*			renderTarget	= ( CD3D11Surface* )InNewRenderTarget;

	// Set render target
	{
		ID3D11RenderTargetView*		d3d11RenderTargetView	= renderTarget ? renderTarget->GetRenderTargetView() : nullptr;
		if ( d3d11RenderTargetView != stateCache.renderTargetViews[InTargetIndex] )
		{
			stateCache.renderTargetViews[InTargetIndex] = d3d11RenderTargetView;
			deviceContext->GetD3D11DeviceContext()->OMSetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, &stateCache.renderTargetViews[0], stateCache.depthStencilView );
		}
	}
}

/*
==================
CD3D11RHI::EndDrawingViewport
==================
*/
void CD3D11RHI::EndDrawingViewport( class CBaseDeviceContextRHI* InDeviceContext, class CBaseViewportRHI* InViewport, bool InIsPresent, bool InLockToVsync )
{
	Assert( InViewport );

#if FRAME_CAPTURE_MARKERS
	EndDrawEvent( InDeviceContext );
#endif // FRAME_CAPTURE_MARKERS

	if ( InIsPresent )
	{
		InViewport->Present( InLockToVsync );
	}
}

#if WITH_EDITOR
#include <d3dcompiler.h>
#include <string>

#include "Containers/StringConv.h"
#include "Misc/CoreGlobals.h"
#include "Misc/Misc.h"
#include "System/Archive.h"
#include "System/BaseFileSystem.h"

/**
 * @ingroup D3D11RHI
 * An implementation of the D3DX include interface to access a ShaderCompilerEnvironment
 */
class CD3D11IncludeEnvironment : public ID3DInclude
{
public:
	/**
	 * Constructor
	 * 
	 * @param[in] InEnvironment Shader compiler environment
	 */
	CD3D11IncludeEnvironment( const ShaderCompilerEnvironment& InEnvironment ) :
		environment( InEnvironment )
	{}

	/**
	 * Open file
	 * 
	 * @param[in] InType Include type
	 * @param[in] InName File name
	 * @param[in] InParentData ???
	 * @param[Out] OutData Readed data from file
	 * @param[Out] OutBytes Count readed bytes from file
	 */
	STDMETHOD( Open )( D3D_INCLUDE_TYPE InType, LPCSTR InName, LPCVOID InParentData, LPCVOID* OutData, UINT* OutBytes )
	{
		std::wstring		filename( ANSI_TO_TCHAR( InName ) );

		// For including 'VertexFactory.hlsl' we take path from Environment
		if ( filename == TEXT( "VertexFactory.hlsl" ) )
		{
			filename = Sys_ShaderDir() + TEXT( "VertexFactory/" ) + environment.vertexFactoryFileName;
		}
		// Else we search in root shader dir
		else
		{
			filename = Sys_ShaderDir() + filename;
		}

		CArchive*		archive = g_FileSystem->CreateFileReader( filename );
		if ( !archive )
		{
			Errorf( TEXT( "Not found included shader file '%s'\n" ), filename.c_str() );
			return E_FAIL;
		}

		// Create data buffer and fill '\0'
		*OutBytes = archive->GetSize() + 1;
		byte*		data = new byte[ *OutBytes ];
		Memory::Memzero( data, *OutBytes );

		// Serialize data to buffer
		archive->Serialize( data, *OutBytes );

		*OutData = ( LPCVOID )data;
		delete archive;
		return S_OK;
	}

	/**
	 * Close file
	 * 
	 * @param[in] InData Readed data from file
	 */
	STDMETHOD( Close )( LPCVOID InData )
	{
		delete[] InData;
		return S_OK;
	}

private:
	ShaderCompilerEnvironment			environment;		/**< Shader compiler environment */
};

/*
==================
TranslateCompilerFlagD3D11
==================
*/
static DWORD TranslateCompilerFlagD3D11( ECompilerFlags CompilerFlag )
{
	switch ( CompilerFlag )
	{
	case CF_PreferFlowControl:			return D3D10_SHADER_PREFER_FLOW_CONTROL;
	case CF_Debug:						return D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
	case CF_AvoidFlowControl:			return D3D10_SHADER_AVOID_FLOW_CONTROL;
	default:							return 0;
	};
}

/*
==================
CD3D11RHI::CompileShader
==================
*/
bool CD3D11RHI::CompileShader( const tchar* InSourceFileName, const tchar* InFunctionName, EShaderFrequency InFrequency, const ShaderCompilerEnvironment& InEnvironment, ShaderCompilerOutput& OutOutput, bool InDebugDump /* = false */, const tchar* InShaderSubDir /* = TEXT( "" ) */ )
{
	CArchive*		shaderArchive = g_FileSystem->CreateFileReader( InSourceFileName );
	if ( !shaderArchive )
	{
		return false;
	}

	// Create string buffer and fill '\0'
	uint32				archiveSize = shaderArchive->GetSize() + 1;
	byte*				buffer = new byte[ archiveSize ];
	Memory::Memzero( buffer, archiveSize );

	// Serialize data to string buffer
	shaderArchive->Serialize( buffer, archiveSize );

	// Getting shader profile
	std::string			shaderProfile;
	switch ( InFrequency )
	{
	case SF_Vertex:
		shaderProfile = "vs_5_0";
		break;

	case SF_Hull:
		shaderProfile = "hs_5_0";
		break;

	case SF_Domain:
		shaderProfile = "ds_5_0";
		break;

	case SF_Geometry:
		shaderProfile = "gs_5_0";
		break;

	case SF_Pixel:
		shaderProfile = "ps_5_0";
		break;

	case SF_Compute:
		shaderProfile = "cs_5_0";
		break;

	default:
		Sys_Errorf( TEXT( "Unknown shader frequency %i" ), InFrequency );
		return false;
	}

	// Translate the input environment's definitions to D3DXMACROs
	std::vector< D3D_SHADER_MACRO >				macros;
	for ( auto it = InEnvironment.difinitions.begin(), itEnd = InEnvironment.difinitions.end(); it != itEnd; ++it )
	{
		const std::wstring&			name = it->first;
		const std::wstring&			difinition = it->second;

		// Create temp C-string of name and value difinition
		achar*						tName = new achar[ name.size() + 1 ];
		achar*						tDifinition = new achar[ difinition.size() + 1 ];
		strncpy( tName, TCHAR_TO_ANSI( name.c_str() ), name.size() + 1 );
		strncpy( tDifinition, TCHAR_TO_ANSI( difinition.c_str() ), difinition.size() + 1 );

		D3D_SHADER_MACRO			d3dMacro;
		d3dMacro.Name = tName;
		d3dMacro.Definition = tDifinition;
		macros.push_back( d3dMacro );
	}

	// Terminate the Macros list
	macros.push_back( D3D_SHADER_MACRO{ nullptr, nullptr } );

	// Getting compile flags
	DWORD				compileFlags = 0;
	for ( uint32 indexFlag = 0, countFlags = ( uint32 )InEnvironment.compilerFlags.size(); indexFlag < countFlags; ++indexFlag )
	{
		// Accumulate flags set by the shader
		compileFlags |= TranslateCompilerFlagD3D11( InEnvironment.compilerFlags[ indexFlag ] );
	}

	if ( InDebugDump )
	{
		compileFlags |= TranslateCompilerFlagD3D11( CF_Debug );
	}
	else
	{
		compileFlags |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
	}

	CD3D11IncludeEnvironment		includeEnvironment( InEnvironment );
	ID3DBlob*						shaderBlob = nullptr;
	ID3DBlob*						errorsBlob = nullptr;
	HRESULT							result = D3DCompile( buffer, archiveSize, TCHAR_TO_ANSI( InSourceFileName ), macros.data(), &includeEnvironment, TCHAR_TO_ANSI( InFunctionName ), shaderProfile.c_str(), compileFlags, 0, &shaderBlob, &errorsBlob );
	if ( FAILED( result ) )
	{
		// Copy the error text to the output.
		void*		errorBuffer = errorsBlob ? errorsBlob->GetBufferPointer() : nullptr;
		if ( errorBuffer )
		{
			OutOutput.errorMsg		= ANSI_TO_TCHAR( errorBuffer );
			errorsBlob->Release();
		}
		else
		{
			OutOutput.errorMsg		= TEXT( "Compile Failed without warnings!" );	
		}

		Errorf( TEXT( "%s\n" ), OutOutput.errorMsg.c_str() );
		return false;
	}

	// Save code of shader and getting reflector of shader
	uint32		numShaderBytes = ( uint32 )shaderBlob->GetBufferSize();
	OutOutput.code.resize( numShaderBytes );
	Memory::Memcpy( OutOutput.code.data(), shaderBlob->GetBufferPointer(), numShaderBytes );

	ID3D11ShaderReflection*				reflector = nullptr;
	result = D3DReflect( OutOutput.code.data(), OutOutput.code.size(), IID_ID3D11ShaderReflection, ( void** ) &reflector );
	Assert( result == S_OK );

	// Read the constant table description.
	D3D11_SHADER_DESC					shaderDesc;
	reflector->GetDesc( &shaderDesc );

	// Add parameters for shader resources (constant buffers, textures, samplers, etc)
	for ( uint32 resourceIndex = 0; resourceIndex < shaderDesc.BoundResources; ++resourceIndex )
	{
		D3D11_SHADER_INPUT_BIND_DESC 		bindDesc;
		reflector->GetResourceBindingDesc( resourceIndex, &bindDesc );

		if ( bindDesc.Type == D3D10_SIT_CBUFFER || bindDesc.Type == D3D10_SIT_TBUFFER )
		{
			const uint32 								cbIndex = bindDesc.BindPoint;
			ID3D11ShaderReflectionConstantBuffer* 		constantBuffer = reflector->GetConstantBufferByName( bindDesc.Name );
			D3D11_SHADER_BUFFER_DESC 					cbDesc;
			constantBuffer->GetDesc( &cbDesc );

			if ( cbDesc.Size > GConstantBufferSizes[ cbIndex ] )
			{
				Sys_Errorf( TEXT( "Set GConstantBufferSizes[%d] to >= %d" ), cbIndex, cbDesc.Size) ;
			}

			// Track all of the variables in this constant buffer
			for ( uint32 constantIndex = 0; constantIndex < cbDesc.Variables; ++constantIndex )
			{
				ID3D11ShaderReflectionVariable* 		variable = constantBuffer->GetVariableByIndex( constantIndex );
				D3D11_SHADER_VARIABLE_DESC 				variableDesc;
				variable->GetDesc( &variableDesc );

				if ( variableDesc.uFlags & D3D10_SVF_USED )
				{
					OutOutput.parameterMap.AddParameterAllocation(
						ANSI_TO_TCHAR( variableDesc.Name ),
						cbIndex,
						variableDesc.StartOffset,
						variableDesc.Size,
						0
						);
				}
				else
				{
					Warnf( TEXT( "Found Unused shader parameter: %s at offset: %i size: %i\n" ), ANSI_TO_TCHAR( variableDesc.Name ), variableDesc.StartOffset, variableDesc.Size );
				}
			}
		}
		else if ( bindDesc.Type == D3D10_SIT_TEXTURE )
		{
			std::wstring		officialName = ANSI_TO_TCHAR( bindDesc.Name );
			uint32				bindCount = 1;

			// Assign the name and optionally strip any "[#]" suffixes
			std::size_t			bracketLocation = officialName.find( TEXT( '[' ) );
			if ( bracketLocation != std::wstring::npos )
			{
				officialName.erase( bracketLocation, officialName.size() );
				const uint32 numCharactersBeforeArray	= officialName.size();

				// In SM5, for some reason, array suffixes are included in Name, i.e. "LightMapTextures[0]", rather than "LightMapTextures"
				// Additionally elements in an array are listed as SEPERATE bound resources.
				// However, they are always contiguous in resource index, so iterate over the samplers and textures of the initial association
				// and count them, identifying the bindpoint and bindcounts
				while ( resourceIndex + 1 < shaderDesc.BoundResources )
				{
					D3D11_SHADER_INPUT_BIND_DESC		bindDesc2;
					reflector->GetResourceBindingDesc( resourceIndex + 1, &bindDesc2 );

					if ( bindDesc2.Type == D3D10_SIT_TEXTURE && strncmp( bindDesc2.Name, bindDesc.Name, numCharactersBeforeArray ) == 0 )
					{
						// Skip over this resource since it is part of an array
						bindCount++;						
						resourceIndex++;
					}
					else
					{
						break;
					}
				}
			}

			// Add a parameter for the texture only, the sampler index will be invalid
			OutOutput.parameterMap.AddParameterAllocation(
				officialName.c_str(),
				0,
				bindDesc.BindPoint,
				bindCount,
				-1
			);
		}
		else if ( bindDesc.Type == D3D11_SIT_UAV_RWTYPED || bindDesc.Type >= D3D11_SIT_STRUCTURED )
		{
			// TODO BS yehor.pohuliaka: Arrays are not yet supported
			uint32		bindCount = 1;

			OutOutput.parameterMap.AddParameterAllocation(
				ANSI_TO_TCHAR( bindDesc.Name ),
				0,
				bindDesc.BindPoint,
				bindCount,
				-1
			);
		}
		else if ( bindDesc.Type == D3D10_SIT_SAMPLER )
		{ 
			std::wstring		officialName = ANSI_TO_TCHAR( bindDesc.Name );
			uint32				bindCount = 1;

			// Assign the name and optionally strip any "[#]" suffixes
			std::size_t			bracketLocation = officialName.find( TEXT( '[' ) );
			if ( bracketLocation != std::wstring::npos )
			{
				officialName.erase( bracketLocation, officialName.size() );
				const uint32	numCharactersBeforeArray = officialName.size();

				// In SM5, for some reason, array suffixes are included in Name, i.e. "LightMapTextures[0]", rather than "LightMapTextures"
				// Additionally elements in an array are listed as SEPERATE bound resources.
				// However, they are always contiguous in resource index, so iterate over the samplers and textures of the initial association
				// and count them, identifying the bindpoint and bindcounts
				while ( resourceIndex + 1 < shaderDesc.BoundResources )
				{
					D3D11_SHADER_INPUT_BIND_DESC	bindDesc2;
					reflector->GetResourceBindingDesc( resourceIndex + 1, &bindDesc2 );

					if ( bindDesc2.Type == D3D10_SIT_SAMPLER && strncmp( bindDesc2.Name, bindDesc.Name, numCharactersBeforeArray ) == 0 )
					{
						// Skip over this resource since it is part of an array
						bindCount++;
						resourceIndex++;
					}
					else
					{
						break;
					}
				}
			}

			// Add a parameter for the sampler only, the texture index will be invalid
			OutOutput.parameterMap.AddParameterAllocation(
				officialName.c_str(),
				0,
				-1,
				bindCount,
				bindDesc.BindPoint
			);
		}
	}

	// Set the number of instructions
	OutOutput.numInstructions = shaderDesc.InstructionCount;

	// Reflector is a com interface, so it needs to be released.
	reflector->Release();

	// We don't need the reflection data anymore, and it just takes up space
	if ( !InDebugDump )
	{
		ID3DBlob*		strippedShader = nullptr;
		result = D3DStripShader( OutOutput.code.data(), OutOutput.code.size(), D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS, &strippedShader );
		if ( result == S_OK )
		{
			uint32		numShaderBytes = strippedShader->GetBufferSize();

			OutOutput.code.resize( numShaderBytes );
			Memory::Memcpy( OutOutput.code.data(), strippedShader->GetBufferPointer(), numShaderBytes );
			strippedShader->Release();
		}
	}

	// Free temporary strings allocated for the macros
	for ( uint32 indexMacro = 0, countMacros = ( uint32 ) macros.size(); indexMacro < countMacros; ++indexMacro )
	{
		D3D_SHADER_MACRO&			macro = macros[ indexMacro ];
		delete[] macro.Name;
		delete[] macro.Definition;
	}

	shaderBlob->Release();
	delete[] buffer;
	delete shaderArchive;
	return true;
}
#endif // WITH_EDITOR

/*
==================
CD3D11RHI::GetShaderPlatform
==================
*/
EShaderPlatform CD3D11RHI::GetShaderPlatform() const
{
	return SP_PCD3D_SM5;
}

#if WITH_IMGUI
/*
==================
CD3D11RHI::InitImGUI
==================
*/
void CD3D11RHI::InitImGUI( class CBaseDeviceContextRHI* InDeviceContext )
{
	Assert( d3d11Device && InDeviceContext );
	CD3D11DeviceContext* deviceContext = ( CD3D11DeviceContext* ) InDeviceContext;

	bool		result = ImGui_ImplDX11_Init( d3d11Device, deviceContext->GetD3D11DeviceContext() );
	Assert( result );
	ImGui_ImplDX11_NewFrame();
}

/*
==================
CD3D11RHI::ShutdownImGUI
==================
*/
void CD3D11RHI::ShutdownImGUI( class CBaseDeviceContextRHI* InDeviceContext )
{
	ImGui_ImplDX11_Shutdown();
}

/*
==================
CD3D11RHI::DrawImGUI
==================
*/
void CD3D11RHI::DrawImGUI( class CBaseDeviceContextRHI* InDeviceContext, ImDrawData* InImGUIDrawData )
{
	ImGui_ImplDX11_RenderDrawData( InImGUIDrawData );
}
#endif // WITH_IMGUI

#if FRAME_CAPTURE_MARKERS
/*
==================
CD3D11RHI::BeginDrawEvent
==================
*/
void CD3D11RHI::BeginDrawEvent( class CBaseDeviceContextRHI* InDeviceContext, const CColor& InColor, const tchar* InName )
{
	D3DPERF_BeginEvent( D3DCOLOR_RGBA( InColor.r, InColor.g, InColor.b, InColor.a ), InName );
}

/*
==================
CD3D11RHI::EndDrawEvent
==================
*/
void CD3D11RHI::EndDrawEvent( class CBaseDeviceContextRHI* InDeviceContext )
{
	D3DPERF_EndEvent();
}
#endif // FRAME_CAPTURE_MARKERS

/*
==================
CD3D11RHI::GetRHIName
==================
*/
const tchar* CD3D11RHI::GetRHIName() const
{
	return TEXT( "D3D11RHI" );
}

/*
==================
CD3D11RHI::GetCachedBlendState
==================
*/
ID3D11BlendState* CD3D11RHI::GetCachedBlendState( const D3D11_BLEND_DESC& InBlendState, uint8* InColorWriteMasks )
{
	// Calculate hash
	uint64		hash = FastHash( InBlendState );
	hash = FastHash( InColorWriteMasks, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT * sizeof( uint8 ), hash );

	// Try find cached blend state
	auto		it = cachedBlendStates.find( hash );
	if ( it != cachedBlendStates.end() )
	{
		return it->second;
	}

	// If we not found then will create new blend state
	ID3D11BlendState*	d3d11BlendState = nullptr;
	D3D11_BLEND_DESC	d3d11BlendStateDesc;
	Memory::Memcpy( &d3d11BlendStateDesc, &InBlendState, sizeof( D3D11_BLEND_DESC ) );

	for ( uint32 renderTargetIndex = 0; renderTargetIndex < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++renderTargetIndex )
	{
		d3d11BlendStateDesc.RenderTarget[renderTargetIndex].RenderTargetWriteMask = InColorWriteMasks[renderTargetIndex];
	}
	d3d11BlendStateDesc.IndependentBlendEnable = true;

#if ENABLED_ASSERT
	HRESULT				result = d3d11Device->CreateBlendState( &d3d11BlendStateDesc, &d3d11BlendState );
	Assert( result == S_OK );
#else
	d3d11Device->CreateBlendState( &d3d11BlendStateDesc, &d3d11BlendState );
#endif // ENABLED_ASSERT

	cachedBlendStates[hash] = d3d11BlendState;
	return d3d11BlendState;
}

/*
==================
CD3D11RHI::GetCachedDepthStencilState
==================
*/
ID3D11DepthStencilState* CD3D11RHI::GetCachedDepthStencilState( const D3D11_DEPTH_STENCIL_DESC& InDepthStateInfo, const D3D11_DEPTH_STENCIL_DESC& InStencilStateInfo )
{
	// Calculate hash
	uint64		hash = FastHash( InDepthStateInfo );
	hash = FastHash( InStencilStateInfo, hash );

	// Try find cached depth stencil state
	auto	it = cachedDepthStencilStates.find( hash );
	if ( it != cachedDepthStencilStates.end() )
	{
		return it->second;
	}

	// If we not found then will create new depth stencil state
	ID3D11DepthStencilState*		d3d11DepthStencilState = nullptr;
	D3D11_DEPTH_STENCIL_DESC		d3d11DepthStencilDesc;
	Memory::Memcpy( &d3d11DepthStencilDesc, &InStencilStateInfo, sizeof( D3D11_DEPTH_STENCIL_DESC ) );
	d3d11DepthStencilDesc.DepthEnable		= InDepthStateInfo.DepthEnable;
	d3d11DepthStencilDesc.DepthWriteMask	= InDepthStateInfo.DepthWriteMask;
	d3d11DepthStencilDesc.DepthFunc			= InDepthStateInfo.DepthFunc;

#if ENABLED_ASSERT
	HRESULT				result = d3d11Device->CreateDepthStencilState( &d3d11DepthStencilDesc, &d3d11DepthStencilState );
	Assert( result == S_OK );
#else
	d3d11Device->CreateDepthStencilState( &d3d11DepthStencilDesc, &d3d11DepthStencilState );
#endif // ENABLED_ASSERT

	cachedDepthStencilStates[hash] = d3d11DepthStencilState;
	return d3d11DepthStencilState;
}

/*
==================
D3D11SetDebugName
==================
*/
void D3D11SetDebugName( ID3D11DeviceChild* InObject, const achar* InName )
{
	if ( !InName )			return;
	InObject->SetPrivateData( WKPDID_D3DDebugObjectName, ( uint32 )strlen( InName ), InName );
}

/*
==================
D3D11SetDebugName
==================
*/
void D3D11SetDebugName( IDXGIObject* InObject, const achar* InName )
{
	if ( !InName )			return;
	InObject->SetPrivateData( WKPDID_D3DDebugObjectName, ( uint32 )strlen( InName ), InName );
}