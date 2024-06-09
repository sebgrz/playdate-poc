#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "pd_api.h"

#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"

const double RAD_ZERO_ANGLE = 0;
const double RAD_FULL_ANGLE = 360 * (M_PI / 180);

const int RAYS = 10;
const int FOV_ANGLE = 60;
const int FOV_LENGTH = 150;
const LCDRect WALLS[] = {
    {.left = 50, .top = 10, .right = 350, .bottom = 5},
    {.left = 50, .top = 50, .right = 100, .bottom = 150},
    {.left = 100, .top = 150, .right = 200, .bottom = 150},
    {.left = 50, .top = 50, .right = 200, .bottom = 30},
    {.left = 260, .top = 180, .right = 300, .bottom = 30},
};
const size_t WALLS_SIZE = sizeof(WALLS) / sizeof(WALLS[0]);

PlaydateAPI *globalPlaydate;

int previousPlayerX = 0, playerX = 100, previousPlayerY = 0, playerY = 80;
int positionAngle = 0;
const LCDRect **selectedWalls = NULL;
LCDRect *visibleRays = NULL;

static bool lineSegmentIntersectsCircleOptimized(float x1, float y1, float x2,
                                                 float y2, float cx, float cy,
                                                 int r);
static float distanceBetweenPoints(float p0_x, float p0_y, float p1_x,
                                   float p1_y);
static double angleOf(double p0_x, double p0_y, double p1_x, double p1_y);
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
  arrfree(visibleRays);

  playdate->graphics->clear(kColorWhite);
  int i;
  const LCDRect *wall;
  for (i = 0; i < WALLS_SIZE; i++) {
    wall = &WALLS[i];
    playdate->graphics->drawLine(wall->left, wall->top, wall->right,
                                 wall->bottom, 1, kColorBlack);
  }

  // take all nearest walls around player - fov radius
  if (previousPlayerX != playerX || previousPlayerY != playerY) {
    arrfree(selectedWalls);
    for (i = 0; i < WALLS_SIZE; i++) {
      wall = &WALLS[i];
      if (lineSegmentIntersectsCircleOptimized(wall->left, wall->top,
                                               wall->right, wall->bottom,
                                               playerX, playerY, FOV_LENGTH)) {
        arrput(selectedWalls, wall);
      }
    }

    playdate->system->logToConsole("Size of selected walls: %d",
                                   arrlen(selectedWalls));
  }

  //
  int startAngle = normalizeAngle(round(positionAngle - (FOV_ANGLE / 2)));
  int endAngle = normalizeAngle(round(positionAngle + (FOV_ANGLE / 2)));
  double playerRadStartAngle = startAngle * (M_PI / 180);
  double playerRadEndAngle = endAngle * (M_PI / 180);
  double middleAngle = playerRadEndAngle;
  double beginningAngle = playerRadStartAngle;

  if (playerRadEndAngle < playerRadStartAngle) {
    middleAngle = RAD_FULL_ANGLE;
    beginningAngle = RAD_ZERO_ANGLE;
  }

  // draw segments to all visible points of walls
  for (i = 0; i < arrlen(selectedWalls); i++) {
    wall = selectedWalls[i];

    float startDistancePoint =
        sqrt(pow(wall->left - playerX, 2) + pow(wall->top - playerY, 2));
    float endDistancePoint =
        sqrt(pow(wall->right - playerX, 2) + pow(wall->bottom - playerY, 2));
    double angleStartPoint = angleOf(wall->left, wall->top, playerX, playerY);
    double angleEndPoint = angleOf(wall->right, wall->bottom, playerX, playerY);

    if ((playerRadStartAngle <= angleStartPoint &&
         angleStartPoint <= middleAngle) ||
        (beginningAngle <= angleStartPoint &&
         angleStartPoint <= playerRadEndAngle)) {
      // check distance to the point
      if (distanceBetweenPoints(wall->left, wall->top, playerX, playerY) <=
          FOV_LENGTH) {
        arrput(visibleRays, ((LCDRect){.left = wall->left,
                                       .top = wall->top,
                                       .right = playerX,
                                       .bottom = playerY}));
      }
    }

    if ((playerRadStartAngle <= angleEndPoint &&
         angleEndPoint <= middleAngle) ||
        (beginningAngle <= angleEndPoint &&
         angleEndPoint <= playerRadEndAngle)) {
      // check distance to the point
      if (distanceBetweenPoints(wall->right, wall->bottom, playerX, playerY) <=
          FOV_LENGTH) {
        arrput(visibleRays, ((LCDRect){.left = wall->right,
                                       .top = wall->bottom,
                                       .right = playerX,
                                       .bottom = playerY}));
      }
    }
  }

  // draw visible rays
  for (i = 0; i < arrlen(visibleRays); i++) {
    LCDRect *raySegment = &visibleRays[i];
    playdate->graphics->drawLine(raySegment->left, raySegment->top,
                                 raySegment->right, raySegment->bottom, 1,
                                 kColorBlack);
  }

  double distanceX = playerX + FOV_LENGTH, distanceY = playerY;

  // draw start edge of fov
  double startX = (distanceX - playerX) * cos(playerRadStartAngle) -
                  (distanceY - playerY) * sin(playerRadStartAngle) + playerX;
  double startY = (distanceX - playerX) * sin(playerRadStartAngle) +
                  (distanceY - playerY) * cos(playerRadStartAngle) + playerY;
  playdate->graphics->drawLine(startX, startY, playerX, playerY, 1,
                               kColorBlack);

  // draw end edge of fov
  startX = (distanceX - playerX) * cos(playerRadEndAngle) -
           (distanceY - playerY) * sin(playerRadEndAngle) + playerX;
  startY = (distanceX - playerX) * sin(playerRadEndAngle) +
           (distanceY - playerY) * cos(playerRadEndAngle) + playerY;
  playdate->graphics->drawLine(startX, startY, playerX, playerY, 1,
                               kColorBlack);

  previousPlayerX = playerX;
  previousPlayerY = playerY;
  /*
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
  */

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

static double angleOf(double pointX, double pointY, double centerX,
                      double centerY) {
  double deltaY = centerY - pointY;
  double deltaX = centerX - pointX;
  return atan2(deltaY, deltaX) + M_PI;
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
    *intersectionDistance = distanceBetweenPoints(*i_x, *i_y, p0_x, p0_y);
    return true;
  }

  return false; // No collision
}

static float distanceBetweenPoints(float p0_x, float p0_y, float p1_x,
                                   float p1_y) {
  return sqrt(pow(p1_x - p0_x, 2) + pow(p1_y - p0_y, 2));
}

// https://stackoverflow.com/a/67117213
static bool lineSegmentIntersectsCircleOptimized(float x1, float y1, float x2,
                                                 float y2, float cx, float cy,
                                                 int r) {
  float x_linear = x2 - x1;
  float x_constant = x1 - cx;
  float y_linear = y2 - y1;
  float y_constant = y1 - cy;
  float a = x_linear * x_linear + y_linear * y_linear;
  float half_b = x_linear * x_constant + y_linear * y_constant;
  float c = x_constant * x_constant + y_constant * y_constant - r * r;

  return half_b * half_b >= a * c &&
         (-half_b <= a || c + half_b + half_b + a <= 0) &&
         (half_b <= 0 || c <= 0);
}
