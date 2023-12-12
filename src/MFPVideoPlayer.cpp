#include "pch.h"
#include "LiveWallpaper.h"
#include <Shlwapi.h>
#include <new>

#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "shlwapi.lib")


//-----------------------------------------------------------------------------
// CreateInstance
//
// Creates an instance of the MFPlayer2 object.
//-----------------------------------------------------------------------------

HRESULT MFPVideoPlayer::CreateInstance(HWND hwndEvent, HWND hwndVideo, MFPVideoPlayer** ppPlayer)
{
	HRESULT hr = S_OK;

	MFPVideoPlayer* pPlayer = new (std::nothrow)MFPVideoPlayer(hwndEvent);
	if (!pPlayer)
		return E_OUTOFMEMORY;

	hr = pPlayer->Initialize(hwndVideo);
	if (SUCCEEDED(hr)) {
		*ppPlayer = pPlayer;
		(*ppPlayer)->AddRef();
	}

	SafeRelease(&pPlayer);
	return hr;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

MFPVideoPlayer::MFPVideoPlayer(HWND hwndEvent) : m_cRef(1), m_pPlayer(nullptr),
m_hwndEvent(hwndEvent), m_bHasVideo(false), m_caps(0)
{
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

MFPVideoPlayer::~MFPVideoPlayer()
{
	SafeRelease(&m_pPlayer);
}

//------------------------------------------------------------------------------
//  Initialize
//  Creates an instance of the MFPlay player object.
//
//  hwndVideo: 
//  Handle to the video window.
//------------------------------------------------------------------------------

HRESULT MFPVideoPlayer::Initialize(HWND hwndVideo)
{
	HRESULT hr = S_OK;

	SafeRelease(&m_pPlayer);

	hr = MFPCreateMediaPlayer(
		NULL,
		FALSE,          // Start playback automatically?
		0,              // Flags
		this,           // Callback pointer   
		hwndVideo,      // Video window
		&m_pPlayer
	);

	return hr;
}


//***************************** IUnknown methods *****************************//

//------------------------------------------------------------------------------
//  AddRef
//------------------------------------------------------------------------------

ULONG MFPVideoPlayer::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

//------------------------------------------------------------------------------
//  Release
//------------------------------------------------------------------------------

ULONG MFPVideoPlayer::Release()
{
	ULONG uCount = InterlockedDecrement(&m_cRef);
	if (uCount == 0)
	{
		delete this;
	}
	return uCount;
}

//------------------------------------------------------------------------------
//  QueryInterface
//------------------------------------------------------------------------------

STDMETHODIMP MFPVideoPlayer::QueryInterface(REFIID riid, void** ppv)
{
	static const QITAB qit[] = 
	{
		QITABENT(MFPVideoPlayer, IMFPMediaPlayerCallback),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

//********************* IMFPMediaPlayerCallback methods **********************//


//-------------------------------------------------------------------
// OnMediaPlayerEvent
// 
// Implements IMFPMediaPlayerCallback::OnMediaPlayerEvent.
// This callback method handles events from the MFPlay object.
//-------------------------------------------------------------------

void MFPVideoPlayer::OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader)
{
	if (FAILED(pEventHeader->hrEvent)) {
		NotifyError(pEventHeader->hrEvent);
		return;
	}

	switch (pEventHeader->eEventType) {
	case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
		OnMediaItemCreated(MFP_GET_MEDIAITEM_CREATED_EVENT(pEventHeader));
		break;

	case MFP_EVENT_TYPE_MEDIAITEM_SET:
		OnMediaItemSet(MFP_GET_MEDIAITEM_SET_EVENT(pEventHeader));
		break;

	case MFP_EVENT_TYPE_RATE_SET:
		break;

	case MFP_EVENT_TYPE_PLAYBACK_ENDED:
		SetPosition(0);
		__fallthrough;
	case MFP_EVENT_TYPE_STOP:
		{
			/*HWND hwndVideo = NULL;
			if (SUCCEEDED(m_pPlayer->GetVideoWindow(&hwndVideo)))
				InvalidateRect(hwndVideo, NULL, FALSE);*/
		}
		break;
	}

	NotifyState(pEventHeader->eState);
}

//-------------------------------------------------------------------
// OpenURL
//
// Open a media file by URL.
//-------------------------------------------------------------------

HRESULT MFPVideoPlayer::OpenURL(const WCHAR* sURL)
{
	HRESULT hr = S_OK;

	if (sURL == NULL)
	{
		return E_POINTER;
	}

	if (m_pPlayer == NULL)
	{
		return E_UNEXPECTED;
	}

	// Create a new media item for this URL.
	hr = m_pPlayer->CreateMediaItemFromURL(sURL, FALSE, 0, NULL);

	// The CreateMediaItemFromURL method completes asynchronously. When it does,
	// MFPlay sends an MFP_EVENT_TYPE_MEDIAITEM_CREATED event.

	return hr;
}

//-----------------------------------------------------------------------------
// Shutdown
//
// Shutdown the MFPlay object.
//-----------------------------------------------------------------------------

HRESULT MFPVideoPlayer::Shutdown()
{
	HRESULT hr = S_OK;
	if (m_pPlayer)
		hr = m_pPlayer->Shutdown();
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

//-----------------------------------------------------------------------------
// CanSeek
//
// Queries whether the current media file is seekable.
//-----------------------------------------------------------------------------

HRESULT MFPVideoPlayer::CanSeek(BOOL *pbCanSeek)
{
	*pbCanSeek = ((m_caps & MFP_MEDIAITEM_CAN_SEEK) && !(m_caps  & MFP_MEDIAITEM_HAS_SLOW_SEEK));
	return S_OK;
}

//-----------------------------------------------------------------------------
// GetDuration
//
// Gets the playback duration.
//-----------------------------------------------------------------------------

HRESULT MFPVideoPlayer::GetDuration(MFTIME *phnsDuration)
{
	HRESULT hr = E_FAIL;

	PROPVARIANT var;
	PropVariantInit(&var);
	if (m_pPlayer) {
		hr = m_pPlayer->GetDuration(MFP_POSITIONTYPE_100NS, &var);
		if (SUCCEEDED(hr)) {
			*phnsDuration = var.uhVal.QuadPart;
		}
	}

	PropVariantClear(&var);
	return hr;
}

//-----------------------------------------------------------------------------
// GetCurrentPosition
// 
// Gets the current playback position.
//-----------------------------------------------------------------------------

HRESULT MFPVideoPlayer::GetCurrentPosition(MFTIME *phnsPosition)
{
	HRESULT hr = E_FAIL;

	PROPVARIANT var;
	PropVariantInit(&var);
	if (m_pPlayer) {
		hr = m_pPlayer->GetPosition(MFP_POSITIONTYPE_100NS, &var);
		if (SUCCEEDED(hr)) {
			*phnsPosition = var.hVal.QuadPart;
		}
	}

	PropVariantClear(&var);
	return hr;
}

//-----------------------------------------------------------------------------
// SetPosition
//
// Sets the current playback position.
//-----------------------------------------------------------------------------

HRESULT MFPVideoPlayer::SetPosition(MFTIME hnsPosition)
{
	HRESULT hr = E_FAIL;

	PROPVARIANT var;
	PropVariantInit(&var);
	if (m_pPlayer) {
		var.vt = VT_I8;
		var.hVal.QuadPart = hnsPosition;
		hr = m_pPlayer->SetPosition(MFP_POSITIONTYPE_100NS, &var);
	}

	PropVariantClear(&var);
	return hr;
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
		NotifyError(hr);
		//ShowErrorMessage(L"Error playing this file.", hr);
	}
}

//-------------------------------------------------------------------
// OnMediaItemSet
//
// Called when the IMFPMediaPlayer::SetMediaItem method completes.
//-------------------------------------------------------------------

void MFPVideoPlayer::OnMediaItemSet(MFP_MEDIAITEM_SET_EVENT* pEvent)
{
	HRESULT hr = pEvent->header.hrEvent;
	while (1) {
		if (FAILED(hr))
			break;

		if (pEvent->pMediaItem) {
			hr = pEvent->pMediaItem->GetCharacteristics(&m_caps);
			if (FAILED(hr))
				break;
		}

		hr = m_pPlayer->Play();
		if (FAILED(hr)) {
			NotifyError(hr);
			//ShowErrorMessage(L"IMFPMediaPlayer::Play failed.", hr);
			break;
		}
		m_pPlayer->SetMute(TRUE);
		return;
	}
	if (FAILED(hr)) {
		//NotifyError(hr);
	}
}
