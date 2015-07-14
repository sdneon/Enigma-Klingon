//JSLint static code analysis options
/*jslint browser:true, unparam:true, sloppy:true, plusplus:true, indent:4, white:true */
/*global Pebble, console*/

var VERSION = 22,   //i.e. v2.2; for sending to config page
    //Defaults:
    DEF_DECODE = 4,
    DEF_LANG = 1,
    DEF_VIBES = 0X10A14,
    watchConfig = {
        KEY_DECODE: DEF_DECODE,
        KEY_LANG: DEF_LANG,
        KEY_VIBES: DEF_VIBES
    },
    //masks for vibes:
    MASKV_BTDC = 0x20000,
    MASKV_HOURLY = 0x10000,
    MASKV_FROM = 0xFF00,
    MASKV_TO = 0x00FF;

//Load saved config from localStorage
function loadConfig()
{
    var decode = parseInt(localStorage.getItem(0), 10),
        lang = parseInt(localStorage.getItem(1), 10),
        vibes = parseInt(localStorage.getItem(2), 10);
    if (isNaN(decode))
    {
        decode = DEF_DECODE;
    }
    if (isNaN(lang))
    {
        lang = DEF_LANG;
    }
    if (isNaN(vibes))
    {
        vibes = DEF_VIBES;
    }
    watchConfig.KEY_DECODE = decode;
    watchConfig.KEY_LANG = lang;
    watchConfig.KEY_VIBES = vibes;
}

//Save config to localStorage
function saveConfig()
{
    localStorage.setItem(0, watchConfig.KEY_DECODE);
    localStorage.setItem(1, watchConfig.KEY_LANG);
    localStorage.setItem(2, watchConfig.KEY_VIBES);
}

function sendOptions(options)
{
    Pebble.sendAppMessage(options,
        function(e) {
            console.log('Successfully delivered message');
        },
        function(e) {
            console.log('Unable to deliver message');
        }
    );
}

Pebble.addEventListener('webviewclosed',
    function(e) {
        console.log('Configuration window returned: ' + e.response);
        if (!e.response)
        {
            return;
        }
        var options = JSON.parse(e.response),
            noOptions = true,
            value;
        if (options.decode !== undefined)
        {
            value = parseInt(options.decode, 10);
            watchConfig.KEY_DECODE = value;
            noOptions = false;
        }
        if (options.lang !== undefined)
        {
            value = parseInt(options.lang, 10);
            watchConfig.KEY_LANG = value;
            noOptions = false;
        }
        if (options.vibes !== undefined)
        {
            value = parseInt(options.vibes, 10);
            watchConfig.KEY_VIBES = value;
            noOptions = false;
        }
        if (noOptions)
        {
            return;
        }

        saveConfig();
        sendOptions(watchConfig);
    }
);


Pebble.addEventListener('showConfiguration',
    function(e) {
        try {
            var url = 'http://yunharla.altervista.org/pebble/config-rosetta.html?ver=' + VERSION + '&lang=';
            //var url = 'https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/config/config.html'; //shows as text! as GitHub returns mime type as plain text.
            //var url = 'https://cdn.rawgit.com/sdneon/Enigma-Klingon/master/config/config.html';
            //Send/show current config in config page:
            url += watchConfig.KEY_LANG + '&decode=' + watchConfig.KEY_DECODE + '&vibes=0x' + watchConfig.KEY_VIBES.toString(16);

            // Show config page
            Pebble.openURL(url);
        }
        catch (ex)
        {
            console.log('ERR: showConfiguration exception');
        }
    }
);

Pebble.addEventListener('ready',
    function(e) {
        console.log('ready');
        loadConfig();
        sendOptions(watchConfig);
    });
