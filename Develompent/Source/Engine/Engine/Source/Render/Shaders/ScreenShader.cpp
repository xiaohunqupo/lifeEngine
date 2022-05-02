#include "Render/Shaders/ScreenShader.h"
#include "Render/VertexFactory/SimpleElementVertexFactory.h"

IMPLEMENT_SHADER_TYPE(, FScreenPixelShader, TEXT( "ScreenPixelShader.hlsl" ), TEXT( "MainPS" ), SF_Pixel, true );

#if WITH_EDITOR
bool FScreenPixelShader::ShouldCache( EShaderPlatform InShaderPlatform, class FVertexFactoryMetaType* InVFMetaType /* = nullptr */ )
{
	if ( !InVFMetaType )
	{
		return true;
	}

	return InVFMetaType->GetHash() == FSimpleElementVertexFactory::staticType.GetHash();
}
#endif // WITH_EDITOR

void FScreenPixelShader::Init( const FShaderCache::FShaderCacheItem& InShaderCacheItem )
{
	FShader::Init( InShaderCacheItem );
	textureParameter.Bind( InShaderCacheItem.parameterMap, TEXT( "texture0" ) );
	samplerStateParameter.Bind( InShaderCacheItem.parameterMap, TEXT( "texture0Sampler") );
}