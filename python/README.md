# Python3 Bindings for LibOSDP

This is an initial effort at enabling python3 support for LibOSDP. This document
assumes that you are already familiar with [LibOSDP API][1] and device life
cycle management.

## Build/Install

A python module `osdp` and classes `ControlPanel` and `PeripheralDevice` are
made available when the sources of this directory is built and installed.

After cloning this repo (with `--recurse-submodules`), you can do the following
(from the repo root) to build the python module.

```bash
mkdir build && cd build
cmake ..
make python
make python_install
```

If you want to undo this install, you can run `make python_uninstall`.

## How to use this module?

### Control Panel Mode

```python
import osdp

## Master key for secure channel
key = '01020304050607080910111213141516'

## populate osdp_pd_info_t from python
pd_info = [
    {
        "address": 101,
        "channel_type": "uart",
        "channel_speed": 115200,
        "channel_device": '/dev/ttyUSB0',
    },
    {
        "address": 102,
        "channel_type": "uart",
        "channel_speed": 115200,
        "channel_device": '/dev/ttyUSB1',
    }
]

## Setup OSDP device in Control Panel mode
cp = osdp.ControlPanel(pd_info, master_key=key)

## call this refresh method once every 50ms
cp.refresh()

## create and output command
output_cmd = {
    "command": osdp.CMD_OUTPUT,
    "output_no": 0,
    "control_code": 1,
    "timer_count": 10
}

## send a output command to PD-1
cp.send_command(1, output_cmd)
```

see [samples/cp_app.py][2] for more details.

### Peripheral Device mode:

```python
## Describe the PD
pd_info = {
    "address": 101,
    "channel_type": "uart",
    "channel_speed": 115200,
    "channel_device": '/dev/ttyUSB0',

    "version": 1,
    "model": 1,
    "vendor_code": 0xCAFEBABE,
    "serial_number": 0xDEADBEAF,
    "firmware_version": 0x0000F00D
}

## Indicate the PD's capabilities to LibOSDP.
pd_cap = [
    {
        # PD is capable of handling output commands
        "function_code": osdp.CAP_OUTPUT_CONTROL,
        "compliance_level": 1,
        "num_items": 1
    },
]

## Setup OSDP device in Peripheral Device mode
pd = osdp.PeripheralDevice(pd_info, capabilities=pd_cap)

## call this refresh method periodically (at lease once every 50ms)
pd.refresh()

## Define a callback handler. Must return 0 on success, -ve on failures.
def handle_command(address, command):
    if address != pd_info['address']:
        return { "return_code": -1 }
    print("PD received command: ", command)
    return { "return_code": 0 }

## Set a handler for incoming commands from CP
pd.set_command_callback(handle_command)
```

see [samples/pd_app.py][3] for more details.

[1]: https://libosdp.sidcha.dev/api/
[2]: https://github.com/goToMain/libosdp/blob/master/samples/python/cp_app.py
[3]: https://github.com/goToMain/libosdp/blob/master/samples/python/pd_app.py
