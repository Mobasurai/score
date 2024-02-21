#pragma once
#include <ossia/detail/config.hpp>
#if defined(OSSIA_PROTOCOL_ARTNET)
#include <score/tools/std/StringHash.hpp>

#include <ossia/detail/variant.hpp>

#include <QString>

#include <utility>
#include <vector>
#include <verdigris>

namespace Protocols
{

namespace Artnet
{
struct BaseCapability
{
  QString type;
  QString comment;
  QString effectName;
};

struct SingleCapability : BaseCapability
{
};
struct RangeCapability : BaseCapability
{
  std::pair<int, int> range;
};

using FixtureCapabilities
    = ossia::variant<SingleCapability, std::vector<RangeCapability>>;
struct Channel
{
  QString name;
  FixtureCapabilities capabilities;
  std::vector<QString> fineChannels;
  int defaultValue{};
};

struct ModeInfo
{
  std::vector<QString> channelNames;
};

struct Fixture
{
  QString fixtureName;
  QString modeName;
  ModeInfo mode;
  std::vector<Channel> controls;
  int address{};
};

}

struct ArtnetSpecificSettings
{
  std::vector<Artnet::Fixture> fixtures;
  QString host;
  int rate{20};
  int universe{1};
  enum
  {
    ArtNet, // Artnet:/Channel-{}
    E131,
    DMXUSBPRO,
    ArtNetV2, // Artnet:/{}
  } transport{ArtNetV2};
  enum
  {
    Source, // score sends DMX
    Sink    // score receives DMX
  } mode;
};
}

Q_DECLARE_METATYPE(Protocols::ArtnetSpecificSettings)
W_REGISTER_ARGTYPE(Protocols::ArtnetSpecificSettings)
#endif
