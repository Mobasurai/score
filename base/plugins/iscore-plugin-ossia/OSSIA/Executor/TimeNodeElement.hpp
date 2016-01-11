#pragma once
#include <QObject>
#include <memory>
#include <iscore_plugin_ossia_export.h>

class DeviceList;
class TimeNodeModel;

namespace OSSIA
{
    class TimeNode;
}

namespace RecreateOnPlay
{
class ISCORE_PLUGIN_OSSIA_EXPORT TimeNodeElement final : public QObject
{
    public:
        TimeNodeElement(
                std::shared_ptr<OSSIA::TimeNode> ossia_tn,
                const TimeNodeModel& element,
                const DeviceList& devlist,
                QObject* parent);

        std::shared_ptr<OSSIA::TimeNode> OSSIATimeNode() const;

    private:
        std::shared_ptr<OSSIA::TimeNode> m_ossia_node;
        const TimeNodeModel& m_iscore_node;

        const DeviceList& m_deviceList;
};

}
