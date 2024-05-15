#include <stdio.h>

#include "pd_api.h"

PlaydateAPI *globalPlaydate;

static int update(void *userdata);

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg) {
  if (event == kEventInit) {
    globalPlaydate = playdate;
    playdate->system->setUpdateCallback(update, playdate);
  }
}

static int update(void *userdata) {
  PlaydateAPI *playdate = userdata;

  return 1;
}
