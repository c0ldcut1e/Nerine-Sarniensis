#include "nerine_overlay.hpp"

#include <epoxy/gl.h>
#include <glib-unix.h>
#include <gtk4-layer-shell.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>

static gboolean OnFrameTick(GtkWidget *widget, GdkFrameClock *frameClock, gpointer data)
{
    (void) frameClock;
    (void) data;

    gtk_gl_area_queue_render(GTK_GL_AREA(widget));
    return G_SOURCE_CONTINUE;
}

NerineOverlay::NerineOverlay()
{
    m_window              = NULL;
    m_glArea              = NULL;
    m_focused             = false;
    m_rendererInitialized = false;
    m_showDemo            = true;
    m_lastFrameTime       = 0;
    m_nextoBot.Initialize(m_virtualGamepad);
    m_configManager.Initialize(m_ballPrediction, m_carBoostAmounts, m_carHitbox, m_boostPadTimers, m_radar, m_matchDisplay, m_offscreenIndicators,
                               m_demoMarkers);
}

int NerineOverlay::Run()
{
    GtkCssProvider *cssProvider;
    GtkEventController *keyController;
    GtkGesture *clickGesture;
    GtkEventController *motionController;
    GtkEventController *scrollController;
    GMainLoop *mainLoop;

    gdk_set_allowed_backends("wayland");
    g_setenv("GSK_RENDERER", "gl", FALSE);
    gtk_init();
    if (!gtk_layer_is_supported())
    {
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename        = NULL;
    ImGui::GetIO().ClipboardUserData  = this;
    ImGui::GetIO().GetClipboardTextFn = GetClipboardText;
    ImGui::GetIO().SetClipboardTextFn = SetClipboardText;
    ImGui::StyleColorsDark();

    m_window = GTK_WINDOW(gtk_window_new());
    gtk_window_set_decorated(m_window, FALSE);
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(cssProvider, "window, glarea { background-color: transparent; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER + 1);
    g_object_unref(cssProvider);
    gtk_layer_init_for_window(m_window);
    gtk_layer_set_namespace(m_window, "sarniensis");
    gtk_layer_set_layer(m_window, GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_anchor(m_window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(m_window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_anchor(m_window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(m_window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_exclusive_zone(m_window, -1);
    gtk_layer_set_keyboard_mode(m_window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);

    m_glArea = GTK_GL_AREA(gtk_gl_area_new());
    gtk_gl_area_set_required_version(m_glArea, 3, 3);
    gtk_gl_area_set_auto_render(m_glArea, FALSE);
    gtk_gl_area_set_has_depth_buffer(m_glArea, FALSE);
    gtk_gl_area_set_has_stencil_buffer(m_glArea, FALSE);
    g_signal_connect(m_glArea, "render", G_CALLBACK(OnRender), this);

    keyController = gtk_event_controller_key_new();
    g_signal_connect(keyController, "key-pressed", G_CALLBACK(OnKeyPressed), this);
    gtk_widget_add_controller(GTK_WIDGET(m_window), keyController);

    clickGesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(clickGesture), 0);
    g_signal_connect(clickGesture, "pressed", G_CALLBACK(OnPressed), this);
    g_signal_connect(clickGesture, "released", G_CALLBACK(OnReleased), this);
    gtk_widget_add_controller(GTK_WIDGET(m_window), GTK_EVENT_CONTROLLER(clickGesture));

    motionController = gtk_event_controller_motion_new();
    g_signal_connect(motionController, "motion", G_CALLBACK(OnMotion), this);
    gtk_widget_add_controller(GTK_WIDGET(m_window), motionController);

    scrollController = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
    g_signal_connect(scrollController, "scroll", G_CALLBACK(OnScroll), this);
    gtk_widget_add_controller(GTK_WIDGET(m_window), scrollController);

    mainLoop = g_main_loop_new(NULL, FALSE);
    g_signal_connect_swapped(m_window, "destroy", G_CALLBACK(g_main_loop_quit), mainLoop);
    g_unix_signal_add(SIGUSR1, OnToggleFocus, this);
    gtk_window_set_child(m_window, GTK_WIDGET(m_glArea));
    gtk_widget_add_tick_callback(GTK_WIDGET(m_glArea), OnFrameTick, NULL, NULL);
    gtk_window_present(m_window);
    SetFocused(false);

    g_main_loop_run(mainLoop);
    g_main_loop_unref(mainLoop);

    return 0;
}

bool NerineOverlay::StartVirtualGamepad()
{
    return m_virtualGamepad.AutoStart();
}

const char *NerineOverlay::GetClipboardText(void *data)
{
    NerineOverlay *overlay  = (NerineOverlay *) data;
    GdkClipboard *clipboard = gdk_display_get_clipboard(gtk_widget_get_display(GTK_WIDGET(overlay->m_window)));
    gdk_clipboard_read_text_async(clipboard, NULL, OnClipboardTextRead, overlay);
    return "";
}

void NerineOverlay::SetClipboardText(void *data, const char *text)
{
    NerineOverlay *overlay  = (NerineOverlay *) data;
    GdkClipboard *clipboard = gdk_display_get_clipboard(gtk_widget_get_display(GTK_WIDGET(overlay->m_window)));
    gdk_clipboard_set_text(clipboard, text);
}

void NerineOverlay::OnClipboardTextRead(GObject *source, GAsyncResult *result, gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;
    char *text             = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), result, NULL);
    if (!overlay->m_focused || !text)
    {
        g_free(text);
        return;
    }

    ImGui::GetIO().AddInputCharactersUTF8(text);
    g_free(text);
}

gboolean NerineOverlay::OnRender(GtkGLArea *area, GdkGLContext *context, gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;
    gint64 frameTime;

    gdk_gl_context_make_current(context);
    if (!overlay->m_rendererInitialized)
    {
        ImGui_ImplOpenGL3_Init("#version 330");
        overlay->m_rendererInitialized = true;
        overlay->m_lastFrameTime       = g_get_monotonic_time();
    }

    ImGuiIO &io = ImGui::GetIO();

    frameTime                = g_get_monotonic_time();
    io.DeltaTime             = (float) (frameTime - overlay->m_lastFrameTime) / 1000000;
    overlay->m_lastFrameTime = frameTime;
    io.DisplaySize           = ImVec2(gtk_widget_get_width(GTK_WIDGET(area)), gtk_widget_get_height(GTK_WIDGET(area)));
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (overlay->m_focused)
    {
        ImGui::Begin("Sarniensis", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PushID("config_manager");
        overlay->m_configManager.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("ball_prediction");
        overlay->m_ballPrediction.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("car_boost_amounts");
        overlay->m_carBoostAmounts.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("car_hitbox");
        overlay->m_carHitbox.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("boost_pad_timers");
        overlay->m_boostPadTimers.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("radar");
        overlay->m_radar.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("match_display");
        overlay->m_matchDisplay.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("offscreen_indicators");
        overlay->m_offscreenIndicators.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("demo_markers");
        overlay->m_demoMarkers.DrawMenu();
        ImGui::PopID();

        ImGui::PushID("bot");
        overlay->m_nextoBot.DrawMenu();
        ImGui::PopID();

        ImGui::End();
    }

    overlay->m_ballPrediction.Render(io.DisplaySize.x, io.DisplaySize.y);
    overlay->m_carBoostAmounts.Render(io.DisplaySize.x, io.DisplaySize.y);
    overlay->m_carHitbox.Render(io.DisplaySize.x, io.DisplaySize.y);
    overlay->m_boostPadTimers.Render(io.DisplaySize.x, io.DisplaySize.y);
    overlay->m_radar.Render(io.DisplaySize.x, io.DisplaySize.y);
    overlay->m_matchDisplay.Render(io.DisplaySize.x, io.DisplaySize.y);
    overlay->m_offscreenIndicators.Render(io.DisplaySize.x, io.DisplaySize.y);
    overlay->m_demoMarkers.Render(io.DisplaySize.x, io.DisplaySize.y);
    overlay->m_nextoBot.Tick();

    ImGui::Render();

    glViewport(0, 0, gtk_widget_get_width(GTK_WIDGET(area)), gtk_widget_get_height(GTK_WIDGET(area)));
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return TRUE;
}

gboolean NerineOverlay::OnKeyPressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;
    bool ctrlHeld          = (state & GDK_CONTROL_MASK) != 0;
    guint32 codepoint;

    (void) controller;
    (void) keycode;
    if (keyval == GDK_KEY_Shift_R)
    {
        overlay->SetFocused(!overlay->m_focused);
        return TRUE;
    }

    if (keyval == GDK_KEY_Escape)
    {
        gtk_window_destroy(overlay->m_window);
        return TRUE;
    }

    if (!overlay->m_focused)
    {
        return FALSE;
    }

    if (keyval == GDK_KEY_BackSpace)
    {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Backspace, true);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Backspace, false);
        return TRUE;
    }

    ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, ctrlHeld);
    if (ctrlHeld && (keyval == GDK_KEY_c || keyval == GDK_KEY_C))
    {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_C, true);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_C, false);
        return TRUE;
    }

    if (ctrlHeld && (keyval == GDK_KEY_v || keyval == GDK_KEY_V))
    {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_V, true);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_V, false);
        return TRUE;
    }

    if (ctrlHeld)
    {
        return TRUE;
    }

    codepoint = gdk_keyval_to_unicode(keyval);
    if (codepoint != 0)
    {
        ImGui::GetIO().AddInputCharacter(codepoint);
    }

    return TRUE;
}

void NerineOverlay::OnPressed(GtkGestureClick *gesture, gint nPress, gdouble x, gdouble y, gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;

    (void) nPress;
    if (!overlay->m_focused)
    {
        return;
    }

    ImGui::GetIO().AddMousePosEvent(x, y);
    ImGui::GetIO().AddMouseButtonEvent((int) gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)) - 1, true);
}

void NerineOverlay::OnReleased(GtkGestureClick *gesture, gint nPress, gdouble x, gdouble y, gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;

    (void) nPress;
    if (!overlay->m_focused)
    {
        return;
    }

    ImGui::GetIO().AddMousePosEvent(x, y);
    ImGui::GetIO().AddMouseButtonEvent((int) gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)) - 1, false);
}

void NerineOverlay::OnMotion(GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;

    (void) controller;
    if (!overlay->m_focused)
    {
        return;
    }

    ImGui::GetIO().AddMousePosEvent(x, y);
}

gboolean NerineOverlay::OnScroll(GtkEventControllerScroll *controller, gdouble dx, gdouble dy, gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;

    (void) controller;
    if (!overlay->m_focused)
    {
        return FALSE;
    }

    ImGui::GetIO().AddMouseWheelEvent((float) -dx, (float) -dy);
    return TRUE;
}

gboolean NerineOverlay::OnToggleFocus(gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;

    overlay->SetFocused(!overlay->m_focused);
    return G_SOURCE_CONTINUE;
}

gboolean NerineOverlay::OnRenderTick(gpointer data)
{
    NerineOverlay *overlay = (NerineOverlay *) data;

    gtk_gl_area_queue_render(overlay->m_glArea);
    return G_SOURCE_CONTINUE;
}

void NerineOverlay::SetFocused(bool focused)
{
    GdkSurface *surface;
    cairo_region_t *region;
    cairo_rectangle_int_t rect;

    m_focused = focused;
    ImGui::GetIO().AddFocusEvent(m_focused);
    surface = gtk_native_get_surface(GTK_NATIVE(m_window));
    if (m_focused)
    {
        rect.x      = 0;
        rect.y      = 0;
        rect.width  = G_MAXINT16;
        rect.height = G_MAXINT16;
        region      = cairo_region_create_rectangle(&rect);
        gdk_surface_set_input_region(surface, region);
        cairo_region_destroy(region);
        gtk_layer_set_keyboard_mode(m_window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
        return;
    }

    region = cairo_region_create();
    gdk_surface_set_input_region(surface, region);
    cairo_region_destroy(region);
    gtk_layer_set_keyboard_mode(m_window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
}
