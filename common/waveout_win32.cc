#include <vector>
#include "gears/base/common/waveout_win32.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/common_win32.h"
#include "gears/base/common/wave_desc.h"


#define ASSUME(a) \
  do { \
    assert(a); \
    if(!(a)) { \
      data_ = NULL; \
      data_length_ = 0;\
      wfx_ = NULL; \
      return false;\
    } \
  } while(0)

bool WaveOutWindow::ParseWaveForm (boost::shared_array<char> wav, int length) {
  RIFF_ChunkHeader *riff = (RIFF_ChunkHeader*)wav.get();

  //RIFF header sanity check
  ASSUME(riff);
  ASSUME(riff->ChunkID == kWaveFormChunkID);
  ASSUME(riff->Format == kWaveFormFormatIdentifier);
  ASSUME(riff->ChunkSize + 8 == length); //only chunk

  std::vector<RIFF_SubchunkHeader*> subchunks;
  char *eow = wav.get() + length;
  char *ptr = wav.get() + sizeof(RIFF_ChunkHeader);

  RIFF_SubchunkHeader *format_header, *data_header;

  //extract subchunks into vector
  while(ptr != eow) {
    RIFF_SubchunkHeader *current_subchunk = (RIFF_SubchunkHeader*)ptr;
    //pick what we needed
    switch(current_subchunk->SubchunkID) {
      case kWaveFormDataSubChunkID:
        data_header = current_subchunk;
        break;
      case kWaveFormFormatSubChunkID:
        format_header = current_subchunk;
        break;
      default:
        break;
    }
    ptr += current_subchunk->SubchunkSize + sizeof(RIFF_SubchunkHeader);
    ASSUME(ptr <= eow);
    subchunks.push_back(current_subchunk);
  }


  //format_header subchunk sanity check
  wfx_ = (WAVEFORMATEX*)((char*)format_header + sizeof(RIFF_SubchunkHeader));
  ASSUME(wfx_->nBlockAlign * 8 == wfx_->nChannels * wfx_->wBitsPerSample);
  ASSUME(wfx_->nAvgBytesPerSec == wfx_->nSamplesPerSec * wfx_->nBlockAlign);

  //data subchunk sanity check
  ASSUME(data_header->SubchunkID == kWaveFormDataSubChunkID);
  data_ = (char*)data_header + sizeof(RIFF_SubchunkHeader);
  data_length_ = data_header->SubchunkSize;
  return true;
}

#undef ASSUME

WaveOutWindow::WaveOutWindow():
                     wav_(),
                     length_(0),
                     wfx_(NULL),
                     data_(NULL),
                     played_(false),
                     callback_() {
}

#define CHECK_RETURN(a) \
  do { \
    if((a)) { \
      if (IsWindow()) { \
        DestroyWindow(); \
      } \
      return false; \
    } \
  } while(0)

bool WaveOutWindow::Play(boost::shared_array<char> wav, int length, CallbackType callback) {
  if(played_) {
    assert(!played_);
    return false;
  }
  callback_ = callback;
  length_ = length;
  wav_ = wav;

  CHECK_RETURN(!ParseWaveForm(wav, length) ||
      !Create(kMessageOnlyWindowParent, NULL, NULL, kMessageOnlyWindowStyle));

  ZeroMemory(&wave_header_, sizeof(WAVEHDR));
  CHECK_RETURN(waveOutOpen(&hwo_, WAVE_MAPPER, wfx_, (DWORD_PTR)(m_hWnd), NULL, CALLBACK_WINDOW) != MMSYSERR_NOERROR);

  wave_header_.lpData = data_;
  wave_header_.dwBufferLength = data_length_;
  CHECK_RETURN(waveOutPrepareHeader(hwo_, &wave_header_, sizeof(WAVEHDR)));
  CHECK_RETURN(waveOutWrite(hwo_, &wave_header_, sizeof(WAVEHDR)));

  self_ = shared_from_this();
  return true;
}

#undef CHECK_RETURN

LRESULT WaveOutWindow::OnDone(UINT msg, WPARAM wp, LPARAM lp, BOOL handled) {
  HWAVEOUT tHwo = (HWAVEOUT)wp;
  if (tHwo != hwo_) {
    return S_OK;
  }

  WAVEHDR* pwh = (WAVEHDR*) lp;
  if(pwh != &wave_header_) {
    return S_OK;
  }

  if (pwh->dwFlags & WHDR_PREPARED) {
    waveOutUnprepareHeader(tHwo, pwh, sizeof(WAVEHDR));
  }

  waveOutClose(tHwo);
  return S_OK;
}

LRESULT WaveOutWindow::OnClose(UINT msg, WPARAM wp, LPARAM lp, BOOL handled) {
  HWAVEOUT tHwo = (HWAVEOUT)wp;
  if (tHwo != hwo_) {
    return S_OK;
  }

  if (IsWindow()) {
    DestroyWindow();
  }
  //notify caller that we are done, let it remove our weak_ptr
  if(callback_) {
    callback_(self_);
  }
  return S_OK;
}

void WaveOutWindow::OnFinalMessage(HWND hWnd) {
  self_.reset();
}

void WaveOutWindow::ForceExit() {
  waveOutReset(hwo_);
  if (wave_header_.dwFlags & WHDR_PREPARED) {
    waveOutUnprepareHeader(hwo_, &(wave_header_), sizeof(WAVEHDR));
  }
  waveOutClose(hwo_);
  if (IsWindow()) {
    DestroyWindow();
  }
}

