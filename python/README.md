# LibOSDP for Python

This package exposes the C/C++ library for OSDP devices to python to enable rapid
prototyping of these devices. There are two modules exposed by this package:

- `osdp_sys`: A thin wrapper around the C/C++ API; this has been around for a long
time now and hence has the most tested surface.

- `osdp`: Another wrapper over `osdp_sys` to provide a python friendly API; this
is a new implementation which is not powering the integration testing suit used
for all changes made to this project. Over time, this will be the primary means
of interacting with this library.

## Install

You can install LibOSDP from PyPI using,

```sh
pip install libosdp
```

## Usage

### Control Panel Mode

```python
from osdp import *

## populate osdp_pd_info_t from python
pd_info = [
    PDInfo(101, scbk=KeyStore.gen_key(), name='chn-0'),
]

## Create a CP device and kick-off the handler thread
cp = ControlPanel(pd_info, log_level=LogLevel.Debug)
cp.start()
cp.sc_wait_all()

## create and output command
output_cmd = {
    "command": osdp.CMD_OUTPUT,
    "output_no": 0,
    "control_code": 1,
    "timer_count": 10
}

## create a LED Command to be used later
led_cmd = {
    'command': Command.LED,
    'reader': 1,
    'led_number': 0,
    'control_code': 1,
    'on_count': 10,
    'off_count': 10,
    'on_color': CommandLEDColor.Red,
    'off_color': CommandLEDColor.Black,
    'timer_count': 10,
    'temporary': True
}

while True:
    ## Check if we have an event from PD
    event = cp.get_event(pd_info[0].address)
    if event:
        print(f"PD-0 Sent Event {event}")

    # Send LED command to PD-0
    cp.send_command(pd_info[0].address, led_cmd)
```

see [samples/cp_app.py][2] for more details.

### Peripheral Device mode:

```python
from osdp import *

## Describe the PD (setting scbk=None puts the PD in install mode)
pd_info = PDInfo(101, scbk=None, name='chn-0')

## Indicate the PD's capabilities to LibOSDP.
pd_cap = PDCapabilities([
    (Capability.OutputControl, 1, 1),
    (Capability.LEDControl, 1, 1),
    (Capability.AudibleControl, 1, 1),
    (Capability.TextOutput, 1, 1),
])

## Create a PD device and kick-off the handler thread
pd = PeripheralDevice(pd_info, PDCapabilities(), log_level=LogLevel.Debug)
pd.start()

## create a card read event to be used later
card_event = {
    'event': Event.CardRead,
    'reader_no': 1,
    'direction': 1,
    'format': CardFormat.ASCII,
    'data': bytes([9,1,9,2,6,3,1,7,7,0]),
}

while True:
    ## Send a card read event to CP
    pd.notify_event(card_event)

    ## Check if we have any commands from the CP
    cmd = pd.get_command()
    if cmd:
        print(f"CP sent command: {cmd}")
```

see [samples/pd_app.py][3] for more details.

[1]: https://libosdp.sidcha.dev/api/
[2]: https://github.com/goToMain/libosdp/blob/master/samples/python/cp_app.py
[3]: https://github.com/goToMain/libosdp/blob/master/samples/python/pd_app.py
