#include <X11/Xlib.h>
#include <string.h>
#include <locale.h>
#include <fontconfig/fontconfig.h>
#include <fcitx-utils/log.h>
#include <libintl.h>
#include <X11/Xft/Xft.h>
#include "lightui.h"
#include "font.h"

void CreateFont (FcitxLightUI* lightui)
{
    int i;
    FcPattern *pattern;
    FcPattern *configured;
    FcPattern *match;
    FcResult result;
    XftFont *font;

    if(lightui->xftfont) {
        for (i = 0; i < lightui->xftfont->set->nfont; i++) {
            if(lightui->xftfont->fonts[i])
                XftFontClose(lightui->dpy, lightui->xftfont->fonts[i]);
        }
        FcFontSetDestroy(lightui->xftfont->set);
        free(lightui->xftfont->fonts);
        free(lightui->xftfont);
    }

    pattern = FcNameParse((FcChar8 *) lightui->font);
    if (!pattern)
        FcitxLog(FATAL, _("can't open font %s."), lightui->font);

    FcPatternAddDouble(pattern, FC_PIXEL_SIZE, lightui->fontSize);

    configured = FcPatternDuplicate(pattern);
    if (!configured)
        FcitxLog(FATAL, _("can't open font %s."), lightui->font);

    FcConfigSubstitute(NULL, configured, FcMatchPattern);
    XftDefaultSubstitute(lightui->dpy, DefaultScreen(lightui->dpy), configured);

    match = FcFontMatch(NULL, configured, &result);
    if (!match)
        FcitxLog(FATAL, _("can't open font %s."), lightui->font);

    font = XftFontOpenPattern(lightui->dpy, match);
    if (!font)
        FcitxLog(FATAL, _("can't open font %s."), lightui->font);

    lightui->xftfont = malloc(sizeof(XFont));

    lightui->xftfont->ascent = font->ascent;
    lightui->xftfont->descent = font->descent;
    lightui->xftfont->height = font->ascent + font->descent;
    lightui->xftfont->fontsize = lightui->fontSize;
    XftFontClose(lightui->dpy, font);

    lightui->xftfont->set = FcFontSort(0, configured, 1, 0, &result);
    lightui->xftfont->fonts = malloc(sizeof(XftFont *) * lightui->xftfont->set->nfont);
    for (i = 0; i < lightui->xftfont->set->nfont; i++) {
        lightui->xftfont->fonts[i] = NULL;
    }

    FcitxLog(INFO, "Font: %s, size: %d, height: %d",
                   lightui->font,
                   lightui->xftfont->fontsize,
                   lightui->xftfont->height);

    FcPatternDestroy(configured);
    FcPatternDestroy(pattern);
}

#define UTF_INVALID 0xFFFD
#define UTF_SIZ     4
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))

static const unsigned char utfbyte[UTF_SIZ + 1] = {0x80,    0, 0xC0, 0xE0, 0xF0};
static const unsigned char utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[UTF_SIZ + 1] = {       0,    0,  0x80,  0x800,  0x10000};
static const long utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static long
utf8decodebyte(const char c, size_t *i)
{
    for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
        if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
            return (unsigned char)c & ~utfmask[*i];
    return 0;
}

static size_t
utf8validate(long *u, size_t i)
{
    if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
        *u = UTF_INVALID;
    for (i = 1; *u > utfmax[i]; ++i)
        ;
    return i;
}

static size_t
utf8decode(const char *c, long *u, size_t clen)
{
    size_t i, j, len, type;
    long udecoded;

    *u = UTF_INVALID;
    if (!clen)
        return 0;
    udecoded = utf8decodebyte(c[0], &len);
    if (!BETWEEN(len, 1, UTF_SIZ))
        return 1;
    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
        if (type)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    utf8validate(u, len);

    return len;
}

static int DrawString(Display *dpy, XftDraw *draw, XftColor *color, XFont *font, int x, int y, const char *s, int len)
{
    long u8char;
    int i;
    int xp;
    int u8clen;
    XftFont *sfont;
    FcResult fcres;
    XGlyphInfo ext;
    int w;
    /*
     * Step through all UTF-8 characters one by one and search in the font
     * cache ring buffer, whether there was some font found to display the
     * unicode value of that UTF-8 character.
     */
    for (xp = x; len > 0; ) {
        u8clen = utf8decode(s, &u8char, len);
        if (u8clen == 0) {
            break;
        }

        s += u8clen;
        len -= u8clen;

        w = 0;
        for (i = 0; i < font->set->nfont; i++) {
            if (font->fonts[i] == NULL) {
                FcPattern *m1 = FcPatternDuplicate(font->set->fonts[i]);
                FcPatternAddDouble(m1, FC_PIXEL_SIZE, font->fontsize);
                FcConfigSubstitute(NULL, m1, FcMatchPattern);
                XftDefaultSubstitute(dpy, DefaultScreen(dpy), m1);
                FcPattern *m2 = FcFontMatch(NULL, m1, &fcres);
                if (m2) {
                    font->fonts[i] = XftFontOpenPattern(dpy, m2);
                }
                FcPatternDestroy(m1);
            }

            sfont = font->fonts[i];
            if(sfont) {
                FT_UInt idx = XftCharIndex(dpy, sfont, u8char);
                if (idx) {
                    XftGlyphExtents(dpy, sfont, &idx, 1, &ext);
                    w = ext.xOff;
                    if (draw)
                        XftDrawGlyphs(draw, color, sfont, xp, y, &idx, 1);
                    break;
                }
            }
        }

        if (i == font->set->nfont) {
            XftTextExtentsUtf8(dpy, font->fonts[0], (XftChar8 *) "?", 1, &ext);
            w = ext.xOff;
            if (draw)
                XftDrawStringUtf8(draw, color, font->fonts[0], xp, y, (XftChar8 *) "?", 1);
        }

        xp += w;
    }
    return xp;
}

void OutputString (Display* dpy, XftDraw* xftDraw, Drawable window, XFont *font, char *str, int x, int y, FcitxConfigColor color)
{
    if (!font || !str)
        return;

    y += FontHeight(dpy, font);

    XftColor        xftColor;
    XRenderColor    renderColor;

    if (!font || !str)
        return;

    renderColor.red = color.r * 65535;
    renderColor.green = color.g * 65535;
    renderColor.blue = color.b * 65535;
    renderColor.alpha = 0xFFFF;

    XftColorAllocValue (dpy, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)), &renderColor, &xftColor);
    XftDrawChange (xftDraw, window);
    DrawString(dpy, xftDraw, &xftColor, font, x, y, str, strlen (str));

    XftColorFree (dpy, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)), &xftColor);
}


int StringWidth (Display* dpy, XFont * font, char *str)
{
    if (!font || !str)
        return 0;

    return DrawString(dpy, NULL, NULL, font, 0, 0, str, strlen(str));
}

int FontHeight (Display* dpy, XFont * font)
{
    if (!font)
        return 0;

    return font->height;
}
