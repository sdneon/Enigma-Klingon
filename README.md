Enigma Klingon
==============
Enigma watch face variant with Klingon text for Pebble Time. (Colour, Digital).
The time & weekday name are initially in Klingon and subsequently decoded (to roman numerals & English text respectively).

Modified from my [Enigma Plus](https://github.com/sdneon/Enigma-plus) watch face.

## Display
1. 3 of 5 rows of info:
  1. Day of week (abbreviated 3 letters, uppercase).
    * This mini-bar (top-left) doubles as 'bluetooth status' indicator.
        * Coloured: connected
        * Dark: disconnected.
  2. Date (DDMM).
  3. Coloured time bar
    * Red: AM
    * Blue: PM.
2. Battery-level indicator: 3rd column.
  * Top down orange bar: charging,
  * bottom-up bar: charge remaining.

'Year' display has been removed, so as to show more Klingon text.

### Screenshots
![screenshot 1](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/pebble-screenshot-1-AM.png "Watch face: AM, bluetooth connected, battery not charging")
Watch face: AM, bluetooth connected, battery not charging

![screenshot 2](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/pebble-screenshot-2-AM,DC.png "Watch face: AM, bluetooth disconnected, battery low")
Watch face: AM, bluetooth disconnected, battery low

![screenshot 3](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/pebble-screenshot-3-PM,charging.png "Watch face: PM, bluetooth connected, battery charging")
Watch face: PM, bluetooth connected, battery charging

![screenshot 4](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/pebble-screenshot-4-before-decode.png "Watch face: AM, bluetooth connected, before decode")
Watch face: AM, bluetooth connected, before decode

## Build
You will need to download the [Klingon font](http://www.dafont.com/klingon-font.font) and place it in _/resources/fonts_ folder.
 * You could use alternative fonts for other alien texts or Wingdings, etc. Just change the _file_ paths in appinfo.json acoordingly.
 * If you are using other locales (than EN), you may need to redefine the FONT_CUSTOM_30's _characterRegex_ definition to include characters needed for the weekday name in your locale.

## Changelog
* v1.1
  * Date & time text alignments are now OK.
  * Added (DECODE) flag to choose any combination of weekday name, data & time to decode. May choose to decode nothing if you can read Klingon well =)
* v1.0
  * Initial draft.
  
## TODO
1. Add option to re-encode after some time or inactivity?
2. Expose configuration options?

## Credits
* [Klingon font](http://www.dafont.com/klingon-font.font) by kaiserzharkhan - thanks for creating such a beautiful font.
