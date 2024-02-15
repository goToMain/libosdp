# LibOSDP Usage Samples

These samples are meant to act as a reference for the API calls of LibOSDP. They
demonstrate the right initialization and refresh workflows the CP/PD to work
properly but are not working examples.

Assuming you have already built LibOSDP, you can run the following commands to
to compile `cp_sample` and `pd_sample`. If you want look at a real world,
implementation have a look at [osdpctl](/osdpctl)

```sh
gcc cp_app.c -o cp_sample -l osdp -I ../../include/ -L ../../build/lib
gcc pd_app.c -o pd_sample -l osdp -I ../../include/ -L ../../build/lib
```
