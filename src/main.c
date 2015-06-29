/**
 * Enigma watch face variant with alien text.
 * (Mod of Enigma Plus)
 *
 * Credits:
 * > Klingon font (http://www.dafont.com/klingon-font.font) by kaiserzharkhan
 * Notes:
 * > Digit size: 20x29 px
 *   > 36x48 max
 *
 * Differences from Enigma-Plus
 * > Alien (Klingon) enigma text, with date & time decoded/revealed a while later.
 * > Removed year display, so as to show more alien text.
 *
 * Changes:
 * > Coloured time bar:
 *   > red for AM, blue for PM.
 * > Added rows for: DDMM, 'day of week'.
 * > 'day of week' mini-bar (top-left) doubles as 'bluetooth status' indicator.
 *   > Coloured: connected; dark: disconnected.
 * > Added battery-level indicator: it's the 3rd column.
 *   > Top down orange bar: charging; bottom-up bar: charge remaining.
 **/
#include <pebble.h>

#define TXT_LAYER_DAY_OF_WEEK 4
#define BATTERY_LVL_COL 2

//Interval (in ms) to decode/reveal date & time
#define INTERVAL_REVEAL_INITIAL 1000
#define INTERVAL_REVEAL_NEXT 250

Window *my_window;
Layer *time_box_layer, *battery_level_layer;
TextLayer *text_layer[5], *m_ptxtDate, *m_ptxtTime;
PropertyAnimation *animations[5] = {0};
GRect to_rect[6];
int center;

char digits[4][32], digitsBkup[4][32], digitsDate[8] = {32}, digitsTime[8] = {32};
int offsets[4][10];
int order[4][10];
bool m_bIsAm = false;
static char s_dayOfWeek_buffer[4];

static TextLayer *s_font_layer;
static GFont s_custom_font;
static AppTimer *m_sptimer1;
static int m_nRevealStep = 0;
int digitTime1, digitTime2, digitTime3, digitTime4,
    digitDate1, digitDate2, digitDate3, digitDate4;

//forward declaration
void animation_stopped(struct Animation *animation);


//
//Bluetooth stuff
//
bool m_bBtConnected = false;
static void bt_handler(bool connected) {
    m_bBtConnected = connected;
    TextLayer *layer = text_layer[TXT_LAYER_DAY_OF_WEEK];
    if (layer)
    {
        text_layer_set_background_color(layer, m_bBtConnected? (m_bIsAm? GColorRed: GColorBlue): GColorBlack);
    }
}

//
// Battery stuff
//
BatteryChargeState m_sBattState = {
    0,      //charge_percent
    false,  //is_charging
    false   //is_plugged
};
void update_battery_level_display()
{
    if (!battery_level_layer) return;
    GRect bounds = layer_get_frame(battery_level_layer);
    int third = (bounds.size.h - 4) / 3; //height of screen
    //int y = third * 33 / 100; //DEBUG
    int y = third * m_sBattState.charge_percent / 100;
    to_rect[5] = layer_get_frame(battery_level_layer);
    to_rect[4] = layer_get_frame(battery_level_layer);
    if (m_sBattState.is_charging)
    {
        to_rect[5].origin.y = -(2 + 3*third); //from
        to_rect[4].origin.y = -(2 + 3*third) + y;
        //Note: charge_percent seems incorrect when charging! (e.g. shows 10% instead of 60% actual capacity).
    }
    else
    {
        to_rect[5].origin.y = -2; //from
        to_rect[4].origin.y = -2 - y;
    }

    if (animations[4])
    {
        property_animation_destroy(animations[4]);
        animations[4] = NULL;
    }

    animations[4] = property_animation_create_layer_frame(battery_level_layer,
        &to_rect[5], &to_rect[4]);
    animation_set_duration((Animation*) animations[4], 1500);
    animation_schedule((Animation*) animations[4]);
}

static void battery_handler(BatteryChargeState new_state) {
    m_sBattState = new_state;
    if (battery_level_layer)
    {
        //layer_mark_dirty(battery_level_layer);
        update_battery_level_display();
    }
}

void battery_level_layer_update_proc(Layer *layer, GContext *ctx) {
    if (!my_window) return;
    GRect bounds = layer_get_bounds(layer);
    bounds = layer_get_frame(layer);

    GRect r = GRect(0, 0, bounds.size.w, bounds.size.h);
    graphics_context_set_stroke_color(ctx, GColorOrange);
    graphics_draw_rect(ctx, r);
    graphics_draw_rect(ctx, grect_crop(r, 1));

    //battery level bar is coloured in the middle third (with 2 pixels top & bottom): ||   ===   ||
    int third = (bounds.size.h - 4) / 3; //height of screen
//    r = GRect(0, 0.5 * third + 2, bounds.size.w, 1.5 * third + 2);
    r = GRect(0, third + 2, bounds.size.w, 2 * third + 2);
    graphics_context_set_fill_color(ctx, GColorOrange);
    graphics_fill_rect(ctx, r, 0, GCornerNone);
}

//
// Enigma display stuff
//
void time_box_layer_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    GPoint center = grect_center_point(&bounds);
    GRect r = GRect(bounds.origin.x, center.y - 24, bounds.size.w, 48);

    graphics_context_set_stroke_color(ctx, m_bIsAm? GColorRed: GColorBlue);
    graphics_context_set_fill_color(ctx, m_bIsAm? GColorRed: GColorBlue);
//    graphics_draw_rect(ctx, r);
    graphics_fill_rect(ctx, r, 0, GCornerNone);
//    graphics_draw_rect(ctx, grect_crop(r, 1));
}

//overwrites 'digits' with our desired 'ddmm' or 'year' digit (num)
void change_digit(int col, int ref, int num, int off)
{
    if (num <= 10)
    {
        num += '0';
    }
    int p = (order[col][ref] * 2) + 4 + off;
    digits[col][p] = num;
    if (p < 8) {
    p += 20;
    digits[col][p] = num;
    } else if(p >= 20) {
        p -= 20;
        digits[col][p] = num;
    }
}

/**
 * Change the digit in given column, and animate the column by rolling
 * the newly selected digit into position (3rd row).
 **/
void set_digit(int col, int num, bool toCallback) {
    Layer *layer = text_layer_get_layer(text_layer[col]);

    to_rect[col] = layer_get_frame(layer);
    to_rect[col].origin.y = (offsets[col][num] * -42) + center - 28;

    if (animations[col])
    {
        property_animation_destroy(animations[col]);
        animations[col] = NULL;
    }

    animations[col] = property_animation_create_layer_frame(layer, NULL, &to_rect[col]);
    if (toCallback)
    {
        // You may set handlers to listen for the start and stop events
        animation_set_handlers((Animation*) animations[col], (AnimationHandlers) {
            .started = NULL,
            .stopped = (AnimationStoppedHandler) animation_stopped,
        }, NULL);
    }
    animation_set_duration((Animation*) animations[col], 1000);
    animation_schedule((Animation*) animations[col]);
}

/**
 * Change the digits in given column.
 * @param col column to change.
 * @param numTime one of the hhmm (time) digits.
 * @param numTop one of the ddmm (date) digits.
 **/
void change_digits(int col, int numTime, int numTop, bool toCallback)
{
    change_digit(col, numTime, numTop, -2);
    set_digit(col, numTime, toCallback);
}

/**
 * Change the digits in given column.
 * @param col column to change.
 * @param numTime one of the hhmm (time) digits.
 **/
void hide_digits(int col, int numTime)
{
    change_digit(col, numTime, ' ', -2);
    change_digit(col, numTime, ' ', 0);
    text_layer_set_text(text_layer[col], &digits[col][0]);
}

void revealDateTime(void *a_pData)
{
    switch (m_nRevealStep)
    {
        case 0:
            change_digit(0, digitTime1, ' ', -2);
            text_layer_set_text(text_layer[0], &digits[0][0]);
            digitsDate[0] = '0' + digitDate1;
            ++m_nRevealStep;
            m_sptimer1 = app_timer_register(INTERVAL_REVEAL_NEXT, (AppTimerCallback) revealDateTime, NULL);
            break;
        case 1:
            change_digit(1, digitTime2, ' ', -2);
            text_layer_set_text(text_layer[1], &digits[1][0]);
            change_digit(0, digitTime1, ' ', 0);
            text_layer_set_text(text_layer[0], &digits[0][0]);
            digitsDate[2] = '0' + digitDate2;
            digitsTime[0] = '0' + digitTime1;
            ++m_nRevealStep;
            m_sptimer1 = app_timer_register(INTERVAL_REVEAL_NEXT, (AppTimerCallback) revealDateTime, NULL);
            break;
        case 2:
            change_digit(2, digitTime3, ' ', -2);
            text_layer_set_text(text_layer[2], &digits[2][0]);
            change_digit(1, digitTime2, ' ', 0);
            text_layer_set_text(text_layer[1], &digits[1][0]);
            digitsDate[4] = '0' + digitDate3;
            digitsTime[2] = '0' + digitTime2;
            ++m_nRevealStep;
            m_sptimer1 = app_timer_register(INTERVAL_REVEAL_NEXT, (AppTimerCallback) revealDateTime, NULL);
            break;
        case 3:
            change_digit(3, digitTime4, ' ', -2);
            text_layer_set_text(text_layer[3], &digits[3][0]);
            change_digit(2, digitTime3, ' ', 0);
            text_layer_set_text(text_layer[2], &digits[2][0]);
            digitsDate[6] = '0' + digitDate4;
            digitsTime[4] = '0' + digitTime3;
            ++m_nRevealStep;
            m_sptimer1 = app_timer_register(INTERVAL_REVEAL_NEXT, (AppTimerCallback) revealDateTime, NULL);
            break;
        case 4:
        default:
            change_digit(3, digitTime4, ' ', 0);
            text_layer_set_text(text_layer[3], &digits[3][0]);
            digitsTime[6] = '0' + digitTime4;
            ++m_nRevealStep;
            break;
    }
    text_layer_set_text(m_ptxtDate, digitsDate);
    text_layer_set_text(m_ptxtTime, digitsTime);
}

void animation_stopped(struct Animation *animation)
{
    m_nRevealStep = 0;
    m_sptimer1 = app_timer_register(INTERVAL_REVEAL_INITIAL, (AppTimerCallback) revealDateTime, NULL);
}

/**
 * Convert string to uppercase.
 * @param a_pchStr string to be converted.
 * @param a_nMaxLen size of string.
 **/
void toUpperCase(char *a_pchStr, int a_nMaxLen)
{
    for (int i = 0; (i < a_nMaxLen) && (a_pchStr[i] != 0); ++i)
    {
        if ((a_pchStr[i] >= 'a') && (a_pchStr[i] <= 'z'))
        {
            a_pchStr[i] -= 32;
        }
    }
}

/**
 * Changes the 3-letters representation of 'day of week' (e.g. SUN).
 * Its background colour acts as 'bluetooth connectivity' indication:
 *   Coloured (AM/PM colour): connected; dark (black): disconnected.
 * TODO: Maybe change to 200% height half-coloured bar for animation.
 *
 * @param daysSinceSun day of week (0: Sun, etc).
 **/
void changeDayOfWeek(struct tm* tick_time)
{
    //write abbreviated 'day of week' (weekday name) according to the current locale:
    strftime(s_dayOfWeek_buffer, sizeof(s_dayOfWeek_buffer), "%a", tick_time);
    toUpperCase(s_dayOfWeek_buffer, sizeof(s_dayOfWeek_buffer));

    TextLayer *layer = text_layer[TXT_LAYER_DAY_OF_WEEK];
    text_layer_set_text(layer, s_dayOfWeek_buffer);
    //text_layer_set_background_color(layer, m_bIsAm? GColorRed: GColorBlue);
    //text_layer_set_background_color(layer, GColorBlack);
    text_layer_set_background_color(layer, m_bBtConnected? (m_bIsAm? GColorRed: GColorBlue): GColorBlack);

//    to_rect[TXT_LAYER_DAY_OF_WEEK] = layer_get_frame((Layer*)layer);
//    to_rect[TXT_LAYER_DAY_OF_WEEK].origin.y = 0;
//
//    if (animations[TXT_LAYER_DAY_OF_WEEK])
//        property_animation_destroy(animations[TXT_LAYER_DAY_OF_WEEK]);
//
//    animations[TXT_LAYER_DAY_OF_WEEK] = property_animation_create_layer_frame((Layer*)layer, NULL, &to_rect[TXT_LAYER_DAY_OF_WEEK]);
//    animation_set_duration((Animation*) animations[TXT_LAYER_DAY_OF_WEEK], 1000);
//    animation_schedule((Animation*) animations[TXT_LAYER_DAY_OF_WEEK]);
}

void display_time(struct tm* tick_time) {
    //erase decoded numerals:
    digitsDate[0] = 32; digitsDate[2] = 32; digitsDate[4] = 32; digitsDate[6] = 32;
    digitsTime[0] = 32; digitsTime[2] = 32; digitsTime[4] = 32; digitsTime[6] = 32;
    text_layer_set_text(m_ptxtDate, digitsDate);
    text_layer_set_text(m_ptxtTime, digitsTime);

    int h = tick_time->tm_hour;
    int m = tick_time->tm_min;

    m_bIsAm = (h < 12);

    // If watch is in 12hour mode
    if(!clock_is_24h_style()) {
        if(h == 0) { //Midnight to 1am
            h = 12;
        } else if(h > 12) { //1pm to 11:59pm
            h -= 12;
        }
    }

    //restore digits:
    memcpy(digits, digitsBkup, (4*32));
    /**
     * Change 3 rows as follows:
     *   2nd row: ddmm (date less year)
     *   3rd row: hhmm (time)
     *   4th row: year
     **/
    int mthsSinceJan = tick_time->tm_mon + 1; //base 0
    int dayOfMth = tick_time->tm_mday; //base 1
    digitTime1 = h/10;
    digitTime2 = h%10;
    digitTime3 = m/10;
    digitTime4 = m%10;
    digitDate1 = dayOfMth / 10;
    digitDate2 = dayOfMth % 10;
    digitDate3 = mthsSinceJan / 10;
    digitDate4 = mthsSinceJan % 10;
    change_digits(0, digitTime1, digitDate1, false);
    change_digits(1, digitTime2, digitDate2, false);
    change_digits(2, digitTime3, digitDate3, false);
    change_digits(3, digitTime4, digitDate4, true);

    //show day of week in row above ddmm (i.e. 1st row):
    changeDayOfWeek(tick_time);
}

void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {
    display_time(tick_time);
}

/**
 * digits comprises 4 cols of 14 digits each.
 * In each column, digits in 1st 4 rows are the same as those in last 4 rows.
 **/
void fill_digits(int i) {
    for (int n=0; n<10; ++n) {
        int p = (order[i][n] * 2) + 4;
        digits[i][p] = '0' + n;
        digits[i][p+1] = '\n';

        //make digits in 1st 4 rows same as those in last 4 rows
        if (p < 8) {
            p += 20;
            digits[i][p] = '0' + n;
            digits[i][p+1] = '\n';
        } else if(p >= 20) {
            p -= 20;
            digits[i][p] = '0' + n;
            digits[i][p+1] = '\n';
        }
    }

    digits[i][28] = '\0';
}

void fill_offsets(int i) {
    for(int n=0; n<10; ++n) {
        offsets[i][n] = order[i][n] + 2;
    }
}

//fill given column i with digits 0 - 9 at random locations
void fill_order(int i) {
    //fill col with digits 0-9
    for(int n=0; n<10; ++n) {
        order[i][n] = n;
    }

    //randomly swap digits' locations
    for(int n=0; n<10; ++n) {
        int k = rand() % 10;
        if(n != k) {
            int tmp = order[i][k];
            order[i][k] = order[i][n];
            order[i][n] = tmp;
        }
    }
}

//
//Window setup sutff
//
void handle_init(void)
{
    my_window = window_create();
    window_stack_push(my_window, true);
    window_set_background_color(my_window, GColorBlack);

    s_dayOfWeek_buffer[0] = 0;
    for (int j = 0; j < 8; ++j)
    {
        digitsDate[j] = 32;
        digitsTime[j] = 32;
    }
    digitsDate[7] = 0;
    digitsTime[7] = 0;

    Layer *root_layer = window_get_root_layer(my_window);
    GRect frame = layer_get_frame(root_layer);

    //Add coloured time row layer 1st so that it is below digits (text) layers:
    time_box_layer = layer_create(frame);
    layer_set_update_proc(time_box_layer, time_box_layer_update_proc);
    layer_add_child(root_layer, time_box_layer);

    //Add coloured battery-level column layer 1st so that it is below digits (text) layers:
    battery_level_layer = layer_create(GRect(BATTERY_LVL_COL*frame.size.w/4, -2, frame.size.w/4, frame.size.h*3 + 4));
    layer_set_update_proc(battery_level_layer, battery_level_layer_update_proc);
    layer_add_child(root_layer, battery_level_layer);

    center = frame.size.h/2;

    srand(time(NULL));

    s_custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_42));

    for (int i = 0; i < 4; ++i)
    {
        fill_order(i);
        fill_offsets(i);
        fill_digits(i);
        //backup digits:
        memcpy(digitsBkup, digits, (4*32));

        text_layer[i] = text_layer_create(GRect(i*frame.size.w/4, 0, frame.size.w/4, 800));
        text_layer_set_text_color(text_layer[i], GColorWhite);
        text_layer_set_background_color(text_layer[i], GColorClear);
        //text_layer_set_background_color(text_layer[i],
        //  (i != 2)? GColorClear:
        //  m_bIsAm? GColorRed: GColorBlue);
        text_layer_set_font(text_layer[i],
            //fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
            //fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS)); //Can't use, as numbers only
            s_custom_font);
        text_layer_set_text(text_layer[i], &digits[i][0]);
        text_layer_set_text_alignment(text_layer[i], GTextAlignmentCenter);
        layer_add_child(root_layer, text_layer_get_layer(text_layer[i]));
    }
    //Example: view live log using "pebble logs --phone=192.168.1.X" command
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "digits0 %s", digits[0]);

    //Add 'day of week' top row layer above 'digits' layers:
    text_layer[TXT_LAYER_DAY_OF_WEEK] = text_layer_create(GRect(0, -8, frame.size.w / 2 + 6, 32));
    //text_layer[TXT_LAYER_DAY_OF_WEEK] = text_layer_create(GRect(6, -8, frame.size.w / 2 - 2, /*32*/64));
    //layer_set_bounds((Layer*)text_layer[TXT_LAYER_DAY_OF_WEEK], GRect(6, -8, frame.size.w / 2 - 2, 10));
    text_layer_set_text_color(text_layer[TXT_LAYER_DAY_OF_WEEK], GColorWhite);
    text_layer_set_background_color(text_layer[TXT_LAYER_DAY_OF_WEEK], GColorClear);
    text_layer_set_font(text_layer[TXT_LAYER_DAY_OF_WEEK],
        fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text(text_layer[TXT_LAYER_DAY_OF_WEEK], s_dayOfWeek_buffer);
    text_layer_set_text_alignment(text_layer[TXT_LAYER_DAY_OF_WEEK], GTextAlignmentLeft);
    //text_layer_set_overflow_mode(text_layer[TXT_LAYER_DAY_OF_WEEK], GTextOverflowModeWordWrap);
    layer_add_child(root_layer, text_layer_get_layer(text_layer[TXT_LAYER_DAY_OF_WEEK]));

    //Add decoded (roman numeral) date (ddmm) & time layers above enigma (enigma alien text) layers
    m_ptxtDate = text_layer_create(GRect(0, frame.size.h / 8 - 7, frame.size.w, frame.size.h / 4));
    m_ptxtTime = text_layer_create(GRect(0, frame.size.h * 3 / 8 - 7, frame.size.w, frame.size.h / 4));
    text_layer_set_text_color(m_ptxtDate, GColorWhite);
    text_layer_set_background_color(m_ptxtDate, GColorClear);
    text_layer_set_font(m_ptxtDate, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
    text_layer_set_text(m_ptxtDate, "");
    text_layer_set_text_alignment(m_ptxtDate, GTextAlignmentLeft);
    layer_add_child(root_layer, text_layer_get_layer(m_ptxtDate));
    text_layer_set_text_color(m_ptxtTime, GColorWhite);
    text_layer_set_background_color(m_ptxtTime, GColorClear);
    text_layer_set_font(m_ptxtTime, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
    text_layer_set_text(m_ptxtTime, "");
    text_layer_set_text_alignment(m_ptxtTime, GTextAlignmentLeft);
    layer_add_child(root_layer, text_layer_get_layer(m_ptxtTime));

    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    display_time(tick_time);

    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);

    // Subscribe to Bluetooth updates
    bluetooth_connection_service_subscribe(bt_handler);
    // Show current connection state
    bt_handler(bluetooth_connection_service_peek());

    // Subscribe to the Battery State Service
    battery_state_service_subscribe(battery_handler);
    // Get the current battery level
    battery_handler(battery_state_service_peek());
}

void handle_deinit(void) {
    for (int i = 0; i < 5; ++i)
    {
        text_layer_destroy(text_layer[i]);
        if (animations[i])
            property_animation_destroy(animations[i]);
    }
    text_layer_destroy(m_ptxtDate);
    text_layer_destroy(m_ptxtTime);
    layer_destroy(time_box_layer);
    layer_destroy(battery_level_layer);
    fonts_unload_custom_font(s_custom_font);
    app_timer_cancel(m_sptimer1);
    window_destroy(my_window);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}

