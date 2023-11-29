#include "Containers/String.h"
#include "Containers/StringConv.h"
#include "Misc/UIGlobals.h"
#include "Misc/WorldEdGlobals.h"
#include "Windows/AudioBankEditorWindow.h"
#include "Windows/FileDialog.h"
#include "Components/AudioComponent.h"
#include "Windows/DialogWindow.h"
#include "System/AssetsImport.h"
#include "System/EditorEngine.h"

/** Macro size button in menu bar */
#define  AUDIOBANKEDITOR_MENUBAR_BUTTONSIZE		ImVec2( 16.f, 16.f )

/*
==================
CAudioBankEditorWindow::CAudioBankEditorWindow
==================
*/
CAudioBankEditorWindow::CAudioBankEditorWindow( const TSharedPtr<CAudioBank>& InAudioBank )
	: CImGUILayer( CString::Format( TEXT( "Audio Bank Editor - %s" ), InAudioBank->GetAssetName().c_str() ) )
	, audioBank( InAudioBank )
	, audioBankHandle( nullptr )
{
	flags |= ImGuiWindowFlags_MenuBar | LF_DestroyOnHide;

	// Subscribe to event when assets try destroy of editing audio bank and reload. It need is block
	assetsCanDeleteHandle = EditorDelegates::onAssetsCanDelete.Add(	std::bind(	&CAudioBankEditorWindow::OnAssetsCanDelete, this, std::placeholders::_1, std::placeholders::_2	) );
	assetsReloadedHandle = EditorDelegates::onAssetsReloaded.Add(		std::bind(	&CAudioBankEditorWindow::OnAssetsReloaded, this, std::placeholders::_1							) );

	// Create audio component
	audioComponent = new CAudioComponent();
	audioComponent->SetAudioBank( audioBank->GetAssetHandle() );
	audioComponent->SetUISound( true );
	audioComponent->SetStreamable( true );
	audioComponent->AddToRoot();
}

/*
==================
CAudioBankEditorWindow::~CAudioBankEditorWindow
==================
*/
CAudioBankEditorWindow::~CAudioBankEditorWindow()
{
	// Unsubscribe from event when assets try destroy and reload
	EditorDelegates::onAssetsCanDelete.Remove( assetsCanDeleteHandle );
	EditorDelegates::onAssetsReloaded.Remove( assetsReloadedHandle );

	// Delete audio component and close handle
	audioComponent->RemoveFromRoot();
	if ( audioBankHandle )
	{
		audioBank->CloseBank( audioBankHandle );
	}
}

/*
==================
CAudioBankEditorWindow::Init
==================
*/
void CAudioBankEditorWindow::Init()
{
	CImGUILayer::Init();
	SetSize( Vector2D( 365.f, 250.f ) );

	// Update asset info
	UpdateAssetInfo();
}

/*
==================
CAudioBankEditorWindow::UpdateAssetInfo
==================
*/
void CAudioBankEditorWindow::UpdateAssetInfo()
{
	// Close old audio bank handle
	if ( audioBankHandle )
	{
		audioBank->CloseBank( audioBankHandle );
	}

	// Open new audio bank handle
	audioBankHandle = audioBank->OpenBank( audioBankInfo );
}

/*
==================
CAudioBankEditorWindow::OnTick
==================
*/
void CAudioBankEditorWindow::OnTick()
{
	// Menu bar
	if ( ImGui::BeginMenuBar() )
	{
		// Button 'Import'
		{
			if ( ImGui::ImageButton( g_ImGUIEngine->LockTexture( g_EditorEngine->GetIcon( IT_Import ).ToSharedPtr()->GetTexture2DRHI() ), AUDIOBANKEDITOR_MENUBAR_BUTTONSIZE ) )
			{
				std::wstring		errorMsg;
				if ( !g_AssetFactory.Reimport( audioBank, errorMsg ) )
				{
					OpenPopup<CDialogWindow>( TEXT( "Error" ), CString::Format( TEXT( "The audio bank not reimported.\n\nMessage: %s" ), errorMsg.c_str() ), CDialogWindow::BT_Ok );
				}
			}
			if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) )
			{
				ImGui::SetTooltip( "Reimport Audio Bank" );
			}
		}
		ImGui::EndMenuBar();
	}

	// Info about audio bank
	if ( ImGui::BeginTable( "##InfoAudioBankTable", 2 ) )
	{
		// Draw audio format
		ImGui::TableNextColumn();
		ImGui::Text( "Format:" );
		ImGui::TableNextColumn();
		ImGui::Text( audioBankHandle ? TCHAR_TO_ANSI( Sys_SampleFormatToText( audioBankInfo.format ).c_str() ) : "None" );
		ImGui::TableNextColumn();

		// Draw number channels in audio
		ImGui::Text( "Channels:" );
		ImGui::TableNextColumn();
		ImGui::Text( audioBankHandle ? std::to_string( audioBankInfo.numChannels ).c_str() : "0" );
		ImGui::TableNextColumn();

		// Draw audio rate
		ImGui::Text( "Rate:" );
		ImGui::TableNextColumn();
		ImGui::Text( audioBankHandle ? std::to_string( audioBankInfo.rate ).c_str() : "0" );
		ImGui::TableNextColumn();

		// Draw resource size
		ImGui::Text( "Resource Size:" );
		ImGui::TableNextColumn();
		ImGui::Text( audioBankHandle ? TCHAR_TO_ANSI( CString::Format( TEXT( "%.2f Kb" ), ( Sys_GetNumSampleBytes( audioBankInfo.format ) * audioBankInfo.numSamples ) / 1024.f ).c_str() ) : "0 Kb" );
		ImGui::EndTable();
	}

	// File path
	ImGui::Spacing();
	if ( ImGui::CollapsingHeader( "File Path", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		if ( ImGui::BeginTable( "##FilePathTable", 3 ) )
		{
			// Source file
			ImGui::TableNextColumn();
			ImGui::Text( "Source File:" );
			ImGui::TableNextColumn();
			{
				const std::wstring&		originalSourceFile = audioBank->GetAssetSourceFile();
				if ( !originalSourceFile.empty() )
				{
					ImGui::Text( TCHAR_TO_ANSI( originalSourceFile.c_str() ) );
					if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) )
					{
						ImGui::SetTooltip( TCHAR_TO_ANSI( originalSourceFile.c_str() ) );
					}
				}
			}

			// Select new source file path
			ImGui::TableNextColumn();
			if ( ImGui::Button( "..." ) )
			{
				CFileDialogSetup		fileDialogSetup;
				OpenFileDialogResult	openFileDialogResult;

				// Init file dialog settings
				fileDialogSetup.SetMultiselection( false );
				fileDialogSetup.SetTitle( TEXT( "Select New Source File" ) );
				fileDialogSetup.SetDirectory( Sys_GameDir() + PATH_SEPARATOR + TEXT( "Content/" ) );
				fileDialogSetup.AddFormat( g_AssetFactory.GetImporterInfo( AT_AudioBank ), ConvertAssetTypeToText( AT_AudioBank ) );

				// Show open file dialog
				if ( Sys_ShowOpenFileDialog( fileDialogSetup, openFileDialogResult ) && !openFileDialogResult.files.empty() )
				{
					audioBank->SetAssetSourceFile( openFileDialogResult.files[0] );
				}
			}

			// Clear source file path
			ImGui::SameLine();
			if ( ImGui::Button( "X" ) )
			{
				audioBank->SetAssetSourceFile( TEXT( "" ) );
			}
			ImGui::EndTable();
		}
	}

	// Buttons play, pause and stop
	ImGui::NewLine();
	ImGui::Separator();
	if ( ImGui::BeginTable( "##PreviewTable", 3 ) )
	{
		// Play sound
		ImGui::TableNextColumn();
		const ImVec2		buttonSize( ImGui::GetColumnWidth() , 0.f );
		if ( ImGui::Button( "Play", buttonSize ) )
		{
			audioComponent->Play();
		}

		// Pause sound
		ImGui::TableNextColumn();
		if ( ImGui::Button( "Pause", buttonSize ) )
		{
			audioComponent->Pause();
		}

		// Stop sound
		ImGui::TableNextColumn();
		if ( ImGui::Button( "Stop", buttonSize ) )
		{
			audioComponent->Stop();
		}


		ImGui::EndTable();
	}
}

/*
==================
CAudioBankEditorWindow::OnAssetsCanDelete
==================
*/
void CAudioBankEditorWindow::OnAssetsCanDelete( const std::vector<TSharedPtr<CAsset>>& InAssets, CanDeleteAssetResult& OutResult )
{
	// If in InAssets exist audio bank who is editing now - need is block
	for ( uint32 index = 0, count = InAssets.size(); index < count; ++index )
	{
		TSharedPtr<CAsset>		assetRef = InAssets[index];
		if ( assetRef == audioBank )
		{
			OutResult.Set( false );
			return;
		}
	}
}

/*
==================
CAudioBankEditorWindow::OnAssetsReloaded
==================
*/
void CAudioBankEditorWindow::OnAssetsReloaded( const std::vector<TSharedPtr<CAsset>>& InAssets )
{
	// If audio bank who is edition reloaded, we update UI
	for ( uint32 index = 0, count = InAssets.size(); index < count; ++index )
	{
		if ( InAssets[index] == audioBank )
		{
			UpdateAssetInfo();
			return;
		}
	}
}