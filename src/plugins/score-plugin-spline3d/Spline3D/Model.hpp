#pragma once
#include <State/Address.hpp>
#include <State/Unit.hpp>

#include <Process/Process.hpp>

#include <score/serialization/VisitorCommon.hpp>

#include <ossia/dataflow/nodes/spline/spline3d.hpp>

#include <Spline3D/Metadata.hpp>

#include <verdigris>

namespace Spline3D
{

class ProcessModel final : public Process::ProcessModel
{
  SCORE_SERIALIZE_FRIENDS
  PROCESS_METADATA_IMPL(Spline3D::ProcessModel)

  W_OBJECT(ProcessModel)

public:
  static constexpr bool hasExternalUI() noexcept { return true; }
  ProcessModel(
      const TimeVal& duration, const Id<Process::ProcessModel>& id, QObject* parent);
  ~ProcessModel() override;

  template <typename Impl>
  ProcessModel(Impl& vis, QObject* parent)
      : Process::ProcessModel{vis, parent}
  {
    vis.writeTo(*this);
    init();
  }

  void init();
  State::AddressAccessor address() const;
  void setAddress(const State::AddressAccessor& arg);
  State::Unit unit() const;
  void setUnit(const State::Unit&);

  bool tween() const { return m_tween; }
  void setTween(bool tween)
  {
    if(m_tween == tween)
      return;

    m_tween = tween;
    tweenChanged(tween);
  }

  QString prettyName() const noexcept override;

  const ossia::spline3d_data& spline() const noexcept { return m_spline; }
  void setSpline(const ossia::spline3d_data& c)
  {
    if(m_spline != c)
    {
      m_spline = c;
      splineChanged();
    }
  }

  std::unique_ptr<Process::Outlet> outlet;

  void addressChanged(const ::State::AddressAccessor& arg_1)
      W_SIGNAL(addressChanged, arg_1);
  void tweenChanged(bool tween) W_SIGNAL(tweenChanged, tween);
  void unitChanged(const State::Unit& arg_1) W_SIGNAL(unitChanged, arg_1);
  void splineChanged() W_SIGNAL(splineChanged);

private:
  TimeVal contentDuration() const noexcept override;
  void loadPreset(const Process::Preset& preset) override;
  Process::Preset savePreset() const noexcept override;

  ossia::spline3d_data m_spline;

  bool m_tween = false;

  W_PROPERTY(State::Unit, unit READ unit WRITE setUnit NOTIFY unitChanged)

  W_PROPERTY(bool, tween READ tween WRITE setTween NOTIFY tweenChanged)

  W_PROPERTY(
      State::AddressAccessor,
      address READ address WRITE setAddress NOTIFY addressChanged)
};
}
