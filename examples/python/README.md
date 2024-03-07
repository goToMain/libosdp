# Python Examples

To run the samples, you have to install the following python package:

```sh
python3 -m pip install pyserial
```

Then you can run start the CP/PD service as,

```sh
./cp_app.py /dev/ttyUSB0 --baudrate 115200
# (or)
./pd_app.py /dev/ttyUSB0 --baudrate 115200
```

## Note:

To test how the CP and PD would potentially interact with each other, you can
ask socat to create a pair of psudo terminal devices that are connected to each
other. To do this run:

```sh
socat -d -d pty,raw,echo=0,nonblock pty,raw,echo=0,nonblock
```

The output of the above socat invocation should show two pts devices of the
format `/dev/pts/<N>`, where `N` is the number of the device. Then in two other
terminals, you can can start the CP and PD app as shown above but with the pts
devices produced by socat (the baudrate option can be omitted).
