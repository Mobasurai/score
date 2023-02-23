#pragma once
#include <Media/Libav.hpp>
#if SCORE_HAS_LIBAV
#include <score_plugin_media_export.h>
extern "C" {
#include <libavutil/pixfmt.h>
struct AVFrame;
struct AVCodecContext;
struct AVPacket;
}
#include <memory>

namespace Video
{
struct SCORE_PLUGIN_MEDIA_EXPORT VideoMetadata
{
  int width{};
  int height{};
  double fps{};
  AVPixelFormat pixel_format = AVPixelFormat(-1);
  bool realTime{};
  double flicks_per_dts{};
  double dts_per_flicks{};
};

struct SCORE_PLUGIN_MEDIA_EXPORT VideoInterface : VideoMetadata
{
  virtual ~VideoInterface();
  virtual AVFrame* dequeue_frame() noexcept = 0;
  virtual void release_frame(AVFrame* frame) noexcept = 0;
};

struct SCORE_PLUGIN_MEDIA_EXPORT ReadFrame
{
  AVFrame* frame{};
  int error{};
};
struct SCORE_PLUGIN_MEDIA_EXPORT FreeAVFrame
{
  void operator()(AVFrame* f) const noexcept;
};

using AVFramePointer = std::unique_ptr<AVFrame, FreeAVFrame>;

ReadFrame
readVideoFrame(AVCodecContext* codecContext, const AVPacket* pkt, AVFrame* frame, bool ignorePts);
}
#endif
