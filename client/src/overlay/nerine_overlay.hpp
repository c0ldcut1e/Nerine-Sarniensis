#ifndef NERINE_OVERLAY_HPP
#define NERINE_OVERLAY_HPP

#include "overlay/ball_prediction.hpp"
#include "overlay/boost_pad_timers.hpp"
#include "overlay/car_boost_amounts.hpp"
#include "overlay/car_hitbox.hpp"
#include "overlay/config_manager.hpp"
#include "overlay/demo_markers.hpp"
#include "overlay/match_display.hpp"
#include "overlay/nexto_bot.hpp"
#include "overlay/offscreen_indicators.hpp"
#include "overlay/radar.hpp"
#include "overlay/virtual_gamepad.hpp"

#include <gtk/gtk.h>

class NerineOverlay
{
public:
    NerineOverlay();

    NerineOverlay(const NerineOverlay &other)            = delete;
    NerineOverlay &operator=(const NerineOverlay &other) = delete;

    int Run();
    bool StartVirtualGamepad();

private:
    static gboolean OnRender(GtkGLArea *area, GdkGLContext *context, gpointer data);
    static void OnClipboardTextRead(GObject *source, GAsyncResult *result, gpointer data);
    static gboolean OnKeyPressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
    static void OnPressed(GtkGestureClick *gesture, gint nPress, gdouble x, gdouble y, gpointer data);
    static void OnReleased(GtkGestureClick *gesture, gint nPress, gdouble x, gdouble y, gpointer data);
    static void OnMotion(GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer data);
    static gboolean OnScroll(GtkEventControllerScroll *controller, gdouble dx, gdouble dy, gpointer data);
    static gboolean OnToggleFocus(gpointer data);
    static gboolean OnRenderTick(gpointer data);

    static const char *GetClipboardText(void *data);
    static void SetClipboardText(void *data, const char *text);

    void SetFocused(bool focused);

    GtkWindow *m_window;
    GtkGLArea *m_glArea;
    bool m_focused;
    bool m_rendererInitialized;
    bool m_showDemo;
    gint64 m_lastFrameTime;
    BallPrediction m_ballPrediction;
    CarBoostAmounts m_carBoostAmounts;
    CarHitbox m_carHitbox;
    BoostPadTimers m_boostPadTimers;
    Radar m_radar;
    MatchDisplay m_matchDisplay;
    OffscreenIndicators m_offscreenIndicators;
    DemoMarkers m_demoMarkers;
    VirtualGamepad m_virtualGamepad;
    NextoBot m_nextoBot;
    ConfigManager m_configManager;
};

#endif
