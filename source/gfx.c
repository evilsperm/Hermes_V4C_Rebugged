#include "gfx.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>


struct {
    float x, y, dx, dy, r, rs;

} m_twat[32];


void DrawBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
       
    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_End();
}


void init_twat()
{
    int i;

    for(i = 0; i < 32; i++) {
        m_twat[i].x = (rand() % 640) + 104;
        m_twat[i].y = (rand() % 300) + 106;

        m_twat[i].dx = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
        m_twat[i].dy = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
        m_twat[i].r = 0;
        m_twat[i].rs = ((float) ((int) (rand() & 7) - 3)) / 80.0f;
    }
}

void update_twat()
{
    int i;

    for(i = 0; i < 32; i++) {

        if((rand() & 0x1ff) == 5) {
            m_twat[i].dx = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
            m_twat[i].dy = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
            m_twat[i].rs = ((float) ((int) (rand() & 7) - 3)) / 80.0f;
            
        }

        if(m_twat[i].dx == 0.0f && m_twat[i].dy == 0.0f) {m_twat[i].dy = 0.25f; m_twat[i].dx = (rand() & 1) ? 0.25f : -0.25f;}
        if(m_twat[i].rs == 0.0f) m_twat[i].rs = (rand() & 1) ? .001f : -0.001f;
        
        m_twat[i].x += m_twat[i].dx;
        m_twat[i].y += m_twat[i].dy;
        m_twat[i].r += m_twat[i].rs;
    
        if(i & 1) {
            if(m_twat[i].x < 0)  {
                if(m_twat[i].dx < 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y < 0)  {
                if(m_twat[i].dy < 0) m_twat[i].dy = -m_twat[i].dy;
            }

            if(m_twat[i].x >= 600)  {
                if(m_twat[i].dx > 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y >= 480)  {
                if(m_twat[i].dy > 0) m_twat[i].dy = -m_twat[i].dy;
            }
        } else {
            if(m_twat[i].x < 248)  {
                if(m_twat[i].dx < 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y < 32)  {
                if(m_twat[i].dy < 0) m_twat[i].dy = -m_twat[i].dy;
            }

            if(m_twat[i].x >= 848)  {
                if(m_twat[i].dx > 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y >= 512)  {
                if(m_twat[i].dy > 0) m_twat[i].dy = -m_twat[i].dy;
            }
        
        }
        
        draw_twat(m_twat[i].x, m_twat[i].y, m_twat[i].r);
    }
    
}

void draw_twat(float x, float y, float angle)
{
    int n;

    float ang, angs = 6.2831853071796 / 8, angs2 = 6.2831853071796 / 32;

    MATRIX matrix;
    
    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x , y , 65535.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLES);

    ang = 0.0f;

    for(n = 0; n <8; n++) {

        tiny3d_VertexPos(4.0f *sinf(ang), 4.0f *cosf(ang), 0);
        tiny3d_VertexColor(0xffffff30);
        tiny3d_VertexPos(7.0f *sinf(ang+angs/2), 7.0f *cosf(ang+angs/2), 0);
        tiny3d_VertexColor(0xff00ff40);
        tiny3d_VertexPos(4.0f *sinf(ang+angs), 4.0f *cosf(ang+angs), 0);
        tiny3d_VertexColor(0xffffff30);

        ang += angs;
    }

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_POLYGON);

    ang = 0.0f;

    for(n = 0; n <32; n++) {
        tiny3d_VertexPos(3.0f * sinf(ang), 3.0f * cosf(ang), 0);
        if(n & 1) tiny3d_VertexColor(0x80ffff40); else tiny3d_VertexColor(0xffffff40);
        ang += angs2;
    }

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity

}

