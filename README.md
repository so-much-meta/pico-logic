# pico-logic
Pi Pico (RP2040) based logic analyzer

This uses the PIO to perform RLE-like compression to enable streaming data over USB.

I started work on a sigrok driver, but that is not complete.

To use:
- Compile `pico_logic.c` and run
- `uncompress.py` is used to decompress the encoded byte stream
