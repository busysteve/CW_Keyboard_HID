# CW_Keyboard_HID
Use an Arduino Micro to use your Morse code paddle as a make-shift all caps keyboard for your computer to practice CW.

By default the device runs at 10 wpm, but it is adjustable and saves the setting in EEPROM memory for the next time.

To set your word per minute rate hold the long (dah) paddle for 6 tones.  Then follow up with the short paddle (dit) for 6 tones.  This will put you in the setting mode tap the long paddle to increace the WPM and the short paddle to decrease the WPM.  When you are satisfied with your setting do the reverse.  6 dits follow by 6 dahs, this will store the setting.  If you want to cancel the setting mode without saving press both paddles at the same time.  If the setting is accidentaly set too his or low, you can unplug the unit and hold both paddle while pluging the unit back into the USB port to reset it to 10 WPM and try setting it again.

#WPM Setup Mode
```
------ ......
```

#Exit Setup Mode

```
...... ------
```

That's all.

K4SDM


