#ifndef FACET_RENDER_H
#define FACET_RENDER_H
#include "game.h"
void render_init(void);
void render_begin(const Game*g,int frame,int presentation);
void render_scene(const Game*g,int frame);
void render_text(int x,int y,const char*text);
void render_center(int y,const char*text);
void render_rect(int x,int y,int w,int h,int r,int g,int b);
void render_end(void);
#endif
