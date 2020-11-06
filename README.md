# NanoVNA Wi-Fi connection impl.

## Configure Wi-Fi Mode (AP or Station)

IO15 is pulled-up input

- connect to GND: work as station for debug (ID/PASS is set by `idf.py menuconfig`)
- open: work as AP

## Build

require esp-idf https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#get-started-get-esp-idf

`idf.py build`

build and flash

```
idf.py -p /dev/tty.SLAB_USBtoUART flash monitor
```
