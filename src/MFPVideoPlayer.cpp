#include "pch.h"
#include "MFPVideoPlayer.h"
#include <strsafe.h>
#include <new>

#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "shlwapi.lib")


//-------------------------------------------------------------------
// OnMediaPlayerEvent
// 
// Implements IMFPMediaPlayerCallback::OnMediaPlayerEvent.
// This callback method handles events from the MFPlay object.
//-------------------------------------------------------------------

void MediaPlayerCallback::OnMediaPlayerEvent(MFP_EVENT_HEADER * pEventHeader)
{
	if (FAILED(pEventHeader->hrEvent)) {
		m_pPlayer->ShowErrorMessage(L"Playback error", pEventHeader->hrEvent);
		return;
	}

	switch (pEventHeader->eEventType) {
	case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
		m_pPlayer->OnMediaItemCreated(MFP_GET_MEDIAITEM_CREATED_EVENT(pEventHeader));
		break;

	case MFP_EVENT_TYPE_MEDIAITEM_SET:
		m_pPlayer->OnMediaItemSet(MFP_GET_MEDIAITEM_SET_EVENT(pEventHeader));
		break;

	case MFP_EVENT_TYPE_PLAYBACK_ENDED:
		m_pPlayer->OnPlaybackEnded(MFP_GET_MEDIAITEM_SET_EVENT(pEventHeader));
		break;
	}
}


//-------------------------------------------------------------------
// MFPVideoPlayer class
// 
//-------------------------------------------------------------------

MFPVideoPlayer::MFPVideoPlayer() : m_pPlayer(nullptr), m_pPlayerCB(nullptr), m_bHasVideo(false)
{
}

MFPVideoPlayer::~MFPVideoPlayer()
{
	if (m_pPlayer) {
		m_pPlayer->Shutdown();
		m_pPlayer->Release();
		m_pPlayer = nullptr;
	}
	if (m_pPlayerCB) {
		m_pPlayerCB->Release();
		m_pPlayerCB = nullptr;
	}
}

//-------------------------------------------------------------------
// PlayMediaFile
//
// Plays a media file, using the IMFPMediaPlayer interface.
//-------------------------------------------------------------------

HRESULT MFPVideoPlayer::PlayMediaFile(HWND hwnd, const WCHAR* sURL)
{
	HRESULT hr = S_OK;

	// Create the MFPlayer object.
	while (1) {
		if (!m_pPlayer) {
			m_pPlayerCB = new (std::nothrow) MediaPlayerCallback(this);
			if (!m_pPlayerCB) {
				hr = E_OUTOFMEMORY;
				break;
			}

			hr = MFPCreateMediaPlayer(
				NULL,
				FALSE,          // Start playback automatically?
				0,              // Flags
				m_pPlayerCB,    // Callback pointer
				hwnd,           // Video window
				&m_pPlayer
			);
			if (FAILED(hr))
				break;
		}

		// Create a new media item for this URL.
		hr = m_pPlayer->CreateMediaItemFromURL(sURL, FALSE, 0, NULL);
		return hr;
	}
	// The CreateMediaItemFromURL method completes asynchronously. 
	// The application will receive an MFP_EVENT_TYPE_MEDIAITEM_CREATED 
	// event. See MediaPlayerCallback::OnMediaPlayerEvent().
	return hr;
}

MFP_MEDIAPLAYER_STATE MFPVideoPlayer::GetState() noexcept
{
	if (m_pPlayer) {
		MFP_MEDIAPLAYER_STATE state = MFP_MEDIAPLAYER_STATE_EMPTY;
		HRESULT hr = m_pPlayer->GetState(&state);
		if (SUCCEEDED(hr))
			return state;
	}
	return MFP_MEDIAPLAYER_STATE_EMPTY;
}

bool MFPVideoPlayer::Play() noexcept
{
	if (m_pPlayer) {
		HRESULT hr = m_pPlayer->Play();
		return SUCCEEDED(hr);
	}
	return false;
}

bool MFPVideoPlayer::Pause() noexcept
{
	if (m_pPlayer) {
		HRESULT hr = m_pPlayer->Pause();
		return SUCCEEDED(hr);
	}
	return false;
}

bool MFPVideoPlayer::Stop() noexcept
{
	if (m_pPlayer) {
		HRESULT hr = m_pPlayer->Stop();
		return SUCCEEDED(hr);
	}
	return false;
}

float MFPVideoPlayer::GetVolume() noexcept
{
	float fVolume = 0.0f;
	if (m_pPlayer)
		m_pPlayer->GetVolume(&fVolume);
	return fVolume;
}

bool MFPVideoPlayer::SetVolume(float fVolume) noexcept
{
	if (m_pPlayer) {
		HRESULT hr = m_pPlayer->SetVolume(fVolume);
		return SUCCEEDED(hr);
	}
	return false;
}

bool MFPVideoPlayer::GetMute() noexcept
{
	BOOL bMute = FALSE;
	if (m_pPlayer)
		m_pPlayer->GetMute(&bMute);
	return bMute;
}

bool MFPVideoPlayer::SetMute(bool bMute) noexcept
{
	if (m_pPlayer) {
		HRESULT hr = m_pPlayer->SetMute(bMute);
		return SUCCEEDED(hr);
	}
	return false;
}

//-------------------------------------------------------------------
// OnMediaItemCreated
//
// Called when the IMFPMediaPlayer::CreateMediaItemFromURL method
// completes.
//-------------------------------------------------------------------

void MFPVideoPlayer::OnMediaItemCreated(MFP_MEDIAITEM_CREATED_EVENT* pEvent)
{
	HRESULT hr = S_OK;

	// The media item was created successfully.
	if (m_pPlayer) {
		BOOL bHasVideo = FALSE, bIsSelected = FALSE;
		while (1) {
			// Check if the media item contains video.
			hr = pEvent->pMediaItem->HasVideo(&bHasVideo, &bIsSelected);
			if (FAILED(hr))
				break;

			m_bHasVideo = (bHasVideo && bIsSelected);

			// Set the media item on the player. This method completes asynchronously.
			hr = m_pPlayer->SetMediaItem(pEvent->pMediaItem);
			break;
		}
	}
	if (FAILED(hr)) {
		ShowErrorMessage(L"Error playing this file.", hr);
	}
}

//-------------------------------------------------------------------
// OnMediaItemSet
//
// Called when the IMFPMediaPlayer::SetMediaItem method completes.
//-------------------------------------------------------------------

void MFPVideoPlayer::OnMediaItemSet(MFP_MEDIAITEM_SET_EVENT* /*pEvent*/) 
{
	HRESULT hr = m_pPlayer->Play();
	if (FAILED(hr)) {
		ShowErrorMessage(L"IMFPMediaPlayer::Play failed.", hr);
		return;
	}
	m_pPlayer->SetMute(TRUE);
}

void MFPVideoPlayer::OnPlaybackEnded(MFP_MEDIAITEM_SET_EVENT* pEvent)
{
	HRESULT hr = m_pPlayer->Play();
	if (FAILED(hr)) {
		ShowErrorMessage(L"IMFPMediaPlayer::Play failed.", hr);
	}
}

void MFPVideoPlayer::ShowErrorMessage(PCWSTR format, HRESULT hrErr)
{
	WCHAR msg[MAX_PATH];
	HRESULT hr = StringCbPrintf(msg, sizeof(msg), L"%s (hr=0x%X)", format, hrErr);
	if (SUCCEEDED(hr)) {
		MessageBox(NULL, msg, L"Error", MB_ICONERROR);
	}
}
