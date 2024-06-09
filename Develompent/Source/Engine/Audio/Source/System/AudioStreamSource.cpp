#include "System/AudioStreamSource.h"
#include "Logger/LoggerMacros.h"

/*
==================
CAudioStreamRunnable::CAudioStreamRunnable
==================
*/
CAudioStreamRunnable::CAudioStreamRunnable( CAudioStreamSource* InStreamSource )
	: streamSource( InStreamSource )
{}

/*
==================
CAudioStreamRunnable::Init
==================
*/
bool CAudioStreamRunnable::Init()
{
	return true;
}

/*
==================
CAudioStreamRunnable::Run
==================
*/
uint32 CAudioStreamRunnable::Run()
{
	bool			bRequestStop = false;
	Assert( streamSource );

	{
		// Check if the thread was launched is stopped
		CScopeLock		scopeLock( &streamSource->mutexStreamData );
		if ( streamSource->status == ASS_Stoped )
		{
			streamSource->bIsStreaming = false;
			return 0;
		}
	}

	// Resize sample buffer for streaming audio
	samples.resize( streamSource->audioBankInfo.rate * streamSource->audioBankInfo.numChannels );

	// Create the buffers
	alGenBuffers( BufferCount, alBuffers );

	// Fill the queue
	bRequestStop = FillQueue();

	// Play the sound
	alSourcePlay( streamSource->GetALHandle() );

	{
		// Check if the thread was launched is paused
		CScopeLock		scopeLock( &streamSource->mutexStreamData );
		if ( streamSource->status == ASS_Paused )
		{
			alSourcePause( streamSource->GetALHandle() );
		}
	}

	while ( true )
	{
		TSharedPtr<CAudioBank>		audioBankRef = streamSource->audioBank.ToSharedPtr();		// Lock audio bank for him not unloaded ahead of time
		{
			CScopeLock		scopeLock( &streamSource->mutexStreamData );
			if ( !streamSource->bIsStreaming || !audioBankRef )
			{
				break;
			}
		}

		// The stream has been interrupted!
		if ( streamSource->GetStatus() == ASS_Stoped )
		{
			// If not request stop - just continue
			if ( !bRequestStop )
			{
				alSourcePlay( streamSource->GetALHandle() );
			}
			else
			{
				// End streaming
				CScopeLock		scopeLock( &streamSource->mutexStreamData );
				streamSource->bIsStreaming = false;
			}
		}

		// Get the number of buffers that have been processed (i.e. ready for reuse)
		ALint			processed = 0;
		alGetSourcei( streamSource->GetALHandle(), AL_BUFFERS_PROCESSED, &processed );

		while ( processed-- )
		{
			// Pop the first unused buffer from the queue
			ALuint			buffer = 0;
			alSourceUnqueueBuffers( streamSource->GetALHandle(), 1, &buffer );

			// Find its number
			uint32		bufferID = 0;
			for ( uint32 index = 0; index < BufferCount; ++index )
			{
				if ( alBuffers[ index ] == buffer )
				{
					bufferID = index;
					break;
				}
			}

			// Fill it and push it back into the playing queue
			if ( FillAndPushBuffer( bufferID ) )
			{
				bRequestStop = true;
			}
		}

		// Leave some time for the other threads if the stream is still playing
		if ( streamSource->GetStatus() != ASS_Stoped )
		{
			Sys_Sleep( 0.01f );		// 10 milliseconds
		}
	}

	// Stop the playback
	alSourceStop( streamSource->GetALHandle() );

	// Dequeue any buffer left in the queue
	ClearQueue();

	// Delete the buffers
	alSourcei( streamSource->GetALHandle(), AL_BUFFER, 0 );
	alDeleteBuffers( BufferCount, alBuffers );
	return 0;
}

/*
==================
CAudioStreamRunnable::Stop
==================
*/
void CAudioStreamRunnable::Stop()
{}

/*
==================
CAudioStreamRunnable::Exit
==================
*/
void CAudioStreamRunnable::Exit()
{}

/*
==================
CAudioStreamRunnable::FillQueue
==================
*/
bool CAudioStreamRunnable::FillQueue()
{	
	// Fill and enqueue all the available buffers
	bool			requestStop = false;
	for ( uint32 index = 0; index < BufferCount && !requestStop; ++index )
	{
		// Since no sound has been loaded yet, we can't schedule loop seeks preemptively,
		// So if we start on EOF or Loop End, we let FillAndPushBuffer() adjust the sample count
		if ( FillAndPushBuffer( index ) )
		{
			requestStop = true;
		}
	}

	return requestStop;
}

/*
==================
CAudioStreamRunnable::ClearQueue
==================
*/
void CAudioStreamRunnable::ClearQueue()
{
	// Get the number of buffers still in the queue
	ALint			queued;
	alGetSourcei( streamSource->GetALHandle(), AL_BUFFERS_QUEUED, &queued );

	// Dequeue them all
	ALuint			buffer;
	for ( ALint index = 0; index < queued; ++index )
	{
		alSourceUnqueueBuffers( streamSource->GetALHandle(), 1, &buffer );
	}
}

/*
==================
CAudioStreamRunnable::FillAndPushBuffer
==================
*/
bool CAudioStreamRunnable::FillAndPushBuffer( uint32 InBufferIndex )
{
	bool		requestStop = false;

	// Acquire audio data, also address EOF and error cases if they occur
	Chunk		data = { nullptr, 0 };
	for ( uint32 retryCount = 0; !GetData( data ) && retryCount < BufferRetries; ++retryCount )
	{
		// Check if the stream must loop or stop
		if ( !streamSource->bIsLoop )
		{
			requestStop = true;
			break;
		}

		//  If we looped - move to start file
		streamSource->audioBank.ToSharedPtr()->SeekBankPCM( streamSource->audioBankHandle, 0 );
	}

	// Fill the buffer if some data was returned
	if ( data.samples && data.numSamples )
	{
		uint32			buffer = alBuffers[ InBufferIndex ];

		// Fill the buffer and push it into the sound queue
		alBufferData( buffer, Sys_SampleFormatToEngine( streamSource->audioBankInfo.format ), data.samples, data.numSamples, streamSource->audioBankInfo.rate );
		alSourceQueueBuffers( streamSource->GetALHandle(), 1, &buffer );
	}
	else
	{
		// If we get here, we most likely ran out of retries
		requestStop = true;
	}

	return requestStop;
}

/*
==================
CAudioStreamRunnable::GetData
==================
*/
bool CAudioStreamRunnable::GetData( Chunk& OutData )
{
	CScopeLock	scopeLock( &streamSource->mutexStreamData );
	TSharedPtr<CAudioBank>		audioBankRef	= streamSource->audioBank.ToSharedPtr();
	uint32						toFill			= samples.size();
	uint64						currentOffset	= audioBankRef->GetOffsetBankPCM( streamSource->audioBankHandle );
	uint64						numSamples		= streamSource->audioBankInfo.numSamples;

	// If there are less samples left than the buffer size, we count how many samples need to be read
	if ( currentOffset + toFill > numSamples )
	{
		toFill = numSamples - currentOffset;
	}

	// Fill the chunk parameters
	OutData.samples		= &samples[ 0 ];
	OutData.numSamples	= audioBankRef->ReadBankPCM( streamSource->audioBankHandle, &samples[ 0 ], toFill );
	currentOffset		+= OutData.numSamples;

	// Check if we have stopped obtaining samples or reached either the EOF or the loop end point
	return OutData.numSamples != 0 && currentOffset != numSamples;
}


/*
==================
CAudioStreamSource::CAudioStreamSource
==================
*/
CAudioStreamSource::CAudioStreamSource()
	: bIsStreaming( false )
	, bIsLoop( false )
	, status( ASS_Stoped )
	, audioBankHandle( nullptr )
	, audioStreamRunnable( nullptr )
	, threadStreamData( nullptr )
{}

/*
==================
CAudioStreamSource::~CAudioStreamSource
==================
*/
CAudioStreamSource::~CAudioStreamSource()
{
	if ( audioBankHandle )
	{
		SetAudioBank( nullptr );
	}
}

/*
==================
CAudioStreamSource::Play
==================
*/
void CAudioStreamSource::Play()
{
	// If audio bank handle is not valid or source is muted, do nothing
	if ( !audioBankHandle || IsMuted() )
	{
		return;
	}

	bool					isStreaming = false;
	EAudioSourceStatus		status = ASS_Stoped;

	// Getting values state
	{
		CScopeLock		scopeLock( &mutexStreamData );
		isStreaming		= bIsStreaming;
		status			= this->status;
	}

	// If sound is streaming and thread state is ASS_Paused - start play sound
	// If sound is streaming and thread state is ASS_Playing - do nothing
	if ( isStreaming && status == ASS_Paused )
	{
		CScopeLock		scopeLock( &mutexStreamData );
		this->status = ASS_Playing;
		alSourcePlay( GetALHandle() );
		return;
	}
	else if ( isStreaming && status == ASS_Playing )
	{
		return;
	}

	// Start thread for streaming data
	bIsStreaming		= true;
	this->status		= ASS_Playing;
	audioStreamRunnable = new CAudioStreamRunnable( this );
	threadStreamData	= CRunnableThread::Create( audioStreamRunnable, TEXT( "AudioStream" ), true, true, 0, TP_Normal );
}

/*
==================
CAudioStreamSource::Pause
==================
*/
void CAudioStreamSource::Pause()
{
	// If audio source is muted, do nothing
	if ( IsMuted() )
	{
		return;
	}

	// Set state to ASS_Paused
	{
		CScopeLock		scopeLock( &mutexStreamData );
		if ( !bIsStreaming )
		{
			return;
		}
		status = ASS_Paused;
	}

	alSourcePause( GetALHandle() );
}

/*
==================
CAudioStreamSource::Stop
==================
*/
void CAudioStreamSource::Stop()
{
	// If audio source is muted, do nothing
	if ( IsMuted() )
	{
		return;
	}

	// Request the thread to terminate
	{
		CScopeLock		scopeLock( &mutexStreamData );
		bIsStreaming	= false;
	}

	// Wait for the thread to terminate
	if ( threadStreamData )
	{
		threadStreamData->WaitForCompletion();
		audioStreamRunnable		= nullptr;
		threadStreamData		= nullptr;
	}

	// Move to the beginning
	TSharedPtr<CAudioBank>		audioBankRef = audioBank.ToSharedPtr();
	if ( audioBankHandle && audioBankRef )
	{
		audioBankRef->SeekBankPCM( audioBankHandle, 0 );
	}
}

/*
==================
CAudioStreamSource::SetLoop
==================
*/
void CAudioStreamSource::SetLoop( bool InIsLoop )
{
	bIsLoop = InIsLoop;
}

/*
==================
CAudioStreamSource::OpenBank
==================
*/
void CAudioStreamSource::OpenBank( const TAssetHandle<CAudioBank>& InAudioBank )
{
	// If already opened bank - close
	if ( audioBankHandle )
	{
		CloseBank();
	}

	// If new bank is nullptr - exit from method
	TSharedPtr<CAudioBank>		audioBankRef = InAudioBank.ToSharedPtr();
	if ( !audioBankRef )
	{
		return;
	}

	// Open bank
	audioBankHandle = audioBankRef->OpenBank( audioBankInfo );
	bIsStreaming = false;

	// Check if the format is valid
	if ( audioBankInfo.format == SF_Unknown )
	{
		CloseBank();
		Warnf( TEXT( "Unsupported sample format in audio bank '%s'\n" ), audioBankRef->GetAssetName().c_str() );
		return;
	}
}

/*
==================
CAudioStreamSource::CloseBank
==================
*/
void CAudioStreamSource::CloseBank()
{
	if ( !audioBankHandle )
	{
		return;
	}

	Stop();
	{
		TSharedPtr<CAudioBank>		audioBankRef = audioBank.ToSharedPtr();
		if ( audioBankRef )
		{
			audioBankRef->CloseBank( audioBankHandle );
		}
	}

	audioBankHandle = nullptr;
}

/*
==================
CAudioStreamSource::SetAudioBank
==================
*/
void CAudioStreamSource::SetAudioBank( const TAssetHandle<CAudioBank>& InAudioBank )
{
	// If bank opened - close
	bool		bIsPlaying = GetStatus() == ASS_Playing;
	if ( audioBankHandle )
	{
		CloseBank();
	}

	// Open new bank
	if ( InAudioBank.IsAssetValid() )
	{
		OpenBank( InAudioBank );
	}

	// If current audio bank is not equal with InAudioBank, we unsubscribe from old event delegate
#if WITH_EDITOR
	if ( audioBank != InAudioBank )
	{
		// Unsubscribe from old event delegate
		TSharedPtr<CAudioBank>		audioBankRef = audioBank.ToSharedPtr();
		if ( audioBankRef )
		{
			audioBankRef->OnAudioBankUpdated().Remove( audioBankUpdatedHandle );
		}
		else
		{
			audioBankUpdatedHandle = nullptr;
		}

		// Subscribe to new event delegate
		audioBankRef		= InAudioBank.ToSharedPtr();
		if ( audioBankRef )
		{
			audioBankUpdatedHandle	= audioBankRef->OnAudioBankUpdated().Add( std::bind( &CAudioStreamSource::OnAudioBankUpdated, this, std::placeholders::_1 ) );
		}
	}
#endif // WITH_EDITOR

	audioBank = InAudioBank;

	// If the early the audio was playing we turn on back
	if ( audioBank.IsAssetValid() && bIsPlaying )
	{
		Play();
	}
}

/*
==================
CAudioStreamSource::IsLooped
==================
*/
bool CAudioStreamSource::IsLooped() const
{
	return bIsLoop;
}

/*
==================
CAudioStreamSource::GetStatus
==================
*/
EAudioSourceStatus CAudioStreamSource::GetStatus() const
{
	ALint			alStatus = 0;
	alGetSourcei( GetALHandle(), AL_SOURCE_STATE, &alStatus );

	switch ( alStatus )
	{
	case AL_PLAYING:	return ASS_Playing;
	case AL_PAUSED:		return ASS_Paused;	
	case AL_STOPPED:
	default:
	{
		CScopeLock		scopeLock( &mutexStreamData );
		if ( bIsStreaming )
		{
			return status;
		}
		return ASS_Stoped;
	}
	}
}

#if WITH_EDITOR
/*
==================
CAudioStreamSource::OnAudioBankUpdated
==================
*/
void CAudioStreamSource::OnAudioBankUpdated( class CAudioBank* InAudioBank )
{
	// Update data from audio bank
	Assert( InAudioBank );
	EAudioSourceStatus		currentStatus = GetStatus();
	SetAudioBank( InAudioBank->GetAssetHandle() );

	// Keep playing the sound if need
	if ( currentStatus == ASS_Playing )
	{
		Play();
	}
}
#endif // WITH_EDITOR