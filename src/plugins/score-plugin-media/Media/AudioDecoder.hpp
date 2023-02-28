#pragma once
#include <Process/TimeValue.hpp>

#include <Media/AudioArray.hpp>

#include <ossia/detail/flicks.hpp>
#include <ossia/detail/optional.hpp>

#include <QThread>

#include <score_plugin_media_export.h>

#include <atomic>
#include <vector>
#include <verdigris>

struct AVFrame;
struct SwrContext;

namespace Media
{
struct AudioInfo
{
  int32_t fileRate{};
  int32_t convertedRate{};
  int64_t channels{};
  int64_t fileLength{};
  int64_t convertedLength{};
  int64_t max_arr_length{};
  std::optional<double> tempo;

  // Duration
  TimeVal duration() const noexcept;
};

class SCORE_PLUGIN_MEDIA_EXPORT AudioDecoder : public QObject
{
  W_OBJECT(AudioDecoder)

public:
  AudioDecoder(int rate);
  ~AudioDecoder();
  static std::optional<AudioInfo> do_probe(const QString& path);
  void decode(const QString& path, int track, audio_handle hdl);

  static std::optional<std::pair<AudioInfo, audio_array>>
  decode_synchronous(const QString& path, int rate);

  static QHash<QString, AudioInfo>& database();

  int32_t fileSampleRate{};
  int32_t convertedSampleRate{};
  int32_t channels{};
  int32_t track{-1};
  std::size_t decoded{};

public:
  void newData() W_SIGNAL(newData);
  void finishedDecoding(audio_handle hdl) W_SIGNAL(finishedDecoding, hdl);

  void startDecode(QString str, audio_handle hdl) W_SIGNAL(startDecode, str, hdl);

public:
  void on_startDecode(QString, audio_handle hdl);
  W_SLOT(on_startDecode);

private:
  static double read_length(const QString& path);

  QThread* m_baseThread{};
  QThread m_decodeThread;

  template <typename Decoder>
  void decodeFrame(Decoder dec, audio_array& data, AVFrame& frame);

  template <typename Decoder>
  void decodeRemaining(Decoder dec, audio_array& data, AVFrame& frame);
  std::vector<SwrContext*> resampler;
  void initResample();
};
}
