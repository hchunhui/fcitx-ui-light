#include "fcitx-config/fcitx-config.h"
#include "lightui.h"

static void FilterCopyUseTray(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void *value, FcitxConfigSync sync, void *filterArg);

CONFIG_BINDING_BEGIN(FcitxLightUI)
CONFIG_BINDING_REGISTER("LightUI", "MainWindowOffsetX", iMainWindowOffsetX)
CONFIG_BINDING_REGISTER("LightUI", "MainWindowOffsetY", iMainWindowOffsetY)
CONFIG_BINDING_REGISTER("LightUI", "Font", font)
#ifndef _ENABLE_PANGO
CONFIG_BINDING_REGISTER("LightUI", "FontLocale", strUserLocale)
#endif
CONFIG_BINDING_REGISTER_WITH_FILTER("LightUI", "UseTray", bUseTrayIcon_, FilterCopyUseTray)
CONFIG_BINDING_REGISTER("LightUI", "SkinType", skinType)
CONFIG_BINDING_REGISTER("LightUI", "MainWindowHideMode", hideMainWindow)
CONFIG_BINDING_REGISTER("LightUI", "VerticalList", bVerticalList)
CONFIG_BINDING_REGISTER("LightUI", "FontSize", fontSize)
CONFIG_BINDING_REGISTER("LightUI", "BackgroundColor", backcolor)
CONFIG_BINDING_REGISTER("LightUI", "BorderColor", bordercolor)

CONFIG_BINDING_REGISTER("LightUI","TipColor",fontColor[MSG_TIPS])
CONFIG_BINDING_REGISTER("LightUI","InputColor",fontColor[MSG_INPUT])
CONFIG_BINDING_REGISTER("LightUI","IndexColor",fontColor[MSG_INDEX])
CONFIG_BINDING_REGISTER("LightUI","UserPhraseColor",fontColor[MSG_USERPHR])
CONFIG_BINDING_REGISTER("LightUI","FirstCandColor",fontColor[MSG_FIRSTCAND])
CONFIG_BINDING_REGISTER("LightUI","CodeColor",fontColor[MSG_CODE])
CONFIG_BINDING_REGISTER("LightUI","OtherColor",fontColor[MSG_OTHER])
CONFIG_BINDING_REGISTER("LightUI","ActiveMenuColor",menuFontColor[MENU_ACTIVE])
CONFIG_BINDING_REGISTER("LightUI","InactiveMenuColor",menuFontColor[MENU_INACTIVE])

CONFIG_BINDING_REGISTER("LightUI", "ActiveColor", activeColor)
CONFIG_BINDING_REGISTER("LightUI", "LineColor", lineColor)
CONFIG_BINDING_REGISTER("LightUI", "CursorColor", cursorColor)

CONFIG_BINDING_END()

void FilterCopyUseTray(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void *value, FcitxConfigSync sync, void *filterArg) {
    static boolean firstRunOnUseTray = true;
    FcitxLightUI *lightui = (FcitxLightUI*) config;
    boolean *b = (boolean*)value;
    if (sync == Raw2Value && b)
    {
        if (firstRunOnUseTray)
            lightui->bUseTrayIcon = *b;
        firstRunOnUseTray = false;
    }
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
