#pragma once
#include <score/plugins/Interface.hpp>

#include <score_lib_base_export.h>

#include <memory>
#include <vector>

namespace score
{
struct ApplicationContext;
struct GUIApplicationContext;

// Reimplement in plug-in if the plug-in offers an IMPLEMENTATION of an
// abstract type offered in another plug-in.
// Example : the Scenario plug-in provides inspector widget factories
// implementations for Interval and event
class SCORE_LIB_BASE_EXPORT FactoryInterface_QtInterface
{
public:
  virtual ~FactoryInterface_QtInterface();
  virtual std::vector<score::InterfaceBase*>
  factories(const score::ApplicationContext& ctx, const score::InterfaceKey& key) const;
  virtual std::vector<score::InterfaceBase*> guiFactories(
      const score::GUIApplicationContext& ctx, const score::InterfaceKey& key) const;
};
}
