#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include "Reflection/Class.h"
#include "Misc/Misc.h"
#include "Misc/CoreGlobals.h"
#include "Misc/EngineGlobals.h"
#include "Logger/LoggerMacros.h"
#include "System/Package.h"
#include "System/BaseEngine.h"
#include "Containers/StringConv.h"
#include "Render/StaticMesh.h"
#include "Commandlets/ImportMeshCommandlet.h"

IMPLEMENT_CLASS( CImportMeshCommandlet )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CImportMeshCommandlet )

/*
==================
CImportMeshCommandlet::Main
==================
*/
bool CImportMeshCommandlet::Main( const CCommandLine& InCommandLine )
{
	std::wstring			srcFilename;
	std::wstring			dstFilename;
	std::wstring			nameMesh;

	// Parse arguments
	{
		srcFilename = InCommandLine.GetFirstValue( TEXT( "src" ) );
		dstFilename = InCommandLine.GetFirstValue( TEXT( "dst" ) );
		nameMesh	= InCommandLine.GetFirstValue( TEXT( "n" ) );
	}

	// If source and destination files is empty - this error
	AssertMsg( !srcFilename.empty() || !dstFilename.empty(), TEXT( "Not entered source file name and destination file name" ) );

	// If name mesh not seted - use name from srcFilename
	if ( nameMesh.empty() )
	{
		nameMesh = srcFilename;
		Warnf( TEXT( "Mesh name is not specified, by default it is assigned %s\n" ), srcFilename.c_str() );
	}

	// Convert static mesh
	TSharedPtr<CStaticMesh>		staticMesh = ConvertStaticMesh( srcFilename, nameMesh );
	if ( !staticMesh )
	{
		return false;
	}

	PackageRef_t		package = g_PackageManager->LoadPackage( dstFilename, true );
	package->Add( TAssetHandle<CStaticMesh>( staticMesh, MakeSharedPtr<AssetReference>( AT_StaticMesh, staticMesh->GetGUID() ) ) );
	return package->Save( dstFilename );
}

/*
==================
CImportMeshCommandlet::ConvertStaticMesh
==================
*/
TSharedPtr<CStaticMesh> CImportMeshCommandlet::ConvertStaticMesh( const std::wstring& InPath, const std::wstring& InAssetName )
{
	// Loading mesh with help Assimp
	Assimp::Importer		aiImport;
	const aiScene*			aiScene = aiImport.ReadFile(
		TCHAR_TO_ANSI( InPath.c_str() ),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights | aiProcess_Triangulate );
	if ( !aiScene )
	{
		return nullptr;
	}

	// Fill array meshes from Assimp scene
	AiMeshesMap_t			meshes;
	ProcessNode( aiScene->mRootNode, aiScene, meshes );
	if ( meshes.empty() )
	{
		Errorf( TEXT( "In file not found meshes\n" ) );
		aiImport.FreeScene();
		return nullptr;
	}

	// TODO BS yehor.pohuliaka - I'm not sure how correct this code is, it may need to be rewritten
	// Go through the material ID, take the mesh and write its vertices, and indices
	// to the shared buffer
	std::vector< StaticMeshVertexType >		verteces;
	std::vector< uint32 >						indeces;
	std::vector< StaticMeshSurface >			surfaces;
	std::vector< TAssetHandle<CMaterial> >		materials;
	for ( auto itRoot = meshes.begin(), itRootEnd = meshes.end(); itRoot != itRootEnd; ++itRoot )
	{
		StaticMeshSurface							surface;
		Memory::Memzero( &surface, sizeof( StaticMeshSurface ) );

		surface.firstIndex = indeces.size();
		surface.materialID = materials.size();

		for ( auto itMesh = itRoot->second.begin(), itMeshEnd = itRoot->second.end(); itMesh != itMeshEnd; ++itMesh )
		{
			std::vector< StaticMeshVertexType >	vertexBuffer;
			StaticMeshVertexType					vertex;
			aiMesh*									mesh = ( *itMesh ).mesh;
			Memory::Memzero( &vertex, sizeof( StaticMeshVertexType ) );

			// Prepare the vertex buffer.
			// If the vertices of the mesh do not fit into the buffer, then
			// expand it
			if ( vertexBuffer.size() < mesh->mNumVertices )
			{
				vertexBuffer.resize( vertexBuffer.size() + mesh->mNumVertices );
			}

			// Read all verteces
			for ( uint32 index = 0; index < mesh->mNumVertices; ++index )
			{
				aiVector3D		tempVector = ( *itMesh ).transformation * mesh->mVertices[ index ];
				vertex.position.x = tempVector.x;
				vertex.position.y = tempVector.y;
				vertex.position.z = tempVector.z;
				vertex.position.w = 1.f;

				tempVector = ( aiMatrix3x3 ) ( *itMesh ).transformation * mesh->mNormals[ index ];
				vertex.normal.x = tempVector.x;
				vertex.normal.y = tempVector.y;
				vertex.normal.z = tempVector.z;
				vertex.normal.w = 0.f;

				if ( mesh->mTangents )
				{
					tempVector = ( aiMatrix3x3 ) ( *itMesh ).transformation * mesh->mTangents[ index ];
					vertex.tangent.x = tempVector.x;
					vertex.tangent.y = tempVector.y;
					vertex.tangent.z = tempVector.z;
					vertex.tangent.w = 0.f;
				}

				if ( mesh->mBitangents )
				{
					tempVector = ( aiMatrix3x3 ) ( *itMesh ).transformation * mesh->mBitangents[ index ];
					vertex.binormal.x = tempVector.x;
					vertex.binormal.y = tempVector.y;
					vertex.binormal.z = tempVector.z;
					vertex.binormal.w = 0.f;
				}

				if ( mesh->mTextureCoords[ 0 ] )
				{
					tempVector = mesh->mTextureCoords[ 0 ][ index ];
					vertex.texCoord.x = tempVector.x;
					vertex.texCoord.y = tempVector.y;
				}

				vertexBuffer[ index ] = vertex;
			}

			// Read all indeces
			for ( uint32 index = 0; index < mesh->mNumFaces; ++index )
			{
				aiFace* face = &mesh->mFaces[ index ];
				for ( uint32 indexVertex = 0; indexVertex < face->mNumIndices; ++indexVertex )
				{
					uint32		index = face->mIndices[ indexVertex ];
					auto		it = find( verteces.begin(), verteces.end(), vertexBuffer[ index ] );

					// Look for the vertex index in the shared vertex buffer,
					// if not found, add the vertex to the buffer,
					// and then write its index
					if ( it == verteces.end() )
					{
						indeces.push_back( verteces.size() );
						verteces.push_back( vertexBuffer[ index ] );
					}
					else
					{
						indeces.push_back( it - verteces.begin() );
					}
				}
			}
		}

		// We process material
		if ( itRoot->first < aiScene->mNumMaterials )
		{
			materials.push_back( g_Engine->GetDefaultMaterial() );
		}
		else
		{
			Warnf( TEXT( "Material with id %i large. Surface not created\n" ), itRoot->first );
			continue;
		}

		surface.numPrimitives = ( indeces.size() - surface.firstIndex ) / 3.f;		// 1 primitive = 3 indeces (triangles)
		surfaces.push_back( surface );
	}

	// Serialize static mesh in archive
	TSharedPtr<CStaticMesh>		staticMeshRef = MakeSharedPtr<CStaticMesh>();
	staticMeshRef->SetAssetName( InAssetName );
	staticMeshRef->SetData( verteces, indeces, surfaces, materials );

	// Clean up all data
	aiImport.FreeScene();
	return staticMeshRef;
}

/*
==================
CImportMeshCommandlet::ProcessNode
==================
*/
void CImportMeshCommandlet::ProcessNode( aiNode* InNode, const aiScene* InScene, AiMeshesMap_t& OutMeshes )
{
	for ( uint32 index = 0; index < InNode->mNumMeshes; ++index )
	{
		aiMesh*			mesh = InScene->mMeshes[ InNode->mMeshes[ index ] ];
		OutMeshes[ mesh->mMaterialIndex ].push_back( AiMesh( InNode->mTransformation, mesh ) );
	}

	for ( uint32 index = 0; index < InNode->mNumChildren; ++index )
	{
		ProcessNode( InNode->mChildren[ index ], InScene, OutMeshes );
	}
}