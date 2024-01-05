/**
 * @file
 * @addtogroup Core Core
 *
 * Copyright BSOD-Games, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>
#include <rapidjson/document.h>

#include "Core.h"

#undef GetObject

/**
 * @ingroup Core
 * @brief Enumeration config type
 */
enum EConfigType
{
	CT_Engine,		/**< Engine config */
	CT_Game,		/**< Game config */
	CT_Input,		/**< Input config */
	CT_Editor,		/**< Editor config */
	CT_Num			/**< Number configs type */
};

/**
 * @ingroup Core
 * @brief Enumeration layer of the config
 */
enum EConfigLayer
{
	CL_Engine,		/**< Engine */
	CL_Game,		/**< Game */
	CL_User,		/**< User */
	CL_Num			/**< Number layers */
};

 /**
  * @ingroup Core
  * @brief Class object of config
  */
class CConfigObject
{
public:
	/**
	 * @brief Constructor
	 */
	FORCEINLINE CConfigObject()
	{}

	/**
	 * @brief Constructor of copy
	 * @param[in] InCopy Copy of object
	 */
	FORCEINLINE CConfigObject( const CConfigObject& InCopy ) :
		values( InCopy.values )
	{}

	/**
	 * @brief Destructor
	 */
	FORCEINLINE ~CConfigObject()
	{
		Clear();
	}

	/**
	 * @brief Clear
	 */
	FORCEINLINE void Clear()
	{
		values.clear();
	}

	/**
	 * @brief Copy
	 * @param[in] InCopy Copy of object
	 */
	FORCEINLINE void Copy( const CConfigObject& InCopy )
	{
		values = InCopy.values;
	}

	/**
	 * @brief Convert object to JSON string
	 * 
	 * @param[in] InCountTabs Number of tabs for indentation
	 * @return Return converted object in JSON string
	 */
	std::string ToJSON( uint32 InCountTabs = 0 ) const;

	/**
	 * @brief Set object from RapidJSON value
	 * @param[in] InValue RapidJSON value
	 * @param[in] InName RapidJSON name for print name of object in log while error
	 */
	void Set( const rapidjson::Value& InValue, const tchar* InName = TEXT( "UNKNOWN" ) );

	/**
	 * @brief Set value
	 *
	 * @param[in] InName Name of value
	 * @param[in] InValue Value
	 */
	void SetValue( const tchar* InName, const class CConfigValue& InValue );

	/**
	 * @brief Get value
	 *
	 * @param[in] InName Name of value
	 * @return Return value from object. If not exist value in object - return empty
	 */
	class CConfigValue GetValue( const tchar* InName ) const;

	/**
	 * @brief Operator = for copy value
	 * @param[in] InCopy Copy of value
	 */
	FORCEINLINE CConfigObject& operator=( const CConfigObject& InCopy )
	{
		Copy( InCopy );
		return *this;
	}

private:
	std::unordered_map< std::wstring, class CConfigValue >			values;		/**< Values in object */
};

/**
 * @ingroup Core
 * @brief Class value of config
 */
class CConfigValue
{
public:
	/**
	 * @brief Enumeration of types value
	 */
	enum EType
	{
		T_None,				/**< No type */
		T_Bool,				/**< Bool type */
		T_Int,				/**< Integer type */
		T_Float,			/**< Float type */
		T_String,			/**< String type */
		T_Object,			/**< Object type (ConfigObject) */
		T_Array				/**< Array type */
	};

	/**
	 * @brief Constructor
	 */
	FORCEINLINE CConfigValue() :
		type( T_None ),
		value( nullptr )
	{}

	/**
	 * @brief Constructor of copy
	 * @param[in] InCopy Copy of value
	 */
	FORCEINLINE CConfigValue( const CConfigValue& InCopy ) :
		type( T_None ),
		value( nullptr )
	{
		Copy( InCopy );
	}

	/**
	 * @brief Destructor
	 */
	FORCEINLINE ~CConfigValue()
	{
		Clear();
	}

	/**
	 * @brief Clear value
	 */
	void Clear();

	/**
	 * @brief Copy value
	 * @param[in] InCopy Copy
	 */
	void Copy( const CConfigValue& InCopy );

	/**
	 * @brief Convert value to JSON string
	 * 
	 * @param[in] InCountTabs Number of tabs for indentation
	 * @return Return converted value in JSON string
	 */
	std::string ToJSON( uint32 InCountTabs = 0 ) const;

	/**
	 * Is valid config value
	 * @return Return true if value is valid, else false
	 */
	FORCEINLINE bool IsValid() const
	{
		return type != T_None;
	}

	/**
	 * Is config value is a type
	 * 
	 * @param[in] InType Checked type
	 * @return Return if type value is InType return true, else returned false
	 */
	FORCEINLINE bool IsA( EType InType ) const
	{
		return type == InType;
	}

	/**
	 * @brief Set value from RapidJSON value
	 * @param[in] InValue RapidJSON value
	 * @param[in] InName RapidJSON name for print name of value in log while error
	 */
	void Set( const rapidjson::Value& InValue, const tchar* InName = TEXT( "UNKNOWN" ) );

	/**
	 * @brief Set bool
	 * @param[in] InValue Value
	 */
	FORCEINLINE void SetBool( bool InValue )
	{
		if ( type != T_Bool )
		{
			Clear();
		}

		if ( !value )
		{
			value = new bool;
		}

		*static_cast< bool* >( value ) = InValue;
		type = T_Bool;
	}

	/**
	 * @brief Set int
	 * @param[in] InValue Value
	 */
	FORCEINLINE void SetInt( int32 InValue )
	{
		if ( type != T_Int )
		{
			Clear();
		}

		if ( !value )
		{
			value = new int32;
		}

		*static_cast< int32* >( value ) = InValue;
		type = T_Int;
	}

	/**
	 * @brief Set float
	 * @param[in] InValue Value
	 */
	FORCEINLINE void SetFloat( float InValue )
	{
		if ( type != T_Float )
		{
			Clear();
		}

		if ( !value )
		{
			value = new float;
		}

		*static_cast< float* >( value ) = InValue;
		type = T_Float;
	}

	/**
	 * @brief Set string
	 * @param[in] InValue Value
	 */
	FORCEINLINE void SetString( const std::wstring& InValue )
	{
		if ( type != T_String )
		{
			Clear();
		}

		if ( !value )
		{
			value = new std::wstring();
		}

		*static_cast< std::wstring* >( value ) = InValue;
		type = T_String;
	}

	/**
	 * @brief Set object
	 * @param[in] InValue Value
	 */
	FORCEINLINE void SetObject( const CConfigObject& InValue )
	{
		if ( type != T_Object )
		{
			Clear();
		}

		if ( !value )
		{
			value = new CConfigObject();
		}

		*static_cast< CConfigObject* >( value ) = InValue;
		type = T_Object;
	}

	/**
	 * @brief Set array
	 * @param[in] InValue Value
	 */
	FORCEINLINE void SetArray( const std::vector< CConfigValue >& InValue )
	{
		if ( type != T_Array )
		{
			Clear();
		}

		if ( !value )
		{
			value = new std::vector< CConfigValue >();
		}

		*static_cast< std::vector< CConfigValue >* >( value ) = InValue;
		type = T_Array;
	}

	/**
	 * @brief Get type value
	 * @return Type of value
	 */
	FORCEINLINE EType GetType() const
	{
		return type;
	}

	/**
	 * @brief Get bool
	 * 
	 * @param InDefaultValue	Default value
	 * @return Value with type bool, if type not correct returns InDefaultValue
	 */
	FORCEINLINE bool GetBool( bool InDefaultValue = false ) const
	{
		if ( type != T_Bool || !value )
		{
			return InDefaultValue;
		}

		return *static_cast< bool* >( value );
	}

	/**
	 * Get number
	 * 
	 * @param InDefaultValue	Default value
	 * @return Return int type if value is T_Int, return float type if value is T_Float, else returns InDefaultValue
	 */
	FORCEINLINE float GetNumber( float InDefaultValue = 0.f ) const
	{
		if ( type != T_Int && type != T_Float || !value )
		{
			return InDefaultValue;
		}

		if ( type == T_Int )
		{
			return ( float )GetInt();
		}
		else
		{
			return GetFloat();
		}
	}

	/**
	 * @brief Get int
	 * 
	 * @param InDefaultValue	Default value
	 * @return Value with type integer, if type not correct returns InDefaultValue
	 */
	FORCEINLINE int32 GetInt( int32 InDefaultValue = 0 ) const
	{
		if ( type != T_Int || !value )
		{
			return InDefaultValue;
		}

		return *static_cast< int32* >( value );
	}

	/**
	 * @brief Get float
	 * 
	 * @param InDefaultValue	Default value
	 * @return Value with type float, if type not correct returns InDefaultValue
	 */
	FORCEINLINE float GetFloat( float InDefaultValue = 0.f ) const
	{
		if ( type != T_Float || !value )
		{
			return InDefaultValue;
		}

		return *static_cast< float* >( value );
	}

	/**
	 * @brief Get string
	 * 
	 * @param InDefaultValue	Default value
	 * @return Value with type string, if type not correct returns InDefaultValue
	 */
	FORCEINLINE std::wstring GetString( const std::wstring InDefaultValue = TEXT( "" ) ) const
	{
		if ( type != T_String || !value )
		{
			return InDefaultValue;
		}

		return *static_cast< std::wstring* >( value );
	}

	/**
	 * @brief Get object
	 * @return Value with type object, if type not correct return empty object
	 */
	FORCEINLINE CConfigObject GetObject() const
	{
		if ( type != T_Object || !value )
		{
			return CConfigObject();
		}

		return *static_cast< CConfigObject* >( value );
	}

	/**
	 * @brief Get array
	 * @return Value with type array, if type not correct return empty array
	 */
	FORCEINLINE std::vector< CConfigValue > GetArray() const
	{
		if ( type != T_Array || !value )
		{
			return std::vector< CConfigValue >();
		}

		return *static_cast< std::vector< CConfigValue >* >( value );
	}

	/**
	 * @brief Operator = for copy value
	 * @param[in] InCopy Copy of value
	 */
	FORCEINLINE CConfigValue& operator=( const CConfigValue& InCopy )
	{
		Copy( InCopy );
		return *this;
	}

private:
	EType			type;		/**< Type of value */
	void*			value;		/**< Pointer to value */
};

/**
 * @ingroup Core
 * @brief Class for work with config files
 */
class CConfig
{
public:
	/**
	 * @brief Load file
	 * 
	 * @param InFile	Path to file
	 * @return Return TRUE if file has been successful parsed, otherwise returns FALSE
	 */
	bool LoadFile( const std::wstring& InFile );

	/**
	 * @brief Save into file
	 * 
	 * @param InFile	Path to file
	 * @return Return TRUE if file has been successful saved, otherwise returns FALSE
	 */
	bool SaveFile( const std::wstring& InFile );

	/**
	 * @brief Set value
	 * 
	 * @param[in] InGroup Name of group in config
	 * @param[in] InName Name of value in config group
	 * @param[in] InValue Value
	 */
	FORCEINLINE void SetValue( const tchar* InGroup, const tchar* InName, const CConfigValue& InValue )
	{
		groups[ InGroup ].SetValue( InName, InValue );
	}

	/**
	 * @brief Get value
	 * 
	 * @param[in] InGroup Name of group in config
	 * @param[in] InName Name of value in config group
	 * @return Return value from config, if not founded return empty value
	 */
	FORCEINLINE CConfigValue GetValue( const tchar* InGroup, const tchar* InName ) const
	{
		MapGroups_t::const_iterator		itGroup = groups.find( InGroup );
		if ( itGroup == groups.end() )
		{
			return CConfigValue();
		}

		return itGroup->second.GetValue( InName );
	}

private:
	/**
	 * @brief Serialize
	 *
	 * @param InArchive		Archive for serialization
	 * @return Return TRUE if config has been successful serialized, otherwise returns FALSE
	 */
	bool Serialize( class CArchive& InArchive );

	typedef std::unordered_map< std::wstring, CConfigObject >		MapGroups_t;
	MapGroups_t			groups;			/**< Config values */
};

/**
 * @ingroup Core
 * @brief Manager for work with all of layers config (Engine, Game, User)
 */
class CConfigManager
{
public:
	/**
	 * @brief Initialize configs
	 */
	void Init();

	/**
	 * @brief Shutdown configs
	 */
	FORCEINLINE void Shutdown()
	{
		configs.clear();
	}

	/**
		 * @brief Get config
		 *
		 * @param InType	Config type
		 * @return Return config object
		 */
	FORCEINLINE const CConfig& GetConfig( EConfigType InType ) const
	{
		std::unordered_map<EConfigType, CConfig>::const_iterator		itConfig = configs.find( InType );
		if ( itConfig == configs.end() )
		{
			AssertMsg( false, TEXT( "All types of configs must be exist all time" ) );
		}

		return itConfig->second;
	}

	/**
	 * @brief Get config
	 *
	 * @param InType	Config type
	 * @return Return config object
	 */
	FORCEINLINE CConfig& GetConfig( EConfigType InType )
	{
		std::unordered_map<EConfigType, CConfig>::iterator		itConfig = configs.find( InType );
		if ( itConfig == configs.end() )
		{
			AssertMsg( false, TEXT( "All types of configs must be exist all time" ) );
		}

		return itConfig->second;
	}

	/**
	 * @brief Set value
	 *
	 * @param InType	Config type
	 * @param InGroup	Name of group in config
	 * @param InName	Name of value in config group
	 * @param InValue Value
	 */
	FORCEINLINE void SetValue( EConfigType InType, const tchar* InGroup, const tchar* InName, const CConfigValue& InValue )
	{
		CConfig&	config = GetConfig( InType );
		config.SetValue( InGroup, InName, InValue );
	}

	/**
	 * @brief Get value
	 *
	 * @param InType	Config type
	 * @param InGroup	Name of group in config
	 * @param InName	Name of value in config group
	 * @return Return value from config, if not founded return empty value
	 */
	FORCEINLINE CConfigValue GetValue( EConfigType InType, const tchar* InGroup, const tchar* InName ) const
	{
		const CConfig&	config = GetConfig( InType );
		return config.GetValue( InGroup, InName );
	}

private:
	std::unordered_map<EConfigType, CConfig>		configs;		/**< Configs */
};

#endif // !CONFIG_H
