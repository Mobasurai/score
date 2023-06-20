#pragma once
#include <Process/Dataflow/Port.hpp>

#include <score/tools/Debug.hpp>

#include <ossia/detail/math.hpp>
#include <ossia/network/domain/domain.hpp>

#include <type_traits>

namespace WidgetFactory
{

struct LinearNormalizer
{
  static constexpr double to01(double min, double range, double val) noexcept
  {
    return (val - min) / range;
  }

  static constexpr double from01(double min, double range, double val) noexcept
  {
    return min + val * range;
  }

  template <typename T>
  static double to01(const T& slider, double val) noexcept
  {
    auto min = slider.getMin();
    auto max = slider.getMax();
    if(max - min == 0)
      max = min + 1;
    return to01(min, max - min, val);
  }

  template <typename T>
  static double from01(const T& slider, double val) noexcept
  {
    auto min = slider.getMin();
    auto max = slider.getMax();
    if(max - min == 0)
      max = min + 1;
    return from01(min, max - min, val);
  }

  template <typename T>
  static ossia::vec2f to01(const T& slider, ossia::vec2f val) noexcept
  {
    ossia::vec2f res;
    const auto min = slider.getMin();
    const auto max = slider.getMax();
    res[0] = to01(min[0], max[0] - min[0], val[0]);
    res[1] = to01(min[1], max[1] - min[1], val[1]);
    return res;
  }

  template <typename T>
  static ossia::vec2f from01(const T& slider, ossia::vec2f val) noexcept
  {
    ossia::vec2f res;
    const auto min = slider.getMin();
    const auto max = slider.getMax();
    res[0] = from01(min[0], max[0] - min[0], val[0]);
    res[1] = from01(min[1], max[1] - min[1], val[1]);
    return res;
  }

  template <typename T>
  static ossia::vec3f to01(const T& slider, ossia::vec3f val) noexcept
  {
    ossia::vec3f res;
    const auto min = slider.getMin();
    const auto max = slider.getMax();
    res[0] = to01(min[0], max[0] - min[0], val[0]);
    res[1] = to01(min[1], max[1] - min[1], val[1]);
    res[2] = to01(min[2], max[2] - min[2], val[2]);
    return res;
  }

  template <typename T>
  static ossia::vec3f from01(const T& slider, ossia::vec3f val) noexcept
  {
    ossia::vec3f res;
    const auto min = slider.getMin();
    const auto max = slider.getMax();
    res[0] = from01(min[0], max[0] - min[0], val[0]);
    res[1] = from01(min[1], max[1] - min[1], val[1]);
    res[2] = from01(min[2], max[2] - min[2], val[2]);
    return res;
  }

  template <typename T>
  static ossia::vec4f to01(const T& slider, ossia::vec4f val) noexcept
  {
    ossia::vec4f res;
    const auto min = slider.getMin();
    const auto max = slider.getMax();
    res[0] = to01(min[0], max[0] - min[0], val[0]);
    res[1] = to01(min[1], max[1] - min[1], val[1]);
    res[2] = to01(min[2], max[2] - min[2], val[2]);
    res[3] = to01(min[3], max[3] - min[3], val[3]);
    return res;
  }

  template <typename T>
  static ossia::vec4f from01(const T& slider, ossia::vec4f val) noexcept
  {
    ossia::vec4f res;
    const auto min = slider.getMin();
    const auto max = slider.getMax();
    res[0] = from01(min[0], max[0] - min[0], val[0]);
    res[1] = from01(min[1], max[1] - min[1], val[1]);
    res[2] = from01(min[2], max[2] - min[2], val[2]);
    res[3] = from01(min[3], max[3] - min[3], val[3]);
    return res;
  }
};

struct LogNormalizer
{
  static double to01(double min, double range, double val) noexcept
  {
    return ossia::log_to_normalized(min, range, val);
  }

  static double from01(double min, double range, double val) noexcept
  {
    return ossia::normalized_to_log(min, range, val);
  }

  template <typename T>
  static double to01(const T& slider, double val) noexcept
  {
    auto min = slider.getMin();
    auto max = slider.getMax();
    if(max - min == 0)
      max = min + 1;
    return to01(min, max - min, val);
  }

  template <typename T>
  static double from01(const T& slider, double val) noexcept
  {
    auto min = slider.getMin();
    auto max = slider.getMax();
    if(max - min == 0)
      max = min + 1;
    return from01(min, max - min, val);
  }
};

template <typename Norm_T>
struct FixedNormalizer
{
  double min{};
  double max{};
  template <typename T>
  FixedNormalizer(const T& slider)
  {
    min = slider.getMin();
    max = slider.getMax();
    if(max - min == 0)
      max = min + 1;
  }

  constexpr double to01(double val) const noexcept
  {
    return Norm_T::to01(min, max - min, val);
  }

  constexpr double from01(double val) const noexcept
  {
    return Norm_T::from01(min, max - min, val);
  }
};

template <typename Norm_T, typename Slider_T>
struct UpdatingNormalizer
{
  const Slider_T& slider;

  UpdatingNormalizer(const Slider_T& sl)
      : slider{sl}
  {
  }

  constexpr double to01(double val) const noexcept { return Norm_T::to01(slider, val); }

  constexpr double from01(double val) const noexcept
  {
    return Norm_T::from01(slider, val);
  }
};

template <typename T, typename Control_T, typename Widget_T>
static void bindFloatDomain(const T& slider, Control_T& inlet, Widget_T& widget)
{
  auto min = slider.getMin();
  auto max = slider.getMax();
  if(max - min == 0)
    max = min + 1;

  widget.setRange(min, max);

  if constexpr(std::is_base_of_v<Process::ControlInlet, T>)
  {
    SCORE_ASSERT(&slider == &inlet);
    QObject::connect(&inlet, &Control_T::domainChanged, &widget, [&slider, &widget] {
      auto min = slider.getMin();
      auto max = slider.getMax();
      if(max - min == 0)
        max = min + 1;

      widget.setRange(min, max);
    });
  }
}

template <typename T, typename Control_T, typename Widget_T>
static void bindIntDomain(const T& slider, Control_T& inlet, Widget_T& widget)
{
  auto min = slider.getMin();
  auto max = slider.getMax();
  if(max - min == 0)
    max = min + 1;

  widget.setRange(min, max);

  if constexpr(std::is_base_of_v<Process::ControlInlet, T>)
  {
    SCORE_ASSERT(&slider == &inlet);
    QObject::connect(&inlet, &Control_T::domainChanged, &widget, [&slider, &widget] {
      auto min = slider.getMin();
      auto max = slider.getMax();
      if(max - min == 0)
        max = min + 1;

      widget.setRange(min, max);
    });
  }
}

template <typename T, typename Control_T, typename Widget_T>
static void bindVec2Domain(const T& slider, Control_T& inlet, Widget_T& widget)
{
  auto update_range = [&widget, &inlet] {
    auto min = ossia::get_min(inlet.domain());
    auto max = ossia::get_max(inlet.domain());
    auto min_float = min.template target<float>();
    auto max_float = max.template target<float>();
    if(min_float && max_float)
    {
      if(*max_float - *min_float == 0)
        *max_float = *min_float + 1;
      widget.setRange({*min_float, *min_float}, {*max_float, *max_float});
    }
    else
    {
      auto min_vec2 = min.template target<ossia::vec2f>();
      auto max_vec2 = max.template target<ossia::vec2f>();
      if(min_vec2 && max_vec2)
      {
        auto& min = *min_vec2;
        auto& max = *max_vec2;
        for(std::size_t i = 0; i < min.size(); i++)
        {
          if(max[i] - min[i] == 0)
            max[i] = min[i] + 1;
        }

        widget.setRange(min, max);
      }
    }
  };

  update_range();

  if constexpr(std::is_base_of_v<Process::ControlInlet, T>)
  {
    SCORE_ASSERT(&slider == &inlet);
    QObject::connect(&inlet, &Control_T::domainChanged, &widget, update_range);
  }
}

template <typename T, typename Control_T, typename Widget_T>
static void bindVec3Domain(const T& slider, Control_T& inlet, Widget_T& widget)
{
  auto update_range = [&widget, &inlet] {
    auto min = ossia::get_min(inlet.domain());
    auto max = ossia::get_max(inlet.domain());
    auto min_float = min.template target<float>();
    auto max_float = max.template target<float>();
    if(min_float && max_float)
    {
      if(*max_float - *min_float == 0)
        *max_float = *min_float + 1;
      widget.setRange(
          {*min_float, *min_float, *min_float}, {*max_float, *max_float, *max_float});
    }
    else
    {
      auto min_vec3 = min.template target<ossia::vec3f>();
      auto max_vec3 = max.template target<ossia::vec3f>();
      if(min_vec3 && max_vec3)
      {
        auto& min = *min_vec3;
        auto& max = *max_vec3;
        for(std::size_t i = 0; i < min.size(); i++)
        {
          if(max[i] - min[i] == 0)
            max[i] = min[i] + 1;
        }

        widget.setRange(min, max);
      }
    }
  };

  update_range();

  if constexpr(std::is_base_of_v<Process::ControlInlet, T>)
  {
    SCORE_ASSERT(&slider == &inlet);
    QObject::connect(&inlet, &Control_T::domainChanged, &widget, update_range);
  }
}
}
