// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "Component.hpp"

#include "JSAPIWrapper.hpp"
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <JS/Qml/ValueTypes.Qt5.hpp>
#else
#include <JS/Qml/ValueTypes.Qt6.hpp>
#endif

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <Scenario/Execution/score2OSSIA.hpp>

#include <Execution/DocumentPlugin.hpp>
#include <JS/ConsolePanel.hpp>
#include <JS/JSProcessModel.hpp>
#include <JS/Qml/Metatypes.hpp>
#include <Library/LibrarySettings.hpp>

#include <score/serialization/AnySerialization.hpp>
#include <score/serialization/MapSerialization.hpp>
#include <score/tools/Bind.hpp>

#include <ossia/dataflow/graph/graph_interface.hpp>
#include <ossia/dataflow/graph_edge.hpp>
#include <ossia/dataflow/graph_edge_helpers.hpp>
#include <ossia/dataflow/port.hpp>
#include <ossia/detail/logger.hpp>
#include <ossia/detail/ssize.hpp>
#include <ossia/editor/scenario/time_interval.hpp>
#include <ossia/editor/state/message.hpp>
#include <ossia/editor/state/state.hpp>

#include <ossia-qt/invoke.hpp>
#include <ossia-qt/js_utilities.hpp>
#include <ossia-qt/time.hpp>
#include <ossia-qt/token_request.hpp>

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QQmlComponent>
#include <QQmlContext>

#include <vector>

namespace JS
{
namespace Executor
{
class js_node final : public ossia::graph_node
{
public:
  js_node(ossia::execution_state& st);
  ~js_node();

  void setScript(const QString& val);

  void run(const ossia::token_request& t, ossia::exec_state_facade) noexcept override;

  ossia::execution_state& m_st;
  QQmlEngine* m_engine{};
  std::vector<Inlet*> m_jsInlets;
  std::vector<std::pair<ControlInlet*, ossia::inlet_ptr>> m_ctrlInlets;
  std::vector<std::pair<Impulse*, ossia::inlet_ptr>> m_impulseInlets;
  std::vector<std::pair<ValueInlet*, ossia::inlet_ptr>> m_valInlets;
  std::vector<std::pair<ValueOutlet*, ossia::outlet_ptr>> m_valOutlets;
  std::vector<std::pair<AudioInlet*, ossia::inlet_ptr>> m_audInlets;
  std::vector<std::pair<AudioOutlet*, ossia::outlet_ptr>> m_audOutlets;
  std::vector<std::pair<MidiInlet*, ossia::inlet_ptr>> m_midInlets;
  std::vector<std::pair<MidiOutlet*, ossia::outlet_ptr>> m_midOutlets;
  JS::Script* m_object{};
  ExecStateWrapper* m_execFuncs{};
  QJSValueList m_tickCall;
  std::size_t m_gcIndex{};

  void setupComponent_gui(JS::Script*);

  void setControl(std::size_t index, const QVariant& val)
  {
    if(index > m_jsInlets.size())
      return;
    if(auto v = qobject_cast<ValueInlet*>(m_jsInlets[index]))
      v->setValue(val);
  }
  void impulse(std::size_t index)
  {
    if(index > m_jsInlets.size())
      return;
    if(auto v = qobject_cast<Impulse*>(m_jsInlets[index]))
      v->impulse();
  }

private:
  void setupComponent(QQmlComponent& c);
};

struct js_process final : public ossia::node_process
{
  using node_process::node_process;
  js_node& js() const { return static_cast<js_node&>(*node); }
  void start() override
  {
    if(auto obj = js().m_object)
      if(obj->start().isCallable())
        obj->start().call();
  }
  void stop() override
  {
    if(auto obj = js().m_object)
      if(obj->stop().isCallable())
        obj->stop().call();
  }
  void pause() override
  {
    if(auto obj = js().m_object)
      if(obj->pause().isCallable())
        obj->pause().call();
  }
  void resume() override
  {
    if(auto obj = js().m_object)
      if(obj->resume().isCallable())
        obj->resume().call();
  }
  void transport_impl(ossia::time_value date) override
  {
    QMetaObject::invokeMethod(
        js().m_object, "transport", Qt::DirectConnection,
        Q_ARG(QVariant, double(date.impl)));
  }
  void offset_impl(ossia::time_value date) override
  {
    QMetaObject::invokeMethod(
        js().m_object, "offset", Qt::DirectConnection,
        Q_ARG(QVariant, double(date.impl)));
  }
};
Component::Component(
    JS::ProcessModel& element, const ::Execution::Context& ctx, QObject* parent)
    : ::Execution::ProcessComponent_T<JS::ProcessModel, ossia::node_process>{
        element, ctx, "JSComponent", parent}
{
  std::shared_ptr<js_node> node
      = ossia::make_node<js_node>(*ctx.execState, *ctx.execState);
  this->node = node;
  auto proc = std::make_shared<js_process>(node);
  m_ossia_process = proc;

  on_scriptChange(element.qmlData());
  con(element, &JS::ProcessModel::qmlDataChanged, this, &Component::on_scriptChange,
      Qt::DirectConnection);
}

Component::~Component() { }

void Component::on_scriptChange(const QString& script)
{
  auto& setup = system().setup;
  Execution::Transaction commands{system()};
  auto old_node = std::dynamic_pointer_cast<js_node>(node);

  // 0. Unregister all the previous inlets / outlets
  setup.unregister_node_soft(
      process().inlets(), process().outlets(), old_node, commands);

  // 1. Create new inlet & outlet arrays
  ossia::inlets inls;
  ossia::outlets outls;
  std::vector<Execution::ExecutionCommand> controlSetups;

  {
    if(auto object = process().currentObject())
    {
      int idx = 0;
      for(auto n : object->children())
      {
        if(auto ctrl_in = qobject_cast<ControlInlet*>(n))
        {
          inls.push_back(new ossia::value_inlet);
          inls.back()->target<ossia::value_port>()->is_event = false;

          ++idx;
        }
        else if(auto val_in = qobject_cast<ValueInlet*>(n))
        {
          inls.push_back(new ossia::value_inlet);
          auto& vp = *inls.back()->target<ossia::value_port>();

          vp.is_event = !val_in->isEvent();
          if(auto ctrl = qobject_cast<Process::ControlInlet*>(process().inlets()[idx]))
          {
            vp.type = ctrl->value().get_type();
            vp.domain = ctrl->domain().get();

            disconnect(ctrl, nullptr, this, nullptr);
            if(auto impulse = qobject_cast<Process::ImpulseButton*>(ctrl))
            {
              connect(
                  ctrl, &Process::ControlInlet::valueChanged, this,
                  [=](const ossia::value& val) {
                this->in_exec([old_node, idx] { old_node->impulse(idx); });
                  });
            }
            else
            {
              // Common case
              connect(
                  ctrl, &Process::ControlInlet::valueChanged, this,
                  [=](const ossia::value& val) {
                this->in_exec([old_node, val, idx] {
                  old_node->setControl(idx, val.apply(ossia::qt::ossia_to_qvariant{}));
                });
                  });
              controlSetups.push_back([old_node, val = ctrl->value(), idx] {
                old_node->setControl(idx, val.apply(ossia::qt::ossia_to_qvariant{}));
              });
            }
          }

          ++idx;
        }
        else if(auto aud_in = qobject_cast<AudioInlet*>(n))
        {
          inls.push_back(new ossia::audio_inlet);

          ++idx;
        }
        else if(auto mid_in = qobject_cast<MidiInlet*>(n))
        {
          inls.push_back(new ossia::midi_inlet);

          ++idx;
        }
        else if(auto val_out = qobject_cast<ValueOutlet*>(n))
        {
          outls.push_back(new ossia::value_outlet);
        }
        else if(auto aud_out = qobject_cast<AudioOutlet*>(n))
        {
          outls.push_back(new ossia::audio_outlet);
        }
        else if(auto mid_out = qobject_cast<MidiOutlet*>(n))
        {
          outls.push_back(new ossia::midi_outlet);
        }
      }
    }
  }

  // Send the updates to the node
  auto recable = std::shared_ptr<ossia::recabler>(
      new ossia::recabler{old_node, system().execGraph, inls, outls});
  commands.push_back([old_node, script, recable]() mutable {
    // Note: we need to do this because we try to keep the Javascript node around
    // because it's slow to recreate.
    // But this causes a lot of problems, it'd be better to do like e.g. the faust
    // process and entirely recreate a new node, + call update node.
    (*recable)();

    old_node->setScript(std::move(script));
  });

  commands.commands.reserve(commands.commands.size() + controlSetups.size());
  // OPTIMIZEME std::move algorithm
  for(auto& cmd : controlSetups)
    commands.commands.push_back(std::move(cmd));
  controlSetups.clear();

  // Register the new inlets
  SCORE_ASSERT(process().inlets().size() == inls.size());
  SCORE_ASSERT(process().outlets().size() == outls.size());

  for(std::size_t i = 0; i < inls.size(); i++)
  {
    setup.register_inlet(*process().inlets()[i], inls[i], node, commands);
  }
  for(std::size_t i = 0; i < outls.size(); i++)
  {
    setup.register_outlet(*process().outlets()[i], outls[i], node, commands);
  }

  commands.run_all();

  m_oldInlets = process().inlets();
  m_oldOutlets = process().outlets();
}

js_node::js_node(ossia::execution_state& st)
    : m_st{st}
{
}

js_node::~js_node()
{
  delete m_engine;
}

void js_node::setupComponent(QQmlComponent& c)
{
  auto object = c.create();
  if((m_object = qobject_cast<JS::Script*>(object)))
  {
    m_object->setParent(m_engine);
    int input_i = 0;
    int output_i = 0;

    for(auto n : m_object->children())
    {
      if(auto imp_in = qobject_cast<Impulse*>(n))
      {
        m_jsInlets.push_back(imp_in);
        m_impulseInlets.push_back({imp_in, m_inlets[input_i++]});
      }
      else if(auto ctrl_in = qobject_cast<ControlInlet*>(n))
      {
        m_jsInlets.push_back(ctrl_in);
        m_ctrlInlets.push_back({ctrl_in, m_inlets[input_i++]});
      }
      else if(auto val_in = qobject_cast<ValueInlet*>(n))
      {
        m_jsInlets.push_back(val_in);
        m_valInlets.push_back({val_in, m_inlets[input_i++]});
      }
      else if(auto aud_in = qobject_cast<AudioInlet*>(n))
      {
        m_jsInlets.push_back(aud_in);
        m_audInlets.push_back({aud_in, m_inlets[input_i++]});
      }
      else if(auto mid_in = qobject_cast<MidiInlet*>(n))
      {
        m_jsInlets.push_back(mid_in);
        m_midInlets.push_back({mid_in, m_inlets[input_i++]});
      }
      else if(auto val_out = qobject_cast<ValueOutlet*>(n))
      {
        m_valOutlets.push_back({val_out, m_outlets[output_i++]});
      }
      else if(auto aud_out = qobject_cast<AudioOutlet*>(n))
      {
        m_audOutlets.push_back({aud_out, m_outlets[output_i++]});
      }
      else if(auto mid_out = qobject_cast<MidiOutlet*>(n))
      {
        m_midOutlets.push_back({mid_out, m_outlets[output_i++]});
      }
    }
  }
  else
  {
    delete object;
  }
}
void js_node::setScript(const QString& val)
{
  if(!m_engine)
  {
    m_engine = new QQmlEngine;
    m_execFuncs = new ExecStateWrapper{m_st, m_engine};
    m_engine->rootContext()->setContextProperty("Device", m_execFuncs);

    QObject::connect(
        m_execFuncs, &ExecStateWrapper::system, qApp,
        [=](const QString& code) {
      std::thread{[=] { ::system(code.toStdString().c_str()); }}.detach();
        },
        Qt::QueuedConnection);

    if(auto* js_panel = score::GUIAppContext().findPanel<JS::PanelDelegate>())
    {
      QObject::connect(
          m_execFuncs, &ExecStateWrapper::exec, js_panel, &JS::PanelDelegate::evaluate,
          Qt::QueuedConnection);
      QObject::connect(
          m_execFuncs, &ExecStateWrapper::compute, m_execFuncs,
          [=](const QString& code, const QString& cbname) {
        // Exec thread

        // Callback ran in UI thread
        auto cb = [=](QVariant v) {
          // Go back to exec thread, we have to go through the normal engine exec ctx
          ossia::qt::run_async(m_execFuncs, [=] {
            auto mo = m_object->metaObject();
            for(int i = 0; i < mo->methodCount(); i++)
            {
              if(mo->method(i).name() == cbname)
              {
                mo->method(i).invoke(
                    m_object, Qt::DirectConnection, QGenericReturnArgument(),
                    QArgument<QVariant>{"v", v});
              }
            }
          });
        };

        // Go to ui thread
        ossia::qt::run_async(js_panel, [js_panel, code, cb]() {
          js_panel->compute(code, cb); // This invokes cb
        });
          },
          Qt::DirectConnection);
    }
  }

  m_jsInlets.clear();
  m_ctrlInlets.clear();
  m_impulseInlets.clear();
  m_valInlets.clear();
  m_audInlets.clear();
  m_midInlets.clear();
  m_valOutlets.clear();
  m_audOutlets.clear();
  m_midOutlets.clear();

  delete m_object;
  m_object = nullptr;

  if(val.trimmed().startsWith("import"))
  {
    QQmlComponent c{m_engine};
    {
      auto& lib = score::AppContext().settings<Library::Settings::Model>();
      // FIXME QTBUG-107204
      QString path = lib.getDefaultLibraryPath() + QDir::separator() + "Scripts"
                     + QDir::separator() + "include" + QDir::separator();

      c.setData(val.toUtf8(), QUrl::fromLocalFile(path));
    }
    const auto& errs = c.errors();
    if(!errs.empty())
    {
      ossia::logger().error(
          "Uncaught exception at line {} : {}", errs[0].line(),
          errs[0].toString().toStdString());
    }
    else
    {
      setupComponent(c);
    }
  }
  else
  {
    qDebug() << "URL: " << val << QUrl::fromLocalFile(val);
    QQmlComponent c{m_engine, QUrl::fromLocalFile(val)};
    const auto& errs = c.errors();
    if(!errs.empty())
    {
      ossia::logger().error(
          "Uncaught exception at line {} : {}", errs[0].line(),
          errs[0].toString().toStdString());
    }
    else
    {
      setupComponent(c);
    }
  }
}

void js_node::run(
    const ossia::token_request& tk, ossia::exec_state_facade estate) noexcept
{
  if(!m_engine || !m_object)
    return;

  auto& tick = m_object->tick();
  if(!tick.isCallable())
    return;
  // if (t.date == ossia::Zero)
  //   return;

  QEventLoop e;
  // Copy audio
  for(std::size_t inl_i = 0; inl_i < m_audInlets.size(); inl_i++)
  {
    auto& dat = m_audInlets[inl_i].second->target<ossia::audio_port>()->get();

    const int dat_size = std::ssize(dat);
    QVector<QVector<double>> audio(dat_size);
    for(int i = 0; i < dat_size; i++)
    {
      const int dat_i_size = dat[i].size();
      audio[i].resize(dat_i_size);
      for(int j = 0; j < dat_i_size; j++)
        audio[i][j] = dat[i][j];
    }
    m_audInlets[inl_i].first->setAudio(audio);
  }

  // Copy values
  for(std::size_t i = 0; i < m_valInlets.size(); i++)
  {
    auto& vp = *m_valInlets[i].second->target<ossia::value_port>();
    auto& dat = vp.get_data();

    m_valInlets[i].first->clear();
    if(dat.empty())
    {
      if(vp.is_event)
      {
        m_valInlets[i].first->setValue(QVariant{});
      }
      else
      {
        // Use control or same method as before
      }
    }
    else
    {
      for(auto& val : dat)
      {
        // TODO why not js_value_outbound_visitor ? it makes more sense.
        auto qvar = val.value.apply(ossia::qt::ossia_to_qvariant{});
        m_valInlets[i].first->setValue(qvar);
        m_valInlets[i].first->addValue(
            QVariant::fromValue(InValueMessage{(double)val.timestamp, std::move(qvar)}));
      }
    }
  }

  // Impulses are handed separately

  for(std::size_t i = 0; i < m_impulseInlets.size(); i++)
  {
    auto& vp = *m_impulseInlets[i].second->target<ossia::value_port>();
    auto& dat = vp.get_data();

    for(int k = 0; k < dat.size(); k++)
    {
      m_impulseInlets[i].first->impulse();
    }
  }

  // Copy controls
  for(std::size_t i = 0; i < m_ctrlInlets.size(); i++)
  {
    auto& vp = *m_ctrlInlets[i].second->target<ossia::value_port>();
    auto& dat = vp.get_data();

    m_ctrlInlets[i].first->clear();
    if(!dat.empty())
    {
      auto var = dat.back().value.apply(ossia::qt::ossia_to_qvariant{});
      m_ctrlInlets[i].first->setValue(std::move(var));
    }
  }

  // Copy midi
  for(std::size_t i = 0; i < m_midInlets.size(); i++)
  {
    auto& dat = m_midInlets[i].second->target<ossia::midi_port>()->messages;
    m_midInlets[i].first->setMidi(dat);
  }

  if(m_tickCall.empty())
    m_tickCall = {{}, {}};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  m_tickCall[0] = m_engine->toScriptValue(tk);
  m_tickCall[1] = m_engine->toScriptValue(estate);
#else
  m_tickCall[0] = m_engine->toScriptValue(TokenRequestValueType{tk});
  m_tickCall[1] = m_engine->toScriptValue(ExecutionStateValueType{estate});
#endif

  auto res = tick.call(m_tickCall);
  if(res.isError())
  {
    qDebug() << "JS Error at " << res.property("lineNumber").toInt() << ": "
             << res.toString();
  }

  const auto [tick_start, d] = estate.timings(tk);
  for(std::size_t i = 0; i < m_valOutlets.size(); i++)
  {
    auto& ossia_port = *m_valOutlets[i].second->target<ossia::value_port>();
    auto& js_port = *m_valOutlets[i].first;

    const QJSValue& v = js_port.value();
    if(!v.isNull() && !v.isError() && !v.isUndefined())
    {
      ossia_port.write_value(ossia::qt::value_from_js(v), tick_start);
    }
    for(auto& v : js_port.values)
    {
      ossia_port.write_value(ossia::qt::value_from_js(std::move(v.value)), v.timestamp);
    }
    js_port.clear();
  }

  for(std::size_t i = 0; i < m_midOutlets.size(); i++)
  {
    auto& dat = *m_midOutlets[i].second->target<ossia::midi_port>();
    for(const auto& mess : m_midOutlets[i].first->midi())
    {
      libremidi::message m;
      m.bytes.resize(mess.size());
      for(int j = 0; j < mess.size(); j++)
      {
        m.bytes[j] = mess[j];
      }
      dat.messages.push_back(std::move(m));
    }
    m_midOutlets[i].first->clear();
  }

  for(std::size_t out = 0; out < m_audOutlets.size(); out++)
  {
    auto& src = m_audOutlets[out].first->audio();
    auto& snk = m_audOutlets[out].second->target<ossia::audio_port>()->get();
    snk.resize(src.size());
    for(int chan = 0; chan < src.size(); chan++)
    {
      snk[chan].resize(src[chan].size() + tick_start);

      for(int j = 0; j < src[chan].size(); j++)
        snk[chan][j + tick_start] = src[chan][j];
    }
  }

  e.processEvents();
  if(m_gcIndex++ % 64 == 0)
    m_engine->collectGarbage();
}
}
}
