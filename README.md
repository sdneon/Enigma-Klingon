Enigma Klingon
==============
Enigma watch face variant with Klingon text (from Star Trek universe) for Pebble Time. (Colour, Digital).
You may now configure what information to display in decoded form (roman numerals for date & time, English text for weekday name); all other information remains in Klingon. Choices are:
1. None (all info stays in Klingon)
2. All
3. Time only (default)
4. Date & Time only

Shake to decode all information temporarily for 5 secs.

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
![screenshot 5](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/pebble-screenshot-5.png "Watch face: v1.2")
Watch face: v1.2

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
 * You could use alternative fonts for other alien texts or Wingdings, etc. Just change the _file_ paths in appinfo.json accordingly.
   * As an example, /build/Enigma-Aurebesh.pbw has been uploaded, demonstrating the use of the [Aurebesh font](http://www.dafont.com/aurebesh.font) from Star Wars universe.

![screenshot 5](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/Aurebesh Cantina Bold.png "Watch face: Aurebesh, Star Wars font")
Watch face: Aurebesh, Star Wars font

 * If you are using other locales (than EN), you may need to redefine the FONT_CUSTOM_30's _characterRegex_ definition to include characters needed for the weekday name in your locale.

## Changelog
* v1.3
  * Expose configuration option to choose what to decode. You may choose from:
    1. None (all info stays in Klingon)
    2. All
    3. Time only
    4. Date & Time only
* v1.2
  * Display (less time) reverts to alien text after 5 secs.
    * Shake to reveal again.
  * Unsubscribe from services upon exit.
* v1.1
  * Date & time text alignments are now OK.
  * Added (DECODE) flag to choose any combination of weekday name, data & time to decode. May choose to decode nothing if you can read Klingon well =)
* v1.0
  * Initial draft.
  
## TODO
* Fork a Rosetta/all-in-one version with choice of fonts, like Klingon, Aurebesh, etc.

## Credits
Thanks for creating these beautiful fonts:
* [Klingon font](http://www.dafont.com/klingon-font.font) from Star Trek universe, by kaiserzharkhan.
* [Aurebesh font](http://www.dafont.com/aurebesh.font) from Star Wars universe, by Pixel Saga.
