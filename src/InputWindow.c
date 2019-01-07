/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <libintl.h>

#include "fcitx/ui.h"
#include "fcitx/module.h"
#include "fcitx/profile.h"
#include "fcitx/frontend.h"
#include "fcitx/configfile.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"

#include "InputWindow.h"
#include "lightui.h"
#include "draw.h"
#include <fcitx/module/x11/fcitx-x11.h>
#include "MainWindow.h"
#include <fcitx-utils/log.h>

static boolean InputWindowEventHandler(void *arg, XEvent* event);
static void InitInputWindow(InputWindow* inputWindow);
static void ReloadInputWindow(void* arg, boolean enabled);

void InitInputWindow(InputWindow* inputWindow)
{
    XSetWindowAttributes    attrib;
    unsigned long   attribmask;
    char        strWindowName[]="Fcitx Input Window";
    int depth;
    Colormap cmap;
    Visual * vs;
    FcitxLightUI* lightui = inputWindow->owner;
    int iScreen = lightui->iScreen;
    Display* dpy = lightui->dpy;
    inputWindow->window = None;
    inputWindow->iInputWindowHeight = INPUTWND_HEIGHT;
    inputWindow->iInputWindowWidth = INPUTWND_WIDTH;
    inputWindow->iOffsetX = 0;
    inputWindow->iOffsetY = 8;
    inputWindow->dpy = dpy;
    inputWindow->iScreen = iScreen;

    inputWindow->iInputWindowHeight= INPUTWND_HEIGHT;
    vs= NULL;
    LightUIInitWindowAttribute(lightui, &vs, &cmap, &attrib, &attribmask, &depth);

    inputWindow->window=XCreateWindow (dpy,
                                       RootWindow(dpy, iScreen),
                                       lightui->iMainWindowOffsetX,
                                       lightui->iMainWindowOffsetY,
                                       inputWindow->iInputWindowWidth,
                                       inputWindow->iInputWindowHeight,
                                       0,
                                       depth,InputOutput,
                                       vs,attribmask,
                                       &attrib);

    inputWindow->pixmap = XCreatePixmap(dpy,
                                 inputWindow->window,
                                 INPUT_BAR_MAX_WIDTH,
                                 INPUT_BAR_MAX_HEIGHT,
                                 depth);
    inputWindow->pixmap2 = XCreatePixmap(dpy,
                                 inputWindow->pixmap,
                                 INPUT_BAR_MAX_WIDTH,
                                 INPUT_BAR_MAX_HEIGHT,
                                 depth);

    XGCValues gcvalues;
    inputWindow->window_gc = XCreateGC(inputWindow->dpy, inputWindow->window, 0, &gcvalues);
    inputWindow->pixmap_gc = XCreateGC(inputWindow->dpy, inputWindow->pixmap, 0, &gcvalues);
    inputWindow->pixmap2_gc = XCreateGC(inputWindow->dpy, inputWindow->pixmap2, 0, &gcvalues);
    inputWindow->xftDraw = XftDrawCreate(inputWindow->dpy, inputWindow->pixmap, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)));

    XSelectInput (dpy, inputWindow->window, ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | ExposureMask);

    LightUISetWindowProperty(lightui, inputWindow->window, FCITX_WINDOW_DOCK, strWindowName);
}

InputWindow* CreateInputWindow(FcitxLightUI *lightui)
{
    InputWindow* inputWindow;

    inputWindow = fcitx_utils_malloc0(sizeof(InputWindow));
    inputWindow->owner = lightui;
    InitInputWindow(inputWindow);

    FcitxX11AddXEventHandler(lightui->owner, InputWindowEventHandler, inputWindow);
    FcitxX11AddCompositeHandler(lightui->owner, ReloadInputWindow, inputWindow);

    inputWindow->msgUp = FcitxMessagesNew();
    inputWindow->msgDown = FcitxMessagesNew();
    return inputWindow;
}

boolean InputWindowEventHandler(void *arg, XEvent* event)
{
    InputWindow* inputWindow = arg;
    if (event->xany.window == inputWindow->window)
    {
        switch (event->type)
        {
        case Expose:
            DrawInputWindow(inputWindow);
            break;
        case ButtonPress:
            switch (event->xbutton.button) {
            case Button1:
            {
                int             x,
                y;
                x = event->xbutton.x;
                y = event->xbutton.y;
                LightUIMouseClick(inputWindow->owner, inputWindow->window, &x, &y);

                FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inputWindow->owner->owner);

                if (ic)
                    FcitxInstanceSetWindowOffset(inputWindow->owner->owner, ic, x, y);

                DrawInputWindow(inputWindow);
            }
            break;
            }
            break;
        }
        return true;
    }
    return false;
}

void DisplayInputWindow (InputWindow* inputWindow)
{
    FcitxLog(DEBUG, _("DISPLAY InputWindow"));
    MoveInputWindowInternal(inputWindow);
    XMapRaised (inputWindow->dpy, inputWindow->window);
}

void DrawInputWindow(InputWindow* inputWindow)
{
    int lastW = inputWindow->iInputWindowWidth, lastH = inputWindow->iInputWindowHeight;
    int cursorPos = FcitxUINewMessageToOldStyleMessage(inputWindow->owner->owner, inputWindow->msgUp, inputWindow->msgDown);
    DrawInputBar(inputWindow, cursorPos, inputWindow->msgUp, inputWindow->msgDown, &inputWindow->iInputWindowHeight ,&inputWindow->iInputWindowWidth);

    /* Resize Window will produce Expose Event, so there is no need to draw right now */
    if (lastW != inputWindow->iInputWindowWidth || lastH != inputWindow->iInputWindowHeight)
    {
        XResizeWindow(
            inputWindow->dpy,
            inputWindow->window,
            inputWindow->iInputWindowWidth,
            inputWindow->iInputWindowHeight);
        MoveInputWindowInternal(inputWindow);
    }

    XCopyArea(
            inputWindow->dpy,
            inputWindow->pixmap,
            inputWindow->window,
            inputWindow->window_gc,
            0, 0,
            inputWindow->iInputWindowWidth,
            inputWindow->iInputWindowHeight,
            0, 0
             );

    XFlush(inputWindow->dpy);
}

void MoveInputWindowInternal(InputWindow* inputWindow)
{
    int x = 0, y = 0, w = 0, h = 0;

    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inputWindow->owner->owner);
    FcitxInstanceGetWindowRect(inputWindow->owner->owner, ic, &x, &y, &w, &h);
    FcitxRect rect = GetScreenGeometry(inputWindow->owner, x, y);

    int iTempInputWindowX, iTempInputWindowY;

    if (x < rect.x1)
        iTempInputWindowX = rect.x1;
    else
        iTempInputWindowX = x + inputWindow->iOffsetX;

    if (y < rect.y1)
        iTempInputWindowY = rect.y1;
    else
        iTempInputWindowY = y + h + inputWindow->iOffsetY;

    if ((iTempInputWindowX + inputWindow->iInputWindowWidth) > rect.x2)
        iTempInputWindowX = rect.x2 - inputWindow->iInputWindowWidth;

    if ((iTempInputWindowY + inputWindow->iInputWindowHeight) > rect.y2) {
        if ( iTempInputWindowY > rect.y2 )
            iTempInputWindowY = rect.y2 - inputWindow->iInputWindowHeight - 40;
        else
            iTempInputWindowY = iTempInputWindowY - inputWindow->iInputWindowHeight - ((h == 0)?40:h) - 2 * inputWindow->iOffsetY;
    }
    XMoveWindow (inputWindow->dpy, inputWindow->window, iTempInputWindowX, iTempInputWindowY);
}

void CloseInputWindowInternal(InputWindow* inputWindow)
{
    XUnmapWindow (inputWindow->dpy, inputWindow->window);
}

void ReloadInputWindow(void* arg, boolean enabled)
{
    InputWindow* inputWindow = (InputWindow*) arg;
    boolean visable = WindowIsVisable(inputWindow->dpy, inputWindow->window);

    XFreeGC(inputWindow->dpy, inputWindow->window_gc);
    XFreeGC(inputWindow->dpy, inputWindow->pixmap_gc);
    XFreeGC(inputWindow->dpy, inputWindow->pixmap2_gc);

    XFreePixmap(inputWindow->dpy, inputWindow->pixmap2);
    XFreePixmap(inputWindow->dpy, inputWindow->pixmap);

    XDestroyWindow(inputWindow->dpy, inputWindow->window);
    XftDrawDestroy(inputWindow->xftDraw);

    inputWindow->window = None;

    InitInputWindow(inputWindow);

    if (visable)
        ShowInputWindowInternal(inputWindow);
}

void ShowInputWindowInternal(InputWindow* inputWindow)
{
    XMapRaised(inputWindow->dpy, inputWindow->window);
    DrawInputWindow(inputWindow);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
