# Teensy 4.0 / 4.1 usb host passthrough

This is a simplistic initial version of my USB host passthrough device for the teensy. You can plug in a keyboard and mouse into it and let it passthrough the input at exceptionally fast speed so that it is un-noticable. These inputs can then be modified by incomming commands over serial, or by an algorithim in the code if you so choose to add one.

# NOTE:
  The code in this repository requires you to apply the following patches to your teensy core source [https://github.com/ConnorBP/teensy4-core-custom](https://github.com/ConnorBP/teensy4-core-custom)
The fixes there correct some bugs with state resets and integer overflows in the core library.

# License

No warrenties or licenses are given, use at own risk.

No selling, and give credit.
