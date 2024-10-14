// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "Component.hpp"

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <Scenario/Execution/score2OSSIA.hpp>

#include <Execution/DocumentPlugin.hpp>
#include <YSFX/ProcessModel.hpp>

#include <score/serialization/AnySerialization.hpp>
#include <score/serialization/MapSerialization.hpp>
#include <score/tools/Bind.hpp>

#include <ossia/dataflow/execution_state.hpp>
#include <ossia/dataflow/graph/graph_interface.hpp>
#include <ossia/dataflow/graph_edge.hpp>
#include <ossia/dataflow/graph_edge_helpers.hpp>
#include <ossia/dataflow/port.hpp>
#include <ossia/detail/logger.hpp>
#include <ossia/detail/ssize.hpp>
#include <ossia/editor/scenario/time_interval.hpp>
#include <ossia/editor/state/message.hpp>
#include <ossia/editor/state/state.hpp>

#include <QEventLoop>
#include <QQmlComponent>
#include <QQmlContext>
#include <QTimer>

#include <vector>

namespace YSFX
{
namespace Executor
{
class ysfx_node final : public ossia::graph_node
{
public:
  ysfx_node(std::shared_ptr<ysfx_t>, ossia::execution_state& st);

  void run(const ossia::token_request& t, ossia::exec_state_facade) noexcept override;
  [[nodiscard]] std::string label() const noexcept override
  {
    return fmt::format("ysfx ({})", ysfx_get_name(fx.get()));
  }

  std::shared_ptr<ysfx_t> fx;
  ossia::execution_state& m_st;

  ossia::audio_inlet* audio_in{};
  ossia::midi_inlet* midi_in{};
  ossia::midi_outlet* midi_out{};
  ossia::audio_outlet* audio_out{};
  std::vector<ossia::value_port*> sliders;
};

Component::Component(
    YSFX::ProcessModel& proc, const ::Execution::Context& ctx, QObject* parent)
    : ::Execution::ProcessComponent_T<YSFX::ProcessModel, ossia::node_process>{
        proc, ctx, "YSFXComponent", parent}
{
  std::shared_ptr<ysfx_node> node
      = ossia::make_node<ysfx_node>(*ctx.execState, proc.fx, *ctx.execState);
  this->node = node;
  m_ossia_process = std::make_shared<ossia::node_process>(node);

  int firstControlIndex = ysfx_get_num_inputs(proc.fx.get()) > 0 ? 2 : 1;
  for(std::size_t i = firstControlIndex, N = proc.inlets().size(); i < N; i++)
  {
    auto inlet = static_cast<Process::ControlInlet*>(proc.inlets()[i]);
    // *node->controls[i - firstControlIndex].second
    //     = ossia::convert<float>(inlet->value());

    auto inl = node->sliders[i - firstControlIndex];
    inlet->setupExecution(*node->root_inputs()[i]);
    connect(
        inlet, &Process::ControlInlet::valueChanged, this,
        [this, inl](const ossia::value& v) {
      system().executionQueue.enqueue(
          [inl, val = v]() mutable { inl->write_value(std::move(val), 0); });
        });
  }

  auto c = con(ctx.doc.coarseUpdateTimer, &QTimer::timeout, this, [node, &proc] {
    auto y = proc.fx.get();
    if(std::bitset<64> res = ysfx_fetch_slider_changes(y); res.any())
    {
      for(int i = 0; i < 64; i++)
      {
        if(res.test(i))
        {
          // See ProcessModel.hpp around the loop:
          // for (uint32_t i = 0; i < ysfx_max_sliders; ++i)
          int idx = 4 + i;
          if(auto inl
             = static_cast<Process::ControlInlet*>(proc.inlet(Id<Process::Port>{idx})))
            inl->setExecutionValue(ysfx_slider_get_value(y, i));
          else
            qDebug() << "Error while trying to access inlet " << idx;
        }
      }
    }
  });
}

Component::~Component() { }

ysfx_node::ysfx_node(std::shared_ptr<ysfx_t> ffx, ossia::execution_state& st)
    : fx{std::move(ffx)}
    , m_st{st}
{
  auto y = this->fx.get();

  ysfx_set_block_size(y, st.bufferSize);
  ysfx_set_sample_rate(y, st.sampleRate);
  ysfx_init(y);

  if(ysfx_get_num_inputs(y) > 0)
  {
    this->m_inlets.push_back(audio_in = new ossia::audio_inlet);
  }

  this->m_inlets.push_back(midi_in = new ossia::midi_inlet);

  if(ysfx_get_num_outputs(y) > 0)
  {
    this->m_outlets.push_back(audio_out = new ossia::audio_outlet);
  }

  this->m_outlets.push_back(midi_out = new ossia::midi_outlet);

  for(uint32_t i = 0; i < ysfx_max_sliders; ++i)
  {
    auto inl = new ossia::value_inlet;
    this->m_inlets.push_back(inl);
    this->sliders.push_back(&**inl);
    if(ysfx_slider_is_enum(y, i))
    {
    }
    else if(ysfx_slider_is_path(y, i))
    {
    }
    else
    {
      ysfx_slider_range_t range{};
      ysfx_slider_get_range(y, i, &range);

      (*inl)->domain = ossia::make_domain(range.min, range.max);
    }
  }
}

void ysfx_node::run(
    const ossia::token_request& tk, ossia::exec_state_facade estate) noexcept
{
  assert(fx);
  auto y = this->fx.get();

  const auto [tick_start, d] = estate.timings(tk);

  // Setup audio input
  double** ins{};
  int in_count{};
  if(audio_in)
  {
    in_count = this->audio_in->data.channels();
    if(in_count < (int)ysfx_get_num_inputs(y))
    {
      in_count = ysfx_get_num_inputs(y);
      this->audio_in->data.set_channels(in_count);
    }
    ins = (double**)alloca(sizeof(double*) * in_count);
    for(int i = 0; i < in_count; i++)
    {
      {
        this->audio_in->data.get()[i].resize(d);
        ins[i] = this->audio_in->data.channel(i).data();
      }
    }
  }

  // Setup audio output
  double** outs{};
  int out_count{};
  if(audio_out)
  {
    audio_out->data.set_channels(ysfx_get_num_outputs(y));
    out_count = this->audio_out->data.channels();
    outs = (double**)alloca(sizeof(double*) * out_count);
    for(int i = 0; i < out_count; i++)
    {

      this->audio_out->data.get()[i].resize(
          estate.bufferSize(), boost::container::default_init);
      outs[i] = this->audio_out->data.channel(i).data();
    }
  }

  // Setup controls
  for(std::size_t i = 0; i < sliders.size(); i++)
  {
    auto& vp = *sliders[i];
    auto& dat = vp.get_data();

    if(!dat.empty())
    {
      auto& val = dat.back().value;
      if(float* v = val.target<float>())
      {
        ysfx_slider_set_value(y, i, *v);
      }
      else if(int* v = val.target<int>())
      {
        ysfx_slider_set_value(y, i, *v);
      }
    }
  }

  // Setup midi
  for(auto& msg : this->midi_in->data.messages)
  {
    ysfx_midi_event_t ev;
    ev.bus = 0;
    ev.data = msg.bytes.data();
    ev.offset = msg.timestamp;
    ev.size = msg.bytes.size();
    ysfx_send_midi(y, &ev);
  }

  ysfx_time_info_t info;
  info.time_position = tk.prev_date.impl * estate.modelToSamples() / estate.sampleRate();
  info.time_signature[0] = tk.signature.upper;
  info.time_signature[1] = tk.signature.lower;
  info.beat_position = tk.musical_start_position;
  info.playback_state = ysfx_playback_playing;
  info.tempo = tk.tempo;

  ysfx_set_time_info(y, &info);
  ysfx_process_double(y, ins, outs, in_count, out_count, d);

  ysfx_midi_event_t ev;
  while(ysfx_receive_midi(y, &ev))
  {
    libremidi::message msg;
    msg.bytes.assign(ev.data, ev.data + ev.size);
    msg.timestamp = ev.offset;
    this->midi_out->data.messages.push_back(msg);
  }
}
}
}
