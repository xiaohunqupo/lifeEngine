#include "Reflection/ReflectionEnvironment.h"
#include "Reflection/Property.h"
#include "Reflection/Object.h"
#include "Logger/LoggerMacros.h"

IMPLEMENT_CLASS( CProperty )
IMPLEMENT_CLASS( CByteProperty )
IMPLEMENT_CLASS( CIntProperty )
IMPLEMENT_CLASS( CBoolProperty )
IMPLEMENT_CLASS( CFloatProperty )
IMPLEMENT_CLASS( CColorProperty )
IMPLEMENT_CLASS( CObjectProperty )
IMPLEMENT_CLASS( CVectorProperty )
IMPLEMENT_CLASS( CRotatorProperty )
IMPLEMENT_CLASS( CAssetProperty )
IMPLEMENT_CLASS( CArrayProperty )
IMPLEMENT_CLASS( CStructProperty )
IMPLEMENT_CLASS( CStringProperty )

IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CProperty )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CIntProperty )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CBoolProperty )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CFloatProperty )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CColorProperty )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CVectorProperty )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CRotatorProperty )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CAssetProperty )
IMPLEMENT_DEFAULT_INITIALIZE_CLASS( CStringProperty )

/*
==================
CProperty::CProperty
==================
*/
CProperty::CProperty()
	: offset( 0 )
	, flags( CPF_None )
{}

/*
==================
CProperty::CProperty
==================
*/
CProperty::CProperty( const CName& InCategory, const std::wstring& InDescription, uint32 InOffset, uint32 InFlags, uint32 InArraySize /*= 1*/ )
	: category( InCategory )
	, description( InDescription )
	, offset( InOffset )
	, flags( InFlags )
	, arraySize( InArraySize )
{
	AddObjectFlag( OBJECT_Native );
	GetOuterCField()->AddProperty( this );
}

/*
==================
CProperty::IsContainsObjectReference
==================
*/
bool CProperty::IsContainsObjectReference() const
{
	return false;
}

/*
==================
CProperty::EmitReferenceInfo
==================
*/
void CProperty::EmitReferenceInfo( CGCReferenceTokenStream* InReferenceTokenStream, uint32 InBaseOffset ) const
{}


/*
==================
CByteProperty::StaticInitializeClass
==================
*/
void CByteProperty::StaticInitializeClass()
{
	CClass*		theClass = StaticClass();
	theClass->EmitObjectReference( STRUCT_OFFSET( CByteProperty, cenum ) );
}

/*
==================
CByteProperty::GetPropertyValue
==================
*/
uint32 CByteProperty::GetElementSize() const
{
	return sizeof( byte );
}

/*
==================
CByteProperty::GetPropertyValue
==================
*/
uint32 CByteProperty::GetMinAlignment() const
{
	return alignof( byte );
}

/*
==================
CByteProperty::GetPropertyValue
==================
*/
bool CByteProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.byteValue = *( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CByteProperty::SetPropertyValue
==================
*/
bool CByteProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( InObjectAddress + offset ) = InPropertyValue.byteValue;
		bResult = true;
	}
	return bResult;
}


/*
==================
CIntProperty::GetPropertyValue
==================
*/
uint32 CIntProperty::GetElementSize() const
{
	return sizeof( int32 );
}

/*
==================
CIntProperty::GetPropertyValue
==================
*/
uint32 CIntProperty::GetMinAlignment() const
{
	return alignof( int32 );
}

/*
==================
CIntProperty::GetPropertyValue
==================
*/
bool CIntProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.intValue = *( int32* )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CIntProperty::SetPropertyValue
==================
*/
bool CIntProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( int32* )( InObjectAddress + offset ) = InPropertyValue.intValue;
		bResult = true;
	}
	return bResult;
}


/*
==================
CFloatProperty::GetPropertyValue
==================
*/
uint32 CFloatProperty::GetElementSize() const
{
	return sizeof( float );
}

/*
==================
CFloatProperty::GetPropertyValue
==================
*/
uint32 CFloatProperty::GetMinAlignment() const
{
	return alignof( float );
}

/*
==================
CFloatProperty::GetPropertyValue
==================
*/
bool CFloatProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.floatValue = *( float* )( InObjectAddress + offset );	
		bResult = true;
	}

	return bResult;
}

/*
==================
CFloatProperty::SetPropertyValue
==================
*/
bool CFloatProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( float* )( InObjectAddress + offset ) = InPropertyValue.floatValue;
		bResult = true;
	}
	return bResult;
}


/*
==================
CBoolProperty::GetPropertyValue
==================
*/
uint32 CBoolProperty::GetElementSize() const
{
	return sizeof( bool );
}

/*
==================
CBoolProperty::GetPropertyValue
==================
*/
uint32 CBoolProperty::GetMinAlignment() const
{
	return alignof( bool );
}

/*
==================
CBoolProperty::GetPropertyValue
==================
*/
bool CBoolProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.boolValue = *( bool* )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CFloatProperty::SetPropertyValue
==================
*/
bool CBoolProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( bool* )( InObjectAddress + offset ) = InPropertyValue.boolValue;
		bResult = true;
	}
	return bResult;
}


/*
==================
CColorProperty::GetPropertyValue
==================
*/
uint32 CColorProperty::GetElementSize() const
{
	return sizeof( CColor );
}

/*
==================
CColorProperty::GetPropertyValue
==================
*/
uint32 CColorProperty::GetMinAlignment() const
{
	return alignof( CColor );
}

/*
==================
CColorProperty::GetPropertyValue
==================
*/
bool CColorProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.colorValue = *( CColor* )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CColorProperty::SetPropertyValue
==================
*/
bool CColorProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( CColor* )( InObjectAddress + offset ) = InPropertyValue.colorValue;
		bResult = true;
	}
	return bResult;
}


/*
==================
CObjectProperty::StaticInitializeClass
==================
*/
void CObjectProperty::StaticInitializeClass()
{
	CClass*		theClass = StaticClass();
	theClass->EmitObjectReference( STRUCT_OFFSET( CObjectProperty, propertyClass ) );
}

/*
==================
CObjectProperty::GetPropertyValue
==================
*/
uint32 CObjectProperty::GetElementSize() const
{
	return sizeof( void* );
}

/*
==================
CObjectProperty::GetPropertyValue
==================
*/
uint32 CObjectProperty::GetMinAlignment() const
{
	return alignof( void* );
}

/*
==================
CObjectProperty::GetPropertyValue
==================
*/
bool CObjectProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.objectValue = *( CObject** )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CObjectProperty::SetPropertyValue
==================
*/
bool CObjectProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( CObject** )( InObjectAddress + offset ) = InPropertyValue.objectValue;
		bResult = true;
	}
	return bResult;
}

/*
==================
CObjectProperty::IsContainsObjectReference
==================
*/
bool CObjectProperty::IsContainsObjectReference() const
{
	return true;
}

/*
==================
CObjectProperty::EmitReferenceInfo
==================
*/
void CObjectProperty::EmitReferenceInfo( CGCReferenceTokenStream* InReferenceTokenStream, uint32 InBaseOffset ) const
{
	GCReferenceFixedArrayTokenHelper fixedArrayHelper( InReferenceTokenStream, InBaseOffset + offset, arraySize, sizeof( CObject* ) );
	InReferenceTokenStream->EmitReferenceInfo( GCReferenceInfo( GCRT_Object, InBaseOffset + offset ) );
}


/*
==================
CVectorProperty::GetPropertyValue
==================
*/
uint32 CVectorProperty::GetElementSize() const
{
	return sizeof( Vector );
}

/*
==================
CVectorProperty::GetPropertyValue
==================
*/
uint32 CVectorProperty::GetMinAlignment() const
{
	return alignof( Vector );
}

/*
==================
CVectorProperty::GetPropertyValue
==================
*/
bool CVectorProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.vectorValue = *( Vector* )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CVectorProperty::SetPropertyValue
==================
*/
bool CVectorProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( Vector* )( InObjectAddress + offset ) = InPropertyValue.vectorValue;
		bResult = true;
	}
	return bResult;
}


/*
==================
CRotatorProperty::GetPropertyValue
==================
*/
uint32 CRotatorProperty::GetElementSize() const
{
	return sizeof( CRotator );
}

/*
==================
CRotatorProperty::GetPropertyValue
==================
*/
uint32 CRotatorProperty::GetMinAlignment() const
{
	return alignof( CRotator );
}

/*
==================
CRotatorProperty::GetPropertyValue
==================
*/
bool CRotatorProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.rotatorValue = *( CRotator* )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CRotatorProperty::SetPropertyValue
==================
*/
bool CRotatorProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( CRotator* )( InObjectAddress + offset ) = InPropertyValue.rotatorValue;
		bResult = true;
	}
	return bResult;
}


/*
==================
CAssetProperty::GetPropertyValue
==================
*/
uint32 CAssetProperty::GetElementSize() const
{
	return sizeof( TAssetHandle<CAsset> );
}

/*
==================
CAssetProperty::GetPropertyValue
==================
*/
uint32 CAssetProperty::GetMinAlignment() const
{
	return alignof( TAssetHandle<CAsset> );
}

/*
==================
CAssetProperty::GetPropertyValue
==================
*/
bool CAssetProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.assetValue = *( TAssetHandle<CAsset>* )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CAssetProperty::SetPropertyValue
==================
*/
bool CAssetProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( TAssetHandle<CAsset>* )( InObjectAddress + offset ) = InPropertyValue.assetValue;
		bResult = true;
	}
	return bResult;
}


/*
==================
CArrayProperty::StaticInitializeClass
==================
*/
void CArrayProperty::StaticInitializeClass()
{
	CClass*		theClass = StaticClass();
	theClass->EmitObjectReference( STRUCT_OFFSET( CArrayProperty, innerProperty ) );
}

/*
==================
CArrayProperty::AddProperty
==================
*/
void CArrayProperty::AddProperty( class CProperty* InProperty )
{
	innerProperty = InProperty;
}

/*
==================
CArrayProperty::GetPropertyValue
==================
*/
uint32 CArrayProperty::GetElementSize() const
{
	return sizeof( std::vector<byte> );
}

/*
==================
CArrayProperty::GetPropertyValue
==================
*/
uint32 CArrayProperty::GetMinAlignment() const
{
	return alignof( std::vector<byte> );
}

/*
==================
CArrayProperty::GetPropertyValue
==================
*/
bool CArrayProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.arrayValue = ( std::vector<byte>* )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CArrayProperty::SetPropertyValue
==================
*/
bool CArrayProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		std::vector<byte>*		dstArray = ( std::vector<byte>* )( InObjectAddress + offset );
		dstArray->resize( InPropertyValue.arrayValue->size() );
		memcpy( dstArray->data(), InPropertyValue.arrayValue->data(), dstArray->size() );
		bResult = true;
	}
	return bResult;
}

/*
==================
CArrayProperty::IsContainsObjectReference
==================
*/
bool CArrayProperty::IsContainsObjectReference() const
{
	Assert( innerProperty );
	return innerProperty->IsContainsObjectReference();
}

/*
==================
CArrayProperty::EmitReferenceInfo
==================
*/
void CArrayProperty::EmitReferenceInfo( CGCReferenceTokenStream* InReferenceTokenStream, uint32 InBaseOffset ) const
{
	if ( innerProperty->IsContainsObjectReference() )
	{
		if ( IsA<CStructProperty>( innerProperty ) )
		{
			InReferenceTokenStream->EmitReferenceInfo( GCReferenceInfo( GCRT_ArrayStruct, InBaseOffset + offset ) );
			InReferenceTokenStream->EmitStride( innerProperty->GetElementSize() );
			const uint32	skipIndexIndex = InReferenceTokenStream->EmitSkipIndexPlaceholder();
			innerProperty->EmitReferenceInfo( InReferenceTokenStream, 0 );
			const uint32	skipIndex = InReferenceTokenStream->EmitReturn();
			InReferenceTokenStream->UpdateSkipIndexPlaceholder( skipIndexIndex, skipIndex );
		}
		else if ( IsA<CObjectProperty>( innerProperty ) )
		{
			InReferenceTokenStream->EmitReferenceInfo( GCReferenceInfo( GCRT_ArrayObject, InBaseOffset + offset ) );
		}
		else
		{
			Sys_Errorf( TEXT( "Encountered unknown property containing object or name reference: %s in %s" ), innerProperty->GetName().c_str(), GetName().c_str() );
		}
	}
}


/*
==================
CStructProperty::StaticInitializeClass
==================
*/
void CStructProperty::StaticInitializeClass()
{
	CClass*		theClass = StaticClass();
	theClass->EmitObjectReference( STRUCT_OFFSET( CStructProperty, propertyStruct ) );
}

/*
==================
CStructProperty::GetPropertyValue
==================
*/
uint32 CStructProperty::GetElementSize() const
{
	return propertyStruct->GetPropertiesSize();
}

/*
==================
CStructProperty::GetPropertyValue
==================
*/
uint32 CStructProperty::GetMinAlignment() const
{
	return propertyStruct->GetMinAlignment();
}

/*
==================
CStructProperty::GetPropertyValue
==================
*/
bool CStructProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	return false;
}

/*
==================
CStructProperty::SetPropertyValue
==================
*/
bool CStructProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	return false;
}

/*
==================
CStructProperty::IsContainsObjectReference
==================
*/
bool CStructProperty::IsContainsObjectReference() const
{
	// Prevent recursion in the case of structs containing dynamic arrays of themselves
	static std::list<const CStructProperty*>	encounteredStructProps;
	if ( std::find( encounteredStructProps.begin(), encounteredStructProps.end(), this ) != encounteredStructProps.end() )
	{
		return false;
	}
	else
	{
		Assert( propertyStruct );
		encounteredStructProps.push_back( this );

		std::vector<CProperty*>		properties;
		propertyStruct->GetProperties( properties );
		for ( uint32 index = 0, count = properties.size(); index < count; ++index )
		{
			CProperty*	property = properties[index];
			if ( property->IsContainsObjectReference() )
			{
				encounteredStructProps.erase( std::find( encounteredStructProps.begin(), encounteredStructProps.end(), this ) );
				return true;
			}
		}

		encounteredStructProps.erase( std::find( encounteredStructProps.begin(), encounteredStructProps.end(), this ) );
		return false;
	}
}

/*
==================
CStructProperty::EmitReferenceInfo
==================
*/
void CStructProperty::EmitReferenceInfo( CGCReferenceTokenStream* InReferenceTokenStream, uint32 InBaseOffset ) const
{
	Assert( propertyStruct );
	if ( IsContainsObjectReference() )
	{
		GCReferenceFixedArrayTokenHelper	fixedArrayHelper( InReferenceTokenStream, InBaseOffset + offset, arraySize, GetElementSize() );
		std::vector<CProperty*>				properties;
		
		propertyStruct->GetProperties( properties );
		for ( uint32 index = 0, count = properties.size(); index < count; ++index )
		{
			CProperty*	property = properties[index];
			property->EmitReferenceInfo( InReferenceTokenStream, InBaseOffset + offset );
		}
	}
}


/*
==================
CStringProperty::GetPropertyValue
==================
*/
uint32 CStringProperty::GetElementSize() const
{
	return sizeof( std::wstring );
}

/*
==================
CStringProperty::GetPropertyValue
==================
*/
uint32 CStringProperty::GetMinAlignment() const
{
	return alignof( std::wstring );
}

/*
==================
CStringProperty::GetPropertyValue
==================
*/
bool CStringProperty::GetPropertyValue( byte* InObjectAddress, UPropertyValue& OutPropertyValue ) const
{
	bool	bResult = false;
	if ( InObjectAddress )
	{
		OutPropertyValue.stringValue = ( std::wstring* )( InObjectAddress + offset );
		bResult = true;
	}

	return bResult;
}

/*
==================
CStringProperty::SetPropertyValue
==================
*/
bool CStringProperty::SetPropertyValue( byte* InObjectAddress, const UPropertyValue& InPropertyValue )
{
	bool	bResult = false;
	if ( !( flags & CPF_Const ) && InObjectAddress )
	{
		*( std::wstring* )( InObjectAddress + offset ) = *InPropertyValue.stringValue;
		bResult = true;
	}
	return bResult;
}