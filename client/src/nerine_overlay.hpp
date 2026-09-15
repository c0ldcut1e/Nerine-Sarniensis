#ifndef NERINE_OVERLAY_HPP
#define NERINE_OVERLAY_HPP

#include "ball_prediction.hpp"
#include "boost_pad_timers.hpp"
#include "car_boost_amounts.hpp"
#include "car_hitbox.hpp"
#include "config_manager.hpp"

#include <gtk/gtk.h>

class NerineOverlay
{
public:
    NerineOverlay();

    NerineOverlay(const NerineOverlay &other)            = delete;
    NerineOverlay &operator=(const NerineOverlay &other) = delete;

    int Run();

private:
    static gboolean OnRender(GtkGLArea *area, GdkGLContext *context, gpointer data);
    static gboolean OnKeyPressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
    static void OnPressed(GtkGestureClick *gesture, gint nPress, gdouble x, gdouble y, gpointer data);
    static void OnReleased(GtkGestureClick *gesture, gint nPress, gdouble x, gdouble y, gpointer data);
    static void OnMotion(GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer data);
    static gboolean OnScroll(GtkEventControllerScroll *controller, gdouble dx, gdouble dy, gpointer data);
    static gboolean OnToggleFocus(gpointer data);
    static gboolean OnRenderTick(gpointer data);

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
    ConfigManager m_configManager;
};

#endif
