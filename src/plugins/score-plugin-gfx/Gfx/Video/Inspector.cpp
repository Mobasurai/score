#include "Inspector.hpp"

#include <Inspector/InspectorLayout.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/tools/Bind.hpp>
#include <score/widgets/SignalUtils.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

namespace Gfx::Video
{
InspectorWidget::InspectorWidget(
    const Gfx::Video::Model& object, const score::DocumentContext& context,
    QWidget* parent)
    : InspectorWidgetDelegate_T{object, parent}
    , m_dispatcher{context.commandStack}
{
  auto lay = new Inspector::Layout{this};
  {
    auto edit = new QLineEdit{object.path(), this};
    lay->addRow(tr("Path"), edit);

    con(object, &Gfx::Video::Model::pathChanged, this, [edit](const QString& str) {
      if(str != edit->text())
        edit->setText(str);
    });
    connect(edit, &QLineEdit::editingFinished, this, [this, edit] {
      if(edit->text() != this->process().path())
        this->m_dispatcher.submit<ChangeVideo>(this->process(), edit->text());
    });
  }

  {
    auto cb = new QCheckBox{};
    cb->setChecked(!object.ignoreTempo());

    auto spin = new QDoubleSpinBox;
    spin->setRange(1., 500.);
    spin->setDecimals(1);
    spin->setValue(object.nativeTempo());

    auto combo = new QComboBox;
    combo->addItems(
        {tr("Original size"), tr("Expand (Black bars)"), tr("Expand (Fill)"),
         tr("Stretch")});
    combo->setCurrentIndex((int)object.scaleMode());

    con(object, &Gfx::Video::Model::ignoreTempoChanged, this, [=](bool ignore) {
      if(cb->isChecked() != (!ignore))
        cb->setChecked(!ignore);
    });

    con(object, &Gfx::Video::Model::nativeTempoChanged, this, [=](double tempo) {
      if(spin->value() != tempo)
        spin->setValue(tempo);
    });
    con(object, &Gfx::Video::Model::scaleModeChanged, this,
        [=](score::gfx::ScaleMode sm) {
      int idx = (int)sm;
      if(combo->currentIndex() != idx)
        combo->setCurrentIndex(idx);
    });

    connect(cb, &QCheckBox::toggled, this, [this](bool t) {
      if((!t) != this->process().ignoreTempo())
      {
        this->m_dispatcher.submit<ChangeIgnoreTempo>(this->process(), !t);
      }
    });

    connect(
        spin, SignalUtils::SpinBox_valueChanged<QDoubleSpinBox>(), this,
        [this](double t) {
      if(t != this->process().nativeTempo())
      {
        this->m_dispatcher.submit<ChangeTempo>(this->process(), t);
      }
        });

    connect(
        combo, qOverload<int>(&QComboBox::currentIndexChanged), &object,
        [this](int idx) {
      auto new_mode = (score::gfx::ScaleMode)idx;
      if(new_mode != this->process().scaleMode())
      {
        this->m_dispatcher.submit<ChangeVideoScaleMode>(this->process(), new_mode);
      }
        });

    lay->addRow(tr("Scale"), combo);
    lay->addRow(tr("Enable tempo"), cb);
    lay->addRow(tr("Tempo"), spin);
  }
}

InspectorWidget::~InspectorWidget() { }
}
