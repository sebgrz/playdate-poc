#include <stdio.h>

#include "pd_api.h"

LCDBitmap *bitmap, *bitmap2;

static int update(void *userdata);

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg) {
  if (event == kEventInit) {
    // Load gfx
    const char *err_no = NULL;
    bitmap = playdate->graphics->loadBitmap("images/sprites", &err_no);
    if (err_no != NULL) {
      playdate->system->logToConsole("Err: %s", err_no);
      return -1;
    }

    // Update
    playdate->system->logToConsole("%p", playdate);
    playdate->system->setUpdateCallback(update, playdate);
  }
  return 0;
}

static int update(void *userdata) {
  PlaydateAPI *playdate = userdata;
  playdate->system->drawFPS(10, 10);

  playdate->graphics->drawBitmap(bitmap, 32, 32, kBitmapUnflipped);
  playdate->graphics->tileBitmap(bitmap, 32, 0, 32, 32, kBitmapUnflipped);

  return 1;
}
