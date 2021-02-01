#include <M5StickC.h>

const float deg2rad = PI / 180.0;
const float rad2deg = 180.0 / PI;

const int W = TFT_WIDTH;
const int H = TFT_HEIGHT;
const int Ws = W / 10;
const int Hs = H / 20;
const int Wc = W / 2;
const int Hc = H / 2;

TFT_eSprite display = TFT_eSprite(&M5.Lcd);

char s[64];
float the_angle = 0.0;

const int NGRAINS = 2500;

enum
{
  CELL_EMPTY,
  CELL_OCCUPIED,
  CELL_STATIC,
  CELL_NSTATES
};

enum
{
  DO_CENTER,
  DO_LEFT,
  DO_RIGHT,
  DO_L_OR_R,
  DO_REST,
  DO_DEFER,
  DO_NACTIONS
};

int gx[NGRAINS];
int gy[NGRAINS];
int gagain[NGRAINS];
byte gcells[H][W];

const byte gactions[CELL_NSTATES * CELL_NSTATES * CELL_NSTATES] = {
  DO_CENTER, // EEE
  DO_CENTER, // EEO
  DO_CENTER, // EES
  DO_L_OR_R, // EOE
  DO_LEFT,   // EOO
  DO_LEFT,   // EOS
  DO_L_OR_R, // ESE
  DO_LEFT,   // ESO
  DO_LEFT,   // ESS
  DO_CENTER, // OEE
  DO_CENTER, // OEO
  DO_CENTER, // OES
  DO_RIGHT,  // OOE
  DO_DEFER,  // OOO
  DO_DEFER,  // OOS
  DO_RIGHT,  // OSE
  DO_DEFER,  // OSO
  DO_DEFER,  // OSS
  DO_CENTER, // SEE
  DO_CENTER, // SEO
  DO_CENTER, // SES
  DO_RIGHT,  // SOE
  DO_DEFER,  // SOO
  DO_DEFER,  // SOS
  DO_RIGHT,  // SSE
  DO_DEFER,  // SSO
  DO_REST,   // SSS
};

void initGrains()
{
  int i;
  int x;
  int y;

  for (y = 0; y < H; y++) {
    for (x = 0; x < W; x++) {
      uint16_t c = display.readPixel(x, y);
      gcells[y][x] = c == BLACK ? CELL_EMPTY : CELL_STATIC;
    }
  }

  for (y = 0; y < H; y++) {
    byte filler = CELL_STATIC;
    for (x = 0; x < W - 1; x++) {
      if (gcells[y][x] == CELL_STATIC) {
        if (gcells[y][x + 1] == CELL_STATIC) {
          break;
        }
        filler ^= (CELL_EMPTY ^ CELL_STATIC);
        continue;
      }
      if (filler == CELL_EMPTY && i < NGRAINS) {
        gx[i] = x;
        gy[i] = y;
        gcells[y][x] = CELL_OCCUPIED;
        i++;
        continue;
      }
      gcells[y][x] = filler;
    }
  }
}

void drawGrains(uint16_t color)
{
  int i;

  for (i = 0; i < NGRAINS; i++) {
    int x = gx[i];
    int y = gy[i];
    display.drawPixel(x, y, color);
  }
}

void updateGrains(float angle)
{
  int i;
  int j;
  int k;
  int n = 0;

  float r = sqrt(2);

  float ca_c = r * cos(angle * deg2rad) + 0.5;
  float sa_c = r * sin(angle * deg2rad) + 0.5;

  float ca_l = r * cos((angle - 45) * deg2rad) + 0.5;
  float sa_l = r * sin((angle - 45) * deg2rad) + 0.5;

  float ca_r = r * cos((angle + 45) * deg2rad) + 0.5;
  float sa_r = r * sin((angle + 45) * deg2rad) + 0.5;

  // queue all grains for computation
  for (n = 0; n < NGRAINS; n++) {
    gagain[n] = n;
  }

  while (n > 0) {
    k = 0;
    for (j = 0; j < n; j++) {
      int i = gagain[j];
      int x = gx[i];
      int y = gy[i];
      int x_c = x + ca_c;
      int y_c = y + sa_c;
      int x_l = x + ca_l;
      int y_l = y + sa_l;
      int x_r = x + ca_r;
      int y_r = y + sa_r;
      byte cell_c = gcells[y_c][x_c];
      byte cell_l = gcells[y_l][x_l];
      byte cell_r = gcells[y_r][x_r];
      int action_index = cell_l * CELL_NSTATES * CELL_NSTATES + cell_c * CELL_NSTATES + cell_r;
      switch (gactions[action_index]) {
        case DO_CENTER:
          gcells[y][x] = CELL_EMPTY;
          gcells[y_c][x_c] = CELL_OCCUPIED;
          gx[i] = x_c;
          gy[i] = y_c;
          break;
        case DO_LEFT:
          gcells[y][x] = CELL_EMPTY;
          gcells[y_l][x_l] = CELL_OCCUPIED;
          gx[i] = x_l;
          gy[i] = y_l;
          break;
        case DO_RIGHT:
          gcells[y][x] = CELL_EMPTY;
          gcells[y_r][x_r] = CELL_OCCUPIED;
          gx[i] = x_r;
          gy[i] = y_r;
          break;
        case DO_L_OR_R:
          gcells[y][x] = CELL_EMPTY;
          if (random(2) == 0) {
            gcells[y_l][x_l] = CELL_OCCUPIED;
            gx[i] = x_l;
            gy[i] = y_l;
          } else {
            gcells[y_r][x_r] = CELL_OCCUPIED;
            gx[i] = x_r;
            gy[i] = y_r;
          }
          break;
        case DO_REST:
          gcells[y][x] = CELL_STATIC;
          break;
        case DO_DEFER:
          gagain[k] = i;
          k++;
          break;
        default:
          break;
      }
    }
    if (n == k) {
      break;
    }
    n = k;
  }
}

void drawArrow(int cx, int cy, float size, float angle, uint16_t color)
{
  float halfsize = size / 2.0;
  float headsize = size / 6.0;
  float r = angle * deg2rad;
  int x1 = cx - halfsize * cos(r);
  int x2 = cx + halfsize * cos(r);
  int y1 = cy - halfsize * sin(r);
  int y2 = cy + halfsize * sin(r);
  int lx = x2 + headsize * cos(r + 150 * deg2rad);
  int ly = y2 + headsize * sin(r + 150 * deg2rad);
  int rx = x2 + headsize * cos(r - 150 * deg2rad);
  int ry = y2 + headsize * sin(r - 150 * deg2rad);
  display.drawLine(x1, y1, x2, y2, color);
  display.drawLine(x2, y2, lx, ly, color);
  display.drawLine(x2, y2, rx, ry, color);
}

float incAngle(float angle, float delta)
{
  return fmod(angle + delta, 360);
}

float diffAngle(float target, float source)
{
  float d = fmod(target - source, 360);
  if (d < -180) {
    d += 360;
  } else if (d > 179) {
    d -= 360;
  }
  return d;
}

float getDownAngle()
{
  float x, y, z;
  M5.IMU.getAccelData(&x, &y, &z);
  return atan2(y, -x) * rad2deg;
}

void drawHourglass()
{
  // x1,y1 ------------ x2,y1
  //       |          |
  // x1,y2 |          | x2,y2
  //        \        /
  //         \      /
  //          \    /
  //     x3,y3 \  / x4,y3
  //            ||
  //     x3,y4 /  \ x4,y4
  //          /    \ -
  //         /      \ -
  //        /        \ -
  // x1,y5 |          | x2,y5
  //       |          |
  // x1,y6 ------------ x2,y6

  int x1 = Ws;
  int y1 = Hs;
  int x2 = W - Ws;
  int y2 = 4 * Hs;
  int x3 = Wc - Ws / 2;
  int y3 = Hc - Hs / 2;
  int x4 = Wc + (Ws - Ws / 2);
  int y4 = Hc + (Hs - Hs / 2);
  int y5 = H - 4 * Hs;
  int y6 = H - Hs;

  display.drawLine(x1, y1, x2, y1, WHITE);

  display.drawLine(x1, y1, x1, y2, WHITE);
  display.drawLine(x2, y1, x2, y2, WHITE);

  display.drawLine(x1, y2, x3, y3, WHITE);
  display.drawLine(x2, y2, x4, y3, WHITE);

  display.drawLine(x3, y3, x3, y4, WHITE);
  display.drawLine(x4, y3, x4, y4, WHITE);

  display.drawLine(x3, y4, x1, y5, WHITE);
  display.drawLine(x4, y4, x2, y5, WHITE);

  display.drawLine(x1, y5, x1, y6, WHITE);
  display.drawLine(x2, y5, x2, y6, WHITE);

  display.drawLine(x1, y6, x2, y6, WHITE);
}

void setup()
{
  M5.begin();
  M5.IMU.Init();
  display.createSprite(W, H);
  display.fillScreen(BLACK);
  display.setTextSize(1);
  drawHourglass();
  display.pushSprite(0, 0);
  s[0] = '\0';
  initGrains();
}

void loop()
{
  display.fillScreen(BLACK);

  drawHourglass();

  float target_angle = getDownAngle();
  float delta = diffAngle(target_angle, the_angle);
  float alpha = 0.2;
  the_angle = incAngle(the_angle, delta * alpha);

  drawGrains(display.color565(0xCD, 0x85, 0x3F));
  updateGrains(the_angle);

  //drawArrow(TFT_WIDTH/2, TFT_HEIGHT/2, 40, the_angle, WHITE);
  //sprintf(s, "%7.2f", the_angle);
  //display.setTextColor(WHITE);
  //display.drawString(s, 10, 10, 1);

  display.pushSprite(0, 0);

  //delay(10);
}
