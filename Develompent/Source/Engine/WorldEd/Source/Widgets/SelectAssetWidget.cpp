#include "Logger/LoggerMacros.h"
#include "Misc/EngineGlobals.h"
#include "Misc/WorldEdGlobals.h"
#include "Misc/UIGlobals.h"
#include "System/BaseEngine.h"
#include "System/AssetsImport.h"
#include "System/EditorEngine.h"
#include "System/DragNDrop.h"
#include "Widgets/SelectAssetWidget.h"
#include "Windows/ContentBrowserWindow.h"
#include "ImGUI/ImGUIEngine.h"
#include "ImGUI/imgui_stdlib.h"

/** Macro size image buttons */
#define  SELECTWIDGETASSET_IMAGEBUTTONSIZE		ImVec2( 16.f, 16.f )

/** Macro size of asset preview */
#define  SELECTWIDGETASSET_ASSETPREVIEWSIZE		ImVec2( 64.f, 64.f )

/*
==================
CSelectAssetWidget::CSelectAssetWidget
==================
*/
CSelectAssetWidget::CSelectAssetWidget( uint32 InAssetSlot )
	: bInit( false )
	, bSelectedAsset( false )
	, assetSlot( InAssetSlot )
{}

/*
==================
CSelectAssetWidget::Init
==================
*/
void CSelectAssetWidget::Init()
{
	bInit = true;
}

/*
==================
CSelectAssetWidget::SetAssetReference
==================
*/
void CSelectAssetWidget::SetAssetReference( const std::wstring& InAssetReference, bool InIsEmitEvent /* = true */ )
{
	assetReference = TCHAR_TO_ANSI( InAssetReference.c_str() );
	if ( InIsEmitEvent )
	{
		onSelectedAsset.Broadcast( assetSlot, InAssetReference );
	}
}

/*
==================
CSelectAssetWidget::Tick
==================
*/
void CSelectAssetWidget::Tick()
{
	Assert( bInit );
	ImGui::BeginGroup();
	bSelectedAsset = false;
	
	// Preview asset
	if ( previewTexture )
	{
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
		
		if ( ImGui::ImageButton( g_ImGUIEngine->LockTexture( previewTexture ), SELECTWIDGETASSET_ASSETPREVIEWSIZE ) )
		{
			onOpenAssetEditor.Broadcast( assetSlot );
		}
		if ( !assetReference.empty() && ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) )
		{
			ImGui::SetTooltip( "Open asset editor" );
		}
		
		ImGui::PopStyleColor( 3 );
	}
	ImGui::SameLine();

	// Control panel
	if ( ImGui::BeginTable( "##SelectWidgetControlPanel", 1 ) )
	{
		// Label
		ImGui::TableNextColumn();
		ImGui::Text( label.c_str() );
		ImGui::TableNextColumn();

		// Input asset reference
		ImGui::PushItemWidth( -10 );
		if ( ImGui::InputTextWithHint( ( "##AssetReference_" + std::to_string( assetSlot ) ).c_str(), "None", &assetReference, ImGuiInputTextFlags_EnterReturnsTrue ) )
		{
			onSelectedAsset.Broadcast( assetSlot, ANSI_TO_TCHAR( assetReference.c_str() ) );
			bSelectedAsset = true;
		}
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();

		// Insert button
		if ( ImGui::ImageButton( g_ImGUIEngine->LockTexture( g_EditorEngine->GetIcon( IT_Insert ).ToSharedPtr()->GetTexture2DRHI() ), SELECTWIDGETASSET_IMAGEBUTTONSIZE ) )
		{
			SetAssetReference( g_EditorEngine->GetContentBrowserWindow()->GetSelectedAssetReference() );
		}
		if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) )
		{
			ImGui::SetTooltip( "Use asset selected in content browser" );
		}

		// Browser button
		ImGui::SameLine();
		if ( ImGui::ImageButton( g_ImGUIEngine->LockTexture( g_EditorEngine->GetIcon( IT_Browse ).ToSharedPtr()->GetTexture2DRHI() ), SELECTWIDGETASSET_IMAGEBUTTONSIZE ) )
		{
			g_EditorEngine->GetContentBrowserWindow()->ShowAsset( ANSI_TO_TCHAR( assetReference.c_str() ) );
		}
		if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) )
		{
			ImGui::SetTooltip( "Show asset in content browser" );
		}

		// Remove button
		ImGui::SameLine();
		if ( ImGui::ImageButton( g_ImGUIEngine->LockTexture( g_EditorEngine->GetIcon( IT_Remove ).ToSharedPtr()->GetTexture2DRHI() ), SELECTWIDGETASSET_IMAGEBUTTONSIZE ) )
		{
			SetAssetReference( TEXT( "" ) );
		}
		if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) )
		{
			ImGui::SetTooltip( "Remove reference to asset" );
		}

		ImGui::EndTable();
	}
	
	// Manage of drop a asset's reference to input widget
	ImGui::EndGroup();
	if ( ImGui::BeginDragDropTarget() )
	{
		const ImGuiPayload*		imguiPayload = ImGui::AcceptDragDropPayload( DND_ASSETREFERENCEETYPE );
		if ( imguiPayload )
		{
			tchar*				pData = ( tchar* )imguiPayload->Data;
			Assert( pData );

			// Apply dropped asset reference
			std::vector<std::wstring>	assetReference;
			DND_ParseAssetReferenceData( pData, assetReference, 1 );
			if ( !assetReference.empty() )
			{
				SetAssetReference( assetReference[0] );
			}
		}
		ImGui::EndDragDropTarget();
	}
}