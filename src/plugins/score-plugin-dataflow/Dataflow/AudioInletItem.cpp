#include "AudioInletItem.hpp"

#include <Process/Dataflow/AudioPortComboBox.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <Inspector/InspectorLayout.hpp>

#include <score/document/DocumentContext.hpp>

namespace Dataflow
{
void AudioInletFactory::setupInletInspector(
    const Process::Inlet& port, const score::DocumentContext& ctx, QWidget* parent,
    Inspector::Layout& lay, QObject* context)
{
  auto root = State::Address{"audio", {"in"}};
  auto& device = *ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
  auto d = device.list().audioDevice();
  const auto& node = d->getNode(root);

  auto edit = Process::makeAddressCombo(root, node, port, ctx, parent);
  lay.addRow(port.name(), edit);
}
}
