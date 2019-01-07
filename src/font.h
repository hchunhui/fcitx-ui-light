#ifndef FONT_H
#define FONT_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <fcitx-config/fcitx-config.h>

struct _FcitxLightUI;
typedef struct _XFont {
	int ascent;
	int descent;
	int height;
	XftFont **fonts;
	FcFontSet *set;
	int fontsize;
} XFont;

void GetValidFont(const char* strUserLocale, char **font);
void CreateFont(struct _FcitxLightUI* lightui);
void OutputString (Display* dpy, XftDraw* xftDraw, Drawable window, XFont* font, char* str, int x, int y, FcitxConfigColor color);
int StringWidth (Display* dpy, XFont* font, char* str);
int FontHeight (Display* dpy, XFont* font);

#endif
