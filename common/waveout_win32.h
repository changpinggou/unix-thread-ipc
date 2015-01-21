#ifndef GEARS_BASE_COMMON_WAVEOUT_WIN32_H__
#define GEARS_BASE_COMMON_WAVEOUT_WIN32_H__
#ifdef WIN32

#include <windows.h>
#include <mmsystem.h>
#define BOOST_NO_TYPEID
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "gears/base/common/wtl_namespace_safe_headers.h"
#include "gears/base/common/basictypes.h"


struct FormatSubchunk;
struct DataSubChunk;

class WaveOutWindow : public CWindowImpl<WaveOutWindow>,
                      public boost::enable_shared_from_this<WaveOutWindow> {
 public:
  BEGIN_MSG_MAP(WaveOutWindow)
    MESSAGE_HANDLER(MM_WOM_DONE, OnDone)
    MESSAGE_HANDLER(MM_WOM_CLOSE, OnClose)
  END_MSG_MAP()

  typedef boost::function<void(boost::weak_ptr<WaveOutWindow>)> CallbackType;

  WaveOutWindow();
  bool Play(boost::shared_array<char> wav, int length, CallbackType callback);
  void ForceExit();
 private:
  LRESULT OnDone(UINT msg, WPARAM wp, LPARAM lp, BOOL handled);
  LRESULT OnClose(UINT msg, WPARAM wp, LPARAM lp, BOOL handled);
  virtual void OnFinalMessage(HWND hWnd);
  bool ParseWaveForm(boost::shared_array<char> wav, int length);
  DISALLOW_EVIL_CONSTRUCTORS(WaveOutWindow);

  boost::shared_array<char> wav_;
  char *data_;
  size_t data_length_;
  boost::shared_ptr<WaveOutWindow> self_;
  CallbackType callback_;
  int length_;
  WAVEFORMATEX *wfx_;
  HWAVEOUT hwo_;
  WAVEHDR wave_header_;
  bool played_;
};

#endif //WIN32
#endif //GEARS_BASE_COMMON_WAVEOUT_WIN32_H__
