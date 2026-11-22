#include "lvgl_ui.h"

#include "lvgl.h"
#include "lvgl_port.h"
#include "stm32h7xx_hal.h"

#include <stdint.h>
#include <string.h>

#define UI_WIDTH                 800
#define UI_TOP_BAR_HEIGHT        54
#define UI_PAGE_LOCK             0U
#define UI_PAGE_HOME             1U
#define UI_PAGE_APP              2U
#define UI_APP_COUNT             4U

#define UI_BG                    0x0B1220
#define UI_PANEL                 0x17233A
#define UI_PANEL_ALT             0x20304A
#define UI_TEXT                  0xF4F7FB
#define UI_MUTED                 0x9AAAC1
#define UI_ACCENT                0x55D6BE
#define UI_FOCUS                 0x2B536B

#define UI_PASSWORD              "281815"
#define UI_PASSWORD_LENGTH       6U

typedef struct
{
    const char *icon;
    const char *name;
    const char *subtitle;
    uint32_t color;
    uint8_t index;
} UiAppDefinition;

static const UiAppDefinition g_apps[UI_APP_COUNT] =
{
    {LV_SYMBOL_CALL,     "Phone",    "Contacts", 0x3A86FF, 0U},
    {LV_SYMBOL_ENVELOPE, "Messages", "2 unread", 0x06D6A0, 1U},
    {LV_SYMBOL_IMAGE,    "Gallery",  "Memories", 0xF4A261, 2U},
    {LV_SYMBOL_SETTINGS, "Settings", "System",   0xA78BFA, 3U}
};

static uint8_t g_page;
static uint8_t g_home_index;
static uint8_t g_password_length;
static char g_password_input[UI_PASSWORD_LENGTH + 1U];

static lv_obj_t *g_clock_label;
static lv_obj_t *g_selected_label;
static lv_obj_t *g_password_label;
static lv_obj_t *g_lock_feedback;
static lv_obj_t *g_home_cards[UI_APP_COUNT];
static lv_timer_t *g_clock_timer;
static uint8_t g_page_change_pending;

static void show_home(void);
static void show_app(const UiAppDefinition *app);
static void request_home_page(void);
static void request_app_page(const UiAppDefinition *app);

static lv_obj_t *make_label(lv_obj_t *parent,
                            const char *text,
                            const lv_font_t *font,
                            lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    return label;
}

static void set_page_background(void)
{
    lv_obj_t *screen = lv_scr_act();

    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
}

static void stop_page_timer(void)
{
    if (g_clock_timer != NULL)
    {
        lv_timer_del(g_clock_timer);
        g_clock_timer = NULL;
    }
}

static void clear_page(void)
{
    uint32_t index;

    stop_page_timer();
    g_clock_label = NULL;
    g_selected_label = NULL;
    g_password_label = NULL;
    g_lock_feedback = NULL;

    for (index = 0U; index < UI_APP_COUNT; ++index)
    {
        g_home_cards[index] = NULL;
    }

    lv_group_remove_all_objs(lv_group_get_default());
    lv_obj_clean(lv_scr_act());
    set_page_background();
}

static void add_status_bar(lv_obj_t *parent, const char *title)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_t *left = make_label(bar, title, &lv_font_montserrat_14,
                                lv_color_hex(UI_TEXT));
    lv_obj_t *right = make_label(bar,
                                 LV_SYMBOL_WIFI "  " LV_SYMBOL_BATTERY_3 "  78%",
                                 &lv_font_montserrat_14,
                                 lv_color_hex(UI_MUTED));

    lv_obj_set_size(bar, UI_WIDTH, UI_TOP_BAR_HEIGHT);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(UI_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(bar, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_right(bar, 24, LV_PART_MAIN);
    lv_obj_align(left, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
}

static lv_obj_t *make_action_button(lv_obj_t *parent,
                                    const char *text,
                                    int32_t x,
                                    int32_t y,
                                    int32_t width,
                                    int32_t height,
                                    uint32_t color,
                                    lv_event_cb_t callback,
                                    const void *user_data)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_t *label = make_label(button, text, &lv_font_montserrat_14,
                                 lv_color_hex(UI_TEXT));

    lv_obj_set_size(button, width, height);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_color_hex(UI_FOCUS),
                              LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(button, 3, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(button, lv_color_hex(UI_ACCENT),
                                   LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 18, LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, (void *)user_data);
    lv_obj_add_event_cb(button, callback, LV_EVENT_KEY, (void *)user_data);
    lv_group_add_obj(lv_group_get_default(), button);
    return button;
}

static void update_lock_clock(lv_timer_t *timer)
{
    uint32_t seconds;
    uint32_t minutes;
    uint32_t hour;
    uint32_t minute;

    (void)timer;
    if (g_clock_label == NULL)
    {
        return;
    }

    seconds = HAL_GetTick() / 1000U;
    minutes = (12U * 60U + 48U + seconds / 60U) % (24U * 60U);
    hour = minutes / 60U;
    minute = minutes % 60U;
    lv_label_set_text_fmt(g_clock_label, "%02lu:%02lu",
                          (unsigned long)hour, (unsigned long)minute);
}

static void update_password_view(const char *feedback)
{
    char mask[UI_PASSWORD_LENGTH + 1U];
    uint32_t index;

    for (index = 0U; index < UI_PASSWORD_LENGTH; ++index)
    {
        mask[index] = (index < g_password_length) ? '*' : '-';
    }
    mask[UI_PASSWORD_LENGTH] = '\0';

    if (g_password_label != NULL)
    {
        lv_label_set_text(g_password_label, mask);
    }
    if ((g_lock_feedback != NULL) && (feedback != NULL))
    {
        lv_label_set_text(g_lock_feedback, feedback);
    }
}

static void submit_password(void)
{
    if (g_page_change_pending != 0U)
    {
        return;
    }

    if ((g_password_length == UI_PASSWORD_LENGTH) &&
        (strcmp(g_password_input, UI_PASSWORD) == 0))
    {
        update_password_view("UNLOCKING");
        request_home_page();
    }
    else
    {
        g_password_length = 0U;
        g_password_input[0] = '\0';
        update_password_view("INVALID PASSWORD");
    }
}

static void process_lock_key(uint32_t key)
{
    if ((key >= (uint32_t)'0') && (key <= (uint32_t)'9'))
    {
        if (g_password_length < UI_PASSWORD_LENGTH)
        {
            g_password_input[g_password_length] = (char)key;
            ++g_password_length;
            g_password_input[g_password_length] = '\0';
            update_password_view("");
        }
    }
    else if (key == LV_KEY_BACKSPACE)
    {
        if (g_password_length > 0U)
        {
            --g_password_length;
            g_password_input[g_password_length] = '\0';
        }
        update_password_view("");
    }
    else if (key == LV_KEY_ESC)
    {
        g_password_length = 0U;
        g_password_input[0] = '\0';
        update_password_view("");
    }
    /* Confirm only from LV_EVENT_CLICKED. LVGL generates that event after
       the ENTER key cycle finishes; changing pages from LV_EVENT_KEY would
       let the original ENTER action leak into the newly created home page. */
}

static void handle_navigation_key(lv_event_t *event)
{
    uint32_t key = lv_event_get_key(event);

    if (g_page == UI_PAGE_HOME)
    {
        /* Numeric navigation remains available after keeping the raw
           symbols for lock-screen password entry. */
        if ((key == (uint32_t)'2') || (key == (uint32_t)'4'))
        {
            lv_group_focus_prev(lv_group_get_default());
        }
        else if ((key == (uint32_t)'6') || (key == (uint32_t)'8'))
        {
            lv_group_focus_next(lv_group_get_default());
        }
        return;
    }

    /* Back leaves an application, but never locks an already unlocked UI. */
    if ((g_page == UI_PAGE_APP) &&
        ((key == LV_KEY_BACKSPACE) || (key == LV_KEY_ESC)))
    {
        request_home_page();
    }
    else if ((g_page == UI_PAGE_APP) && (key == LV_KEY_HOME))
    {
        request_home_page();
    }
}

static void lock_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if (g_page != UI_PAGE_LOCK)
    {
        return;
    }

    if (code == LV_EVENT_KEY)
    {
        process_lock_key(lv_event_get_key(event));
    }
    else if (code == LV_EVENT_CLICKED)
    {
        submit_password();
    }
}

static void app_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    const UiAppDefinition *app =
        (const UiAppDefinition *)lv_event_get_user_data(event);

    if ((code == LV_EVENT_CLICKED) &&
        (g_page == UI_PAGE_HOME) && (app != NULL))
    {
        g_home_index = app->index;
        request_app_page(app);
    }
    else if (code == LV_EVENT_KEY)
    {
        handle_navigation_key(event);
    }
}

static void back_event(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) &&
        (g_page == UI_PAGE_APP))
    {
        request_home_page();
    }
    else if (lv_event_get_code(event) == LV_EVENT_KEY)
    {
        handle_navigation_key(event);
    }
}

static void app_focus_event(lv_event_t *event)
{
    const UiAppDefinition *app =
        (const UiAppDefinition *)lv_event_get_user_data(event);

    if ((lv_event_get_code(event) == LV_EVENT_FOCUSED) &&
        (g_page == UI_PAGE_HOME) && (g_selected_label != NULL) && (app != NULL))
    {
        g_home_index = app->index;
        lv_label_set_text_fmt(g_selected_label, "SELECTED: %s  %s",
                              app->icon, app->name);
    }
}

static void add_lock_page(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_t *mark;
    lv_obj_t *hint;
    lv_obj_t *unlock;

    add_status_bar(screen, LV_SYMBOL_KEYBOARD "  LOCK SCREEN");

    g_clock_label = make_label(screen, "12:48", &lv_font_montserrat_28,
                               lv_color_hex(UI_TEXT));
    lv_obj_align(g_clock_label, LV_ALIGN_TOP_MID, 0, 82);

    mark = make_label(screen, LV_SYMBOL_KEYBOARD, &lv_font_montserrat_28,
                      lv_color_hex(UI_ACCENT));
    lv_obj_align(mark, LV_ALIGN_CENTER, 0, -70);

    hint = make_label(screen, "ENTER PASSWORD", &lv_font_montserrat_14,
                      lv_color_hex(UI_MUTED));
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, -28);

    g_password_label = make_label(screen, "------", &lv_font_montserrat_28,
                                  lv_color_hex(UI_TEXT));
    lv_obj_align(g_password_label, LV_ALIGN_CENTER, 0, 12);

    g_lock_feedback = make_label(screen, "6 DIGITS REQUIRED",
                                 &lv_font_montserrat_14,
                                 lv_color_hex(UI_ACCENT));
    lv_obj_align(g_lock_feedback, LV_ALIGN_CENTER, 0, 58);

    unlock = make_action_button(screen, LV_SYMBOL_OK "  UNLOCK",
                                230, 350, 340, 58, UI_PANEL_ALT,
                                lock_event, NULL);
    lv_obj_set_style_radius(unlock, 29, LV_PART_MAIN);
    lv_group_focus_obj(unlock);

    hint = make_label(screen, "0-9 ENTER    B DELETE    C CLEAR",
                      &lv_font_montserrat_14, lv_color_hex(UI_MUTED));
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -22);

    g_clock_timer = lv_timer_create(update_lock_clock, 1000U, NULL);
    update_lock_clock(g_clock_timer);
}

static void add_home_page(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_t *title;
    lv_obj_t *footer;
    uint32_t index;

    add_status_bar(screen, LV_SYMBOL_HOME "  HOME");

    title = make_label(screen, "APP LAUNCHER", &lv_font_montserrat_28,
                       lv_color_hex(UI_TEXT));
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 42, 82);

    g_selected_label = make_label(screen, "SELECTED",
                                  &lv_font_montserrat_14,
                                  lv_color_hex(UI_ACCENT));
    lv_obj_align(g_selected_label, LV_ALIGN_TOP_LEFT, 44, 126);

    for (index = 0U; index < UI_APP_COUNT; ++index)
    {
        int32_t x = 42 + (int32_t)index * 185;
        lv_obj_t *card = make_action_button(screen, "", x, 174, 160, 178,
                                            g_apps[index].color,
                                            app_event, &g_apps[index]);
        lv_obj_t *icon = make_label(card, g_apps[index].icon,
                                    &lv_font_montserrat_28,
                                    lv_color_hex(UI_TEXT));
        lv_obj_t *name = make_label(card, g_apps[index].name,
                                    &lv_font_montserrat_14,
                                    lv_color_hex(UI_TEXT));

        g_home_cards[index] = card;
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 30);
        lv_obj_align(name, LV_ALIGN_BOTTOM_MID, 0, -24);
        lv_obj_add_event_cb(card, app_focus_event, LV_EVENT_FOCUSED,
                            (void *)&g_apps[index]);
    }

    footer = make_label(screen,
                        "A / ENTER OPEN    2/4/6/8 SELECT    B/C BACK",
                        &lv_font_montserrat_14, lv_color_hex(UI_MUTED));
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -20);
}

static void add_back_button(lv_obj_t *screen)
{
    make_action_button(screen, LV_SYMBOL_LEFT "  BACK",
                       24, 74, 120, 48, UI_PANEL_ALT,
                       back_event, NULL);
}

static void add_app_content(lv_obj_t *screen, const UiAppDefinition *app)
{
    lv_obj_t *icon = make_label(screen, app->icon, &lv_font_montserrat_28,
                                lv_color_hex(UI_ACCENT));
    lv_obj_t *title = make_label(screen, app->name, &lv_font_montserrat_28,
                                 lv_color_hex(UI_TEXT));

    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 178, 80);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 228, 84);

    if (app->index == 0U)
    {
        make_label(screen, "RECENT CONTACTS", &lv_font_montserrat_14,
                   lv_color_hex(UI_MUTED));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 132);
        make_label(screen, "Alice        138 0013 2048",
                   &lv_font_montserrat_14, lv_color_hex(UI_TEXT));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 174);
        make_label(screen, "Bob          138 0028 1188",
                   &lv_font_montserrat_14, lv_color_hex(UI_TEXT));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 210);
        make_action_button(screen, LV_SYMBOL_CALL "  CALL",
                           178, 280, 180, 62, 0x247A6C,
                           app_event, NULL);
    }
    else if (app->index == 1U)
    {
        make_label(screen, LV_SYMBOL_BELL "  Alice: See you at 18:00",
                   &lv_font_montserrat_14, lv_color_hex(UI_TEXT));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 150);
        make_label(screen, LV_SYMBOL_ENVELOPE "  System: UI is ready",
                   &lv_font_montserrat_14, lv_color_hex(UI_TEXT));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 198);
        make_action_button(screen, LV_SYMBOL_ENVELOPE "  OPEN THREAD",
                           178, 280, 220, 62, 0x247A6C,
                           app_event, NULL);
    }
    else if (app->index == 2U)
    {
        uint32_t colors[] = {0x2D6A8A, 0x6A4C93, 0xE76F51, 0x2A9D8F};
        uint32_t index;
        for (index = 0U; index < 4U; ++index)
        {
            lv_obj_t *tile = lv_obj_create(screen);
            lv_obj_set_size(tile, 128, 108);
            lv_obj_set_pos(tile, 178 + (int32_t)(index % 2U) * 148,
                           140 + (int32_t)(index / 2U) * 126);
            lv_obj_set_style_bg_color(tile, lv_color_hex(colors[index]),
                                      LV_PART_MAIN);
            lv_obj_set_style_border_width(tile, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(tile, 16, LV_PART_MAIN);
        }
        make_label(screen, LV_SYMBOL_IMAGE "  4 ITEMS",
                   &lv_font_montserrat_14, lv_color_hex(UI_MUTED));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 408);
    }
    else
    {
        make_label(screen, LV_SYMBOL_WIFI "  WiFi                         ON",
                   &lv_font_montserrat_14, lv_color_hex(UI_TEXT));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 150);
        make_label(screen, LV_SYMBOL_VOLUME_MAX "  Brightness                  78%",
                   &lv_font_montserrat_14, lv_color_hex(UI_TEXT));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 198);
        make_label(screen, LV_SYMBOL_SETTINGS "  Display                     RGB565",
                   &lv_font_montserrat_14, lv_color_hex(UI_TEXT));
        lv_obj_align(lv_obj_get_child(screen, lv_obj_get_child_cnt(screen) - 1),
                     LV_ALIGN_TOP_LEFT, 178, 246);
    }
}

static void show_home(void)
{
    g_page = UI_PAGE_HOME;
    clear_page();
    add_home_page();

    if ((g_home_index < UI_APP_COUNT) &&
        (g_home_cards[g_home_index] != NULL))
    {
        lv_group_focus_obj(g_home_cards[g_home_index]);
    }
}

static void show_app(const UiAppDefinition *app)
{
    lv_obj_t *screen;

    if (app == NULL)
    {
        return;
    }

    g_page = UI_PAGE_APP;
    clear_page();
    screen = lv_scr_act();
    add_status_bar(screen, app->name);
    add_back_button(screen);
    add_app_content(screen, app);
}

static void home_page_async(void *user_data)
{
    (void)user_data;
    g_page_change_pending = 0U;

    if (g_page != UI_PAGE_HOME)
    {
        show_home();
    }
}

static void app_page_async(void *user_data)
{
    const UiAppDefinition *app = (const UiAppDefinition *)user_data;

    g_page_change_pending = 0U;
    if ((g_page == UI_PAGE_HOME) && (app != NULL))
    {
        show_app(app);
    }
}

static void request_home_page(void)
{
    if (g_page_change_pending == 0U)
    {
        g_page_change_pending = 1U;
        if (lv_async_call(home_page_async, NULL) != LV_RES_OK)
        {
            g_page_change_pending = 0U;
        }
    }
}

static void request_app_page(const UiAppDefinition *app)
{
    if ((app != NULL) && (g_page_change_pending == 0U))
    {
        g_page_change_pending = 1U;
        if (lv_async_call(app_page_async, (void *)app) != LV_RES_OK)
        {
            g_page_change_pending = 0U;
        }
    }
}

void LVGL_UI_Init(void)
{
    g_page = UI_PAGE_LOCK;
    g_home_index = 0U;
    g_page_change_pending = 0U;
    g_password_length = 0U;
    g_password_input[0] = '\0';
    clear_page();
    add_lock_page();
    lv_refr_now(NULL);
}
