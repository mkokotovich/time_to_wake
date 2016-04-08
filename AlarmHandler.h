#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H

#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Arduino.h>

#define MAX_OPTIONS 5
#define MAX_ALARMS dtNBR_ALARMS //Need to set dtNBR_ALARMS in TimeAlarms.h, default is only 6

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
} alarm_info_t;

class AlarmHandler {

public:
    AlarmHandler();

    bool parse_timer_string(String command,
        String options[],
        int &hour,
        int &minute,
        int &duration);

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

private:
    bool addToLocalList(AlarmID_t id, time_t trigger_time, String action);
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
