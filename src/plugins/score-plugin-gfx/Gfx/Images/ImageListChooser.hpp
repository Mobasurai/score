#pragma once

#include <Process/Dataflow/Port.hpp>

#include <score/serialization/VisitorCommon.hpp>

#include <QGraphicsItem>
#include <QTableWidget>

#include <score_plugin_gfx_export.h>

namespace Gfx::Images
{
struct ImageListChooser;
}

UUID_METADATA(
    SCORE_PLUGIN_GFX_EXPORT, Process::Port, Gfx::Images::ImageListChooser,
    "e9e711ca-62c6-43b7-bb51-bbb9ca1e0306")

namespace WidgetFactory
{
struct ImageListChooserItems
{
  static constexpr Process::PortItemLayout layout() noexcept
  {
    using namespace Process;
    return {};
  }

  static QWidget* make_widget(
      const Gfx::Images::ImageListChooser& inlet, const score::DocumentContext& ctx,
      QWidget* parent, QObject* context);

  static QGraphicsItem* make_item(
      const Gfx::Images::ImageListChooser& slider,
      const Gfx::Images::ImageListChooser& inlet, const score::DocumentContext& ctx,
      QGraphicsItem* parent, QObject* context);
};
}

namespace Gfx::Images
{
struct ImageListChooser : public Process::ControlInlet
{
  MODEL_METADATA_IMPL(ImageListChooser)
  ImageListChooser(
      const std::vector<QString>& init, const QString& name, Id<Process::Port> id,
      QObject* parent);
  ~ImageListChooser();
  ImageListChooser(DataStream::Deserializer& vis, QObject* parent);
  ImageListChooser(JSONObject::Deserializer& vis, QObject* parent);
  ImageListChooser(DataStream::Deserializer&& vis, QObject* parent);
  ImageListChooser(JSONObject::Deserializer&& vis, QObject* parent);

  using Process::ControlInlet::ControlInlet;
};
}
