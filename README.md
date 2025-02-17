# CW_Keyboard_HID
Use an Arduino Micro to use your Morse code paddle as a make-shift all caps keyboard for your computer to practice CW.

By default the device runs at 10 wpm, but it is adjustable and saves the settings in EEPROM memory for the next time.

To set your word per minute rate hold enter ``` -.-.-. -.-.-. ```.  This will put you in the settings menu mode.  Enter ``` -. ``` until you are on the WPM setting menu.  The long paddle to increase the WPM and the short paddle to decrease the WPM.  When you are satisfied with your setting enter the letter "X" ``` -..- ``` to exit the settings mode.  If the setting is accidentaly set too high or low, you can unplug the unit and hold both paddles while pluging the unit back into the USB port to reset it to 10 WPM and try setting it up again.

#Enter Setup Mode with ";;" (Two following semicolons)
```
-.-.-. -.-.-.
```

#Exit Setup Mode with the letter "X"

```
-..-
```

#Traverse the menus with the letter "N" to go forward, and "A" to go backward.

```
-.

.-

```

#Select items in each menu with the letter "T" to go forward, and "E" to go backward.

```
-

.

```
That's all.

K4SDM



![](https://github.com/busysteve/CW_Keyboard_HID/blob/main/CW_Keyboard_HID_Diagram.png)


#Click here for a testing textbox

[Testing Textbox](https://raw.githack.com/busysteve/CW_Keyboard_HID/main/textbox.html).


#Parts List

[Arduino Micro Pro](https://www.amazon.com/dp/B0B6HYLC44)

[Potentiometers](https://www.amazon.com/dp/B07XQ7NTQR)

[3.5mm stereo jacks](https://www.amazon.com/dp/B07MN1RK7F)

[Ear buds](https://www.amazon.com/Sony-MDRE9LP-BLK-Ear-Buds/dp/B004RE3YNA)

[Small Project Boxes](https://www.amazon.com/dp/B07Q11F7DS)


