#include "Containers/StringConv.h"
#include "Misc/EngineGlobals.h"
#include "Misc/WorldEdGlobals.h"
#include "Misc/UIGlobals.h"
#include "System/World.h"
#include "ImGUI/ImGUIEngine.h"
#include "Windows/ActorPropertiesWindow.h"
#include "System/EditorEngine.h"

/*
==================
CActorPropertiesWindow::CActorPropertiesWindow
==================
*/
CActorPropertiesWindow::CActorPropertiesWindow( const std::wstring& InName )
	: CImGUILayer( InName )
	, actorsSelectedDelegate( nullptr )
	, actorsUnselectedDelegate( nullptr )
{
	actorsSelectedDelegate = SEditorDelegates::onActorsSelected.Add(		std::bind( &CActorPropertiesWindow::OnActorsUnSelected, this, std::placeholders::_1 ) );
	actorsUnselectedDelegate = SEditorDelegates::onActorsUnselected.Add(	std::bind( &CActorPropertiesWindow::OnActorsUnSelected, this, std::placeholders::_1 ) );
}

/*
==================
CActorPropertiesWindow::~CActorPropertiesWindow
==================
*/
CActorPropertiesWindow::~CActorPropertiesWindow()
{
	if ( actorsSelectedDelegate )
	{
		SEditorDelegates::onActorsSelected.Remove( actorsSelectedDelegate );
	}

	if ( actorsUnselectedDelegate )
	{
		SEditorDelegates::onActorsUnselected.Remove( actorsUnselectedDelegate );
	}
}

/*
==================
CActorPropertiesWindow::OnTick
==================
*/
void CActorPropertiesWindow::OnTick()
{
	const ImVec2&	itemSpacing = ImGui::GetStyle().ItemSpacing;
	if ( actorProperties.IsEmpty() && componentsProperties.empty() )
	{
		return;
	}

	// Actor properties
	ImGui::Separator();
	ImGui::Text( "Actor" );
	ImGui::Separator();

	actorProperties.Tick( itemSpacing.x );

	// Components properties
	ImGui::Dummy( ImVec2( 0, itemSpacing.y ) );
	ImGui::Separator();
	ImGui::Text( "Components" );
	ImGui::Separator();

	for ( auto itComponent = componentsProperties.begin(), itComponentEnd = componentsProperties.end(); itComponent != itComponentEnd; ++itComponent )
	{
		if ( ImGui::CollapsingHeader( TCHAR_TO_ANSI( itComponent->first.ToString().c_str() ) ) )
		{
			itComponent->second.Tick( itemSpacing.x, true );
		}
	}
}

/*
==================
CActorPropertiesWindow::GetAllPropertiesFromObject
==================
*/
void CActorPropertiesWindow::GetAllPropertiesFromObject( CObject* InObject, CObjectProperties& OutObjectProperties, std::unordered_map<CName, CObjectProperties, CName::SHashFunction>& OutComponentsProperties ) const
{
	// Get all properties of object
	Assert( InObject );
	if ( InObject->GetNumProperties() == 0 )
	{
		return;
	}

	// Get all object's properties
	std::unordered_map<CName, std::vector<CProperty*>, CName::SHashFunction>&	objectProperties = OutObjectProperties.GetProperties();
	{
		std::vector<CProperty*>		tempProperties;
		InObject->GetProperties( tempProperties );
		for ( uint32 index = 0, count = tempProperties.size(); index < count; ++index )
		{
			CProperty*		property = tempProperties[index];
			objectProperties[property->GetCategory()].push_back( property );
		}
	}

	// Get all properties from components
	for ( auto itObjectProperties = objectProperties.begin(); itObjectProperties != objectProperties.end(); )
	{
		std::vector<CProperty*>&		properties = itObjectProperties->second;
		for ( uint32 index = 0; index < properties.size(); )
		{
			CProperty*		property = properties[index];
			if ( property->GetClass()->HasAnyCastFlags( CASTCLASS_CComponentProperty ) )
			{
				UPropertyValue	propertyValue;
				property->GetPropertyValue( ( byte* )InObject, propertyValue );
				Assert( propertyValue.componentValue );
			
				if ( propertyValue.componentValue->GetNumProperties() > 0 )
				{
					CObjectProperties&															componentProps = OutComponentsProperties.insert( std::make_pair( property->GetCName(), CObjectProperties() ) ).first->second;
					std::unordered_map<CName, std::vector<CProperty*>, CName::SHashFunction>&	componentProperties = componentProps.GetProperties();
					std::vector<CProperty*>														tempProperties;
				
					propertyValue.componentValue->GetProperties( tempProperties );
					for ( uint32 compPropIdx = 0, count = tempProperties.size(); compPropIdx < count; ++compPropIdx )
					{
						CProperty*	compProperty = tempProperties[compPropIdx];
						componentProperties[compProperty->GetCategory()].push_back( compProperty );
					}

					Assert( !tempProperties.empty() );
					componentProps.GetObjects().push_back( propertyValue.componentValue );
				}
				properties.erase( properties.begin() + index );
			}
			else
			{
				++index;
			}
		}

		if ( properties.empty() )
		{
			itObjectProperties = objectProperties.erase( itObjectProperties );
		}
		else
		{
			++itObjectProperties;
		}
	}

	// If the array of object properties is not empty, then remember the pointer to it
	if ( !objectProperties.empty() )
	{
		OutObjectProperties.GetObjects().push_back( InObject );
	}
}

/*
==================
CActorPropertiesWindow::RemoveMissingProperties
==================
*/
bool CActorPropertiesWindow::RemoveMissingProperties( const std::vector<CProperty*>& InArrayA, std::vector<CProperty*>& InOutArrayB ) const
{
	bool	bResult = false;
	for ( uint32 propIdx = 0; propIdx < InOutArrayB.size(); )
	{
		bool		bFound = false;
		CProperty*	property = InOutArrayB[propIdx];
		for ( uint32 checkPropIdx = 0, countCheckProps = InArrayA.size(); checkPropIdx < countCheckProps; ++checkPropIdx )
		{
			if ( property == InArrayA[checkPropIdx] )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			InOutArrayB.erase( InOutArrayB.begin() + propIdx );
		}
		else
		{
			bResult = true;
			++propIdx;
		}
	}

	return bResult;
}

/*
==================
CActorPropertiesWindow::RemoveMissingProperties
==================
*/
bool CActorPropertiesWindow::RemoveMissingProperties( const std::unordered_map<CName, std::vector<CProperty*>, CName::SHashFunction>& InArrayA, std::unordered_map<CName, std::vector<CProperty*>, CName::SHashFunction>& InOutArrayB ) const
{
	bool		bWereMatches = false;
	for ( auto itBProperties = InOutArrayB.begin(); itBProperties != InOutArrayB.end(); )
	{
		auto	itAProperties = InArrayA.find( itBProperties->first );
		if ( itAProperties == InArrayA.end() )
		{
			itBProperties = InOutArrayB.erase( itBProperties );
			continue;
		}

		if ( RemoveMissingProperties( itAProperties->second, itBProperties->second ) )
		{
			bWereMatches = true;
			++itBProperties;
			continue;
		}
		else
		{
			itBProperties = InOutArrayB.erase( itBProperties );
		}
	}

	return bWereMatches;
}

/*
==================
CActorPropertiesWindow::OnActorsUnSelected
==================
*/
void CActorPropertiesWindow::OnActorsUnSelected( const std::vector<ActorRef_t>& InActors )
{
	// Clear old object properties
	actorProperties.Clear();
	componentsProperties.clear();

	// Getting an array of selected actors (we ignore InActors because it may not contain all of the selected actors)
	const std::vector<ActorRef_t>&						selectedActors = g_World->GetSelectedActors();
	if ( selectedActors.empty() )
	{
		return;
	}

	// Initialization of the set by the components of the first actor
	GetAllPropertiesFromObject( selectedActors[0], actorProperties, componentsProperties );

	// Checking for the presence of components in other actors
	for ( uint32 actorIdx = 1, actorNum = selectedActors.size(); actorIdx < actorNum; ++actorIdx )
	{
		// Get all properties in the actor
		ActorRef_t																		actor = selectedActors[actorIdx];
		CObjectProperties																localActorProperties;
		std::unordered_map<CName, CObjectProperties, CName::SHashFunction>				localComponentsProperties;
		GetAllPropertiesFromObject( actor, localActorProperties, localComponentsProperties );

		// Remove components that are not present in the current actor
		// Actor properties
		if ( RemoveMissingProperties( localActorProperties.GetProperties(), actorProperties.GetProperties() ) )
		{
			actorProperties.GetObjects().push_back( actor );
		}

		// Components properties
		for ( auto itComponentProps = componentsProperties.begin(); itComponentProps != componentsProperties.end(); )
		{
			auto	itActorComponentProps = localComponentsProperties.find( itComponentProps->first );
			if ( itActorComponentProps == localComponentsProperties.end() )
			{
				itComponentProps = componentsProperties.erase( itComponentProps );
				continue;
			}

			CObjectProperties&		globalObjectProperties = itComponentProps->second;
			CObjectProperties&		localObjectProperties = itActorComponentProps->second;
			bool					bWereMatches = RemoveMissingProperties( localObjectProperties.GetProperties(), globalObjectProperties.GetProperties() );
			if ( bWereMatches )
			{
				// Size array of objects in the local CObjectProperties must be 1, because it contain only one component
				Assert( localObjectProperties.GetObjects().size() == 1 );
				itComponentProps->second.GetObjects().push_back( localObjectProperties.GetObjects()[0] );
			}

			// If array of component properties in the global CObjectProperties is empty then we remove it from array
			if ( globalObjectProperties.GetProperties().empty() )
			{
				itComponentProps = componentsProperties.erase( itComponentProps );
				continue;
			}
			else
			{
				++itComponentProps;
			}
		}
	}
}


/*
==================
CActorPropertiesWindow::CObjectProperties::SetPropertyValue
==================
*/
void CActorPropertiesWindow::CObjectProperties::SetPropertyValue( CProperty* InProperty, const UPropertyValue& InPropertyValue )
{
	for ( uint32 objIdx = 0, countObjs = objects.size(); objIdx < countObjs; ++objIdx )
	{
		CObject*		object = objects[objIdx];
		InProperty->SetPropertyValue( ( byte* )object, InPropertyValue );
		object->PostEditChangeProperty( InProperty, PCT_ValueSet );
	}
}

/*
==================
CActorPropertiesWindow::CObjectProperties::TickProperty
==================
*/
void CActorPropertiesWindow::CObjectProperties::TickProperty( float InItemWidthSpacing, CProperty* InProperty )
{
	bool			bPropertyIsChanged = false;
	CObject*		objectZero = objects[0];
	CClass*			theClass = InProperty->GetClass();
	UPropertyValue	propertyValue;

	// Draw name of property
	ImGui::TableNextColumn();
	ImGui::BeginDisabled( !objectZero->CanEditProperty( InProperty ) );
	ImGui::Dummy( ImVec2( InItemWidthSpacing, 0.f ) );
	ImGui::SameLine();
	ImGui::Text( TCHAR_TO_ANSI( InProperty->GetCName().ToString().c_str() ) );
	if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) )
	{
		ImGui::SetTooltip( TCHAR_TO_ANSI( InProperty->GetDescription().c_str() ) );
	}
	ImGui::TableNextColumn();

	// Draw value of property
	// Bool property
	if ( theClass->HasAnyCastFlags( CASTCLASS_CBoolProperty ) )
	{
		InProperty->GetPropertyValue( ( byte* )objectZero, propertyValue );
		bPropertyIsChanged = ImGui::Checkbox( TCHAR_TO_ANSI( CString::Format( TEXT( "##%s_%p" ), InProperty->GetName().c_str(), this ).c_str() ), &propertyValue.boolValue );	
	}

	// Float property
	else if ( theClass->HasAnyCastFlags( CASTCLASS_CFloatProperty ) )
	{
		InProperty->GetPropertyValue( ( byte* )objectZero, propertyValue );
		bPropertyIsChanged = ImGui::InputFloat( TCHAR_TO_ANSI( CString::Format( TEXT( "##%s_%p" ), InProperty->GetName().c_str(), this ).c_str() ), &propertyValue.floatValue );
	}

	// Color property
	else if ( theClass->HasAnyCastFlags( CASTCLASS_CColorProperty ) )
	{
		InProperty->GetPropertyValue( ( byte* )objectZero, propertyValue );
		
		Vector4D	color = propertyValue.colorValue.ToNormalizedVector4D();
		if ( ImGui::ColorEdit4( TCHAR_TO_ANSI( CString::Format( TEXT( "##%s_%p" ), InProperty->GetName().c_str(), this ).c_str() ), ( float* )&color, ImGuiColorEditFlags_NoInputs ) )
		{
			propertyValue.colorValue = color;
			bPropertyIsChanged = true;
		}	
	}

	// Vector property
	else if ( theClass->HasAnyCastFlags( CASTCLASS_CVectorProperty ) )
	{
		InProperty->GetPropertyValue( ( byte* )objectZero, propertyValue );
		bPropertyIsChanged |= ImGui::DragVectorFloat( CString::Format( TEXT( "%s_%p" ), InProperty->GetName().c_str(), this ), propertyValue.vectorValue, 0.f );
	}

	// Transform property
	else if ( theClass->HasAnyCastFlags( CASTCLASS_CTrasformProperty ) )
	{
		InProperty->GetPropertyValue( ( byte* )objectZero, propertyValue );

		// Location
		if ( ImGui::TreeNodeEx( "Location", ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			Vector		location = propertyValue.transformValue.GetLocation();
			bool		bChangedLocation = ImGui::DragVectorFloat( CString::Format( TEXT( "%s_Location_%p" ), InProperty->GetName().c_str(), this ), location, 0.f, 0.1f );
			if ( bChangedLocation )
			{
				propertyValue.transformValue.SetLocation( location );
				bPropertyIsChanged |= bChangedLocation;
			}
			ImGui::TreePop();
		}

		// Rotation
		if ( ImGui::TreeNodeEx( "Rotation", ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			Vector		rotation = SMath::QuaternionToAngles( propertyValue.transformValue.GetRotation() );
			bool		bChangedRotation = ImGui::DragVectorFloat( CString::Format( TEXT( "%s_Rotation_%p" ), InProperty->GetName().c_str(), this ), rotation, 0.f, 0.1f );
			if ( bChangedRotation )
			{
				propertyValue.transformValue.SetRotation( SMath::AnglesToQuaternionXYZ( rotation ) );
				bPropertyIsChanged |= bChangedRotation;
			}
			ImGui::TreePop();
		}

		// Scale
		if ( ImGui::TreeNodeEx( "Scale", ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			Vector			scale = propertyValue.transformValue.GetScale();
			bool			bChangedScale = ImGui::DragVectorFloat( CString::Format( TEXT( "%s_Scale_%p" ), InProperty->GetName().c_str(), this ), scale, 1.f, 0.1f );
			if ( bChangedScale )
			{
				propertyValue.transformValue.SetScale( scale );
				bPropertyIsChanged |= bChangedScale;
			}
			ImGui::TreePop();
		}
	}

	// Unknown property
	else
	{
		ImGui::TextColored( g_ImGUIEngine->GetStyleColor( IGC_ErrorColor ), TCHAR_TO_ANSI( CString::Format( TEXT( "Unknown type of '%s'" ), InProperty->GetCName().ToString().c_str() ).c_str() ) );
	}
	ImGui::EndDisabled();

	if ( bPropertyIsChanged )
	{
		SetPropertyValue( InProperty, propertyValue );
		g_World->MarkDirty();
	}
}

/*
==================
CActorPropertiesWindow::CObjectProperties::Tick
==================
*/
void CActorPropertiesWindow::CObjectProperties::Tick( float InItemWidthSpacing, bool InApplySpacingToCategories /*= false*/ )
{
	float		currentSpacing = InItemWidthSpacing;
	for ( auto itCategory = properties.begin(), itCategoryEnd = properties.end(); itCategory != itCategoryEnd; ++itCategory )
	{
		if ( InApplySpacingToCategories )
		{
			ImGui::Dummy( ImVec2( InItemWidthSpacing, 0.f ) );
			currentSpacing = InItemWidthSpacing * 2.f;
			ImGui::SameLine();
		}
		
		if ( ImGui::CollapsingHeader( TCHAR_TO_ANSI( CString::Format( TEXT( "%s##%p" ), itCategory->first.ToString().c_str(), this ).c_str() ), ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			ImGui::Dummy( ImVec2( currentSpacing, 0.f ) );
			ImGui::SameLine();
			
			if ( ImGui::BeginTable( TCHAR_TO_ANSI( CString::Format( TEXT( "##TableObjectProperties_%p" ), this ).c_str() ), 2, ImGuiTableFlags_Resizable  ) )
			{
				const std::vector<CProperty*>&		categoryProperties = itCategory->second;
				for ( uint32 propIdx = 0, countProps = categoryProperties.size(); propIdx < countProps; ++propIdx )
				{
					CProperty*		property = categoryProperties[propIdx];
					
					ImGui::TableNextRow();
					ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, !( propIdx % 2 ) ? ImGui::ColorConvertFloat4ToU32( g_ImGUIEngine->GetStyleColor( IGC_TableBgColor0 ) ) : ImGui::ColorConvertFloat4ToU32( g_ImGUIEngine->GetStyleColor( IGC_TableBgColor1 ) ) );
					TickProperty( currentSpacing + InItemWidthSpacing / 10.f, property );
				}

				ImGui::EndTable();
			}
		}
	}
}