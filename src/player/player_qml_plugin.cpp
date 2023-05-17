// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "player_qml_plugin.hpp"

#include <ossia/context.hpp>
#include <ossia/detail/logger.hpp>

#include <ossia-qt/qml_plugin.hpp>

#include <QQmlEngine>

#include <spdlog/sinks/android_sink.h>

namespace score
{
//! A sink to use with spdlog, that will send its log messages through Qt logs.
struct qt_log_sink final : public spdlog::sinks::sink
{
  void log(const spdlog::details::log_msg& msg) override
  {
    QMessageLogger m;
    switch(msg.level)
    {
      case spdlog::level::info:
      case spdlog::level::trace: {
        m.info("%s", msg.payload.data());
        break;
      }
      case spdlog::level::debug: {
        m.debug("%s", msg.payload.data());
        break;
      }
      case spdlog::level::warn: {
        m.warning("%s", msg.payload.data());
        break;
      }
      case spdlog::level::err:
      case spdlog::level::critical: {
        m.critical("%s", msg.payload.data());
        break;
      }
      case spdlog::level::off: {
        break;
      }
    }
  }

  void set_pattern(const std::string& pattern) override { }
  void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override { }
  void flush() override { }
};

QMLPlayer::QMLPlayer()
{
  m_player.init();
#if defined(OSSIA_QML_DEVICE)
  registerDevice(&ossia::qt::qml_singleton_device::instance());
#endif
}

QMLPlayer::~QMLPlayer() { }

int QMLPlayer::port() const
{
  return m_port;
}

void QMLPlayer::load(QString str)
{
  m_player.sig_loadFile(str);
}

void QMLPlayer::play()
{
  m_player.sig_play();
}

void QMLPlayer::stop()
{
  m_player.sig_stop();
}

void QMLPlayer::registerDevice(ossia::qt::qml_device* dev)
{
#if defined(OSSIA_QML_DEVICE)
  if(dev)
    m_player.registerDevice(&dev->device());
#endif
}

void QMLPlayer::setPort(int p)
{
  if(m_port != p)
  {
    m_port = p;
    m_player.sig_setPort(p);
    portChanged(p);
  }
}

PlayerPlugin::PlayerPlugin() { }

void PlayerPlugin::registerTypes(const char* uri)
{
#if defined(__ANDROID__)
  ossia::context c{{std::make_shared<spdlog::sinks::android_sink>()}};
#else
  ossia::context c{{std::make_shared<qt_log_sink>()}};
#endif

  qmlRegisterType<score::QMLPlayer>(uri, 1, 0, "Player");
  ossia::qt::qml_plugin::reg(uri);
}

PlayerPlugin::~PlayerPlugin() { }
}
