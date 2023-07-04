#pragma once
#include <score/plugins/InterfaceList.hpp>

#include <core/application/ApplicationSettings.hpp>

#include <ossia/detail/for_each.hpp>

#include <type_traits>

namespace score
{
struct GUIApplicationContext;
struct ApplicationContext;
}
SCORE_LIB_BASE_EXPORT bool
appcontext_has_ui(const score::GUIApplicationContext&) noexcept;
SCORE_LIB_BASE_EXPORT bool appcontext_has_ui(const score::ApplicationContext&) noexcept;

/**
 * @brief Create a vector filled with pointers to new instances of the template
 * arguments
 */
template <typename Base_T, typename... Args>
auto make_ptr_vector() noexcept
{
  std::vector<std::unique_ptr<Base_T>> vec;

  vec.reserve(sizeof...(Args));
  (vec.push_back(std::unique_ptr<Base_T>((Base_T*)new Args)), ...);

  return vec;
}

/**
 * @class FactoryBuilder
 *
 * This class allows the user to customize the
 * creation of the factory by specializing it with the actual
 * factory type. An example is in score_plugin_scenario.cpp.
 */
template <
    typename Context_T,
    typename Factory_T>
struct FactoryBuilder // sorry padre for I have sinned
{
  static score::InterfaceBase* make(const Context_T& ctx)
  {
    if constexpr(std::is_constructible_v<Factory_T, const Context_T&>)
      return new Factory_T(ctx);
    else
      return new Factory_T();
  }
};

/**
 * @brief Fills an existing vector with factories instantiations
 */
template <typename Context_T, typename... Args>
void fill_ptr_vector(
    const Context_T& context, std::vector<score::InterfaceBase*>& vec) noexcept
{
  vec.reserve(sizeof...(Args));
  (vec.push_back(FactoryBuilder<Context_T, Args>::make(context)), ...);
}

template <typename T>
struct has_ui : public std::false_type
{
};
template <typename T>
  requires(T::ui_interface)
struct has_ui<T>
{
  static const constexpr bool value = T::ui_interface;
};

/**
 * \class FW_T
 * \brief Used to group base classes and concrete classes in a single argument
 * list.
 */
template <typename Factory_T, typename... Types_T>
struct FW_T
{
#if !defined(_MSC_VER)
  static_assert(
      (std::is_base_of<Factory_T, Types_T>::value && ...),
      "A type is not child of the parent.");
#endif
  template <typename Context_T>
  bool operator()(
      const Context_T& ctx, const score::InterfaceKey& fact,
      std::vector<score::InterfaceBase*>& vec) noexcept
  {
    if constexpr(has_ui<Factory_T>::value)
    {
      if(!appcontext_has_ui(ctx))
      {
        return false;
      }
    }

    if(fact == Factory_T::static_interfaceKey())
    {
      vec.reserve(sizeof...(Types_T));
      (vec.push_back(FactoryBuilder<Context_T, Types_T>::make(ctx)), ...);
      return true;
    }

    return false;
  }
};

template <typename Factory_T, typename... Args>
using FW = FW_T<Factory_T, Args...>;

/**
 * @brief instantiate_factories Given a type list of factories, instantiate the
 * ones corresponding to the key
 *
 * e.g.
 * \code
 * instantiate_factories<
 *  Context,
 *    FW<AbstractType1, ConcreteType1_1, ConcreteType1_2>,
 *    FW<AbstractType2, ConcreteType2_1>
 *  >(context, KeyOfAbstractType1, vec);
 * \endcode
 *
 * would return a vector such as :
 * \code
 * { std::make_unique<ConcreteType1_1>(), std::make_unique<ConcreteType1_2>() }
 * \endcode
 */
template <typename Context_T, typename... Args>
auto instantiate_factories(const Context_T& ctx, const score::InterfaceKey& key) noexcept
{
  std::vector<score::InterfaceBase*> vec;

  ossia::for_each_type_if_tagged<Args...>([&](auto t) {
    using fw_t = typename decltype(t)::type;
    return fw_t{}(ctx, key, vec);
  });

  return vec;
}
