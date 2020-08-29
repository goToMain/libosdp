# Python3 Bindings for LibOSDP

This is an initial effort at enabling python3 support for LibOSDP. This document
assumes that you are already familiar with [LibOSDP API][1] and device life
cycle management.

## Build/Install

A python module `osdp` and classes `ControlPanel` and `PeripheralDevice` are
made available when the sources of this directory is built and installed.

After cloning this repo (with `--recurse-submodules`), you can do the following
to build the python module.

```bash
mkdir build && cd build
cmake ..
make python
make python_install
```

If you want to undo this install, you can run `make python_uninstall`.

## How to use this module?

This module is still a work in progress and supports only Control Panel mode of
operation. Please create an issue if you face any issues.

```python
import osdp

## Master key for secure channel
key = '01020304050607080910111213141516'

## populate osdp_pd_info_t from python
pd_info = [
    # PD Address,   Baud Rate,   Device
    ( 101,          115200,      '/dev/ttyUSB0'),  # PD-0
    ( 102,          115200,      '/dev/ttyUSB1'),  # PD-1
]

## Setup OSDP device in control panel mode
cp = osdp.ControlPanel(pd_info, master_key=key)

## call this refresh method once every 50ms
cp.refresh()

## send a command to PD
cp.send(pd=1, command=osdp.CMD_COMSET, address=110, baud_rate=9600)
```

See `pyosdp_cp_send_command()` in pyosdp_cp.c for more information on
`cp.send()`.

[1]: https://libosdp.gotomain.io/api/
