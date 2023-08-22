#ifndef TRADER_INDEPENDENT_H

struct Game_Data
{
  u64 last_frame_time;
};

#include "trader_volatile_game_data.h"

void update_and_render(Game_Data *game_data);
#define TRADER_INDEPENDENT_H
#endif
