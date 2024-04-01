#include "Misc/CoreGlobals.h"
#include "Logger/LoggerMacros.h"
#include "System/Package.h"
#include "Render/Material.h"
#include "Render/VertexFactory/StaticMeshVertexFactory.h"
#include "Render/VertexFactory/SpriteVertexFactory.h"

#if !SHIPPING_BUILD
#include "Render/VertexFactory/DynamicMeshVertexFactory.h"
#endif // !SHIPPING_BUILD

const CName		CMaterial::albedoTextureParamName( TEXT( "Albedo" ) );
const CName		CMaterial::normalTextureParamName( TEXT( "Normal" ) );
const CName		CMaterial::metallicTextureParamName( TEXT( "Metallic" ) );
const CName		CMaterial::roughnessTextureParamName( TEXT( "Roughness" ) );
const CName		CMaterial::emissionTextureParamName( TEXT( "Emission" ) );
const CName		CMaterial::aoTextureParamName( TEXT( "AO" ) );


/*
==================
CMaterial::CMaterial
==================
*/
CMaterial::CMaterial() :
	CAsset( AT_Material ),
	bNeedUpdateShaderMap( true ),
	bTwoSided( false ),
	bWireframe( false ),
	bTranslucency( false ),
	usage( MU_AllMeshes )
{}

/*
==================
CMaterial::~CMaterial
==================
*/
CMaterial::~CMaterial()
{}

/*
==================
CMaterial::Serialize
==================
*/
void CMaterial::Serialize( class CArchive& InArchive )
{
	if ( InArchive.Ver() < VER_ShaderMap )
	{
		return;
	}

	if ( InArchive.IsLoading() )
	{
		bNeedUpdateShaderMap = true;
		scalarParameters.clear();
		vectorParameters.clear();
		textureParameters.clear();
	}

	CAsset::Serialize( InArchive );
	InArchive << bTwoSided;
	InArchive << bWireframe;
	InArchive << usage;

	if ( InArchive.Ver() < VER_RemovedShadersTypeFromMaterial )
	{
		class CShaderMetaType*		shadersType[ SF_NumDrawFrequencies ];
		Memory::Memzero( shadersType, sizeof( shadersType ) );

		for ( uint32 index = 0; index < SF_NumDrawFrequencies; ++index )
		{
			InArchive << shadersType[ index ];
		}
	}

	if ( InArchive.Ver() < VER_CName )
	{
		std::unordered_map<std::wstring, float>							localScalarParameters;
		std::unordered_map<std::wstring, Vector4D>						localVectorParameters;
		std::unordered_map<std::wstring, TAssetHandle<CTexture2D>>		localTextureParameters;
		InArchive << localScalarParameters;
		InArchive << localVectorParameters;
		InArchive << localTextureParameters;

		// Copy to original parameters
		for ( auto it = localScalarParameters.begin(), itEnd = localScalarParameters.end(); it != itEnd; ++it )
		{
			scalarParameters[it->first] = it->second;
		}
		for ( auto it = localVectorParameters.begin(), itEnd = localVectorParameters.end(); it != itEnd; ++it )
		{
			vectorParameters[it->first] = it->second;
		}
		for ( auto it = localTextureParameters.begin(), itEnd = localTextureParameters.end(); it != itEnd; ++it )
		{
			textureParameters[it->first] = it->second;
		}

		Warnf( TEXT( "Deprecated package version (0x%X). Need to re-save the package '%s', because in the future it may not open\n" ), InArchive.Ver(), InArchive.GetPath().c_str() );
	}
	else
	{
		InArchive << scalarParameters;
		InArchive << vectorParameters;
		InArchive << textureParameters;
	}

	if ( InArchive.Ver() >= VER_AddTranslucencyFlag )
	{
		InArchive << bTranslucency;
	}
}

/*
==================
CMaterial::GetShader
==================
*/
CShader* CMaterial::GetShader( uint64 InVertexFactoryHash, EShaderFrequency InShaderFrequency )
{
	Assert( InShaderFrequency < SF_NumDrawFrequencies );
	if ( bNeedUpdateShaderMap )
	{
		CacheShaderMap();
	}

	MeshShaderMap_t::const_iterator		itVT = shaderMap.find( InVertexFactoryHash );
	if ( itVT == shaderMap.end() )
	{
		return nullptr;
	}
	return itVT->second[ InShaderFrequency ];
}

/*
==================
CMaterial::CacheShaderMap
==================
*/
void CMaterial::CacheShaderMap()
{
	if ( !bNeedUpdateShaderMap )
	{
		return;
	}

	// If material usage for render static mesh
	{
		const uint64			vertexFactoryHash = CStaticMeshVertexFactory::staticType.GetHash();
		if ( usage & MU_StaticMesh )
		{	
			shaderMap[ vertexFactoryHash ] = GetMeshShaders( vertexFactoryHash );
		}
		else
		{
			shaderMap.erase( vertexFactoryHash );
		}
	}

	// If material usage for render sprite mesh
	{
		const uint64			vertexFactoryHash = CSpriteVertexFactory::staticType.GetHash();
		if ( usage & MU_Sprite )
		{
			shaderMap[ vertexFactoryHash ] = GetMeshShaders( vertexFactoryHash );
		}
		else
		{
			shaderMap.erase( vertexFactoryHash );
		}
	}

	// Only for debug and WorldEd
#if !SHIPPING_BUILD
	// Dynamic mesh
	{
		const uint64			vertexFactoryHash = CDynamicMeshVertexFactory::staticType.GetHash();
		shaderMap[ vertexFactoryHash ] = GetMeshShaders( vertexFactoryHash );
	}
#endif // !SHIPPING_BUILD

	bNeedUpdateShaderMap = false;
}

/*
==================
CMaterial::GetMeshShaders
==================
*/
std::vector< CShader* > CMaterial::GetMeshShaders( uint64 InVertexFactoryHash ) const
{
	std::vector< CShader* >		result;
	result.resize( SF_NumDrawFrequencies );

	for ( uint32 index = 0; index < SF_NumDrawFrequencies; ++index )
	{
		const CShaderMetaType*		shaderType = GetShaderType( ( EShaderFrequency )index );
		if ( !shaderType || InVertexFactoryHash == ( uint32 )INVALID_HASH )
		{
			continue;
		}

		result[ index ] = g_ShaderManager->FindInstance( shaderType->GetName(), InVertexFactoryHash );
	}

	return result;
}

/*
==================
CMaterial::GetScalarParameterValue
==================
*/
bool CMaterial::GetScalarParameterValue( const CName& InParameterName, float& OutValue ) const
{
	auto		itFind = scalarParameters.find( InParameterName );
	if ( itFind == scalarParameters.end() )
	{
		OutValue = 0.f;
		return false;
	}

	OutValue = itFind->second;
	return true;
}

/*
==================
CMaterial::GetTextureParameterValue
==================
*/
bool CMaterial::GetTextureParameterValue( const CName& InParameterName, TAssetHandle<CTexture2D>& OutValue ) const
{
	auto		itFind = textureParameters.find( InParameterName );
	if ( itFind == textureParameters.end() )
	{
		OutValue = g_PackageManager->FindDefaultAsset( AT_Texture2D );
		return false;
	}

	OutValue = itFind->second.IsAssetValid() ? itFind->second : g_PackageManager->FindDefaultAsset( AT_Texture2D );
	return itFind->second.IsAssetValid();
}

/*
==================
CMaterial::GetVectorParameterValue
==================
*/
bool CMaterial::GetVectorParameterValue( const CName& InParameterName, Vector4D& OutValue ) const
{
	auto		itFind = vectorParameters.find( InParameterName );
	if ( itFind == vectorParameters.end() )
	{
		OutValue = Vector4D( 0.f, 0.f, 0.f, 0.f );
		return false;
	}

	OutValue = itFind->second;
	return true;
}

/*
==================
CMaterial::GetDependentAssets
==================
*/
void CMaterial::GetDependentAssets( SetDependentAssets_t& OutDependentAssets, EAssetType InFilter /* = AT_Unknown */ ) const
{
	// Fill set of dependent assets
	if ( InFilter == AT_Unknown || InFilter == AT_Texture2D )
	{
		for ( auto itTexture = textureParameters.begin(), itTextureEnd = textureParameters.end(); itTexture != itTextureEnd; ++itTexture )
		{
			if ( !itTexture->second.IsAssetValid() )
			{
				continue;
			}

			OutDependentAssets.insert( itTexture->second );
		}
	}
}

/*
==================
CMaterial::ReloadDependentAssets
==================
*/
void CMaterial::ReloadDependentAssets( bool InForce /* = false */ )
{
	for ( auto itTexture = textureParameters.begin(), itTextureEnd = textureParameters.end(); itTexture != itTextureEnd; ++itTexture )
	{
		TAssetHandle<CAsset>		assetHandle = itTexture->second;
		if ( !assetHandle.IsValid() || assetHandle.IsAssetValid() )
		{
			continue;
		}

		TSharedPtr<AssetReference>	assetReference = itTexture->second.GetReference();
		itTexture->second			= g_PackageManager->FindAsset( assetReference->guidPackage, assetReference->guidAsset, assetReference->type );
	}
}