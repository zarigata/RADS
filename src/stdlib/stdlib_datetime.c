#include "stdlib_datetime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// Helper: Create datetime struct instance
static Value make_datetime_struct(int year, int month, int day, int hour, int minute, int second, long long unix_ts) {
    // Create struct instance with datetime fields
    StructInstance* instance = malloc(sizeof(StructInstance));
    if (!instance) return make_null();
    
    instance->fields = NULL;
    
    // Allocate and set year field
    FieldValue* year_field = malloc(sizeof(FieldValue));
    year_field->name = strdup("year");
    year_field->value = malloc(sizeof(Value));
    year_field->value->type = VAL_INT;
    year_field->value->int_val = year;
    year_field->next = instance->fields;
    instance->fields = year_field;
    
    // Allocate and set month field
    FieldValue* month_field = malloc(sizeof(FieldValue));
    month_field->name = strdup("month");
    month_field->value = malloc(sizeof(Value));
    month_field->value->type = VAL_INT;
    month_field->value->int_val = month;
    month_field->next = instance->fields;
    instance->fields = month_field;
    
    // Allocate and set day field
    FieldValue* day_field = malloc(sizeof(FieldValue));
    day_field->name = strdup("day");
    day_field->value = malloc(sizeof(Value));
    day_field->value->type = VAL_INT;
    day_field->value->int_val = day;
    day_field->next = instance->fields;
    instance->fields = day_field;
    
    // Allocate and set hour field
    FieldValue* hour_field = malloc(sizeof(FieldValue));
    hour_field->name = strdup("hour");
    hour_field->value = malloc(sizeof(Value));
    hour_field->value->type = VAL_INT;
    hour_field->value->int_val = hour;
    hour_field->next = instance->fields;
    instance->fields = hour_field;
    
    // Allocate and set minute field
    FieldValue* minute_field = malloc(sizeof(FieldValue));
    minute_field->name = strdup("minute");
    minute_field->value = malloc(sizeof(Value));
    minute_field->value->type = VAL_INT;
    minute_field->value->int_val = minute;
    minute_field->next = instance->fields;
    instance->fields = minute_field;
    
    // Allocate and set second field
    FieldValue* second_field = malloc(sizeof(FieldValue));
    second_field->name = strdup("second");
    second_field->value = malloc(sizeof(Value));
    second_field->value->type = VAL_INT;
    second_field->value->int_val = second;
    second_field->next = instance->fields;
    instance->fields = second_field;
    
    // Allocate and set unix field
    FieldValue* unix_field = malloc(sizeof(FieldValue));
    unix_field->name = strdup("unix");
    unix_field->value = malloc(sizeof(Value));
    unix_field->value->type = VAL_INT;
    unix_field->value->int_val = unix_ts;
    unix_field->next = instance->fields;
    instance->fields = unix_field;
    
    Value result;
    result.type = VAL_STRUCT_INSTANCE;
    result.struct_instance = instance;
    return result;
}

// datetime.now() - Current timestamp
Value stdlib_datetime_now(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    return make_datetime_struct(
        tm_info->tm_year + 1900,
        tm_info->tm_mon + 1,
        tm_info->tm_mday,
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec,
        (long long)now
    );
}

// datetime.from_unix(unix_timestamp) - Create datetime from Unix timestamp
Value stdlib_datetime_from_unix(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: datetime.from_unix() requires 1 argument (unix timestamp)\n");
        return make_null();
    }
    
    if (args[0].type != VAL_INT) {
        fprintf(stderr, "Error: datetime.from_unix() argument must be an integer\n");
        return make_null();
    }
    
    time_t timestamp = (time_t)args[0].int_val;
    struct tm* tm_info = localtime(&timestamp);
    
    return make_datetime_struct(
        tm_info->tm_year + 1900,
        tm_info->tm_mon + 1,
        tm_info->tm_mday,
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec,
        (long long)timestamp
    );
}

// datetime.format(datetime_obj, format_string) - Format datetime to string
Value stdlib_datetime_format(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: datetime.format() requires 2 arguments (datetime, format)\n");
        return make_null();
    }
    
    if (args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: datetime.format() format must be a string\n");
        return make_null();
    }
    
    // Extract datetime fields
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    
    if (args[0].type == VAL_STRUCT_INSTANCE) {
        StructInstance* inst = args[0].struct_instance;
        FieldValue* field = inst->fields;
        while (field) {
            if (strcmp(field->name, "year") == 0 && field->value->type == VAL_INT) {
                year = (int)field->value->int_val;
            } else if (strcmp(field->name, "month") == 0 && field->value->type == VAL_INT) {
                month = (int)field->value->int_val;
            } else if (strcmp(field->name, "day") == 0 && field->value->type == VAL_INT) {
                day = (int)field->value->int_val;
            } else if (strcmp(field->name, "hour") == 0 && field->value->type == VAL_INT) {
                hour = (int)field->value->int_val;
            } else if (strcmp(field->name, "minute") == 0 && field->value->type == VAL_INT) {
                minute = (int)field->value->int_val;
            } else if (strcmp(field->name, "second") == 0 && field->value->type == VAL_INT) {
                second = (int)field->value->int_val;
            }
            field = field->next;
        }
    }
    
    // Convert to time_t
    struct tm tm_info = {0};
    tm_info.tm_year = year - 1900;
    tm_info.tm_mon = month - 1;
    tm_info.tm_mday = day;
    tm_info.tm_hour = hour;
    tm_info.tm_min = minute;
    tm_info.tm_sec = second;
    
    // Format using strftime
    char* format_str = args[1].string_val;
    char buffer[256];
    
    // Convert RADS format to strftime format
    // YYYY -> %Y, MM -> %m, DD -> %d, HH -> %H, mm -> %M, ss -> %S
    char converted_format[512];
    int j = 0;
    for (int i = 0; format_str[i] && j < 510; i++) {
        if (strncmp(&format_str[i], "YYYY", 4) == 0) {
            strcpy(&converted_format[j], "%Y");
            j += 2;
            i += 3;
        } else if (strncmp(&format_str[i], "MM", 2) == 0) {
            strcpy(&converted_format[j], "%m");
            j += 2;
            i += 1;
        } else if (strncmp(&format_str[i], "DD", 2) == 0) {
            strcpy(&converted_format[j], "%d");
            j += 2;
            i += 1;
        } else if (strncmp(&format_str[i], "HH", 2) == 0) {
            strcpy(&converted_format[j], "%H");
            j += 2;
            i += 1;
        } else if (strncmp(&format_str[i], "mm", 2) == 0) {
            strcpy(&converted_format[j], "%M");
            j += 2;
            i += 1;
        } else if (strncmp(&format_str[i], "ss", 2) == 0) {
            strcpy(&converted_format[j], "%S");
            j += 2;
            i += 1;
        } else {
            converted_format[j++] = format_str[i];
        }
    }
    converted_format[j] = '\0';
    
    strftime(buffer, sizeof(buffer), converted_format, &tm_info);
    
    return make_string(buffer);
}

// datetime.parse(string, format) - Parse string to datetime
Value stdlib_datetime_parse(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: datetime.parse() requires 2 arguments (string, format)\n");
        return make_null();
    }
    
    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: datetime.parse() arguments must be strings\n");
        return make_null();
    }
    
    char* date_str = args[0].string_val;
    char* format_str = args[1].string_val;
    
    struct tm tm_info = {0};
    
    // Convert RADS format to strptime format and parse
    // This is a simplified parser for common formats
    // YYYY-MM-DD HH:mm:ss
    if (strstr(format_str, "YYYY-MM-DD") != NULL) {
        if (strstr(format_str, "HH:mm:ss") != NULL) {
            sscanf(date_str, "%d-%d-%d %d:%d:%d",
                   &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
                   &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec);
        } else {
            sscanf(date_str, "%d-%d-%d",
                   &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday);
        }
        tm_info.tm_year -= 1900;
        tm_info.tm_mon -= 1;
    }
    // DD/MM/YYYY
    else if (strstr(format_str, "DD/MM/YYYY") != NULL) {
        sscanf(date_str, "%d/%d/%d",
               &tm_info.tm_mday, &tm_info.tm_mon, &tm_info.tm_year);
        tm_info.tm_year -= 1900;
        tm_info.tm_mon -= 1;
    }
    // MM/DD/YYYY
    else if (strstr(format_str, "MM/DD/YYYY") != NULL) {
        sscanf(date_str, "%d/%d/%d",
               &tm_info.tm_mon, &tm_info.tm_mday, &tm_info.tm_year);
        tm_info.tm_year -= 1900;
        tm_info.tm_mon -= 1;
    }
    // Default: try ISO format
    else {
        sscanf(date_str, "%d-%d-%d",
               &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday);
        tm_info.tm_year -= 1900;
        tm_info.tm_mon -= 1;
    }
    
    time_t timestamp = mktime(&tm_info);
    
    return make_datetime_struct(
        tm_info.tm_year + 1900,
        tm_info.tm_mon + 1,
        tm_info.tm_mday,
        tm_info.tm_hour,
        tm_info.tm_min,
        tm_info.tm_sec,
        (long long)timestamp
    );
}

// datetime.unix(datetime_obj) - Get Unix timestamp
Value stdlib_datetime_unix(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: datetime.unix() requires 1 argument (datetime)\n");
        return make_null();
    }
    
    if (args[0].type == VAL_STRUCT_INSTANCE) {
        StructInstance* inst = args[0].struct_instance;
        FieldValue* field = inst->fields;
        while (field) {
            if (strcmp(field->name, "unix") == 0 && field->value->type == VAL_INT) {
                return make_int(field->value->int_val);
            }
            field = field->next;
        }
    }
    
    return make_int(0);
}

// datetime.year(datetime_obj), datetime.month(), etc. - Extract fields
Value stdlib_datetime_year(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRUCT_INSTANCE) return make_int(0);
    StructInstance* inst = args[0].struct_instance;
    FieldValue* field = inst->fields;
    while (field) {
        if (strcmp(field->name, "year") == 0) return make_int(field->value->int_val);
        field = field->next;
    }
    return make_int(0);
}

Value stdlib_datetime_month(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRUCT_INSTANCE) return make_int(0);
    StructInstance* inst = args[0].struct_instance;
    FieldValue* field = inst->fields;
    while (field) {
        if (strcmp(field->name, "month") == 0) return make_int(field->value->int_val);
        field = field->next;
    }
    return make_int(0);
}

Value stdlib_datetime_day(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRUCT_INSTANCE) return make_int(0);
    StructInstance* inst = args[0].struct_instance;
    FieldValue* field = inst->fields;
    while (field) {
        if (strcmp(field->name, "day") == 0) return make_int(field->value->int_val);
        field = field->next;
    }
    return make_int(0);
}

Value stdlib_datetime_hour(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRUCT_INSTANCE) return make_int(0);
    StructInstance* inst = args[0].struct_instance;
    FieldValue* field = inst->fields;
    while (field) {
        if (strcmp(field->name, "hour") == 0) return make_int(field->value->int_val);
        field = field->next;
    }
    return make_int(0);
}

Value stdlib_datetime_minute(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRUCT_INSTANCE) return make_int(0);
    StructInstance* inst = args[0].struct_instance;
    FieldValue* field = inst->fields;
    while (field) {
        if (strcmp(field->name, "minute") == 0) return make_int(field->value->int_val);
        field = field->next;
    }
    return make_int(0);
}

Value stdlib_datetime_second(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRUCT_INSTANCE) return make_int(0);
    StructInstance* inst = args[0].struct_instance;
    FieldValue* field = inst->fields;
    while (field) {
        if (strcmp(field->name, "second") == 0) return make_int(field->value->int_val);
        field = field->next;
    }
    return make_int(0);
}

// datetime.weekday(datetime_obj) - Day of week (0=Sunday, 6=Saturday)
Value stdlib_datetime_weekday(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: datetime.weekday() requires 1 argument (datetime)\n");
        return make_int(0);
    }
    
    // Extract unix timestamp from datetime object
    long long unix_ts = 0;
    if (args[0].type == VAL_STRUCT_INSTANCE) {
        StructInstance* inst = args[0].struct_instance;
        FieldValue* field = inst->fields;
        while (field) {
            if (strcmp(field->name, "unix") == 0 && field->value->type == VAL_INT) {
                unix_ts = field->value->int_val;
                break;
            }
            field = field->next;
        }
    }
    
    time_t timestamp = (time_t)unix_ts;
    struct tm* tm_info = localtime(&timestamp);
    
    return make_int(tm_info->tm_wday);  // 0=Sunday, 1=Monday, ..., 6=Saturday
}

// datetime.add(datetime_obj, days, hours, minutes, seconds) - Add time
Value stdlib_datetime_add(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: datetime.add() requires at least 2 arguments (datetime, days)\n");
        return make_null();
    }
    
    // Extract unix timestamp from datetime object
    long long unix_ts = 0;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    
    if (args[0].type == VAL_STRUCT_INSTANCE) {
        StructInstance* inst = args[0].struct_instance;
        FieldValue* field = inst->fields;
        while (field) {
            if (strcmp(field->name, "unix") == 0 && field->value->type == VAL_INT) {
                unix_ts = field->value->int_val;
            } else if (strcmp(field->name, "year") == 0) year = (int)field->value->int_val;
            else if (strcmp(field->name, "month") == 0) month = (int)field->value->int_val;
            else if (strcmp(field->name, "day") == 0) day = (int)field->value->int_val;
            else if (strcmp(field->name, "hour") == 0) hour = (int)field->value->int_val;
            else if (strcmp(field->name, "minute") == 0) minute = (int)field->value->int_val;
            else if (strcmp(field->name, "second") == 0) second = (int)field->value->int_val;
            field = field->next;
        }
    }
    
    // Get offset values (can be passed as struct or individual args)
    int days = 0, hours = 0, minutes = 0, seconds = 0;
    
    if (argc >= 2 && args[1].type == VAL_INT) days = (int)args[1].int_val;
    if (argc >= 3 && args[2].type == VAL_INT) hours = (int)args[2].int_val;
    if (argc >= 4 && args[3].type == VAL_INT) minutes = (int)args[3].int_val;
    if (argc >= 5 && args[4].type == VAL_INT) seconds = (int)args[4].int_val;
    
    // Add time
    time_t timestamp = (time_t)unix_ts;
    timestamp += days * 86400 + hours * 3600 + minutes * 60 + seconds;
    
    struct tm* tm_info = localtime(&timestamp);
    
    return make_datetime_struct(
        tm_info->tm_year + 1900,
        tm_info->tm_mon + 1,
        tm_info->tm_mday,
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec,
        (long long)timestamp
    );
}

// datetime.diff(datetime1, datetime2) - Difference in seconds
Value stdlib_datetime_diff(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: datetime.diff() requires 2 arguments (datetime1, datetime2)\n");
        return make_int(0);
    }
    
    long long unix1 = 0, unix2 = 0;
    
    // Extract from first datetime
    if (args[0].type == VAL_STRUCT_INSTANCE) {
        StructInstance* inst = args[0].struct_instance;
        FieldValue* field = inst->fields;
        while (field) {
            if (strcmp(field->name, "unix") == 0 && field->value->type == VAL_INT) {
                unix1 = field->value->int_val;
                break;
            }
            field = field->next;
        }
    }
    
    // Extract from second datetime
    if (args[1].type == VAL_STRUCT_INSTANCE) {
        StructInstance* inst = args[1].struct_instance;
        FieldValue* field = inst->fields;
        while (field) {
            if (strcmp(field->name, "unix") == 0 && field->value->type == VAL_INT) {
                unix2 = field->value->int_val;
                break;
            }
            field = field->next;
        }
    }
    
    return make_int(unix1 - unix2);
}

void stdlib_datetime_register(void) {
    register_native("datetime.now", stdlib_datetime_now);
    register_native("datetime.from_unix", stdlib_datetime_from_unix);
    register_native("datetime.format", stdlib_datetime_format);
    register_native("datetime.parse", stdlib_datetime_parse);
    register_native("datetime.unix", stdlib_datetime_unix);
    register_native("datetime.year", stdlib_datetime_year);
    register_native("datetime.month", stdlib_datetime_month);
    register_native("datetime.day", stdlib_datetime_day);
    register_native("datetime.hour", stdlib_datetime_hour);
    register_native("datetime.minute", stdlib_datetime_minute);
    register_native("datetime.second", stdlib_datetime_second);
    register_native("datetime.weekday", stdlib_datetime_weekday);
    register_native("datetime.add", stdlib_datetime_add);
    register_native("datetime.diff", stdlib_datetime_diff);
}