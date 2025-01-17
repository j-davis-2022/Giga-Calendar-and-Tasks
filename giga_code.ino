#include <Arduino_H7_Video.h>
#include <lvgl.h>
#include <Arduino_GigaDisplay.h>
#include <Arduino_GigaDisplayTouch.h>
#include <RTClib.h>
#include <ArduinoHttpClient.h>
#include "mbed.h"
#include <mbed_mktime.h>
#include <WiFi.h>
#include <WiFiSSLClient.h>
#include <WiFiUdp.h>

#include "secrets.h"
#include "notes.h"

Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch TouchDetector;
GigaDisplayRGB rgb;

int status = WL_IDLE_STATUS;
WiFiSSLClient googleClient;
WiFiUDP Udp;

constexpr auto timeServer { "pool.ntp.org" };
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

unsigned long long edtCorrection = (4 * 3600);
unsigned long long estCorrection = (5 * 3600);

int lastMinute;
int lastDay;

lv_obj_t * obj1; // up until line 70/75, I define variables that need to be available to all functions and thus can't be defined in the setup() function
lv_obj_t * scale_obj;
lv_obj_t * now_line;
lv_obj_t * msg;
lv_obj_t * btn;
lv_obj_t * list;
static lv_style_t task_item_style;
static lv_style_t style_line; // these are the styles for the calendar line, you can add more if you have more than three types of events; this one is the base line
static lv_style_t style_occupied_line; // occupied line for events
static lv_style_t style_commute_line; // commute line for time spent travelling
static lv_style_t style_work_line; // this is for times I have scheduled to work
static lv_point_precise_t now_points[] = { {0, (16 * 24)}, {25, (16 * 24)} };
static lv_point_precise_t points1[] = {{0, 0}, {0, 0}}; // these (points1 - points24) are used to indicate the times of events in the calendar; you can add more if you expect you'll need them
static lv_point_precise_t points2[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points3[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points4[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points5[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points6[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points7[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points8[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points9[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points10[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points11[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points12[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points13[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points14[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points15[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points16[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points17[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points18[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points19[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points20[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points21[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points22[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points23[] = {{0, 0}, {0, 0}};
static lv_point_precise_t points24[] = {{0, 0}, {0, 0}};
static lv_point_precise_t allPoints[][2] = {
  {points1[0], points1[1]}, {points2[0], points2[1]}, {points3[0], points3[1]}, {points4[0], points4[1]}, {points5[0], points5[1]}, {points6[0], points6[1]},
  {points7[0], points7[1]}, {points8[0], points8[1]}, {points9[0], points9[1]}, {points10[0], points10[1]}, {points11[0], points11[1]}, {points12[0], points12[1]},
  {points13[0], points13[1]}, {points14[0], points14[1]}, {points15[0], points15[1]}, {points16[0], points16[1]}, {points17[0], points17[1]}, {points18[0], points18[1]},
  {points19[0], points19[1]}, {points20[0], points20[1]}, {points21[0], points21[1]}, {points22[0], points22[1]}, {points23[0], points23[1]}, {points24[0], points24[1]}
}; // this array makes it possible to loop through the points in the setScale() function

String dailyEvents[24];
uint32_t dailyEventTimes[24][2];
uint32_t newDailyEventTimes[24][2];
uint32_t commuteTimes[24][2];
String workNames[24];
uint32_t workTimes[24][2];


bool boxUp = false;
unsigned long long nextPlay = 0;

void availableNetwork(); // these lines (88-99) list functions that take place below the main loop(); I'm not sure if they're necessary
void setNtpTime();
unsigned long sendNtpPacket();
unsigned long parseNtpPacket();
uint32_t getTimeCorrection();
String callAPICalendar();
String callAPITasks();
void calAnalysis();
void taskAnalysis();
void setScale();
String findEvent();
String getLocaltime();

static void change_date_cb(lv_event_t * e) {
  lv_obj_t * calendar = ((lv_obj_t*)lv_event_get_current_target(e));
  lv_calendar_date_t date;
  if (lv_calendar_get_pressed_date(calendar, &date)) {
    lv_calendar_set_today_date(calendar, date.year, date.month, date.day);
    DateTime newSelected (((uint16_t)date.year), ((uint8_t)date.month), ((uint8_t)date.day), 0, 0, 0);

    calAnalysis(callAPICalendar((newSelected.unixtime() + getTimeCorrection(newSelected)), appointmentCal), NULL, false, newDailyEventTimes, &style_occupied_line);
    calAnalysis(callAPICalendar((newSelected.unixtime() + getTimeCorrection(newSelected)), commuteCal), NULL, true, commuteTimes, &style_commute_line);
    calAnalysis(callAPICalendar((newSelected.unixtime() + getTimeCorrection(newSelected)), workCal), workNames, true, workTimes, &style_work_line);
  }
}

static void close_msg_cb(lv_event_t * e) {
  rgb.off();
  lv_obj_t * msgbox = ((lv_obj_t*)lv_event_get_user_data(e));
  lv_msgbox_close(msgbox);
  boxUp = false;
  nextPlay = 0;
}

static void checkbox_cb(lv_event_t * e) {
  lv_obj_t * cb = ((lv_obj_t *)lv_event_get_target(e));
  lv_obj_t * label = lv_obj_get_child(cb, 0);
  String id = lv_label_get_text(label);
  bool checked;
  if (lv_obj_has_state(cb, LV_STATE_CHECKED)) {
    lv_obj_add_state(cb, LV_STATE_CHECKED);
    checked = true;
  } else {
    lv_obj_remove_state(cb, LV_STATE_CHECKED);
    checked = false;
  }
  if (WiFi.status() == WL_CONNECTED) {
    String postData = "taskId=" + id + "&completedStatus=" + checked;
    HttpClient client = HttpClient(googleClient, "script.google.com", 443);
    client.beginRequest();
    client.post("/macros/s/AKfycbw1bUROXV2m4YUdXL8KmX1oi8_a_zpcyHm4UyFP0K4INrs0pxSeZ0uOEoin2XmZSP0e/exec");
    client.sendHeader("Content-Length", postData.length());
    client.beginBody();
    client.print(postData);
    client.endRequest();

    client.stop();
  }
}

static void btn_event_cb(lv_event_t * e) {
  lv_obj_t * msgbox = lv_msgbox_create(NULL); // this is for a list of reminders of things that help you focus on your work; add more as needed, but use different variable names for each
  lv_msgbox_add_title(msgbox, "Productivity to-do's:");
  lv_obj_t * cb0 = lv_checkbox_create(msgbox);
    lv_checkbox_set_text(cb0, "Go for a brief walk");
    lv_obj_add_event_cb(cb0, msg_checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_style(cb0, &task_item_style, 0);
  lv_obj_t * msg_btn = lv_msgbox_add_footer_button(msgbox, "Start!");
  lv_obj_add_event_cb(msg_btn, close_btn_msg_cb, LV_EVENT_CLICKED, msgbox);
}

static void close_btn_msg_cb(lv_event_t * e) {
  lv_obj_t * msgbox = ((lv_obj_t*)lv_event_get_user_data(e));
  lv_msgbox_close(msgbox);
}

static void msg_checkbox_cb(lv_event_t * e) {
  lv_obj_t * cb = ((lv_obj_t *)lv_event_get_target(e));
  if (lv_obj_has_state(cb, LV_STATE_CHECKED)) {
    lv_obj_add_state(cb, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(cb, LV_STATE_CHECKED);
  }
}

// the following lines define a tune that will play to notify you of events starting, I'm using the start of Kirby: Green Greens
int buzzer = 13;
const int q = 300;
const int h = q/2;
const int e = q/4;
const int d = q*2;
const int hole = q*4;
int intro[] =     {NOTE_DS6, NOTE_D6, NOTE_C6, NOTE_AS5, NOTE_F5, NOTE_D5, NOTE_GS5, NOTE_AS5, NOTE_C6, NOTE_D6, NOTE_B5, 0};
int timeIntro[] = {  q,  h,  h,   h,  h,  q,   h,   h,  h,  h,  q, q};

int start[] =      {NOTE_C6, 0, NOTE_G5, 0, NOTE_DS5, NOTE_D5, NOTE_C5, 0, NOTE_C5, NOTE_D5, NOTE_DS5, NOTE_C5, NOTE_AS4, NOTE_C5, NOTE_G4, 0};
int timeStart[] = {  q, q,  q, q,   q,  q,  q, q,  q,  q,   q,  q,   q,  q,  q, q};

int part1[] =    {NOTE_C6, 0, NOTE_G5, 0, NOTE_DS5, NOTE_D5, NOTE_C5, NOTE_C5, NOTE_D5, NOTE_DS5, NOTE_F5, NOTE_D5, NOTE_AS4, NOTE_C5, NOTE_G4, NOTE_C5, 0};
int timePart1[] = {q, q,  q, q,   q,  q,  q,  h,  h,   q,  q,  q,   q,  q,  q,  q, q};

void play1() {
  for (int count = 0; count < (sizeof(intro) / sizeof(intro[0])); count++) {
    tone(buzzer, intro[count], timeIntro[count]);
    int pauseBetweenNotes = timeIntro[count] * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzer);
  }
}

void play2() {
  for (int count = 0; count < (sizeof(start) / sizeof(start[0])); count++) {
    tone(buzzer, start[count], timeStart[count]);
    int pauseBetweenNotes = timeStart[count] * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzer);
  }
  for (int count = 0; count < (sizeof(part1) / sizeof(part1[0])); count++) {
    tone(buzzer, part1[count], timePart1[count]);
    int pauseBetweenNotes = timePart1[count] * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzer);
  }
}

void setup() {
  availableNetwork();
  DateTime now (time(NULL));
  DateTime nowCorrected (now.unixtime() - getTimeCorrection(now));
  int startHr = nowCorrected.hour();
  int startMins = (nowCorrected.minute() / 60.0);
  int start = (startHr + startMins) * 16;
  lastMinute = nowCorrected.minute();
  lastDay = nowCorrected.day();
  
  Display.begin();
  TouchDetector.begin();
  rgb.begin();

  static lv_style_t bg_style;
  lv_style_init(&bg_style);
  lv_style_set_bg_color(&bg_style, lv_palette_darken(LV_PALETTE_PURPLE, 4));
  
  static lv_coord_t col_dsc[] = {75, 430, 225, LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {75, 260, 75, LV_GRID_TEMPLATE_LAST};
  lv_obj_t * grid = lv_obj_create(lv_scr_act());
  lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
  lv_obj_set_size(grid, Display.width(), Display.height());
  lv_obj_add_style(grid, &bg_style, 0);

  lv_theme_t * theme = lv_theme_default_init(NULL, lv_palette_lighten(LV_PALETTE_DEEP_PURPLE, 1), lv_palette_darken(LV_PALETTE_LIGHT_BLUE, 2), false, &lv_font_montserrat_18);
  lv_display_set_theme(NULL, theme);
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_text_font(&style, &lv_font_montserrat_18); 
  static lv_style_t style2;
  lv_style_init(&style2);
  lv_style_set_text_font(&style2, &lv_font_montserrat_24);

  obj1 = lv_obj_create(grid);
  lv_obj_set_grid_cell(obj1, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_t * label = lv_label_create(obj1);
  lv_label_set_text(label, getLocaltime().c_str());
  lv_obj_add_style(label, &style2, 0);
  lv_obj_set_align(label, LV_ALIGN_CENTER);

  lv_obj_t * cal = lv_calendar_create(grid);
  lv_obj_add_style(cal, &style, 0);
  lv_calendar_set_today_date(cal, nowCorrected.year(), nowCorrected.month(), nowCorrected.day());
  lv_calendar_set_showed_date(cal, nowCorrected.year(), nowCorrected.month());
  lv_calendar_header_arrow_create(cal);
  lv_obj_add_event_cb(cal, change_date_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_set_grid_cell(cal, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 2);

  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, 2);
  lv_style_set_line_color(&style_line, lv_color_black());

  lv_style_init(&style_occupied_line);
  lv_style_set_line_width(&style_occupied_line, 5);
  lv_style_set_line_color(&style_occupied_line, lv_palette_lighten(LV_PALETTE_CYAN, 2));

  lv_style_init(&style_commute_line);
  lv_style_set_line_width(&style_commute_line, 5);
  lv_style_set_line_color(&style_commute_line, lv_palette_darken(LV_PALETTE_TEAL, 2));

  lv_style_init(&style_work_line);
  lv_style_set_line_width(&style_work_line, 5);
  lv_style_set_line_color(&style_work_line, lv_palette_lighten(LV_PALETTE_DEEP_PURPLE, 1));
  
  static lv_style_t style_now_line;
  lv_style_init(&style_now_line);
  lv_style_set_line_width(&style_now_line, 3);
  lv_style_set_line_color(&style_now_line, lv_palette_darken(LV_PALETTE_LIGHT_BLUE, 2));

  scale_obj = lv_obj_create(grid);
  lv_obj_set_grid_cell(scale_obj, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 3);
  lv_obj_t * scale = lv_line_create(scale_obj);
  static lv_point_precise_t scale_points[] = { {0, 0}, {0, (16 * 24)} };
  lv_line_set_points(scale, scale_points, 2);
  lv_obj_add_style(scale, &style_line, 0);

  lv_obj_t * occupied1 = lv_line_create(scale_obj);
  lv_obj_t * occupied2 = lv_line_create(scale_obj);
  lv_obj_t * occupied3 = lv_line_create(scale_obj);
  lv_obj_t * occupied4 = lv_line_create(scale_obj);
  lv_obj_t * occupied5 = lv_line_create(scale_obj);
  lv_obj_t * occupied6 = lv_line_create(scale_obj);
  lv_obj_t * occupied7 = lv_line_create(scale_obj);
  lv_obj_t * occupied8 = lv_line_create(scale_obj);
  lv_obj_t * occupied9 = lv_line_create(scale_obj);
  lv_obj_t * occupied10 = lv_line_create(scale_obj);
  lv_obj_t * occupied11 = lv_line_create(scale_obj);
  lv_obj_t * occupied12 = lv_line_create(scale_obj);
  lv_obj_t * occupied13 = lv_line_create(scale_obj);
  lv_obj_t * occupied14 = lv_line_create(scale_obj);
  lv_obj_t * occupied15 = lv_line_create(scale_obj);
  lv_obj_t * occupied16 = lv_line_create(scale_obj);
  lv_obj_t * occupied17 = lv_line_create(scale_obj);
  lv_obj_t * occupied18 = lv_line_create(scale_obj);
  lv_obj_t * occupied19 = lv_line_create(scale_obj);
  lv_obj_t * occupied20 = lv_line_create(scale_obj);
  lv_obj_t * occupied21 = lv_line_create(scale_obj);
  lv_obj_t * occupied22 = lv_line_create(scale_obj);
  lv_obj_t * occupied23 = lv_line_create(scale_obj);
  lv_obj_t * occupied24 = lv_line_create(scale_obj);

  lv_obj_t * scale1 = lv_line_create(scale_obj);
  static lv_point_precise_t scale_points1[] = { {0, 0}, {7, 0} };
  lv_line_set_points(scale1, scale_points1, 2);
  lv_obj_add_style(scale1, &style_line, 0);
  lv_obj_t * scale2 = lv_line_create(scale_obj);
  static lv_point_precise_t scale_points2[] = { {0, (16 * 6)}, {7, (16 * 6)} };
  lv_line_set_points(scale2, scale_points2, 2);
  lv_obj_add_style(scale2, &style_line, 0);
  lv_obj_t * scale3 = lv_line_create(scale_obj);
  static lv_point_precise_t scale_points3[] = { {0, (16 * 12)}, {7, (16 * 12)} };
  lv_line_set_points(scale3, scale_points3, 2);
  lv_obj_add_style(scale3, &style_line, 0);
  lv_obj_t * scale4 = lv_line_create(scale_obj);
  static lv_point_precise_t scale_points4[] = { {0, (16 * 18)}, {7, (16 * 18)} };
  lv_line_set_points(scale4, scale_points4, 2);
  lv_obj_add_style(scale4, &style_line, 0);
  lv_obj_t * scale5 = lv_line_create(scale_obj);
  static lv_point_precise_t scale_points5[] = { {0, (16 * 24)}, {7, (16 * 24)} };
  lv_line_set_points(scale5, scale_points5, 2);
  lv_obj_add_style(scale5, &style_line, 0);
  now_line = lv_line_create(scale_obj);
  static lv_point_precise_t now_points[] = { {0, ((16 * 24) - start)}, {28, ((16 * 24) - start)} };
  lv_line_set_points(now_line, now_points, 2);
  lv_obj_add_style(now_line, &style_now_line, 0);

  lv_style_init(&task_item_style);
  lv_style_set_pad_ver(&task_item_style, 5);
  lv_style_set_width(&task_item_style, lv_pct(100));
  lv_style_set_border_color(&task_item_style, lv_palette_lighten(LV_PALETTE_CYAN, 3));
  lv_style_set_border_width(&task_item_style, 2);
  lv_style_set_border_side(&task_item_style, LV_BORDER_SIDE_BOTTOM);

  lv_obj_t * btn1 = lv_btn_create(grid); 
  lv_obj_set_size(btn1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_add_event_cb(btn1, btn_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t * label1 = lv_label_create(btn1);
  lv_label_set_text(label1, "Reminders");
  lv_obj_add_style(label1, &style2, 0);
  lv_obj_center(label1);
  lv_obj_set_grid_cell(btn1, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

  list = lv_list_create(grid);
  lv_obj_set_grid_cell(list, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
  lv_list_add_text(list, "Tasks:");

  calAnalysis(callAPICalendar(time(NULL), appointmentCal), dailyEvents, false, dailyEventTimes, &style_occupied_line); // keepEvents defines whether previous events, of any kind, should be kept on the calendar line: false if first call, true if not
  calAnalysis(callAPICalendar(time(NULL), commuteCal), NULL, true, commuteTimes, &style_commute_line);
  calAnalysis(callAPICalendar(time(NULL), workCal), workNames, true, workTimes, &style_work_line);
  taskAnalysis(callAPITasks());
}

void loop() {
  lv_timer_handler();
  DateTime now (time(NULL));
  DateTime nowCorrected (now.unixtime() - getTimeCorrection(now));
  DateTime nextRep (nextPlay);
  lv_obj_t * label = lv_obj_get_child(obj1, 0);
  if (nowCorrected.minute() != lastMinute) { // this adjusts the line indicating the current time on the calendar line
    int startHr = nowCorrected.hour();
    float startMins = (nowCorrected.minute() / 60.0);
    int start = (startHr + startMins) * 16;
    now_points[0] = {0, ((16 * 24) - start)};
    now_points[1] = {25, ((16 * 24) - start)};
    lv_line_set_points(now_line, now_points, 2);
    lv_label_set_text(label, getLocaltime().c_str());
    lastMinute = nowCorrected.minute();
  }
  if (nowCorrected.day() != lastDay) {
    calAnalysis(callAPICalendar(time(NULL), appointmentCal), dailyEvents, false, dailyEventTimes, &style_occupied_line);
    calAnalysis(callAPICalendar(time(NULL), commuteCal), NULL, true, commuteTimes, &style_commute_line);
    calAnalysis(callAPICalendar(time(NULL), workCal), workNames, true, workTimes, &style_work_line);
    taskAnalysis(callAPITasks());
  }
  String currentEvent = findEvent(now.unixtime());
  if (currentEvent != "NULL") {
    play1();
    rgb.on(255, 0, 0);
    msg = lv_msgbox_create(NULL);
    lv_msgbox_add_title(msg, "Event:");
    lv_msgbox_add_text(msg, currentEvent.c_str());
    btn = lv_msgbox_add_footer_button(msg, "Okay!");
    lv_obj_add_event_cb(btn, close_msg_cb, LV_EVENT_CLICKED, msg);
    boxUp = true;
    lv_timer_handler();
    nextPlay = inFiveMin.unixtime();
    play2();
  }
  if (boxUp == true && nowCorrected.unixtime() >= nextRep.unixtime()) { // play the tune again in case the notification wasn't acknowledged
    DateTime inFiveMin (nowCorrected.unixtime() + (5 * 60));
    nextPlay = inFiveMin.unixtime();
    play2();
  }
}

void availableNetwork() {
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass); // this is in secrets.h; there is a way to add connections to more networks, but this is the basics
    delay(1000);
  }

  if (status == WL_CONNECTED) {
    setNtpTime();
  }
}

void setNtpTime() {
  Udp.begin(2390);
  sendNTPpacket(timeServer);
  delay(1000);
  parseNtpPacket();
}

unsigned long sendNTPpacket(const char * address) {
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;
    packetBuffer[1] = 0;
    packetBuffer[2] = 6;
    packetBuffer[3] = 0xEC;
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    Udp.beginPacket(address, 123);
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

unsigned long parseNtpPacket() {
  if (!Udp.parsePacket()) {
    return 0;
  }

  Udp.read(packetBuffer, NTP_PACKET_SIZE);
  const unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  const unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  const unsigned long secsSince1900 = highWord << 16 | lowWord;
  constexpr unsigned long seventyYears = 2208988800UL;
  const unsigned long epoch = secsSince1900 - seventyYears;
  set_time(epoch);
  return epoch;
}

uint32_t getTimeCorrection(DateTime time) { // this is adjusting the time to account for timezones; all the if statements are for daylight savings considerations
  int year = time.year();
  int hour = time.hour();
  if (time.month() == 1 || time.month() == 2 || time.month() == 12) {
    return (estCorrection);
  } else if (time.month() == 4 || time.month() == 5 || time.month() == 6 || time.month() == 7 || time.month() == 8 || time.month() == 9 || time.month() == 10) {
    return (edtCorrection);
  } else if (time.month() == 3) {
    for (int i = 8; i <= 14; i++) {
      DateTime test (year, 3, i, 2, 0, 0);
      if (test.dayOfTheWeek() == 0) {
        if (time <= test) {
          return (estCorrection);
        } else if (time > test) {
          return (edtCorrection);
        }
      }
    }
  } else if (time.month() == 11) {
    for (int i = 1; i <= 7; i++) {
      DateTime test (year, 11, i, 2, 0, 0);
      if (test.dayOfTheWeek() == 0) {
        if (time <= test) {
          return (edtCorrection);
        } else if (time > test) {
          return (estCorrection);
        }
      }
    }
  }
}

String callAPICalendar(uint32_t unixTime, String calToCall) {
  if (WiFi.status() == WL_CONNECTED) {
    HttpClient client = HttpClient(googleClient, "script.google.com", 443);
    client.get("/macros/s/" + calToCall + "/exec?theDate=" + ((String)unixTime) + "000"); // the + 000 is because google measures unixtime in milliseconds

    int responseCode = client.responseStatusCode();
    String response = client.responseBody();

    client.stop();

    if (responseCode == 200) {
      return response.substring((response.indexOf("%!!!") + 4), response.indexOf("!!!%"));
    } else {
      return "";
    }
  }
  return "";
}

String callAPITasks() {
  if (WiFi.status() == WL_CONNECTED) {
    HttpClient client = HttpClient(googleClient, "script.google.com", 443);
    client.get("/macros/s/" + taskList + "/exec");

    int responseCode = client.responseStatusCode();
    String response = client.responseBody();

    client.stop();

    if (responseCode == 200) {
      return response.substring((response.indexOf("%!!!") + 4), response.indexOf("!!!%"));
    } else {
      return "";
    }
  }
  return "";
}

void calAnalysis(String data, String eventNames[24], bool keepEvents, uint32_t events[24][2], lv_style_t* style) {
  if (data != "!!" && data != "") {
    int firstIndex = 0;
    int secondIndex = data.indexOf("; ");
    int count = 0;
    while (secondIndex != -1) {
      String victim = data.substring(firstIndex, secondIndex); // I was tired when I named this variable, but it's funny (at least, to me), so it stays
      if (eventNames != NULL) {
        eventNames[count] = victim.substring(0, victim.indexOf("("));
      }
      String time1 = victim.substring((victim.indexOf("(") + 1), victim.indexOf(" - ", victim.indexOf("(")));
      events[count][0] = (strtoull(time1.c_str(), NULL, 0) / 1000);
      String time2 = victim.substring((victim.indexOf(" - ", victim.indexOf("(")) + 3), victim.indexOf(")"));
      events[count][1] = (strtoull(time2.c_str(), NULL, 0) / 1000);
      firstIndex = secondIndex + 2;
      secondIndex = data.indexOf("; ", (secondIndex + 2));
      count++;
    }
  }  else {
    for (int i = 0; i < 24; i++) {
      events[i][0] = 0;
      events[i][1] = 0;
    }
  }
  setScale(events, keepEvents, style);
}

void taskAnalysis(String data) {
  if (data != "!!" && data != "") {
    int firstIndex = 0;
    int secondIndex = data.indexOf("; ");
    while (secondIndex != -1) {
      String victim = data.substring(firstIndex, secondIndex);
      lv_obj_t * cb = lv_checkbox_create(list);
      String labelText = victim.substring(0, victim.indexOf("("));
      int offset = 0;
      while (((labelText.length() - offset) + 1) > 13) {
        labelText = labelText.substring(0, labelText.lastIndexOf(" ", (offset + 12))) + "\n" + labelText.substring(labelText.lastIndexOf(" ", (offset + 12)) + 1);
        offset = labelText.lastIndexOf("\n") + 2;
      }
      lv_checkbox_set_text(cb, labelText.c_str());
      String id = victim.substring((victim.indexOf("(") + 1), victim.indexOf(")"));
      lv_obj_t * label = lv_label_create(cb);
      lv_label_set_text(label, id.c_str());
      lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_event_cb(cb, checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
      lv_obj_add_style(cb, &task_item_style, 0);
      firstIndex = secondIndex + 2;
      secondIndex = data.indexOf("; ", (secondIndex + 2));
    }
  }
}

void setScale(uint32_t events[24][2], bool keepEvents, lv_style_t* style) {
  int arrayIndex = 0;
  for (int i = 0; i < 24; i++) {
    lv_obj_t * line1 = lv_obj_get_child(scale_obj, (i + 1));
    if (keepEvents && lv_obj_has_flag(line1, LV_OBJ_FLAG_WIDGET_1)) {
      continue;
    }
    if (events[arrayIndex][0] != 0) {
      DateTime unStartUnix (events[arrayIndex][0]);
      DateTime unEndUnix (events[arrayIndex][1]);
      DateTime startUnix (unStartUnix.unixtime() - getTimeCorrection(unStartUnix));
      DateTime endUnix (unEndUnix.unixtime() - getTimeCorrection(unEndUnix));
      int startHr = startUnix.hour();
      float startMins = (startUnix.minute() / 60.0);
      int start = (startHr + startMins) * 16;
      int endHr = endUnix.hour();
      float endMins = (endUnix.minute() / 60.0);
      int end = (endHr + endMins) * 16;
      allPoints[i][0] = {2, ((16 * 24) - start)};
      allPoints[i][1] = {2, ((16 * 24) - end)};
      lv_obj_add_flag(line1, LV_OBJ_FLAG_WIDGET_1);
      lv_line_set_points(line1, allPoints[i], 2);
      lv_obj_add_style(line1, style, 0);
    } else if (events[arrayIndex][0] == 0 && !keepEvents) {
      allPoints[i][0] = {0, 0};
      allPoints[i][1] = {0, 0};
      lv_obj_remove_flag(line1, LV_OBJ_FLAG_WIDGET_1);
      lv_line_set_points(line1, allPoints[i], 2);
      lv_obj_add_style(line1, &style_line, 0);
    }
    arrayIndex++;
  }
}

String findEvent(uint32_t now) {
  for (int i = 0; i < 24; i++) {
    if (dailyEventTimes[i][0] <= now && dailyEventTimes[i][1] >= now) {
      dailyEventTimes[i][0] = 0;
      dailyEventTimes[i][1] = 0;
      return dailyEvents[i];
    }
  }
  for (int i = 0; i < 24; i++) {
    if (workTimes[i][0] <= now && workTimes[i][1] >= now) {
      workTimes[i][0] = 0;
      workTimes[i][1] = 0;
      return workNames[i];
    }
  }
  return "NULL";
}

String getLocaltime() {
  char buffer[32];
  tm t;
  _rtc_localtime((time(NULL) - getTimeCorrection(time(NULL))), &t, RTC_4_YEAR_LEAP_YEAR_SUPPORT);
  strftime(buffer, 32, "%m/%d/%Y %k:%M", &t);
  return String(buffer);
}
