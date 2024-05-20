#include <math.h>
#include <stdio.h>

#include "pd_api.h"

const int RAYS = 10;
const int FOV_ANGLE = 60;
const int FOV_LENGTH = 100;

PlaydateAPI *globalPlaydate;
PDRect wall = {.x = 50, .y = 50, .width = 30, .height = 100};
int playerX = 180, playerY = 80;
int positionAngle = 0;

static int normalizeAngle(int angle);
static int update(void *userdata);
static int updateKey(PDButtons button, int down, uint32_t when, void *userdata);

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg) {
  if (event == kEventInit) {
    globalPlaydate = playdate;
    playdate->system->setButtonCallback(updateKey, playdate, 4);
    playdate->system->setUpdateCallback(update, playdate);
  }

  return 0;
}

static int updateKey(PDButtons button, int down, uint32_t when,
                     void *userdata) {
  PlaydateAPI *playdate = userdata;

  switch (button) {
  case kButtonLeft:
    if (down == 1) {
      positionAngle -= 1;
    }
    break;

  case kButtonRight:
    if (down == 1) {
      positionAngle += 1;
    }
    break;

  default:
    break;
  }

  positionAngle = normalizeAngle(positionAngle);
  playdate->system->logToConsole("angle: %d", positionAngle);
  return 0;
}

static int update(void *userdata) {
  PlaydateAPI *playdate = userdata;

  playdate->graphics->clear(kColorWhite);
  playdate->graphics->drawRect(wall.x, wall.y, wall.width, wall.height,
                               kColorBlack);

  int rayStepAngle = round(FOV_ANGLE / RAYS);
  int startAngle = round(positionAngle - (FOV_ANGLE / 2));
  int ray = 0;
  for (ray = 0; ray < RAYS; ray++) {
    startAngle = normalizeAngle(startAngle);

    double distanceX = playerX + FOV_LENGTH, distanceY = playerY;
    double radAngle = startAngle * (M_PI / 180);

    double tempDistanceX = distanceX, tempDistanceY = distanceY;

    distanceX = (tempDistanceX - playerX) * cos(radAngle) -
                (tempDistanceY - playerY) * sin(radAngle) + playerX;
    distanceY = (tempDistanceX - playerX) * sin(radAngle) +
                (tempDistanceY - playerY) * cos(radAngle) + playerY;

    playdate->graphics->drawLine(playerX, playerY, round(distanceX),
                                 round(distanceY), 1, kColorBlack);

    // playdate->system->logToConsole("ray: %d angle: %d - %f - x: %f y: %f",
    // ray, startAngle, radAngle, distanceX, distanceY);
    startAngle += rayStepAngle;
  }

  return 1;
}

static int normalizeAngle(int angle) {
  if (angle > 360) {
    return angle - 360;
  } else if (angle < 0) {
    return 360 + angle;
  }

  return angle;
}
