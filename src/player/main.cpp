// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "player.hpp"

int main(int argc, char** argv)
{
  if(argc > 1)
  {
    // Create a player instance
    std::atomic_bool ready{};
    score::Player p{[&] { ready = true; }};

    while(!ready)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    p.load(argv[1]);
    p.play();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    p.stop();
  }
  return 0;
}
