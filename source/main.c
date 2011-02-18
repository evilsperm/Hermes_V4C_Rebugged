/* 
   (c) 2011 Hermes <www.elotrolado.net>
   payloadhv4

*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <sysmodule/sysmodule.h>
// for msgdialogs
#include "sysutil/events.h"
#include "sysutil/msgdialog.h"

#include <io/pad.h>

#include <tiny3d.h>
#include <libfont.h>

#include <psl1ght/lv2.h>
#include <lv2/process.h>

#include "syscall8.h"

// you need the Oopo ps3libraries to work with freetype

#include <ft2build.h>
#include <freetype/freetype.h> 
#include <freetype/ftglyph.h>

#include "gfx.h"
#include "pad.h"

#include "payload_groove_hermes.bin.h"
// include fonts

#include "andika_ttf.bin.h"

PadInfo padinfo;
PadData paddata;

/******************************************************************************************************************************************************/
/* TTF functions to load and convert fonts                                                                                                             */
/******************************************************************************************************************************************************/

int ttf_inited = 0;

FT_Library freetype;
FT_Face face;

/* TTFLoadFont can load TTF fonts from device or from memory:

path = path to the font or NULL to work from memory

from_memory = pointer to the font in memory. It is ignored if path != NULL.

size_from_memory = size of the memory font. It is ignored if path != NULL.

*/

int TTFLoadFont(char * path, void * from_memory, int size_from_memory)
{
   
    if(!ttf_inited)
        FT_Init_FreeType(&freetype);
    ttf_inited = 1;

    if(path) {
        if(FT_New_Face(freetype, path, 0, &face)) return -1;
    } else {
        if(FT_New_Memory_Face(freetype, from_memory, size_from_memory, 0, &face)) return -1;
        }

    return 0;
}

/* release all */

void TTFUnloadFont()
{
   FT_Done_FreeType(freetype);
   ttf_inited = 0;
}

/* function to render the character

chr : character from 0 to 255

bitmap: u8 bitmap passed to render the character character (max 256 x 256 x 1 (8 bits Alpha))

*w : w is the bitmap width as input and the width of the character (used to increase X) as output
*h : h is the bitmap height as input and the height of the character (used to Y correction combined with y_correction) as output

y_correction : the Y correction to display the character correctly in the screen

*/

void TTF_to_Bitmap(u8 chr, u8 * bitmap, short *w, short *h, short *y_correction)
{
    FT_Set_Pixel_Sizes(face, (*w), (*h));
    
    FT_GlyphSlot slot = face->glyph;

    memset(bitmap, 0, (*w) * (*h));

    if(FT_Load_Char(face, (char) chr, FT_LOAD_RENDER )) {(*w) = 0; return;}

    int n, m, ww;

    *y_correction = (*h) - 1 - slot->bitmap_top;
    
    ww = 0;

    for(n = 0; n < slot->bitmap.rows; n++) {
        for (m = 0; m < slot->bitmap.width; m++) {

            if(m >= (*w) || n >= (*h)) continue;
            
            bitmap[m] = (u8) slot->bitmap.buffer[ww + m];
        }
    
    bitmap += *w;

    ww += slot->bitmap.width;
    }

    *w = ((slot->advance.x + 31) >> 6) + ((slot->bitmap_left < 0) ? -slot->bitmap_left : 0);
    *h = slot->bitmap.rows;
}




// draw one background color in virtual 2D coordinates

void DrawBackground2D(u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(0  , 0  , 65535);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(847, 0  , 65535);

    tiny3d_VertexPos(847, 511, 65535);

    tiny3d_VertexPos(0  , 511, 65535);
    tiny3d_End();
}


void DrawCenteredBar2D(float y, float w, float h, u32 rgba)
{
    float x = (848.0f - w)/ 2.0f;

    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , 1.0f);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , 1.0f);

    tiny3d_VertexPos(x + w, y + h, 1.0f);

    tiny3d_VertexPos(x    , y + h, 1.0f);
    tiny3d_End();
}

void LoadTexture()
{

    u32 * texture_mem = tiny3d_AllocTexture(64*1024*1024); // alloc 64MB of space for textures (this pointer can be global)    

    u32 * texture_pointer; // use to asign texture space without changes texture_mem

    if(!texture_mem) return; // fail!

    texture_pointer = texture_mem;

    

    ResetFont();

    TTFLoadFont(NULL, (void *) andika_ttf_bin, sizeof(andika_ttf_bin));
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 32, 32, TTF_to_Bitmap);
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 64, 64, TTF_to_Bitmap);
    TTFUnloadFont();
}


volatile int dialog_action = 0;

void my_dialog(msgbutton button, void *userdata)
{
    switch(button) {

        case MSGDIALOG_BUTTON_OK:
            dialog_action = 1;
            break;
        case MSGDIALOG_BUTTON_NO:
        case MSGDIALOG_BUTTON_ESCAPE:
            dialog_action = 2;
            break;
        case MSGDIALOG_BUTTON_NONE:
            dialog_action = -1;
            break;
        default:
		    break;
    }
}


void cls()
{
    tiny3d_Clear(0xff0040ff, TINY3D_CLEAR_ALL);
        
    // Enable alpha Test
    tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

    // Enable alpha blending.
    tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
        NV30_3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | NV30_3D_BLEND_FUNC_DST_ALPHA_ZERO,
        TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
}

/******************************************************************************************************************************************************/
/* Payload functions                                                                                                                                  */
/******************************************************************************************************************************************************/

u64 lv2peek(u64 addr) 
{ return Lv2Syscall1(6, (u64) addr); }

u64 lv2poke(u64 addr, u64 value) 
{ return Lv2Syscall2(7, (u64) addr, (u64) value); }

LV2_SYSCALL lv2launch(u64 addr) 
{ return Lv2Syscall8(9, (u64) addr, 0,0,0,0,0,0,0); }

LV2_SYSCALL sys8_perm(u64 mode) 
{ return Lv2Syscall4(8, (u64) 8 ,(u64) mode, 0, 0); }

LV2_SYSCALL syscall36(char * path) 
{ return Lv2Syscall1(36, (u64) path); }


static char payload_bin[0x1000];
static int payload_len;

int send_payload_code()
{
    int l, n;

    u64 v = lv2peek(0x8000000000017CD0ULL);
        
    if(v != 0x4E8000203C608001ULL) return 1; // if syscall 8 is modified payload is resident...

    

    FILE *fp =fopen("/dev_usb/payload.bin", "r");
    if(!fp) fp =fopen("/dev_hdd0/game/PAYLOADV4/payload.bin", "r");

    if(fp) {


       payload_len = fread(payload_bin, 1, 0x1000, fp);

       fclose(fp);  
    
    }

    if(payload_len <= 0 || !fp) 
        {payload_len = sizeof (payload_groove_hermes_bin); memcpy(payload_bin, &payload_groove_hermes_bin[0], payload_len);}

    if(payload_len > 0x1000) return 2; // payload too big...

    /* i repeat 25 times the patch because it is possible fail patching (cache instruction problem? maybe only in syscall 9 patch
    because the call of syscall 7) */
    
    for(l = 0; l < 25; l++) {
        u8 * p = (u8 *) payload_bin;

        
        for(n = 0; n < payload_len; n += 8) {
        
            static u64 value;

            memcpy(&value, &p[n], 8);

            lv2poke(0x80000000007e0000ULL + (u64) n, value);

            __asm__("sync");

            value =  lv2peek(0x8000000000000000ULL);

            }
        
        // syscall 9 enable
        lv2poke(0x8000000000017CE0ULL , 0x7C6903A64E800420ULL);
        __asm__("sync");
    }


    return 0;
}

typedef struct {

	path_open_entry entries[2];

	char arena[0x420*2];

} path_open_table;

s32 main(s32 argc, const char* argv[])
{
	int flag = 0;
    int n;
        
    flag = send_payload_code();

	tiny3d_Init(1024*1024);

	ioPadInit(7);

	// Load texture

    LoadTexture();

    float x, y;

    cls();

    SetFontSize(32, 64);
    
    x= 0.0; y = 512.0f/2.0f - 32.0f;
    
    SetFontColor(0xffffffff, 0x00000000);
    SetFontAutoCenter(1);

    #if 1
    sleep(2);
    #else
    n = 40;
    while(n>0) {
        usleep(50000); // wait two seconds (piano, piano... XD)
        ps3pad_read();
        if(new_pad & BUTTON_CROSS) break;
        n--;
    }
    #endif

    if(!flag) {

        lv2launch(0x80000000007e0000ULL);
    
        // syscall 9 restore
     //   lv2poke(0x8000000000017CE0ULL , 0x3C60800160630003ULL);  now it is done in the payload
        __asm__("sync");

        DrawString(0, y, "Payload Loaded");
    } else if(flag == 1) DrawString(0, y, "Payload Is Resident");
    else if(flag) DrawString(0, y, "Payload Is Too Big");

    SetFontAutoCenter(0);

    x= (848-640) / 2; y=(512-360)/2;
    DrawBox(x - 16, y - 16, 65535.0f, 640.0f + 32, 360 + 32, 0x00000028);
    DrawBox(x, y, 65535.0f, 640.0f, 360, 0x30003018);
    
    tiny3d_Flip();
    
    sleep(2); // waiting to read message

    #if 0


    if(flag < 2 && (new_pad & BUTTON_CROSS)) {
      
    long frame = 0;
    int option = 0;
    int max_option = 5;
    int skip_versions = 0;

    int buclea = 1;

    init_twat();
    
    new_pad = 0;

    int patch_app_ver = sys8_sys_configure(CFG_UNPATCH_APPVER); // get old value

    sys8_sys_configure(CFG_UNPATCH_APPVER + (patch_app_ver!=0)); // restore old value
   
    while(buclea) {
    
        int flash = (frame >> 5) & 1;

        frame++;

        ps3pad_read();

        cls();

        update_twat();

        SetFontSize(18, 32);
        SetFontAutoCenter(1);
        y = 16;
        SetFontColor(0xffffffff, 0x00000000);
        DrawString(0, y, "Payload Version v4D - Launcher v1.04");
        y += 128 + 8 - 28;

        
        if(option == 0) {DrawCenteredBar2D(y, 480, 40, (flash) ? 0x004080C0 : 0x00000080);SetFontColor(0xffffffff, 0x00000000);}
        else {DrawCenteredBar2D(y, 480, 40, 0x00000080);SetFontColor(0x00ff00ff, 0x00000000);}

        DrawString(0, y, "XMB Debug");
        y += 56;

        if(option == 1) {DrawCenteredBar2D(y, 480, 40, (flash) ? 0x004080C0 : 0x00000080);SetFontColor(0xffffffff, 0x00000000);}
        else {DrawCenteredBar2D(y, 480, 40, 0x00000080);SetFontColor(0x00ff00ff, 0x00000000);}

        DrawString(0, y, "XMB Retail");
        y += 56;

        if(option == 2) {DrawCenteredBar2D(y, 480, 40, (flash) ? 0x004080C0 : 0x00000080);SetFontColor(0xffffffff, 0x00000000);}
        else {DrawCenteredBar2D(y, 480, 40, 0x00000080);SetFontColor(0x00ff00ff, 0x00000000);}

        DrawString(0, y, "XMB Retail With PS3_SYSTEM_VER patch");
        y += 56;

        if(option == 3) {DrawCenteredBar2D(y, 480, 40, (flash) ? 0x004080C0 : 0x00000080);SetFontColor(0xffffffff, 0x00000000);}
        else {DrawCenteredBar2D(y, 480, 40, 0x00000080);SetFontColor(0x00ff00ff, 0x00000000);}

        if(!patch_app_ver) DrawString(0, y, "Press to Patch APP_VER"); else DrawString(0, y, "Press to Unpatch APP_VER");

        y += 56;
      
        if(option == 4 && !((skip_versions >> 4) & 1)) {DrawCenteredBar2D(y, 480, 40, (flash) ? 0x004080C0 : 0x00000080);SetFontColor(0xffffffff, 0x00000000);}
        else {DrawCenteredBar2D(y, 480, 40, 0x00000080);SetFontColor(0xffff00ff, 0x00000000);}

        if(!((skip_versions >> 4) & 1)) DrawFormatString(0, y, "UnMount Devices");

        y += 56;
        
        SetFontAutoCenter(0);

        x= (848-640) / 2; y=(512-360)/2;
        DrawBox(x - 16, y - 16, 65535.0f, 640.0f + 32, 360 + 32, 0x00000028);
        DrawBox(x, y, 65535.0f, 640.0f, 360, 0x30003018);

        tiny3d_Flip();

        if(new_pad & BUTTON_CROSS) {
        
            switch(option) {
                case 0:
                    sys8_sys_configure(CFG_XMB_DEBUG);
                    buclea = 0;
                    break;

                case 1:
                    sys8_sys_configure(CFG_XMB_RETAIL);
                    
                    buclea = 0;
                    break;

                case 2:
                    sys8_sys_configure(CFG_XMB_RETAIL);
                    sys8_sys_configure(CFG_UNMOUNT_DEVICES);
                    syscall36("//dev_bdvd");

                    sys8_sys_configure(CFG_UNPATCH_APPVER + (patch_app_ver!=0));
                    
                    msgtype mdialogyesno = MSGDIALOG_NORMAL | MSGDIALOG_BUTTON_TYPE_YESNO | MSGDIALOG_DISABLE_CANCEL_ON | MSGDIALOG_DEFAULT_CURSOR_NO;

                    MsgDialogOpen(mdialogyesno, "Select YES to load EBOOT.BIN from:\n/dev_usb/ps3game", my_dialog, (void *) 0x11110001, NULL);

                    dialog_action = 0;

                    while(!dialog_action)
                        {
                        sysCheckCallback();tiny3d_Flip();
                        }

                    MsgDialogClose();

                    // use USB redirection
                    if(dialog_action == 1) {
                        
                        u64 dest_table_addr;
                        path_open_table open_table;
                        sys8_path_table(0LL);

	
                        dest_table_addr= 0x80000000007FF000ULL-((sizeof(path_open_table)+15) & ~15);
                        open_table.entries[0].compare_addr= ((uint64_t) &open_table.arena[0]) - ((uint64_t) &open_table) + dest_table_addr;
                        open_table.entries[0].replace_addr= ((uint64_t) &open_table.arena[0x420])- ((uint64_t) &open_table) + dest_table_addr;
                        open_table.entries[1].compare_addr= 0ULL; // the last entry always 0
                        
                        strncpy(&open_table.arena[0], "/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN", 0x100); // compare 1
                        strncpy(&open_table.arena[0x420], "/dev_usb/ps3game/EBOOT.BIN", 0x420);     // replace 1

                        open_table.entries[0].compare_len= strlen(&open_table.arena[0]);		// 1
                        open_table.entries[0].replace_len= strlen(&open_table.arena[0x420]);

                        sys8_memcpy(dest_table_addr, (uint64_t) &open_table, sizeof(path_open_table));

                        // set the path table
                        sys8_path_table( dest_table_addr);
                    }

                    buclea = 0;
                    break;

                case 3:
                    patch_app_ver ^= 1;
                    sys8_sys_configure(CFG_UNPATCH_APPVER + (patch_app_ver!=0));
                    break;
                case 4:
                    max_option = 4;
                    skip_versions |= 1<<4;
                    option = 0;
                    patch_app_ver = 0;
                    sys8_sys_configure(CFG_UNMOUNT_DEVICES);
                    break;
            }

            
        }

        if(new_pad & BUTTON_DOWN) {
        
            option++; if(option >= max_option) option = 0;
        }

        if(new_pad & BUTTON_UP) {
        
            option--; if(option < 0) option = max_option - 1;
        }
    

    }
    
    }
  #endif

	return 0;
}

