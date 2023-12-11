#pragma once
#include <mfplay.h>
#include <mferror.h>


//-------------------------------------------------------------------
//
// MediaPlayerCallback class
// 
// Implements the callback interface for MFPlay events.
//
//-------------------------------------------------------------------

#include <Shlwapi.h>

class MFPVideoPlayer;

class MediaPlayerCallback : public IMFPMediaPlayerCallback
{
	long m_cRef; // Reference count
	MFPVideoPlayer* m_pPlayer;

public:
	MediaPlayerCallback(MFPVideoPlayer* pPlayer) : m_cRef(1), m_pPlayer(pPlayer)
	{
	}

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = 
		{
			QITABENT(MediaPlayerCallback, IMFPMediaPlayerCallback),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}
	STDMETHODIMP_(ULONG) AddRef() 
	{
		return InterlockedIncrement(&m_cRef); 
	}
	STDMETHODIMP_(ULONG) Release()
	{
		ULONG count = InterlockedDecrement(&m_cRef);
		if (count == 0)
		{
			delete this;
			return 0;
		}
		return count;
	}

	// IMFPMediaPlayerCallback methods
	void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader);
};


class MFPVideoPlayer
{
public:
	MFPVideoPlayer();
	~MFPVideoPlayer();

	HRESULT PlayMediaFile(HWND hwnd, const WCHAR* sURL);
	MFP_MEDIAPLAYER_STATE GetState() noexcept;
	bool Play() noexcept;
	bool Pause() noexcept;
	bool Stop() noexcept;
	float GetVolume() noexcept;
	bool SetVolume(float fVolume) noexcept;
	bool GetMute() noexcept;
	bool SetMute(bool bMute) noexcept;
	inline void UpdateVideo() {
		if (m_pPlayer && m_bHasVideo)
			m_pPlayer->UpdateVideo();
	}

protected:
	// MFPlay event handler functions.
	void OnMediaItemCreated(MFP_MEDIAITEM_CREATED_EVENT* pEvent);
	void OnMediaItemSet(MFP_MEDIAITEM_SET_EVENT* pEvent);
	void OnPlaybackEnded(MFP_MEDIAITEM_SET_EVENT* pEvent);
	void ShowErrorMessage(PCWSTR format, HRESULT hrErr);

private:
	IMFPMediaPlayer*		m_pPlayer;		// The MFPlay player object.
	MediaPlayerCallback*	m_pPlayerCB;	// Application callback object.
	bool					m_bHasVideo;

	friend class MediaPlayerCallback;
};
