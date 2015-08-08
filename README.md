Enigma Klingon
==============
Enigma watch face variant with choice of alien text for Pebble Time. (Colour, Digital). _Language_ choices:
* Klingon (from Star Trek universe) (default) ![screenshot Klingon](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/pebble-screenshot-5.png "Watch face in Klingon")
* Human ![screenshot human](https://raw.githubusercontent.com/sdneon/Enigma-plus/master/store/pebble-screenshot-1-AM.png "Watch face in Human")
* Aurebesh (from Star Wars universe) ![screenshot aurebesh](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/pebble-screenshot-6-aurebesh.png "Watch face in Aurebesh, Star Wars font")
* and more...

You may now configure what information to display in decoded form (Human, English); all other information remains in alien. Choices are:
1. None (all info stays in alien)
2. All
3. Time only (default)
4. Date & Time only

Shake to decode all information temporarily for 5 secs.

Modified from my [Enigma Plus](https://github.com/sdneon/Enigma-plus) watch face.

## Display
1. 4 of 5 rows of info:
    1. Day of week (abbreviated 3 letters, uppercase).
        * This mini-bar (top-left) doubles as 'bluetooth status' indicator.
            * Coloured: connected
            * Dark: disconnected.
    2. Date (DDMM).
    3. Coloured time bar
        * Red: AM
        * Blue: PM.
    4. Optional weather info using Yahoo Weather data: weather condition icon & average temperature at bottom right corner.
        * Location (using GPS or predefined location), update interval and temperature units are configurable.
        * Yahoo has 49 different weather conditions. These are listed in this [page](http://yunharla.altervista.org/pebble/weather-codes.html). Several similar ones with differing adjectives like- isolated, scattered.
            * Isolated is ~10-20% coverage (area affected), and depicted by weather icon in the far distance (so it's partly cropped/clipped).
            * Scattered is ~30-50% coverage, and depicted by weather icon in middle distance (less cropping than for _isolated_).
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
 * You could use alternative fonts for other alien texts or Wingdings, etc. Just change the _file_ paths in appinfo.json accordingly.
   * As an example, /build/Enigma-Aurebesh.pbw has been uploaded, demonstrating the use of the [Aurebesh font](http://www.dafont.com/aurebesh.font) from Star Wars universe.

![screenshot 5](https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/store/Aurebesh Cantina Bold.png "Watch face: Aurebesh, Star Wars font")
Watch face: Aurebesh, Star Wars font

 * If you are using other locales (than EN), you may need to redefine the FONT_CUSTOM_30's _characterRegex_ definition to include characters needed for the weekday name in your locale.

## Changelog
* 2.3
  * Added weather.
* 2.2
  * Added Betazed by Pixel Saga. Added size 30 & 42 bitmaps.
  * Added optional vibes for:
    * Bluetooth connection lost: fading vibe.
    * Hourly chirp. Default: Off, 10am to 8pm.
* 2.1
  * More alien fonts added:
    * [Aurebesh, Handwritten (Star Wars)](http://www.fontspace.com/boba-fonts/aurek-besh-hand) by Boba Fonts.
    * [Graalen, Andorian (Star Trek)](http://memory-beta.wikia.com/wiki/Andorian_languages) by Pixel Saga. Added in a 'W'.
    * [Kentaurus, Greek (Star Trek)](http://www.dafont.com/kentaurus.font) by Pixel Saga.
    * [Mandalorian (Star Wars)](http://www.dafont.com/mandalorian.font) by Erikstormtrooper.
* v2.0
  * Aurebesh font integrated.
  * Add choice of languages to configuration.
  * Configuration opens with current settings.
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
* Investigate Pebble JS failure causing configuration page to fail to load.
* Investigate/fix fonts that Pebble cannot load correctly.

## Credits
Thanks for creating these beautiful fonts.
* kaiserzharkhan:
  * [Klingon font](http://www.dafont.com/klingon-font.font) from Star Trek universe.
* Pixel Saga:
  * [Aurebesh font](http://www.dafont.com/aurebesh.font) from Star Wars universe.
  * [Betazed](http://www.dafont.com/betazed.font) from Star Trek universe.
  * [Graalen, Andorian](http://memory-beta.wikia.com/wiki/Andorian_languages)  from Star Trek universe.
  * [Kentaurus, Greek](http://www.dafont.com/kentaurus.font) from Star Trek universe.
* Boba Fonts
  * [Aurebesh, Handwritten (Star Wars)](http://www.fontspace.com/boba-fonts/aurek-besh-hand)  from Star Wars universe.
* Erikstormtrooper
  * [Mandalorian](http://www.dafont.com/mandalorian.font) from Star Wars universe.
