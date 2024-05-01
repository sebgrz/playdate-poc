#include <stdio.h>

#include "pd_api.h"

static int update(void *userdata);

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg) {
  if (event == kEventInit) {
    playdate->system->logToConsole("%p", playdate);
    playdate->system->setUpdateCallback(update, playdate);
  }
  return 0;
}

static int update(void *userdata) { return 1; }
