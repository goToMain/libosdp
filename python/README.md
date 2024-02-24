# LibOSDP for Python

This package exposes the C/C++ library for OSDP devices to python to enable rapid
prototyping of these devices. There are two modules exposed by this package:

- `osdp_sys`: A thin wrapper around the C/C++ API; this is a low level API and
  is no longer recommended to use this directly.

- `osdp`: A wrapper over the `osdp_sys` to provide python friendly API; this
  implementation which is now powering the integration testing suit used to test
  all changes made to this project.

## Install

You can install LibOSDP from PyPI using,

```sh
pip install libosdp
```

Or, from github,

```sh
pip install -e "git+https://github.com/goToMain/libosdp#egg=libosdp&subdirectory=python"
```

Or, from source using,

```sh
git clone https://github.com/goToMain/libosdp --recurse-submodules
cd libosdp/python
python3 setup.py install
```

## Quick Start

### Control Panel Mode

```python
# Create a communication channel
channel = SerialChannel("/dev/ttyUSB0")

# populate osdp_pd_info_t from python
pd_info = [
    PDInfo(101, channel, scbk=KeyStore.gen_key()),
]

# Create a CP device and kick-off the handler thread and wait till a secure
# channel is established.
cp = ControlPanel(pd_info, log_level=LogLevel.Debug)
cp.start()
cp.sc_wait_all()

while True:
    ## Check if we have an event from PD
    led_cmd = { ... }
    event = cp.get_event(pd_info[0].address)
    if event:
        print(f"CP: Received event {event}")

    # Send LED command to PD-0
    cp.send_command(pd_info[0].address, led_cmd)
```

see [examples/cp_app.py][2] for more details.

### Peripheral Device mode:

```python
# Create a communication channel
channel = SerialChannel("/dev/ttyUSB0")

# Describe the PD (setting scbk=None puts the PD in install mode)
pd_info = PDInfo(101, channel, scbk=None)

# Indicate the PD's capabilities to LibOSDP.
pd_cap = PDCapabilities()

# Create a PD device and kick-off the handler thread and wait till a secure
# channel is established.
pd = PeripheralDevice(pd_info, pd_cap)
pd.start()
pd.sc_wait()

while True:
    # Send a card read event to CP
    card_event = { ... }
    pd.notify_event(card_event)

    # Check if we have any commands from the CP
    cmd = pd.get_command()
    if cmd:
        print(f"PD: Received command: {cmd}")
```

see [examples/pd_app.py][3] for more details.

[1]: https://libosdp.sidcha.dev/api/
[2]: https://github.com/goToMain/libosdp/blob/master/examples/python/cp_app.py
[3]: https://github.com/goToMain/libosdp/blob/master/examples/python/pd_app.py
