#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H

#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Arduino.h>

#define MAX_OPTIONS 5
#define MAX_ALARMS dtNBR_ALARMS //Need to set dtNBR_ALARMS in TimeAlarms.h, default is only 6
#define ALARM_FILE "alarm.bak"
#define ALARM_FILE_VERSION_V1 "v1"
#define ALARM_FILE_VERSION ALARM_FILE_VERSION_V1

// Rest UI variables
extern String current_state;
extern String activeAlarms;

// Alarm functions (defined in .ino)

void powerOn();
void powerOff();

typedef enum {
    unknown = 0,
    option,
    time_unknown,
    time_minute,
    time_ampm
} parse_state;

typedef struct {
    AlarmID_t id;
    time_t trigger_time;
    String action;
    bool repeating;
} alarm_info_t;

class AlarmHandler {

public:
    AlarmHandler();

    bool parse_timer_string(String command,
        String options[],
        int &hour,
        int &minute,
        int &duration);

    time_t alarm_time_to_trigger_time(time_t alarm_time);

    bool delete_timer(
        int &hour,
        int &minute);

    bool add_new_timer(
        String options[],
        int &hour,
        int &minute,
        int &duration,
        OnTick_t func,
        String func_action);

    void pauseAllAlarms();
    void cancelAllAlarms();

    void updateActiveAlarms(String &activeAlarms);

    void saveAlarmsToDisk();
    void loadAlarmsFromDisk();

    void addAlarmToManager(time_t alarm_time, OnTick_t func, bool repeating, AlarmID_t &id, time_t &trigger_time);
    bool cancelAlarmsAtTime(time_t trigger_time);
    void addAlarmIfStillValid(time_t trigger_time, String action, bool repeating, OnTick_t off_func, OnTick_t yellow_func, OnTick_t green_func);
    void saveAlarmsToDisk(String current_state);
    void loadAlarmsFromDisk(String &current_state, OnTick_t off_func, OnTick_t yellow_func, OnTick_t green_func);

private:
    bool addToLocalList(AlarmID_t id, time_t trigger_time, String action, bool repeating);
    void cancel(AlarmID_t id);

    String printAlarms();
    String displayAlarmAsString(alarm_info_t *alarm);
    String printDigits(int digits);

    alarm_info_t alarms[MAX_ALARMS];
    bool paused;

};

// Global instance
extern AlarmHandler alarmHandler;

#endif
