rgbnightlight
=============
This is code that supports an RGB nightlight built on neopixels.
It uses interrupts to change the function of the nightlight.

The design uses a keyfob/receiver to switch pins high and low.
The functions are as follows:

Fob key A:  Turns LEDs on/off
Fob key B:  Switches between static color presets
Fob key C:  Switches between brightness level in any mode
Fob key D:  Switches to "effect" mode an the speed of the effect

The effect slowly slews the colors in individual pixels to the next preset from the bottom up.

