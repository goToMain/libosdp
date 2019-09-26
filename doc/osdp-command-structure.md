# OSDP Command structures

LibOSDP exposed the following structures thought `osdp.h`. This document attempts
to document each of its members.

## Command LED

```
struct __osdp_cmd_led_params {
    uint8_t control_code;
    uint8_t on_count;
    uint8_t off_count;
    uint8_t on_color;
    uint8_t off_color;
    uint16_t timer;
};

struct osdp_cmd_led {
    uint8_t reader;
    uint8_t number;
    struct __osdp_cmd_led_params temporary;
    struct __osdp_cmd_led_params permanent;
};
```

**reader:** 0 = First Reader, 1 = Second Reader, etc.
**number:** 0 = first LED, 1 = second LED, etc.
**control_code:** see table below
**on_count:** The ON duration of the flash, in units of 100 ms
**off_count:** The OFF duration of the flash, in units of 100 ms
**on_color:** The color to set during the ON time
**off_color:** The color to set during the OFF time
**timer:** time in units of 100 ms (only for temporary mode)

### Temporary Control Code

| Control Code | Meaning                                                                              |
|:------------:|--------------------------------------------------------------------------------------|
| 0x00         | NOP - do not alter this LED's temporary settings                                     |
| 0x01         | Cancel any temporary operation and display this LED's permanent state immediately    |
| 0x02         | Set the temporary state as given and start timer immediately                         |

### Permanent Control Code

| Control Code | Meaning                                                                              |
|:------------:|--------------------------------------------------------------------------------------|
| 0x00         | NOP - do not alter this LED's permanent settings                                     |
| 0x01         | Set the permanent state as given                                                     |


## Command Output

```
struct osdp_cmd_output {
    uint8_t output_no;
    uint8_t control_code;
    uint16_t tmr_count;
};
```

**output_no:** 0 = First Output, 1 = Second Output, etc.
**control_code:** See blow table
**tmr_count:** Time in units of 100 ms

| Control Code | Meaning                                                            |
|:------------:|:-------------------------------------------------------------------|
| 0x00         | NOP – do not alter this output                                     |
| 0x01         | set the permanent state to OFF, abort timed operation (if any)     |
| 0x02         | set the permanent state to ON, abort timed operation (if any)      |
| 0x03         | set the permanent state to OFF, allow timed operation to complete  |
| 0x04         | set the permanent state to ON, allow timed operation to complete   |
| 0x05         | set the temporary state to ON, resume perm state on timeout        |
| 0x06         | set the temporary state to OFF, resume permanent state on timeout  |

## Command Buzzer

```
struct osdp_cmd_buzzer {
    uint8_t reader;
    uint8_t tone_code;
    uint8_t on_count;
    uint8_t off_count;
    uint8_t rep_count;
};
```

**reader:** 0 = First Reader, 1 = Second Reader, etc.
**tone_code:** 0: no tone, 1: off, 2: default tone, 3+ is TBD.
**on_count:** The ON duration of the flash, in units of 100 ms
**off_count:** The OFF duration of the flash, in units of 100 ms
**rep_count:** The number of times to repeat the ON/OFF cycle; 0: forever

## Command Test

```
struct osdp_cmd_text {
    uint8_t reader;
    uint8_t cmd;
    uint8_t temp_time;
    uint8_t offset_row;
    uint8_t offset_col;
    uint8_t length;
    uint8_t data[32];
};
```

**reader:** 0 = First Reader, 1 = Second Reader, etc.
**cmd:** How to treat the text; see table below
**temp_time:** The duration to display temporary text, in seconds
**offset_row:** The row where the first character will be displayed (1 is the top row)
**offset_col:** The column where the first character will be displayed (1 is the left-most column)
**length:** Number of characters in the string
**data:** The string to display

| Command | Meaning                     |
|:-------:|:----------------------------|
| 0x01    | permanent text, no wrap     |
| 0x02    | permanent text, with wrap   |
| 0x03    | temp text, no wrap          |
| 0x04    | temp text, with wrap        |

## Command Comset

```
struct osdp_cmd_comset {
    uint8_t addr;
    uint32_t baud;
};
```

**addr:** Unit ID to which this PD will respond after the change takes effect
**baud:** baud rate value 9600/38400/115200
