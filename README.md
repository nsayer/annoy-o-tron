# Annoy-o-Tron Tiny

Firmware for http://www.hackaday.io/project/173398/

The Annoy-o-Tron is a very old prank concept. A small battery powered,
microcontroller based device beeps on an irregular schedule once or twice
an hour, with the aim to annoy.

This incarnation uses the ATTiny9, which comes in a SOT23-6 form factor and
can idle at an astonishing 7 µA, which means that it can last for the
better part of a year on a single CR1225 coin cell.

Two lines of the controller are connected to a piezo element and when it's
time to beep, two square waves 180º out of phase are fed into it, so the
element sees effectively double the battery voltage in amplitude.

The firmware will randomly choose one of 5 beep tones - 1-5 kHz - and will
wait anywhere between a half an hour and an hour between.

