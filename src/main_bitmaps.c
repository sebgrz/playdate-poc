#include <stdio.h>

#include "pd_api.h"

PlaydateAPI *globalPlaydate;
LCDBitmap *bitmap;
LCDBitmapTable *bitmapTable;
LCDSprite *sprite;

static int update(void *userdata);
static void spriteDrawFunction(LCDSprite *sprite, PDRect bounds,
                               PDRect drawrect);
static void updateSprite(LCDSprite *sp);

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg) {
  if (event == kEventInit) {
    globalPlaydate = playdate;

    // Load gfx
    const char *err_no = NULL;
    bitmap = playdate->graphics->loadBitmap("images/sprites", &err_no);
    if (err_no != NULL) {
      playdate->system->logToConsole("Err: %s", err_no);
      return -1;
    }

    bitmapTable = playdate->graphics->newBitmapTable(4, 32, 32);
    playdate->graphics->loadIntoBitmapTable("images/sprites", bitmapTable,
                                            &err_no);
    if (err_no != NULL) {
      playdate->system->logToConsole("Err bmp table: %s", err_no);
      return -1;
    }

    // Sprite
    sprite = playdate->sprite->newSprite();
    playdate->sprite->setCenter(sprite, 0, 0);
    playdate->sprite->setBounds(sprite, PDRectMake(0, 0, 32, 32));
    playdate->sprite->addSprite(sprite);
    playdate->sprite->setDrawFunction(sprite, spriteDrawFunction);
    globalPlaydate->system->logToConsole("PS: %p", sprite);

    // Update
    playdate->system->logToConsole("%p", playdate);
    playdate->system->setUpdateCallback(update, playdate);
  }
  return 0;
}

static void spriteDrawFunction(LCDSprite *sprite, PDRect bounds,
                               PDRect drawrect) {
  globalPlaydate->system->logToConsole("draw_sprite_bounds: %f, %f, %f, %f",
                                       bounds.x, bounds.y, bounds.width,
                                       bounds.height);
  globalPlaydate->system->logToConsole("draw_sprite_drawrect: %f, %f, %f, %f",
                                       drawrect.x, drawrect.y, drawrect.width,
                                       drawrect.height);

  globalPlaydate->graphics->pushContext(NULL);
  float x = drawrect.x, y = drawrect.y;
  float tile_x = 32, tile_y = 0;
  globalPlaydate->graphics->setDrawOffset(x, y);
  globalPlaydate->graphics->setClipRect(0, 0, drawrect.width, drawrect.height);
  globalPlaydate->graphics->drawBitmap(bitmap, -tile_x, -tile_y,
                                       kBitmapUnflipped);
  globalPlaydate->graphics->popContext();
  globalPlaydate->graphics->drawRect(drawrect.x, drawrect.y, drawrect.width,
                                     drawrect.height, kColorBlack);
}

static int update(void *userdata) {
  PlaydateAPI *playdate = userdata;

  // playdate->graphics->setClipRect(20, 0, 50, 50);
  playdate->sprite->moveTo(sprite, 10, 100);
  playdate->sprite->updateAndDrawSprites();

  playdate->system->drawFPS(0, 0);

  // 1
  playdate->graphics->tileBitmap(bitmap, 32, 0, 32, 32, kBitmapUnflipped);

  // 2
  playdate->graphics->pushContext(NULL);
  float x = 32, y = 32;
  float tile_x = 32, tile_y = 0;
  globalPlaydate->graphics->setDrawOffset(x, y);
  globalPlaydate->graphics->setClipRect(0, 0, 32, 32);
  playdate->graphics->drawBitmap(bitmap, -tile_x, -tile_y, kBitmapUnflipped);
  playdate->graphics->popContext();
  // 3
  playdate->graphics->drawBitmap(
      playdate->graphics->getTableBitmap(bitmapTable, 2), 32, 64,
      kBitmapUnflipped);

  return 1;
}
