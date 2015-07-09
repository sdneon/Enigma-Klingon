//JSLint static code analysis options
/*jslint browser:true, unparam:true, sloppy:true, plusplus:true, indent:4, white:true */
/*global Pebble, console*/

var VERSION = 22,   //i.e. v2.2; for sending to config page
    //Defaults:
    DEF_DECODE = 4,
    DEF_LANG = 1,
    watchConfig = {
        KEY_DECODE: DEF_DECODE,
        KEY_LANG: DEF_LANG
    };

//Load saved config from localStorage
function loadConfig()
{
    var decode = parseInt(localStorage.getItem(0), 10),
        lang = parseInt(localStorage.getItem(1), 10);
    if (isNaN(decode))
    {
        decode = DEF_DECODE;
    }
    if (isNaN(lang))
    {
        lang = DEF_LANG;
    }
    watchConfig.KEY_DECODE = decode;
    watchConfig.KEY_LANG = lang;
}

//Save config to localStorage
function saveConfig()
{
    localStorage.setItem(0, watchConfig.KEY_DECODE);
    localStorage.setItem(1, watchConfig.KEY_LANG);
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
                //url = 'https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/config/config.html'; //show as text! as GitHub returns mime type as plain text.
                //url = 'https://cdn.rawgit.com/sdneon/Enigma-Klingon/master/config/config.html';
            url += watchConfig.KEY_LANG + '&decode=' + watchConfig.KEY_DECODE; //send/show current config in config page

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
