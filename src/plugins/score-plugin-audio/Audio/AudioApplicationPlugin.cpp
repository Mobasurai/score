#include "AudioApplicationPlugin.hpp"

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <Scenario/Application/ScenarioActions.hpp>

#include <Audio/AudioDevice.hpp>
#include <Audio/AudioInterface.hpp>
#include <Audio/AudioTick.hpp>
#include <Audio/Settings/Model.hpp>

#include <score/actions/ActionManager.hpp>
#include <score/tools/Bind.hpp>
#include <score/widgets/ControlWidgets.hpp>
#include <score/widgets/MessageBox.hpp>
#include <score/widgets/SetIcons.hpp>

#include <core/application/ApplicationSettings.hpp>
#include <core/document/Document.hpp>
#include <core/presenter/DocumentManager.hpp>

#include <ossia/audio/audio_engine.hpp>
#include <ossia/audio/audio_protocol.hpp>

#include <QToolBar>

SCORE_DECLARE_ACTION(RestartAudio, "Restart Audio", Common, QKeySequence::UnknownKey)
namespace Audio
{
ApplicationPlugin::ApplicationPlugin(const score::ApplicationContext& ctx)
    : score::ApplicationPlugin{ctx}
{
  startTimer(50);
}

void ApplicationPlugin::initialize()
{
  auto& set = context.settings<Audio::Settings::Model>();

  // First validate the current audio settings

  auto& engines = score::AppContext().interfaces<Audio::AudioFactoryList>();

  if(auto dev = engines.get(set.getDriver()))
  {
    dev->initialize(set, this->context);
  }

  con(set, &Audio::Settings::Model::changed, this, &ApplicationPlugin::restart_engine,
      Qt::QueuedConnection);
}

void ApplicationPlugin::on_documentChanged(
    score::Document* olddoc, score::Document* newdoc)
{
  restart_engine();
}

ApplicationPlugin::~ApplicationPlugin() { }

void ApplicationPlugin::timerEvent(QTimerEvent*)
{
  if(audio)
    audio->gc();

  for(auto it = previous_audio.begin(); it != previous_audio.end();)
  {
    auto& engine = **it;
    engine.gc();
    if(engine.stop_received)
    {
      it = previous_audio.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

// GUI part

GUIApplicationPlugin::GUIApplicationPlugin(const score::GUIApplicationContext& ctx)
    : score::GUIApplicationPlugin{ctx}
    , m_appPlug{ctx.applicationPlugin<Audio::ApplicationPlugin>()}
{
}

GUIApplicationPlugin::~GUIApplicationPlugin() { }
void GUIApplicationPlugin::initialize() { }

void ApplicationPlugin::restart_engine()
try
{
  if(m_updating_audio)
    return;

#if defined(__EMSCRIPTEN__)
  static bool init = 0;
  if(!init)
  {
    init = true;
    start_engine();
  }
#else
  stop_engine();
  start_engine();
#endif
}
catch(std::exception& e)
{
  score::warning(
      nullptr, tr("Audio Error"),
      tr("Warning: error while restarting the audio engine:\n%1").arg(e.what()));
}
catch(...)
{
  score::warning(
      nullptr, tr("Audio Error"),
      tr("Warning: audio engine stuck. "
         "Operation aborted. "
         "Check the audio settings."));
}

void ApplicationPlugin::stop_engine()
{
  if(audio)
  {
    for(auto d : context.documents.documents())
    {
      auto dev = (Dataflow::AudioDevice*)d->context()
                     .plugin<Explorer::DeviceDocumentPlugin>()
                     .list()
                     .audioDevice();
      if(dev)
      {
        if(auto proto = dev->getProtocol())
        {
          proto->stop();
        }
      }
    }

    audio->stop();
    previous_audio.push_back(std::move(audio));
    audio.reset();
  }
}

void ApplicationPlugin::start_engine()
{
  // auto& audio = m_appPlug.audio;
  // auto& previous_audio = m_appPlug.previous_audio;
  // auto& m_updating_audio = m_appPlug.m_updating_audio;

  if(auto doc = this->context.documents.currentDocument())
  {
    auto dev = (Dataflow::AudioDevice*)doc->context()
                   .plugin<Explorer::DeviceDocumentPlugin>()
                   .list()
                   .audioDevice();
    if(!dev)
      return;

    auto& set = this->context.settings<Audio::Settings::Model>();
    auto& engines = score::AppContext().interfaces<Audio::AudioFactoryList>();

    if(auto dev = engines.get(set.getDriver()))
    {
      try
      {
        SCORE_ASSERT(!audio);
        audio = dev->make_engine(set, this->context);
        if(!audio)
          throw std::runtime_error{""};

        m_updating_audio = true;
        auto bs = audio->effective_buffer_size;
        auto rate = audio->effective_sample_rate;
        set.setBufferSize(bs <= 0 ? 512 : bs);
        set.setRate(rate <= 0 ? 44100 : rate);
        m_updating_audio = false;
      }
      catch(...)
      {
        score::warning(
            nullptr, tr("Audio error"),
            tr("The desired audio settings could not be applied.\nPlease "
               "change "
               "them."));
      }
    }

    if(m_audioEngineAct)
      m_audioEngineAct->setChecked(bool(audio));

    dev->reconnect();
    if(audio)
    {
      audio->set_tick(Audio::makePauseTick(this->context));
    }
  }
}

score::GUIElements GUIApplicationPlugin::makeGUIElements()
{
  GUIElements e;

  auto& toolbars = e.toolbars;

  auto& audio = m_appPlug.audio;
  auto& m_audioEngineAct = m_appPlug.m_audioEngineAct;

  // The toolbar with the volume control
  m_audioEngineAct = new QAction{tr("Restart Audio"), this};
  m_audioEngineAct->setCheckable(true);
  m_audioEngineAct->setChecked(bool(audio));
  m_audioEngineAct->setStatusTip("Restart the audio engine");

  setIcons(
      m_audioEngineAct, QStringLiteral(":/icons/engine_on.png"),
      QStringLiteral(":/icons/engine_off.png"),
      QStringLiteral(":/icons/engine_disabled.png"), false);
  {
    auto bar = new QToolBar(tr("Volume"));
    auto sl = new score::VolumeSlider{bar};
    sl->setFixedSize(100, 20);
    sl->setValue(0.5);
    sl->setStatusTip("Change the master volume");
    bar->addWidget(sl);
    bar->addAction(m_audioEngineAct);
    connect(sl, &score::VolumeSlider::valueChanged, this, [=, &audio](double v) {
      if(!audio)
        return;

      auto doc = context.currentDocument();
      if(!doc)
        return;
      auto dev = (Dataflow::AudioDevice*)doc->plugin<Explorer::DeviceDocumentPlugin>()
                     .list()
                     .audioDevice();
      if(!dev)
        return;
      auto p = dev->getProtocol();
      if(!p)
        return;

      auto root = ossia::net::find_node(dev->getDevice()->get_root_node(), "/out/main");
      if(root)
      {
        if(auto p = root->get_parameter())
        {
          auto audio_p = static_cast<ossia::audio_parameter*>(p);
          audio_p->push_value(v);
        }
      }
    });

    toolbars.emplace_back(
        bar, StringKey<score::Toolbar>("Audio"), Qt::BottomToolBarArea, 400);
  }

  e.actions.container.reserve(2);
  e.actions.add<Actions::RestartAudio>(m_audioEngineAct);

  connect(
      m_audioEngineAct, &QAction::triggered, &m_appPlug,
      &ApplicationPlugin::restart_engine);

  return e;
}
}
