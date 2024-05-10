#pragma once
#include <score/plugins/InterfaceList.hpp>
#include <score/plugins/UuidKeySerialization.hpp>

#include <score_plugin_audio_export.h>

#include <verdigris>
class QComboBox;
namespace ossia
{
class audio_engine;
}
namespace score
{
struct ApplicationContext;
class SettingsCommandDispatcher;
}

namespace Audio
{
namespace Settings
{
class Model;
class View;
}

class SCORE_PLUGIN_AUDIO_EXPORT AudioFactory : public score::InterfaceBase
{
  SCORE_INTERFACE(AudioFactory, "f08e5469-eb29-4c39-9115-1d110cee2369")
public:
  ~AudioFactory() override;

  virtual bool available() const noexcept = 0;
  virtual QString prettyName() const = 0;
  virtual void
  initialize(Audio::Settings::Model& set, const score::ApplicationContext& ctx)
      = 0;
  virtual std::shared_ptr<ossia::audio_engine> make_engine(
      const Audio::Settings::Model& settings, const score::ApplicationContext& ctx)
      = 0;
  virtual QWidget* make_settings(
      Audio::Settings::Model& m, Audio::Settings::View& v,
      score::SettingsCommandDispatcher&, QWidget* parent)
      = 0;

  static QComboBox* addBufferSizeWidget(
      QWidget& widg, Audio::Settings::Model& m, Audio::Settings::View& v);
  static QComboBox* addSampleRateWidget(
      QWidget& widg, Audio::Settings::Model& m, Audio::Settings::View& v);
};

class AudioFactoryList final : public score::InterfaceList<AudioFactory>
{
};
}

Q_DECLARE_METATYPE(UuidKey<Audio::AudioFactory>)
W_REGISTER_ARGTYPE(UuidKey<Audio::AudioFactory>)
