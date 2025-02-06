#include "MidiOutletItem.hpp"

#include <Process/Dataflow/AudioPortComboBox.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <Inspector/InspectorLayout.hpp>

#if defined(SCORE_PLUGIN_PROTOCOLS)
#include <Protocols/MIDI/MIDIProtocolFactory.hpp>
#include <Protocols/MIDI/MIDISpecificSettings.hpp>
#endif

#include <score/document/DocumentContext.hpp>

namespace Dataflow
{
void MidiOutletFactory::setupOutletInspector(
    const Process::Outlet& port, const score::DocumentContext& ctx, QWidget* parent,
    Inspector::Layout& lay, QObject* context)
{
#if defined(SCORE_PLUGIN_PROTOCOLS)
  static const constexpr auto midi_uuid
      = Protocols::MIDIOutputProtocolFactory::static_concreteKey();

  auto& device = *ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
  auto cond = [](Device::DeviceInterface& dev) {
    auto& set = dev.settings();
    if(set.protocol == midi_uuid)
    {
      const auto& midi_set
          = set.deviceSpecificSettings.value<Protocols::MIDISpecificSettings>();
      if(midi_set.io == Protocols::MIDISpecificSettings::IO::Out)
        return true;
    }
    return false;
  };

  lay.addRow(
      port.name(), Process::makeDeviceCombo(cond, device.list(), port, ctx, parent));
#endif
}
}
