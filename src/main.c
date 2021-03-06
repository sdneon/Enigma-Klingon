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
 *   > Reverts to alien text after 5 secs.
 *   > Shake to reveal again.
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

//DEBUG flags
//#define DISABLE_CONFIG

//bitmasks for m_nDecodeMode flag:
#define DECODE_WEEKDAY  1
#define DECODE_DATE     2
#define DECODE_TIME     4

#define DECODE_DATETIME_ONLY (DECODE_DATE | DECODE_TIME)
#define DECODE_ALL (DECODE_WEEKDAY | DECODE_DATE | DECODE_TIME)

/**
 * Specify this flag as control decoding of date & time. Refer to bitmasks above. E.g.:
 * 0: disable
 * 1: decode weekday name
 * 6: decode date & time (less weekday name)
 * 7: decode all
 **/
#define DECODE DECODE_TIME

//Available languages
#define LANG_ROTATE       -1
#define LANG_HUMAN         0
#define LANG_KLINGON       1
#define LANG_AUREBESH      2
#define LANG_AUREBESH_HAND 3
#define LANG_GRAALEN       4
#define LANG_KENTAURUS     5
#define LANG_MANDALOR      6
#define LANG_BETAZED       7

#define MAX_LANG 8

//masks for vibes:
#define MASKV_BTDC   0x20000
#define MASKV_HOURLY 0x10000
#define MASKV_FROM   0xFF00
#define MASKV_TO     0x00FF
//def: disabled, 10am to 8pm
#define DEF_VIBES    0x0A14


#define WEATHER_NA_ICON_ID 48
static const int WEATHER_ICONS = RESOURCE_ID_WEATHER00;

/**
 * Specify the time interval (ms) after reveal, when all text will be re-encoded.
 **/
#define INTERVAL_REENCODE 5000

#define TXT_LAYER_DAY_OF_WEEK_ENCODED 4
#define TXT_LAYER_DAY_OF_WEEK_DECODED 5
#define BATTERY_LVL_COL 2

//Interval (in ms) to decode/reveal date & time
#define INTERVAL_REVEAL_INITIAL 1000
#define INTERVAL_REVEAL_NEXT 15

//height of a row
#define ROW_HEIGHT 47

Window *my_window;
Layer *time_box_layer, *battery_level_layer;
TextLayer *text_layer[6], *lyrTxtDecoded[4];
PropertyAnimation *animations[5] = {0};
GRect to_rect[6];
int center;

char digits[4][32], digitsBkup[4][32], //4 columns of encoded enigma digits
    digitsTimeBkup[4][32],
    digitsDecoded[4][5]; //4 colums (2 rows) of decoded digits
int offsets[4][10];
int order[4][10];
bool m_bIsAm = false;
static char s_dayOfWeek_buffer[4];

static GFont m_sFontLetters[MAX_LANG] = {NULL}, m_sFontNumerals[MAX_LANG] = {NULL};
static AppTimer *m_sptimerRevealAnimation, *m_sptimerRevert2Alien;
static int m_nRevealStep = -1;
int digitTime1, digitTime2, digitTime3, digitTime4,
    digitDate1, digitDate2, digitDate3, digitDate4;
static int m_nDecodeMode = DECODE;
static int m_nLang = LANG_KLINGON;
static int m_nVibes = DEF_VIBES;
static bool m_bUpdatingDisplay = false, m_bReady = false;

// Vibe pattern for loss of BT connection: ON for 400ms, OFF for 100ms, ON for 300ms, OFF 100ms, 100ms:
static const uint32_t const VIBE_SEG_BT_LOSS[] = { 400, 200, 200, 400, 100 };
static const VibePattern VIBE_PAT_BT_LOSS = {
  .durations = VIBE_SEG_BT_LOSS,
  .num_segments = ARRAY_LENGTH(VIBE_SEG_BT_LOSS),
};

//weather stuff
static Layer *m_sLayerWeather, *m_sLayerWeather2;
static TextLayer *m_stxtWeather;
static GBitmap *m_spbmPicWeather = NULL;
static BitmapLayer *m_spbmLayerW;
static bool m_bWeatherEnabled = false;
static int m_nIconId = WEATHER_NA_ICON_ID;
static char m_szTemp[10]; //temperature string

//forward declaration
void animation_stopped(struct Animation *animation);
void display_time(struct tm* tick_time);
void setWeatherIcon();

//
//Configuration stuff via AppMessage API
//
#define KEY_DECODE  0
#define KEY_LANG    1
#define KEY_VIBES   2
#define KEY_WEATHER  3
#define KEY_INTERVAL 4
#define KEY_GPS      5
#define KEY_LOCATION 6
#define KEY_UNITS    7
#define KEY_ICON     8
#define KEY_TEMP     9

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Get the first pair
    Tuple *t = dict_read_first(iterator);
    int nNeedUpdates = 0;
    int nNewValue, i;
    bool bNewValue;
    bool bUpdateWeather = false;

    // Process all pairs present
    while(t != NULL) {
        // Process this pair's key
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Key:%d received with value:%d", (int)t->key, (int)t->value->int32);
        switch (t->key) {
            case KEY_DECODE:
                nNewValue = t->value->int32;
                if (m_nDecodeMode != nNewValue)
                {
                    nNeedUpdates = 1;
                    m_nDecodeMode = nNewValue;
                }
                break;
            case KEY_LANG:
                nNewValue = t->value->int32;
                if (m_nLang != nNewValue)
                {
                    nNeedUpdates = 2;
                    m_nLang = nNewValue;
                }
                break;
            case KEY_VIBES:
                nNewValue = t->value->int32;
                if (m_nVibes != nNewValue)
                {
                    m_nVibes = nNewValue;
                }
                break;
            case KEY_WEATHER:
                bNewValue = t->value->int32 != 0;
                if (m_bWeatherEnabled != bNewValue)
                {
                    m_bWeatherEnabled = bNewValue;
                    layer_set_hidden(m_sLayerWeather, !m_bWeatherEnabled);
                }
                break;
            case KEY_ICON:
                nNewValue = t->value->int32;
                if (m_nIconId != nNewValue)
                {
                    gbitmap_destroy(m_spbmPicWeather);
                    m_spbmPicWeather = NULL;

                    m_nIconId = nNewValue;
                    m_spbmPicWeather = gbitmap_create_with_resource(WEATHER_ICONS + nNewValue);
                    if (m_spbmPicWeather != NULL)
                    {
                        setWeatherIcon();
                    }
                    bUpdateWeather = true;
                }
                break;
            case KEY_TEMP:
                strcpy(m_szTemp, t->value->cstring);
                bUpdateWeather = true;
                break;
        }

        // Get next pair, if any
        t = dict_read_next(iterator);
    }
    if ((nNeedUpdates > 0) && m_bReady && !m_bUpdatingDisplay)
    {
        if (nNeedUpdates > 1)
        {
            for (i = 0; i < 4; ++i)
            {
                text_layer_set_font(text_layer[i], m_sFontNumerals[m_nLang]);
            }
            text_layer_set_font(text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED], m_sFontLetters[m_nLang]);
        }
        time_t now = time(NULL);
        struct tm *tick_time = localtime(&now);
        display_time(tick_time);
    }
    if (bUpdateWeather)
    {
        //update weather icon
        text_layer_set_text(m_stxtWeather, m_szTemp);
        layer_mark_dirty(m_sLayerWeather);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


void setWeatherIcon()
{
    bitmap_layer_set_bitmap(m_spbmLayerW, m_spbmPicWeather);
}

//
//Bluetooth stuff
//
bool m_bBtConnected = false;
static void bt_handler(bool connected) {
    if (!connected && m_bBtConnected //vibrate once upon BT connection lost
        && (m_nVibes & MASKV_BTDC)) //only if option enabled
    {
        vibes_enqueue_custom_pattern(VIBE_PAT_BT_LOSS);
    }
    m_bBtConnected = connected;
    TextLayer *layer = text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED];
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
/*
void hide_digits(int col, int numTime)
{
    change_digit(col, numTime, ' ', -2);
    change_digit(col, numTime, ' ', 0);
    text_layer_set_text(text_layer[col], &digits[col][0]);
}
*/

void revealDayOfWeek()
{
    //reveal decoded weekday name:
    TextLayer *layer = text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED];
    text_layer_set_text(layer, s_dayOfWeek_buffer);
    //hide encoded/alien weekday name:
    layer = text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED];
    text_layer_set_text(layer, "");
    m_bUpdatingDisplay = false;
}

#ifdef INTERVAL_REENCODE
void hideDateTime(void *a_pData)
{
    if (m_nRevealStep >= 0)
    {
        return; //skip if animation in progress
    }
    //restore digits:
    memcpy(digits, digitsTimeBkup, (4*32));
    if (m_nDecodeMode & DECODE_TIME)
    {
        change_digit(0, digitTime1, ' ', 0);
        change_digit(1, digitTime2, ' ', 0);
        change_digit(2, digitTime3, ' ', 0);
        change_digit(3, digitTime4, ' ', 0);
    }
    if (m_nDecodeMode & DECODE_DATE)
    {
        change_digit(0, digitTime1, ' ', -2);
        change_digit(1, digitTime2, ' ', -2);
        change_digit(2, digitTime3, ' ', -2);
        change_digit(3, digitTime4, ' ', -2);
    }
    //show encoded/alien date & time:
    for (int i = 0; i < 4; ++i)
    {
        text_layer_set_text(text_layer[i], digits[i]);
        if ((m_nDecodeMode & DECODE_DATE) == 0)
        {
            digitsDecoded[i][0] = 32; //hide date
        }
        if ((m_nDecodeMode & DECODE_TIME) == 0)
        {
            digitsDecoded[i][2] = 32; //hide time
        }
        text_layer_set_text(lyrTxtDecoded[i], digitsDecoded[i]);
    }
    if ((m_nDecodeMode & DECODE_WEEKDAY) == 0)
    {
        //show encoded/alien weekday name:
        TextLayer *layer = text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED];
        text_layer_set_text(layer, s_dayOfWeek_buffer);
        //hide decoded weekday name:
        layer = text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED];
        text_layer_set_text(layer, "");
    }
}
#endif //INTERVAL_REENCODE

/**
 * Reveal animation:
 * 1. Date row reveals first, one digit at a time (INTERVAL_REVEAL_NEXT interval).
 * 2. After one date digit, time row reveals, one digit at a time (INTERVAL_REVEAL_NEXT interval).
 * 3. When time is completely revealed, weekday name is revealed.
 * @param a_pData (int) we use this to pass an integer flag denoting the decoding mode.
 **/
void revealDateTime(void *a_pData)
{
    int nDecodeMode = (int)a_pData;
    switch (m_nRevealStep)
    {
        case 0:
            if (nDecodeMode & DECODE_DATE)
            {
                //clear encoded digit:
                change_digit(0, digitTime1, ' ', -2);
                text_layer_set_text(text_layer[0], &digits[0][0]);
                //show decoded digit:
                digitsDecoded[0][0] = '0' + digitDate1;
                text_layer_set_text(lyrTxtDecoded[0], digitsDecoded[0]);
            }
            //schedule next step in animation:
            ++m_nRevealStep;
            m_sptimerRevealAnimation = app_timer_register(INTERVAL_REVEAL_NEXT, (AppTimerCallback) revealDateTime, a_pData);
            break;
        case 1:
            if (nDecodeMode & DECODE_DATE)
            {
                change_digit(1, digitTime2, ' ', -2);
                text_layer_set_text(text_layer[1], &digits[1][0]);
                digitsDecoded[1][0] = '0' + digitDate2;
                text_layer_set_text(lyrTxtDecoded[1], digitsDecoded[1]);
            }
            if (nDecodeMode & DECODE_TIME)
            {
                change_digit(0, digitTime1, ' ', 0);
                text_layer_set_text(text_layer[0], &digits[0][0]);
                digitsDecoded[0][2] = '0' + digitTime1;
                text_layer_set_text(lyrTxtDecoded[0], digitsDecoded[0]);
            }
            ++m_nRevealStep;
            m_sptimerRevealAnimation = app_timer_register(INTERVAL_REVEAL_NEXT, (AppTimerCallback) revealDateTime, a_pData);
            break;
        case 2:
            if (nDecodeMode & DECODE_DATE)
            {
                change_digit(2, digitTime3, ' ', -2);
                text_layer_set_text(text_layer[2], &digits[2][0]);
                digitsDecoded[2][0] = '0' + digitDate3;
                text_layer_set_text(lyrTxtDecoded[2], digitsDecoded[2]);
            }
            if (nDecodeMode & DECODE_TIME)
            {
                change_digit(1, digitTime2, ' ', 0);
                text_layer_set_text(text_layer[1], &digits[1][0]);
                digitsDecoded[1][2] = '0' + digitTime2;
                text_layer_set_text(lyrTxtDecoded[1], digitsDecoded[1]);
            }
            ++m_nRevealStep;
            m_sptimerRevealAnimation = app_timer_register(INTERVAL_REVEAL_NEXT, (AppTimerCallback) revealDateTime, a_pData);
            break;
        case 3:
            if (nDecodeMode & DECODE_DATE)
            {
                change_digit(3, digitTime4, ' ', -2);
                text_layer_set_text(text_layer[3], &digits[3][0]);
                digitsDecoded[3][0] = '0' + digitDate4;
                text_layer_set_text(lyrTxtDecoded[3], digitsDecoded[3]);
            }
            if (nDecodeMode & DECODE_TIME)
            {
                change_digit(2, digitTime3, ' ', 0);
                text_layer_set_text(text_layer[2], &digits[2][0]);
                digitsDecoded[2][2] = '0' + digitTime3;
                text_layer_set_text(lyrTxtDecoded[2], digitsDecoded[2]);
            }
            ++m_nRevealStep;
            m_sptimerRevealAnimation = app_timer_register(INTERVAL_REVEAL_NEXT, (AppTimerCallback) revealDateTime, a_pData);
            break;
        case 4:
        default:
            if (nDecodeMode & DECODE_TIME)
            {
                change_digit(3, digitTime4, ' ', 0);
                text_layer_set_text(text_layer[3], &digits[3][0]);
                digitsDecoded[3][2] = '0' + digitTime4;
                text_layer_set_text(lyrTxtDecoded[3], digitsDecoded[3]);
            }
            if (nDecodeMode & DECODE_WEEKDAY)
            {
                revealDayOfWeek();
            }
            else
            {
                m_bUpdatingDisplay = false;
            }
            m_nRevealStep = -1; //flag end of animation
#ifdef INTERVAL_REENCODE
            m_sptimerRevert2Alien = app_timer_register(INTERVAL_REENCODE, (AppTimerCallback) hideDateTime, NULL);
#endif //INTERVAL_REENCODE
            break;
    }
}

void animation_stopped(struct Animation *animation)
{
    if (m_nDecodeMode > 0)
    {
        m_nRevealStep = 0;
        m_sptimerRevealAnimation = app_timer_register(INTERVAL_REVEAL_INITIAL, (AppTimerCallback) revealDateTime, (void*)m_nDecodeMode);
    }
    else
    {
        m_bUpdatingDisplay = false;
    }
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

    TextLayer *layer = text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED];
    text_layer_set_text(layer, "");
    layer = text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED];
    text_layer_set_text(layer, s_dayOfWeek_buffer);
    //text_layer_set_background_color(layer, m_bIsAm? GColorRed: GColorBlue);
    //text_layer_set_background_color(layer, GColorBlack);
    text_layer_set_background_color(layer, m_bBtConnected? (m_bIsAm? GColorRed: GColorBlue): GColorBlack);
}

void display_time(struct tm* tick_time) {
    if (m_bUpdatingDisplay)
    {
        return;
    }
    m_bUpdatingDisplay = true;
    //erase decoded numerals & restore encoded text:
    memcpy(digits, digitsTimeBkup, (4*32));
    for (int i = 0; i < 4; ++i)
    {
        text_layer_set_text(text_layer[i], digits[i]);
        digitsDecoded[i][0] = 32;
        digitsDecoded[i][2] = 32;
        text_layer_set_text(lyrTxtDecoded[i], digitsDecoded[i]);
    }

    int h = tick_time->tm_hour;
    int m = tick_time->tm_min;

    if ((m_nVibes & MASKV_HOURLY) //option enabled to vibrate hourly
        && (m == 0)) //hourly mark reached
    {
        int from = (m_nVibes & MASKV_FROM) >> 8,
            to = m_nVibes & MASKV_TO;
        bool bShake = false;
        if (from <= to)
        {
            bShake = (h >= from) && (h <= to);
        }
        else
        {
            bShake = (h >= from) || (h <= to);
        }
        if (bShake)
        {
            vibes_double_pulse();
        }
    }

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
    memcpy(digitsTimeBkup, digits, (4*32));

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

static void tap_handler(AccelAxisType axis, int32_t direction)
{
    if ((m_nRevealStep < 0) && !m_bUpdatingDisplay //not animating/updating in progress
        && (m_nDecodeMode != DECODE_ALL)) //not already in DECODE_ALL mode
    {
        m_nRevealStep = 0;
        revealDateTime((void*)DECODE_ALL);
    }
}

void readConfig()
{
    if (persist_exists(KEY_DECODE))
    {
        m_nDecodeMode = persist_read_int(KEY_DECODE);
        if (m_nDecodeMode == DECODE_WEEKDAY)
        {
            m_nDecodeMode = DECODE_TIME;
        }
    }
    if (persist_exists(KEY_LANG))
    {
        m_nLang = persist_read_int(KEY_LANG);
    }
    if (persist_exists(KEY_VIBES))
    {
        m_nVibes = persist_read_int(KEY_VIBES);
    }
}

void saveConfig()
{
    persist_write_int(KEY_DECODE, m_nDecodeMode);
    persist_write_int(KEY_LANG, m_nLang);
    persist_write_int(KEY_VIBES, m_nVibes);
}

void loadFonts()
{
    m_sFontLetters[0] = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
    m_sFontNumerals[0] = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
    m_sFontLetters[1] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMA_30));
    m_sFontNumerals[1] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMA_42));
    m_sFontLetters[2] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMB_30));
    m_sFontNumerals[2] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMB_42));
    m_sFontLetters[3] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMC_30));
    m_sFontNumerals[3] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMC_42));
    m_sFontLetters[4] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMD_30));
    m_sFontNumerals[4] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMD_42));
    m_sFontLetters[5] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOME_30));
    m_sFontNumerals[5] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOME_42));
    m_sFontLetters[6] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMF_30));
    m_sFontNumerals[6] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMF_42));
    m_sFontLetters[7] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMG_30));
    m_sFontNumerals[7] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMG_42));
//    m_sFontLetters[8] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMH_30));
//    m_sFontNumerals[8] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOMH_42));
}


//
//Window setup sutff
//
void handle_init(void)
{
    m_szTemp[0] = 0;
#ifndef DISABLE_CONFIG
    readConfig();
    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    // Open AppMessage
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
#endif

    my_window = window_create();
    window_stack_push(my_window, true);
    window_set_background_color(my_window, GColorBlack);

    s_dayOfWeek_buffer[0] = 0;

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

    loadFonts();

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
        text_layer_set_font(text_layer[i], m_sFontNumerals[m_nLang]);
        text_layer_set_text(text_layer[i], &digits[i][0]);
        text_layer_set_text_alignment(text_layer[i], GTextAlignmentCenter);
        layer_add_child(root_layer, text_layer_get_layer(text_layer[i]));

        strncpy(digitsDecoded[i], " \n \n\0", 5);
        lyrTxtDecoded[i] = text_layer_create(GRect(i*frame.size.w/4, 14, frame.size.w/4, 2*ROW_HEIGHT));
        text_layer_set_text_color(lyrTxtDecoded[i], GColorWhite);
        text_layer_set_background_color(lyrTxtDecoded[i], GColorClear);
        text_layer_set_font(lyrTxtDecoded[i], m_sFontNumerals[LANG_HUMAN]);
        text_layer_set_text(lyrTxtDecoded[i], ""); //&digits[i][0]);
        text_layer_set_text_alignment(lyrTxtDecoded[i], GTextAlignmentCenter);
        layer_add_child(root_layer, text_layer_get_layer(lyrTxtDecoded[i]));
    }
    //Example: view live log using "pebble logs --phone=192.168.1.X" command
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "digits0 %s", digits[0]);

    //Add encoded 'day of week' top row layer above 'digits' layers:
    text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED] = text_layer_create(GRect(0, -8, frame.size.w / 2 + 6, 32));
    //text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED] = text_layer_create(GRect(6, -8, frame.size.w / 2 - 2, /*32*/64));
    //layer_set_bounds((Layer*)text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED], GRect(6, -8, frame.size.w / 2 - 2, 10));
    text_layer_set_text_color(text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED], GColorWhite);
    text_layer_set_background_color(text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED], GColorClear);
    text_layer_set_font(text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED], m_sFontLetters[m_nLang]);
    text_layer_set_text(text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED], s_dayOfWeek_buffer);
    text_layer_set_text_alignment(text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED], GTextAlignmentLeft);
    //text_layer_set_overflow_mode(text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED], GTextOverflowModeWordWrap);
    layer_add_child(root_layer, text_layer_get_layer(text_layer[TXT_LAYER_DAY_OF_WEEK_ENCODED]));
    //Add decoded 'day of week' layer
    text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED] = text_layer_create(GRect(0, -8, frame.size.w / 2 + 6, 32));
    text_layer_set_text_color(text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED], GColorWhite);
    text_layer_set_background_color(text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED], GColorClear);
    text_layer_set_font(text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED], m_sFontLetters[LANG_HUMAN]);
    text_layer_set_text(text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED], "");
    text_layer_set_text_alignment(text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED], GTextAlignmentLeft);
    layer_add_child(root_layer, text_layer_get_layer(text_layer[TXT_LAYER_DAY_OF_WEEK_DECODED]));

    //Weather pic layer
    m_sLayerWeather = layer_create(GRect(104, 108, 40, 60));
    layer_add_child(root_layer, m_sLayerWeather);
    m_spbmLayerW = bitmap_layer_create(GRect(0, 0, 40, 40));
    m_spbmPicWeather = gbitmap_create_with_resource(WEATHER_ICONS + m_nIconId);
    bitmap_layer_set_background_color(m_spbmLayerW, GColorFolly);
    bitmap_layer_set_compositing_mode(m_spbmLayerW, GCompOpSet);
    setWeatherIcon();
    layer_add_child(m_sLayerWeather, bitmap_layer_get_layer(m_spbmLayerW));
    layer_set_hidden(m_sLayerWeather, true);
    m_stxtWeather = text_layer_create(GRect(0, 40, 40, 32));
    //strcpy(m_szTemp, "-888 C"); //DEBUG: test text width
    text_layer_set_text(m_stxtWeather, m_szTemp);
    text_layer_set_background_color(m_stxtWeather, GColorFolly);
    text_layer_set_text_color(m_stxtWeather, GColorWhite);
    text_layer_set_font(m_stxtWeather, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(m_stxtWeather, GTextAlignmentCenter);
    layer_add_child(m_sLayerWeather, text_layer_get_layer(m_stxtWeather));

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

    // Subscribe to the Tap Service
    accel_tap_service_subscribe(tap_handler);

    m_bReady = true;
}

void handle_deinit(void)
{
    accel_tap_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    battery_state_service_unsubscribe();
    tick_timer_service_unsubscribe();

#ifndef DISABLE_CONFIG
    app_message_deregister_callbacks();
    saveConfig();
#endif

    int i;
    for (i = 0; i < 6; ++i)
    {
        text_layer_destroy(text_layer[i]);
    }
    for (i = 0; i < 5; ++i)
    {
        if (animations[i])
            property_animation_destroy(animations[i]);
    }
    for (i = 0; i < 4; ++i)
    {
        text_layer_destroy(lyrTxtDecoded[i]);
    }
    layer_destroy(time_box_layer);
    layer_destroy(battery_level_layer);
    for (i = 1; i < MAX_LANG; ++i)
    {
        if (m_sFontLetters[i]) fonts_unload_custom_font(m_sFontLetters[i]);
        if (m_sFontNumerals[i]) fonts_unload_custom_font(m_sFontNumerals[i]);
    }
    if (m_sptimerRevealAnimation) app_timer_cancel(m_sptimerRevealAnimation);
    if (m_sptimerRevert2Alien) app_timer_cancel(m_sptimerRevert2Alien);
    bitmap_layer_destroy(m_spbmLayerW);
    layer_destroy(m_sLayerWeather);
    if (m_spbmPicWeather)
    {
        gbitmap_destroy(m_spbmPicWeather);
    }
    window_destroy(my_window);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
