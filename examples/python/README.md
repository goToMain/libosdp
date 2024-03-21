# Python Examples

To run the samples, you have to install the following python package:

```sh
python3 -m pip install pyserial
python3 -m pip install ../../python
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
socat pty,raw,echo=0,nonblock,link=/tmp/ttyS0 pty,raw,echo=0,nonblock,link=/tmp/ttyS1
```

While the above command is running, you can use `/tmp/ttyS0` and `/tmp/ttyS1` to
start your CP and PD app as,

```
./cp_app.py /tmp/ttyS0
./pd_app.py /tmp/ttyS1
```
