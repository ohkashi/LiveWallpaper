#pragma once
#include <mfplay.h>
#include <mferror.h>


// Private window message to notify the application of playback events.
static const UINT WM_APP_NOTIFY = WM_APP + 1;   // wparam = MFP_MEDIAPLAYER_STATE

// Private window message to notify the application when an error occurs.
static const UINT WM_APP_ERROR = WM_APP + 2;    // wparam = HRESULT


//-------------------------------------------------------------------
//
// MediaPlayerCallback class
// 
// Implements the callback interface for MFPlay events.
//
//-------------------------------------------------------------------

class MFPVideoPlayer : public IMFPMediaPlayerCallback
{
public:
	static HRESULT CreateInstance(HWND hwndEvent, HWND hwndVideo, MFPVideoPlayer** ppPlayer);

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFPMediaPlayerCallback methods
	void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader);

	HRESULT OpenURL(const WCHAR* sURL);
	HRESULT Shutdown();
	MFP_MEDIAPLAYER_STATE GetState() noexcept;
	bool Play() noexcept;
	bool Pause() noexcept;
	bool Stop() noexcept;
	float GetVolume() noexcept;
	bool SetVolume(float fVolume) noexcept;
	bool GetMute() noexcept;
	bool SetMute(bool bMute) noexcept;

	// Seeking
	HRESULT GetDuration(MFTIME *phnsDuration);
	HRESULT CanSeek(BOOL *pbCanSeek);
	HRESULT GetCurrentPosition(MFTIME *phnsPosition);
	HRESULT SetPosition(MFTIME hnsPosition);

	inline void UpdateVideo() {
		if (m_pPlayer && m_bHasVideo)
			m_pPlayer->UpdateVideo();
	}

protected:
	MFPVideoPlayer(HWND hwndEvent);
	virtual ~MFPVideoPlayer();

	HRESULT Initialize(HWND hwndVideo);

	// NotifyState: Notifies the application when the state changes.
	void NotifyState(MFP_MEDIAPLAYER_STATE state)
	{
		PostMessage(m_hwndEvent, WM_APP_NOTIFY, (WPARAM)state, (LPARAM)0);
	}

	// NotifyError: Notifies the application when an error occurs.
	void NotifyError(HRESULT hr)
	{
		PostMessage(m_hwndEvent, WM_APP_ERROR, (WPARAM)hr, 0);
	}

	// MFPlay event handler functions.
	void OnMediaItemCreated(MFP_MEDIAITEM_CREATED_EVENT* pEvent);
	void OnMediaItemSet(MFP_MEDIAITEM_SET_EVENT* pEvent);

private:
	long					m_cRef;			// Reference count
	IMFPMediaPlayer*		m_pPlayer;		// The MFPlay player object.
	HWND					m_hwndEvent;	// App window to receive events.
	bool					m_bHasVideo;
	MFP_MEDIAITEM_CHARACTERISTICS	m_caps;
};
