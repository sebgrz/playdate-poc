#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "pd_api.h"

const int RAYS = 10;
const int FOV_ANGLE = 60;
const int FOV_LENGTH = 150;
const LCDRect WALLS[] = {
    {.left = 50, .top = 50, .right = 100, .bottom = 150},
    {.left = 100, .top = 150, .right = 200, .bottom = 150},
    {.left = 50, .top = 50, .right = 200, .bottom = 30},
};
const size_t WALLS_SIZE = sizeof(WALLS) / sizeof(WALLS[0]);

PlaydateAPI *globalPlaydate;

int playerX = 100, playerY = 80;
int positionAngle = 0;

static bool getLineIntersection(float p0_x, float p0_y, float p1_x, float p1_y,
                                float p2_x, float p2_y, float p3_x, float p3_y,
                                float *i_x, float *i_y, float *distance);
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
  int i;
  const LCDRect *wall;
  for (i = 0; i < WALLS_SIZE; i++) {
    wall = &WALLS[i];
    playdate->graphics->drawLine(wall->left, wall->top, wall->right,
                                 wall->bottom, 1, kColorBlack);
  }

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

    // wall collisions
    int i;
    float colX, colY, tempColX, tempColY, distance = FOV_LENGTH,
                                          tempDistance = FOV_LENGTH;
    const LCDRect *nearestWall = NULL;
    for (i = 0; i < WALLS_SIZE; i++) {
      wall = &WALLS[i];
      if (getLineIntersection(playerX, playerY, distanceX, distanceY,
                              wall->left, wall->top, wall->right, wall->bottom,
                              &tempColX, &tempColY, &tempDistance)) {
        if (tempDistance < distance) {
          distance = tempDistance;
          colX = tempColX;
          colY = tempColY;
          nearestWall = wall;
        }
      }
    }
    // Wall start-end
    if (nearestWall != NULL) {
      playdate->graphics->drawLine(playerX, playerY, nearestWall->left,
                                   nearestWall->top, 1, kColorBlack);

      playdate->graphics->drawLine(playerX, playerY, nearestWall->right,
                                   nearestWall->bottom, 1, kColorBlack);
    }

    if (distance < FOV_LENGTH) {
      playdate->graphics->drawRect(colX, colY, 5, 5, kColorBlack);
      playdate->graphics->drawLine(playerX, playerY, round(colX), round(colY),
                                   1, kColorBlack);
    } else {
      playdate->graphics->drawLine(playerX, playerY, round(distanceX),
                                   round(distanceY), 1, kColorBlack);
    }

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

static bool getLineIntersection(float p0_x, float p0_y, float p1_x, float p1_y,
                                float p2_x, float p2_y, float p3_x, float p3_y,
                                float *i_x, float *i_y,
                                float *intersectionDistance) {
  float s1_x, s1_y, s2_x, s2_y;
  s1_x = p1_x - p0_x;
  s1_y = p1_y - p0_y;
  s2_x = p3_x - p2_x;
  s2_y = p3_y - p2_y;

  float s, t;
  s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) /
      (-s2_x * s1_y + s1_x * s2_y);
  t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) /
      (-s2_x * s1_y + s1_x * s2_y);

  if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
    // Collision detected
    // globalPlaydate->system->logToConsole("p0x: %f t: %f s1_x: %f", p0_x, t,
    // s1_x);
    *i_x = p0_x + (t * s1_x);
    *i_y = p0_y + (t * s1_y);
    *intersectionDistance = sqrt(pow(*i_x - p0_x, 2) + pow(*i_y - p0_y, 2));
    return true;
  }

  return false; // No collision
}
