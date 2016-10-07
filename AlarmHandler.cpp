#include "AlarmHandler.h"
#include "FS.h"

// Global
AlarmHandler alarmHandler = AlarmHandler();

AlarmHandler::AlarmHandler():
    paused(false)
{
    for (int i = 0; i < MAX_ALARMS; i++)
    {
        alarms[i].id = dtINVALID_ALARM_ID;
    }
}

bool AlarmHandler::parse_timer_string(String command,
        String options[],
        int &hour,
        int &minute,
        int &duration)
{
    String temp = "";
    int current_option = 0;
    bool success = false;

    parse_state state = unknown;

    for (int i = 0; i < command.length(); i++)
    {
        // Really simple url decoding, just get rid of it
        if (command.charAt(i) == '%')
        {
            command.setCharAt(i, ' ');
            command.setCharAt(i+1, ' ');
            command.setCharAt(i+2, ' ');
            i+=2;
        }
    }

    for (int i = 0; i < command.length(); i++)
    {
        switch (state)
        {
            case option:
                if (command.charAt(i) == ' ')
                {
                    // End of option
                    if (current_option == MAX_OPTIONS)
                    {
                        // Just skip it
                        continue;
                    }
                    options[current_option++] = temp;
                    temp = "";
                    state = unknown;
                }
                else
                {
                    temp += command.charAt(i);
                }
                break;
            case time_unknown:
                if (command.charAt(i) == ':')
                {
                    // End of the hour
                    hour = temp.toInt();
                    // Fix the issue with 24 hour clocks using 0-23
                    hour = hour % 12;
                    state = time_minute;
                    temp = "";
                }
                else if (command.charAt(i) < '0' || command.charAt(i) > '9')
                {
                    // Reached the end of the time without hitting :, must be duration
                    duration = temp.toInt();
                    success = true;
                    state = option;
                    temp = "";
                    temp += command.charAt(i);
                }
                else
                {
                    temp += command.charAt(i);
                }
                break;
            case time_minute:
                if (command.charAt(i) < '0' || command.charAt(i) > '9')
                {
                    // Reached the end of the minute
                    minute = temp.toInt();
                    state = time_ampm;
                    temp = "";
                    i--; // Re-process this character in ampm state
                }
                else
                {
                    temp += command.charAt(i);
                }
                break;
            case time_ampm:
                if (command.length() >= i+2)
                {
                    String sub = command.substring(i, i+2);
                    if (sub.equalsIgnoreCase("PM"))
                    {
                        // Convert to 24 hour time
                        hour += 12;
                    }
                    if (sub.equalsIgnoreCase("AM") || sub.equalsIgnoreCase("PM"))
                    {
                        // If AM/PM is found, skip it and move to unknown
                        i += 2;
                        success = true;
                        state = unknown;
                    }
                    // Otherwise, there could be a space before the AM/PM. Keep parsing
                }
                else
                {
                    state = unknown;
                    i--; //Re-process this character
                }
                break;
            case unknown:
                if (command.charAt(i) == ' ')
                {
                    // Do nothing, skip the space
                }
                else if (command.charAt(i) < '0' || command.charAt(i) > '9')
                {
                    // Not a number, must be beginning of an option
                    state = option;
                    temp = "";
                    temp += command.charAt(i);
                }
                else if (command.charAt(i) >= '0' && command.charAt(i) <= '9')
                {
                    state = time_unknown;
                    temp = "";
                    temp += command.charAt(i);
                }
                break;
        }
    }
    // Now handle the last temp buffer, if needed
    switch (state)
    {
        case option:
            // End of option
            if (current_option != MAX_OPTIONS)
            {
                options[current_option++] = temp;
            }
            break;
        case time_unknown:
            // Reached the end of the time without hitting :, must be duration
            duration = temp.toInt();
            success = true;
            break;
    }

    if (success != true)
    {
        Serial.println("Unable to parse: " + command);
    }
    else
    {
        if (duration != 0)
        {
            Serial.println("Parsed duration: " + String(duration));
        }
        else
        {
            Serial.println("Parsed time: " + String(hour) + ":" + String(minute));
        }
    }

    return success;
}

bool AlarmHandler::add_new_timer(
        String options[],
        int &hour,
        int &minute,
        int &duration,
        OnTick_t func,
        String func_action)
{
    bool repeating = false;
    AlarmID_t id = 0;
    time_t trigger_time = 0;
    String action = " - " + func_action;

    // Parse the options for timer-specific ones
    for (int i = 0; i < MAX_OPTIONS; i++)
    {
        if (options[i].equalsIgnoreCase("R"))
        {
            repeating = true;
            action += " (Repeating)";
        }
    }

    if (duration == 0)
    {
        // Alarm
        time_t alarm_time = hour * SECS_PER_HOUR + minute * SECS_PER_MIN;
        time_t current_time = now();

        if (repeating)
        {
            id = Alarm.alarmRepeat(alarm_time, func);
        }
        else
        {
            id = Alarm.alarmOnce(alarm_time, func);
        }

        if (elapsedSecsToday(current_time) < alarm_time)
        {
            // Alarm still happens today
            trigger_time = previousMidnight(current_time) + alarm_time;
        }
        else
        {
            // Alarm happens tomorrow
            trigger_time = nextMidnight(current_time) + alarm_time;
        }
    }
    else
    {
        // Timer
        id = Alarm.timerOnce(duration*60, func);
        trigger_time = now() + duration*60;
    }

    if (id == dtINVALID_ALARM_ID)
    {
        Serial.println("Error adding alarm");
        return false;
    }

    return addToLocalList(id, trigger_time, action, repeating);
}

void AlarmHandler::cancelAllAlarms()
{
    for (int i = 0; i < MAX_ALARMS; i++)
    {
        if (alarms[i].id != dtINVALID_ALARM_ID)
        {
            cancel(alarms[i].id);
            alarms[i].id = dtINVALID_ALARM_ID;
        }
    }
    paused = false;
}

void AlarmHandler::pauseAllAlarms()
{
    for (int i = 0; i < MAX_ALARMS; i++)
    {
        if (alarms[i].id != dtINVALID_ALARM_ID)
        {
            if (paused == true)
            {
                Alarm.enable(alarms[i].id);
            }
            else
            {
                Alarm.disable(alarms[i].id);
            }
        }
    }
    paused = !paused;
}

bool AlarmHandler::addToLocalList(AlarmID_t id, time_t trigger_time, String action, bool repeating)
{
    int i = 0;
    for (i = 0; i < MAX_ALARMS; i++)
    {
        if (alarms[i].id == dtINVALID_ALARM_ID)
        {
            alarms[i].id = id;
            alarms[i].trigger_time = trigger_time;
            alarms[i].action = action;
            alarms[i].repeating = repeating;
            break;
        }
    }
    if (i == MAX_ALARMS)
    {
        Serial.println("Already have used all available alarms");
        return false;
    }
    return true;
}

void AlarmHandler::cancel(AlarmID_t id)
{
    Alarm.disable(id);
    Alarm.free(id);
}

void AlarmHandler::updateActiveAlarms(String &activeAlarms)
{
    if (Alarm.count() == 0)
    {
        activeAlarms = "None";
    }
    else
    {
        activeAlarms = printAlarms();
    }
}

String AlarmHandler::printAlarms()
{
    alarm_info_t *active_alarms[MAX_ALARMS];
    int current_alarm = 0;

    time_t current_time = now();

    // Save a pointer to all active alarms
    for (int i = 0; i < MAX_ALARMS; i++)
    {
        if (alarms[i].id != dtINVALID_ALARM_ID)
        {
            if (alarms[i].trigger_time < current_time)
            {
                if (alarms[i].repeating)
                {
                    // This alarm repeats. Update the trigger time
                    alarms[i].trigger_time += SECS_PER_DAY;
                }
                else
                {
                    // This alarm has already triggered, remove it
                    alarms[i].id = dtINVALID_ALARM_ID;
                    continue;
                }
            }
            active_alarms[current_alarm] = &(alarms[i]);
            current_alarm++;
        }
    }
    
    if (current_alarm == 0)
    {
        // No active alarms
        return "None";
    }

    // Sort active_alarms by trigger time
    // Code copied from Running Median example
    for (uint8_t i=0; i< current_alarm-1; i++)
    {
        uint8_t m = i;
        for (uint8_t j=i+1; j< current_alarm; j++)
        {
            if (active_alarms[j]->trigger_time < active_alarms[m]->trigger_time)
            {
                m = j;
            }
        }
        if (m != i)
        {
            alarm_info_t *t = active_alarms[m];
            active_alarms[m] = active_alarms[i];
            active_alarms[i] = t;
        }
    }

    String output = displayAlarmAsString(active_alarms[0]);
    // newline for separator
    String sep = "<br>";
    //String sep = ", ";
    for (int i = 1; i < current_alarm; i++)
    {
        output += sep + displayAlarmAsString(active_alarms[i]);
    }

    //Serial.println("DEBUG: printAlarms: ");
    //Serial.println(output);
    return output;
}

String AlarmHandler::displayAlarmAsString(alarm_info_t *alarm)
{
    time_t t = alarm->trigger_time;
    String output = String(month(t)) + "/" + String(day(t)) + " at " + String(hourFormat12(t)) + printDigits(minute(t)) + (isAM(t) ? " AM" : " PM") + alarm->action;
    if (paused)
    {
        output += " (PAUSED)";
    }
    return output;
}

String AlarmHandler::printDigits(int digits)
{
    // utility for digital clock display: prints preceding colon and leading 0
    String output = ":";
    if(digits < 10)
        output += "0";
    output += String(digits);

    return output;
}

void AlarmHandler::saveAlarmsToDisk()
{
    int num_alarms = 0;
    
    // Count alarms
    for (int i = 0; i < MAX_ALARMS; i++)
    {
        if (alarms[i].id != dtINVALID_ALARM_ID)
        {
            num_alarms++;
        }
    }

    if (num_alarms <= 0)
    {
        Serial.println("No alarms to save");
        return;
    }
    Serial.println("====== Writing to SPIFFS file =========");
    // open file for writing
    File f = SPIFFS.open(ALARM_FILE, "w");
    if (!f)
    {
        Serial.println("file open failed");
        return;
    }
    f.println(ALARM_FILE_VERSION);
    f.println(num_alarms);
    for (int i = 0; i < MAX_ALARMS; i++)
    {
        if (alarms[i].id != dtINVALID_ALARM_ID)
        {
            f.println(alarms[i].id);
            f.println(alarms[i].trigger_time);
            f.println(alarms[i].action);
            f.println((alarms[i].repeating ? "TRUE" : "FALSE"));
        }
    }

    f.close();

}

void AlarmHandler::loadAlarmsFromDisk()
{
    // open file for reading
    String entry;
    int num_alarms = 0;

    Serial.println("====== Cancel existing alarms =======");
    cancelAllAlarms();

    f = SPIFFS.open(ALARM_FILE, "r");
    if (!f)
    {
        Serial.println("file open failed");
        return;
    }
    Serial.println("====== Reading from SPIFFS file =======");
    entry = f.readStringUntil('\n');
    if (entry == String(ALARM_FILE_VERSION_V1))
    {
        num_alarms = f.readStringUntil('\n').toInt()

        if (num_alarms > MAX_ALARMS)
        {
            num_alarms = MAX_ALARMS;
            Serial.println(String("Too many alarms, resetting to ") + MAX_ALARMS);
        }

        for (int i=0; i < num_alarms; i++)
        {
            // add each alarm
            alarms[i].id = f.readStringUntil('\n').toInt();
            alarms[i].trigger_time = f.readStringUntil('\n').toInt();
            alarms[i].action = f.readStringUntil('\n');
            entry = f.readStringUntil('\n');
            alarms[i].repeating = (entry.equals("TRUE") ? true : false);
        }
    }
    else
    {
        Serial.println(String("Invalid alarm file version: ") + entry);
    }

    f.close();
}
