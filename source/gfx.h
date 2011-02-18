#ifndef GFX_H
#define GFX_H

#include <tiny3d.h>
#include <libfont.h>

void DrawBox(float x, float y, float z, float w, float h, u32 rgba);

void init_twat();
void update_twat();
void draw_twat(float x, float y, float angle);


#endif