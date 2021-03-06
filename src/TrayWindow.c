/***************************************************************************
 *   Copyright (C) 2002~2010 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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

#include "fcitx/fcitx.h"

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/xpm.h>
#include <X11/extensions/Xrender.h>

#include "TrayWindow.h"
#include "tray.h"
#include "draw.h"
#include "lightui.h"
#include "fcitx/module/x11/fcitx-x11.h"
#include "fcitx-utils/log.h"
#include "fcitx/frontend.h"
#include "fcitx/module.h"
#include "MenuWindow.h"
#include "fcitx/instance.h"
#include <fcitx-utils/utils.h>
static boolean TrayEventHandler(void *arg, XEvent* event);

void InitTrayWindow(TrayWindow *trayWindow)
{
    FcitxLightUI *lightui = trayWindow->owner;
    Display *dpy = lightui->dpy;
    char   strWindowName[]="Fcitx Tray Window";
    if ( !lightui->bUseTrayIcon )
        return;

    InitTray(dpy, trayWindow);

    XVisualInfo* vi = TrayGetVisual(dpy, trayWindow);
    if (vi && vi->visual) {
        Window p = DefaultRootWindow (dpy);
        Colormap colormap = XCreateColormap(dpy, p, vi->visual, AllocNone);
        XSetWindowAttributes wsa;
        wsa.background_pixmap = 0;
        wsa.colormap = colormap;
        wsa.background_pixel = 0;
        wsa.border_pixel = 0;
        trayWindow->window = XCreateWindow(dpy, p, -1, -1, 1, 1,
                                           0, vi->depth, InputOutput, vi->visual,
                                           CWBackPixmap|CWBackPixel|CWBorderPixel|CWColormap, &wsa);
    }
    else {
        trayWindow->window = XCreateSimpleWindow (dpy, DefaultRootWindow(dpy),
                             -1, -1, 1, 1, 0,
                             BlackPixel (dpy, DefaultScreen (dpy)),
                             WhitePixel (dpy, DefaultScreen (dpy)));
        XSetWindowBackgroundPixmap(dpy, trayWindow->window, ParentRelative);
    }
    if (trayWindow->window == (Window) NULL)
        return;

    XSizeHints size_hints;
    size_hints.flags = PWinGravity | PBaseSize;
    size_hints.base_width = trayWindow->size;
    size_hints.base_height = trayWindow->size;
    XSetWMNormalHints(dpy, trayWindow->window, &size_hints);


    XSelectInput (dpy, trayWindow->window, ExposureMask | KeyPressMask |
                  ButtonPressMask | ButtonReleaseMask | StructureNotifyMask
                  | EnterWindowMask | PointerMotionMask | LeaveWindowMask | VisibilityChangeMask);

    LightUISetWindowProperty(lightui, trayWindow->window, FCITX_WINDOW_DOCK, strWindowName);

    TrayFindDock(dpy, trayWindow);
}

TrayWindow* CreateTrayWindow(FcitxLightUI *lightui) {
    TrayWindow *trayWindow = fcitx_utils_malloc0(sizeof(TrayWindow));
    trayWindow->owner = lightui;
    FcitxX11AddXEventHandler(lightui->owner, TrayEventHandler, trayWindow);
    InitTrayWindow(trayWindow);
    return trayWindow;
}

void ReleaseTrayWindow(TrayWindow *trayWindow)
{
    FcitxLightUI *lightui = trayWindow->owner;
    Display *dpy = lightui->dpy;
    if (trayWindow->window == None)
        return;
    XDestroyWindow(dpy, trayWindow->window);
    trayWindow->window = None;
}

void DrawTrayWindow(TrayWindow* trayWindow) {
    FcitxLightUI *lightui = trayWindow->owner;
    Display *dpy = lightui->dpy;
    char *name = NULL;

    if ( !lightui->bUseTrayIcon )
        return;

    if (FcitxInstanceGetCurrentStatev2(lightui->owner) == IS_ACTIVE)
        name = "tray_active";
    else
        name = "tray_inactive";

    LightUIImage* image = LoadImage(lightui, name);
    if (image && trayWindow->window != None)
    {
        DrawImage(dpy, trayWindow->window, image, 0, 0, trayWindow->size, trayWindow->size);
    }

    if (!trayWindow->bTrayMapped)
        return;

}

boolean TrayEventHandler(void *arg, XEvent* event)
{
    TrayWindow *trayWindow = arg;
    FcitxLightUI *lightui = trayWindow->owner;
    FcitxInstance* instance = lightui->owner;
    Display *dpy = lightui->dpy;
    if (!lightui->bUseTrayIcon)
        return false;
    switch (event->type) {
    case ClientMessage:
        if (event->xclient.message_type == trayWindow->atoms[ATOM_MANAGER]
                && event->xclient.data.l[1] == trayWindow->atoms[ATOM_SELECTION])
        {
            if (trayWindow->window == None)
                InitTrayWindow(trayWindow);
            TrayFindDock(dpy, trayWindow);
            return true;
        }
        break;

    case Expose:
        if (event->xexpose.window == trayWindow->window) {
            DrawTrayWindow (trayWindow);
        }
        break;
    case ConfigureNotify:
        if (trayWindow->window == event->xconfigure.window)
        {
            int size = event->xconfigure.height;
            if (size != trayWindow->size)
            {
                trayWindow->size = size;
                XSizeHints size_hints;
                size_hints.flags = PWinGravity | PBaseSize;
                size_hints.base_width = trayWindow->size;
                size_hints.base_height = trayWindow->size;
                XSetWMNormalHints(dpy, trayWindow->window, &size_hints);
            }

            DrawTrayWindow (trayWindow);
            return true;
        }
        break;
    case ButtonPress:
    {
        if (event->xbutton.window == trayWindow->window)
        {
            switch (event->xbutton.button)
            {
            case Button1:
                if (FcitxInstanceGetCurrentState(instance) == IS_CLOSED) {
                    FcitxInstanceEnableIM(instance, FcitxInstanceGetCurrentIC(instance), false);
                }
                else {
                    FcitxInstanceCloseIM(instance, FcitxInstanceGetCurrentIC(instance));
                }
                break;
            case Button3:
            {
                XlibMenu *mainMenuWindow = lightui->mainMenuWindow;
		GetMenuSize(mainMenuWindow);
		int x = event->xbutton.x_root + event->xbutton.x;
		int y = event->xbutton.y_root + event->xbutton.y;
		FcitxRect rect = GetScreenGeometry(lightui, x, y);

		if (x < rect.x1)
			x = rect.x1;

		if (y < rect.y1)
			y = rect.y1;

		if ((x + mainMenuWindow->width) > rect.x2)
			x = rect.x2 - mainMenuWindow->width;

		if ((y + mainMenuWindow->height) > rect.y2) {
			if (y > rect.y2)
				y = rect.y2 - mainMenuWindow->height;
			else
				y = y - mainMenuWindow->height;
		}

                mainMenuWindow->iPosX = x;
                mainMenuWindow->iPosY = y;

                DrawXlibMenu(mainMenuWindow);
                DisplayXlibMenu(mainMenuWindow);
            }
            break;
            }
            return true;
        }
    }
    break;
    case DestroyNotify:
        if (event->xdestroywindow.window == trayWindow->dockWindow)
        {
            trayWindow->dockWindow = None;
            trayWindow->bTrayMapped = False;
            ReleaseTrayWindow(trayWindow);
            return true;
        }
        break;

    case ReparentNotify:
        if (event->xreparent.parent == DefaultRootWindow(dpy) && event->xreparent.window == trayWindow->window)
        {
            trayWindow->bTrayMapped = False;
            ReleaseTrayWindow(trayWindow);
            return true;
        }
        break;
    }
    return false;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
