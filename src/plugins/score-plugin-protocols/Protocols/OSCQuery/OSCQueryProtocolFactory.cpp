// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "OSCQueryProtocolFactory.hpp"

#include "OSCQueryDevice.hpp"

#include <Device/Protocol/DeviceSettings.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <Protocols/OSCQuery/OSCQueryProtocolSettingsWidget.hpp>
#include <Protocols/OSCQuery/OSCQuerySpecificSettings.hpp>

#include <QNetworkAccessManager>
#include <QNetworkReply>

#if defined(OSSIA_DNSSD)
#include <Protocols/DNSSDDeviceEnumerator.hpp>
#endif

#include <ossia/network/base/device.hpp>

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>

namespace Protocols
{
#if defined(OSSIA_DNSSD)
class OSCQueryEnumerator final : public DNSSDEnumerator
{
public:
  OSCQueryEnumerator()
      : DNSSDEnumerator{"_oscjson._tcp"}
  {
    start();
  }

private:
  void addNewDevice(
      const std::string& instance, const std::string& ip,
      const std::string& port) noexcept override
  {
    using namespace std::literals;

    Device::DeviceSettings set;
    set.name = QString::fromStdString(instance);
    set.protocol = OSCQueryProtocolFactory::static_concreteKey();

    bool websockets = m_serv.get(instance, "WebSockets") == "true"sv;

    {
      QString req = QString("http://%1:%2/?HOST_INFO")
                        .arg(QString::fromStdString(ip))
                        .arg(QString::fromStdString(port));

      QNetworkRequest qreq{QUrl(req)};
      qreq.setTransferTimeout(1000);
      QPointer<QNetworkReply> ret = m_http.get(qreq);
      connect(ret, &QNetworkReply::errorOccurred, this, [] {});
#if QT_CONFIG(ssl)
      connect(ret, &QNetworkReply::sslErrors, this, [] {});
#endif
      connect(ret, &QNetworkReply::finished, this, [=]() mutable {
        auto doc = QJsonDocument::fromJson(ret->readAll());
        QString newName = doc.object()["NAME"].toString();

        if(!newName.isEmpty())
          set.name = newName;
        QString ws_ip = doc.object()["WS_IP"].toString();
        QString ws_port = doc.object()["WS_PORT"].toString();
        websockets |= (!ws_ip.isEmpty() || !ws_port.isEmpty());
        OSCQuerySpecificSettings sub;
        sub.host = QString("%1://%2:%3")
                       .arg(websockets ? "ws" : "http")
                       .arg(ws_ip.isEmpty() ? ip.c_str() : ws_ip)
                       .arg(ws_port.isEmpty() ? port.c_str() : ws_port);

        set.deviceSpecificSettings = QVariant::fromValue(std::move(sub));
        deviceAdded(set);
        ret->deleteLater();
      });
    }
  }

  mutable QNetworkAccessManager m_http;
};

#endif

QString OSCQueryProtocolFactory::prettyName() const noexcept
{
  return QObject::tr("OSCQuery");
}

QString OSCQueryProtocolFactory::category() const noexcept
{
  return StandardCategories::osc;
}

int OSCQueryProtocolFactory::visualPriority() const noexcept
{
  return 2;
}

Device::DeviceEnumerators
OSCQueryProtocolFactory::getEnumerators(const score::DocumentContext& ctx) const
{
#if defined(OSSIA_DNSSD)
  try
  {
    return {{"Network", new OSCQueryEnumerator}};
  }
  catch(...)
  {
  }
#endif
  return {};
}

Device::DeviceInterface* OSCQueryProtocolFactory::makeDevice(
    const Device::DeviceSettings& settings, const Explorer::DeviceDocumentPlugin& plugin,
    const score::DocumentContext& ctx)
{
  return new OSCQueryDevice{settings, plugin.networkContext()};
}

const Device::DeviceSettings& OSCQueryProtocolFactory::defaultSettings() const noexcept
{
  static const Device::DeviceSettings settings = [&]() {
    Device::DeviceSettings s;
    s.protocol = concreteKey();
    s.name = "OSCQuery";
    OSCQuerySpecificSettings specif;
    s.deviceSpecificSettings = QVariant::fromValue(specif);
    return s;
  }();
  return settings;
}

Device::ProtocolSettingsWidget* OSCQueryProtocolFactory::makeSettingsWidget()
{
  return new OSCQueryProtocolSettingsWidget;
}

QVariant OSCQueryProtocolFactory::makeProtocolSpecificSettings(
    const VisitorVariant& visitor) const
{
  return makeProtocolSpecificSettings_T<OSCQuerySpecificSettings>(visitor);
}

void OSCQueryProtocolFactory::serializeProtocolSpecificSettings(
    const QVariant& data, const VisitorVariant& visitor) const
{
  serializeProtocolSpecificSettings_T<OSCQuerySpecificSettings>(data, visitor);
}

bool OSCQueryProtocolFactory::checkCompatibility(
    const Device::DeviceSettings& a, const Device::DeviceSettings& b) const noexcept
{
  return a.name != b.name;
}
}
