#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAXC 30
typedef struct { float x, y; int open; } Chest;
static Chest ch[MAXC];
static float px = 200, py = 120;
static unsigned long long coins = 0;
static int lvl = 0, opened = 0, msgT = 0;
static char msg[64] = "";

static void save(void){
  FILE *f = fopen("sdmc:/pirate_chest.sav", "wb");
  if (f){ fwrite(&coins, 8, 1, f); fwrite(&lvl, 4, 1, f); fclose(f); }
}
static void load(void){
  FILE *f = fopen("sdmc:/pirate_chest.sav", "rb");
  if (f){ fread(&coins, 8, 1, f); fread(&lvl, 4, 1, f); fclose(f); }
}
static void newMap(void){
  opened = 0;
  for (int i = 0; i < MAXC; i++){
    int ok;
    do {
      ch[i].x = 30 + rand() % 340; ch[i].y = 40 + rand() % 180; ch[i].open = 0; ok = 1;
      for (int j = 0; j < i; j++)
        if (fabsf(ch[j].x - ch[i].x) < 34 && fabsf(ch[j].y - ch[i].y) < 30) ok = 0;
    } while (!ok);
  }
}
static double luck(void){ return pow(1.1, lvl); }
static int upCost(void){ return (int)(150.0 * pow(1.1, lvl)); }

static void openChest(void){
  double L = luck(), p5000 = L / 7813, p500 = L / 568, p75 = L / 100, p10 = L / 75;
  double s = p5000 + p500 + p75 + p10;
  if (s > .35){ double k = .35 / s; p5000 *= k; p500 *= k; p75 *= k; p10 *= k; }
  double r = rand() / (double)RAND_MAX; int g = 1;
  if (r < p5000) g = 5000;
  else if (r < p5000 + p500) g = 500;
  else if (r < p5000 + p500 + p75) g = 75;
  else if (r < p5000 + p500 + p75 + p10) g = 10;
  coins += g;
  snprintf(msg, sizeof msg, "+%d piece%s", g, g > 1 ? "s" : "");
  msgT = 90;
}

static C2D_TextBuf tb;
static void text(float x, float y, float s, u32 clr, const char *str){
  C2D_Text t; C2D_TextParse(&t, tb, str); C2D_TextOptimize(&t);
  C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, s, s, clr);
}
static void drawChest(Chest *c, int hl){
  u32 brown = C2D_Color32(160, 90, 28, 255), lid = C2D_Color32(196, 115, 31, 255),
      gold = C2D_Color32(255, 212, 31, 255), dark = C2D_Color32(40, 18, 4, 255);
  if (hl) C2D_DrawCircleSolid(c->x, c->y, 0.2f, 26, C2D_Color32(255, 244, 120, 120));
  if (c->open){
    C2D_DrawRectSolid(c->x - 14, c->y - 14, 0.3f, 28, 10, lid);
    C2D_DrawRectSolid(c->x - 14, c->y - 2, 0.3f, 28, 14, dark);
    return;
  }
  C2D_DrawRectSolid(c->x - 15, c->y - 8, 0.3f, 30, 18, dark);
  C2D_DrawRectSolid(c->x - 14, c->y - 7, 0.3f, 28, 16, brown);
  C2D_DrawRectSolid(c->x - 14, c->y - 14, 0.3f, 28, 8, lid);
  C2D_DrawRectSolid(c->x - 3, c->y - 8, 0.3f, 6, 8, gold);
}

int main(void){
  gfxInitDefault(); C3D_Init(C3D_DEFAULT_CMDBUF_SIZE); C2D_Init(C2D_DEFAULT_MAX_OBJECTS); C2D_Prepare();
  C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  tb = C2D_TextBufNew(4096);
  srand(time(NULL)); load(); newMap();
  char buf[96];
  while (aptMainLoop()){
    hidScanInput();
    u32 kd = hidKeysDown(), kh = hidKeysHeld();
    if (kd & KEY_START) break;
    circlePosition cp; hidCircleRead(&cp);
    float dx = cp.dx / 156.0f, dy = -cp.dy / 156.0f;
    if (kh & KEY_DLEFT) dx = -1; if (kh & KEY_DRIGHT) dx = 1;
    if (kh & KEY_DUP) dy = -1; if (kh & KEY_DDOWN) dy = 1;
    px += dx * 2.5f; py += dy * 2.5f;
    if (px < 15) px = 15; if (px > 385) px = 385; if (py < 35) py = 35; if (py > 225) py = 225;
    Chest *near = NULL; float bd = 40;
    for (int i = 0; i < MAXC; i++){
      float d = hypotf(ch[i].x - px, ch[i].y - py);
      if (!ch[i].open && d < bd){ bd = d; near = &ch[i]; }
    }
    if ((kd & KEY_A) && near){ near->open = 1; opened++; openChest(); save(); }
    if (kd & KEY_TOUCH){
      touchPosition tp; hidTouchRead(&tp);
      if (tp.px > 20 && tp.px < 300 && tp.py > 90 && tp.py < 135){
        int c = upCost(); if (lvl < 50 && coins >= (unsigned long long)c){ coins -= c; lvl++; save(); }
      }
      if (tp.px > 20 && tp.px < 300 && tp.py > 150 && tp.py < 195){ newMap(); }
    }
    if (msgT > 0) msgT--;

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(tb);
    C2D_TargetClear(top, C2D_Color32(63, 154, 70, 255)); C2D_SceneBegin(top);
    for (int i = 0; i < MAXC; i++) drawChest(&ch[i], &ch[i] == near);
    C2D_DrawEllipseSolid(px - 10, py + 20, 0.4f, 20, 7, C2D_Color32(0, 0, 0, 80));
    C2D_DrawRectSolid(px - 2, py - 4, 0.5f, 4, 22, C2D_Color32(255, 255, 255, 255));
    C2D_DrawRectSolid(px - 10, py + 4, 0.5f, 20, 3, C2D_Color32(255, 255, 255, 255));
    C2D_DrawCircleSolid(px, py - 12, 0.5f, 10, C2D_Color32(255, 255, 255, 255));
    snprintf(buf, sizeof buf, "%d / %d", opened, MAXC); text(8, 4, .6f, C2D_Color32(255, 255, 255, 255), buf);
    snprintf(buf, sizeof buf, "Pieces: %llu", coins); text(290, 4, .6f, C2D_Color32(255, 212, 31, 255), buf);
    if (near) text(140, 210, .6f, C2D_Color32(255, 255, 255, 255), "Appuie sur A");
    if (msgT > 0) text(150, 100 - (90 - msgT) / 3, .9f, C2D_Color32(255, 212, 31, 255), msg);
    if (opened >= MAXC) text(110, 120, .8f, C2D_Color32(255, 255, 255, 255), "Map terminee !");

    C2D_TargetClear(bot, C2D_Color32(27, 42, 133, 255)); C2D_SceneBegin(bot);
    text(90, 10, .9f, C2D_Color32(255, 212, 31, 255), "BOUTIQUE");
    snprintf(buf, sizeof buf, "Pieces: %llu", coins); text(20, 50, .6f, C2D_Color32(255, 255, 255, 255), buf);
    C2D_DrawRectSolid(20, 90, 0.3f, 280, 45, C2D_Color32(43, 140, 255, 255));
    if (lvl >= 50) snprintf(buf, sizeof buf, "Chance x%.2f (MAX)", luck());
    else snprintf(buf, sizeof buf, "Chance x%.2f -> cout %d", luck(), upCost());
    text(30, 102, .55f, C2D_Color32(255, 255, 255, 255), buf);
    C2D_DrawRectSolid(20, 150, 0.3f, 280, 45, C2D_Color32(217, 48, 79, 255));
    text(30, 162, .55f, C2D_Color32(255, 255, 255, 255), "Nouvelle map");
    text(20, 215, .45f, C2D_Color32(200, 200, 255, 255), "START = quitter");
    C3D_FrameEnd(0);
  }
  save();
  C2D_TextBufDelete(tb); C2D_Fini(); C3D_Fini(); gfxExit();
  return 0;
}
