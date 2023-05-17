#include <Process/Style/Pixmaps.hpp>

#include <score/model/Skin.hpp>
#include <score/widgets/Pixmap.hpp>

#include <QApplication>
#include <QPainter>
#include <QPainterPath>

namespace Process
{
template <typename Fun>
static QPixmap drawPath(Fun fun)
{
  double dpr = qApp->devicePixelRatio();
  QImage img(10 * dpr, 10 * dpr, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::transparent);

  QPainter p(&img);
  QPainterPath path;
  fun(path, dpr);
  path.closeSubpath();

  p.setRenderHint(QPainter::Antialiasing, true);
  p.fillPath(path, score::Skin::instance().Gray);
  p.end();

  img.setDevicePixelRatio(dpr);
  return QPixmap::fromImage(img);
}

Pixmaps::Pixmaps() noexcept
    : show_ui_off{score::get_pixmap(":/icons/undock_on.png")}
    , show_ui_on{score::get_pixmap(":/icons/undock_off.png")}

    , preset_off{score::get_pixmap(":/icons/process_preset_icon.png")}
    , preset_on{score::get_pixmap(":/icons/process_preset_icon.png")}

    , record_off{score::get_pixmap(":/icons/process_record_off.png")}
    , record_on{score::get_pixmap(":/icons/process_record_on.png")}

    , snapshot_off{score::get_pixmap(":/icons/preset_snapshot_off.png")}
    , snapshot_on{score::get_pixmap(":/icons/preset_snapshot_on.png")}

    , net_sync_slot_header_off{score::get_pixmap(":/icons/net_16/pfa.png")}
    , net_sync_slot_header_on{score::get_pixmap(":/icons/net_16/pssa.png")}

    , net_sync_node_header_off{score::get_pixmap(":/icons/net_20/pfa.png")}
    , net_sync_node_header_on{score::get_pixmap(":/icons/net_20/pssa.png")}

    , close_off{score::get_pixmap(":/icons/close_on.png")}
    , close_on{score::get_pixmap(":/icons/close_off.png")}

    , nodal_off{score::get_pixmap(":/icons/nodal_off.png")}
    , nodal_on{score::get_pixmap(":/icons/nodal_on.png")}

    , timeline_off{score::get_pixmap(":/icons/timeline_off.png")}
    , timeline_on{score::get_pixmap(":/icons/timeline_on.png")}

    , unmuted{score::get_pixmap(":/icons/process_on.png")}
    , muted{score::get_pixmap(":/icons/process_off.png")}

    , play{score::get_pixmap(":/icons/play_off.png").scaled(15, 15)}
    , stop{score::get_pixmap(":/icons/stop_off.png").scaled(15, 15)}

    , unroll{score::get_pixmap(":/icons/rack_button_off.png")}
    , unroll_selected{score::get_pixmap(":/icons/rack_button_off_selected.png")}
    , roll{score::get_pixmap(":/icons/rack_button_on.png")}
    , roll_selected{score::get_pixmap(":/icons/rack_button_on_selected.png")}

    , unroll_small{score::get_pixmap(":/icons/rack_button_small_off.png")}
    , roll_small{score::get_pixmap(":/icons/rack_button_small_on.png")}

    , add{score::get_pixmap(":/icons/process_add_off.png")}
    , interval_play{score::get_pixmap(":/icons/process_interval_play.png")}
    , interval_stop{score::get_pixmap(":/icons/process_interval_stop.png")}

    , metricHandle{drawPath([](QPainterPath& path, double dpr) {
      path.lineTo(10 * dpr, 0);
      path.lineTo(0, 10 * dpr);
      path.lineTo(0, 0);
    })}
    , portHandleClosed{drawPath([](QPainterPath& path, double dpr) {
      path.lineTo(10 * dpr, 5 * dpr);
      path.lineTo(0, 10 * dpr);
    })}
    , portHandleOpen{drawPath([](QPainterPath& path, double dpr) {
      path.lineTo(5 * dpr, 10 * dpr);
      path.lineTo(10 * dpr, 0);
    })}
{
}

Pixmaps::~Pixmaps() { }

const Pixmaps& Pixmaps::instance() noexcept
{
  static const Pixmaps p;
  return p;
}
}
