/**
 * @file
 * @addtogroup D3D11RHI D3D11RHI
 *
 * Copyright BSOD-Games, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef D3D11SHADER_H
#define D3D11SHADER_H

#include "RHI/BaseShaderRHI.h"

 /**
  * @ingroup D3D11RHI
  * @brief Class for work with DirectX shader
  */
class CD3D11ShaderRHI : public CBaseShaderRHI
{
public:
	/**
	 * @brief Constructor
	 * 
	 * @param[in] InFrequency Frequency of shader
	 * @param[in] InData Data of shader code
	 * @param[in] InSize Size of data
	 * @param[in] InShaderName Shader name
	 */
	CD3D11ShaderRHI( EShaderFrequency InFrequency, const byte* InData, uint32 InSize, const tchar* InShaderName );

	/**
	 * Destructor
	 */
	virtual ~CD3D11ShaderRHI();

protected:
	/**
	 * @brief Union of shaders
	 */
	union UShaderHandle
	{
		IUnknown*					object;				/**< General pointer to DirectX 11 object of shader */
		ID3D11VertexShader*			vertexShader;		/**< Pointer to DirectX 11 vertex shader */
		ID3D11HullShader*			hullShader;			/**< Pointer to DirectX 11 hull shader */
		ID3D11DomainShader*			domainShader;		/**< Pointer to DirectX 11 domain shader */
		ID3D11PixelShader*			pixelShader;		/**< Pointer to DirectX 11 pixel shader */
		ID3D11GeometryShader*		geometryShader;		/**< Pointer to DirectX 11 geometry shader */
	};

	UShaderHandle			shader;			/**< Union of shader */
};

/**
 * @ingroup D3D11RHI
 * @brief Class of DirectX vertex shader
 */
class CD3D11VertexShaderRHI : public CD3D11ShaderRHI
{
public:
	/**
	 * @brief Constructor
	 *
	 * @param[in] InData Data of shader code
	 * @param[in] InSize Size of data
	 * @param[in] InShaderName Shader name
	 */
	CD3D11VertexShaderRHI( const byte* InData, uint32 InSize, const tchar* InShaderName ) :
		CD3D11ShaderRHI( SF_Vertex, InData, InSize, InShaderName )
	{
		code.resize( InSize );
		Memory::Memcpy( code.data(), InData, InSize );
	}

	/**
	 * @brief Get bytecode
	 * @return Return reference to bytecode of vertex shader
	 */
	FORCEINLINE const std::vector< byte >& GetCode() const
	{
		return code;
	}

	/**
	 * @brief Get D3D11 resource
	 * @return Pointer to D3D11 resource
	 */
	FORCEINLINE ID3D11VertexShader* GetResource() const
	{
		return shader.vertexShader;
	}

private:
	std::vector< byte >			code;		/**< Bytecode of shader for create D3D11 input layout */
};

/**
 * @ingroup D3D11RHI
 * @brief Class of DirectX hull shader
 */
class CD3D11HullShaderRHI : public CD3D11ShaderRHI
{
public:
	/**
	 * @brief Constructor
	 *
	 * @param[in] InData Data of shader code
	 * @param[in] InSize Size of data
	 * @param[in] InShaderName Shader name
	 */
	CD3D11HullShaderRHI( const byte* InData, uint32 InSize, const tchar* InShaderName ) :
		CD3D11ShaderRHI( SF_Hull, InData, InSize, InShaderName )
	{}

	/**
	 * @brief Get D3D11 resource
	 * @return Pointer to D3D11 resource
	 */
	FORCEINLINE ID3D11HullShader* GetResource() const
	{
		return shader.hullShader;
	}
};

/**
 * @ingroup D3D11RHI
 * @brief Class of DirectX domain shader
 */
class CD3D11DomainShaderRHI : public CD3D11ShaderRHI
{
public:
	/**
	 * @brief Constructor
	 *
	 * @param[in] InData Data of shader code
	 * @param[in] InSize Size of data
	 * @param[in] InShaderName Shader name
	 */
	CD3D11DomainShaderRHI( const byte* InData, uint32 InSize, const tchar* InShaderName ) :
		CD3D11ShaderRHI( SF_Domain, InData, InSize, InShaderName )
	{}

	/**
	 * @brief Get D3D11 resource
	 * @return Pointer to D3D11 resource
	 */
	FORCEINLINE ID3D11DomainShader* GetResource() const
	{
		return shader.domainShader;
	}
};

/**
 * @ingroup D3D11RHI
 * @brief Class of DirectX pixel shader
 */
class CD3D11PixelShaderRHI : public CD3D11ShaderRHI
{
public:
	/**
	 * @brief Constructor
	 *
	 * @param[in] InData Data of shader code
	 * @param[in] InSize Size of data
	 * @param[in] InShaderName Shader name
	 */
	CD3D11PixelShaderRHI( const byte* InData, uint32 InSize, const tchar* InShaderName ) :
		CD3D11ShaderRHI( SF_Pixel, InData, InSize, InShaderName )
	{}

	/**
	 * @brief Get D3D11 resource
	 * @return Pointer to D3D11 resource
	 */
	FORCEINLINE ID3D11PixelShader* GetResource() const
	{
		return shader.pixelShader;
	}
};

/**
 * @ingroup D3D11RHI
 * @brief Class of DirectX geometry shader
 */
class CD3D11GeometryShaderRHI : public CD3D11ShaderRHI
{
public:
	/**
	 * @brief Constructor
	 *
	 * @param[in] InData Data of shader code
	 * @param[in] InSize Size of data
	 * @param[in] InShaderName Shader name
	 */
	CD3D11GeometryShaderRHI( const byte* InData, uint32 InSize, const tchar* InShaderName ) :
		CD3D11ShaderRHI( SF_Geometry, InData, InSize, InShaderName )
	{}

	/**
	 * @brief Get D3D11 resource
	 * @return Pointer to D3D11 resource
	 */
	FORCEINLINE ID3D11GeometryShader* GetResource() const
	{
		return shader.geometryShader;
	}
};

/**
 * @ingroup D3D11RHI
 * @breif Class of vertex declaration
 */
class CD3D11VertexDeclarationRHI : public CBaseVertexDeclarationRHI
{
public:
	/**
	 * @brief Constructor
	 */
	CD3D11VertexDeclarationRHI( const VertexDeclarationElementList_t& InElementList );

	/**
	 * @brief Get hash
	 *
	 * @param[in] InHash Start hash
	 * @return Return calculated hash
	 */
	virtual uint64 GetHash( uint64 InHash = 0 ) const override;

	/**
	 * @brief Get vertex elements
	 * @return Return reference to vertex elements
	 */
	FORCEINLINE const std::vector< D3D11_INPUT_ELEMENT_DESC >& GetVertexElements()
	{
		return vertexElements;
	}

	/**
	 * @brief Get stream count
	 * @return Return stream count
	 */
	FORCEINLINE uint32 GetStreamCount() const
	{
		return streamCount;
	}

private:
	std::vector< D3D11_INPUT_ELEMENT_DESC >			vertexElements;		/**< Array of vertex elements for D3D11 */
	uint32											streamCount;		/**< Stram count */
};

/**
 * @ingroup D3D11RHI
 * @brief Class of bound shader state
 */
class CD3D11BoundShaderStateRHI : public CBaseBoundShaderStateRHI
{
public:
	/**
	 * @brief Constructor
	 * 
	 * @param[in] InDebugName Bound shader state name for debug
	 * @param[in] InKey Key of bound shader state
	 * @param[in] InVertexDeclaration Vertex declaration
	 * @param[in] InVertexShader Vertex shader
	 * @param[in] InPixelShader Pixel shader
	 * @param[in] InHullShader Hull shader
	 * @param[in] InDomainShader Domain shader
	 * @param[in] InGeometryShader Geometry shader
	 */
	CD3D11BoundShaderStateRHI( const tchar* InDebugName, const CBoundShaderStateKey& InKey, VertexDeclarationRHIRef_t InVertexDeclaration, VertexShaderRHIRef_t InVertexShader, PixelShaderRHIRef_t InPixelShader, HullShaderRHIRef_t InHullShader = nullptr, DomainShaderRHIRef_t InDomainShader = nullptr, GeometryShaderRHIRef_t InGeometryShader = nullptr );

	/**
	 * @brief Destructor
	 */
	virtual ~CD3D11BoundShaderStateRHI();

	/**
	 * @brief Get D3D11 input layout
	 * @return Pointer to D3D11 input layout
	 */
	FORCEINLINE ID3D11InputLayout* GetD3D11InputLayout() const
	{
		return d3d11InputLayout;
	}

private:
	ID3D11InputLayout*				d3d11InputLayout;		/**< D3D11 input layout */
};

#endif // !D3D11SHADER_H
