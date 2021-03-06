//JSLint static code analysis options
/*jslint browser:true, unparam:true, sloppy:true, plusplus:true, indent:4, white:true */
/*global Pebble, console*/

var VERSION = 23,   //i.e. v2.3; for sending to config page
    //Defaults:
    DEF_DECODE = 4,
    DEF_LANG = 1,
    DEF_VIBES = 0X00A14,
    watchConfig = {
        KEY_DECODE: DEF_DECODE,
        KEY_LANG: DEF_LANG,
        KEY_VIBES: DEF_VIBES
    },
    //masks for vibes:
    MASKV_BTDC = 0x20000,
    MASKV_HOURLY = 0x10000,
    MASKV_FROM = 0xFF00,
    MASKV_TO = 0x00FF,
    //HTML5 storage keys
    KEY_OPTIONS_DECODE  = 0,
    KEY_OPTIONS_LANG    = 1,
    KEY_OPTIONS_VIBES   = 2,
    KEY_OPTIONS_WEATHER = 'weather',
    //weather stuff
    WEATHER_NA = 3200,
    WEATHER_NA_ICON_ID = 48,
    KEY_WEATHER_LAST = 'lastweather',
    ONE_DAY_IN_MS = 86400000,
    INTERVAL_TIMEOUT = 10000, //in ms, i.e. 10s
    UNIT_TEMPERATURE = {
        c: "\u00B0C",
        f: "\u00B0F"
    },
    UNIT_TEMPERATURE_OLD = { //temperature units with 'degree' symbol indicates old data
        c: " C",
        f: " F"
    },
    timer,
    lastWeatherUpdateTime = 0,
    lastWeatherId = WEATHER_NA_ICON_ID,
    lastWeatherTemp = NaN,
    lastWeatherTempUnit = 'c';

//
// Weather stuff
//
function saveWeather()
{
    var data = {
        time: lastWeatherUpdateTime,
        cond: lastWeatherId,
        temp: lastWeatherTemp,
        unit: lastWeatherTempUnit
    };
    localStorage.setItem(KEY_WEATHER_LAST, JSON.stringify(data));
}

function loadLastWeather()
{
    var data = localStorage.getItem(KEY_WEATHER_LAST),
        curTime;
    if (data !== undefined)
    {
        data = JSON.parse(data);
    }
    if (isNaN(data.time) || (data.time === 0) || !data.time)
    {
        return false; //no old data available
    }
    curTime = (new Date()).getTime();
    if (((curTime - data.time) >= ONE_DAY_IN_MS) || (data.time > curTime))
    {
        return false; //reject old data (over a day old) or invalid future time
    }
    lastWeatherUpdateTime = data.time || 0;
    lastWeatherId = (data.cond !== undefined)? data.cond: WEATHER_NA_ICON_ID;
    lastWeatherTemp = (data.temp !== undefined)? data.temp: NaN;
    lastWeatherTempUnit = data.unit || 'c';
    return true;
}

function sendLastWeather()
{
    var ok = loadLastWeather();
    if (ok)
    {
        Pebble.sendAppMessage({
            weather: watchConfig.weather? 1: 0,
            icon: lastWeatherId,
            temperature: (!isNaN(lastWeatherTemp))? ('' + lastWeatherTemp + UNIT_TEMPERATURE_OLD[lastWeatherTempUnit]): "NA"
        });
    }
}

function getWeatherFromWoeid(woeid) {
    var query = encodeURI("select item.condition from weather.forecast where woeid = " + woeid +
            " and u = \"" + watchConfig.units + "\""),
        url = "http://query.yahooapis.com/v1/public/yql?format=json&q=" + query,
        req = new XMLHttpRequest();

    req.timeout = INTERVAL_TIMEOUT;
    req.ontimeout = function() {
        sendLastWeather();
    };
    req.open('GET', url, true);
    req.onload = function(e) {
        if (req.readyState === 4)
        {
            if (req.status === 200)
            {
                var response = JSON.parse(req.responseText),
                    condition, temperature, icon;
                if (response)
                {
                    condition = response.query.results.channel.item.condition;
                    temperature = condition.temp + UNIT_TEMPERATURE[watchConfig.units];
                    condition.code = parseInt(condition.code, 10);
                    if (isNaN(condition.code) || (condition.code === WEATHER_NA))
                    {
                        icon = WEATHER_NA_ICON_ID;
                    }
                    else
                    {
                        icon = condition.code;
                    }
                    //console.log("temp " + temperature);
                    //console.log("icon " + icon);
                    //console.log("condition " + condition.text);
                    //backup weather update
                    lastWeatherUpdateTime = (new Date()).getTime();
                    lastWeatherId = condition.code;
                    lastWeatherTemp = parseInt(condition.temp, 10);
                    lastWeatherTempUnit = watchConfig.units;
                    saveWeather();
                    Pebble.sendAppMessage({
                        weather: watchConfig.weather? 1: 0,
                        icon : icon,
                        temperature : temperature
                    });
                    return;
                }
            }
            console.log("Error");
            sendLastWeather();
        }
    };
    req.send(null);
}

function getWeatherFromLatLong(latitude, longitude)
{
    var query = encodeURI("select woeid from geo.placefinder where text=\""+latitude+","+longitude + "\" and gflags=\"R\""),
        url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json",
        req = new XMLHttpRequest();

    req.timeout = INTERVAL_TIMEOUT;
    req.ontimeout = function() {
        sendLastWeather();
    };
    req.open('GET', url, true);
    req.onload = function(e) {
        if (req.readyState === 4)
        {
            if (req.status === 200)
            {
                var response = JSON.parse(req.responseText),
                    woeid;
                if (response)
                {
                    woeid = response.query.results.Result.woeid;
                    getWeatherFromWoeid(woeid);
                    return;
                }
            }
            console.log("Error");
            sendLastWeather();
        }
    };
    req.send(null);
}

function getWeatherFromLocation(location_name)
{
    var query = encodeURI("select woeid from geo.places(1) where text=\"" + location_name + "\""),
        url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json",
        req = new XMLHttpRequest();

    req.timeout = INTERVAL_TIMEOUT;
    req.ontimeout = function() {
        sendLastWeather();
    };
    req.open('GET', url, true);
    req.onload = function(e) {
        if (req.readyState === 4)
        {
            if (req.status === 200)
            {
                //console.log(req.responseText);
                var response = JSON.parse(req.responseText),
                    woeid;
                if (response)
                {
                    woeid = response.query.results.place.woeid;
                    getWeatherFromWoeid(woeid);
                    return;
                }
            }
            console.log("Error");
            sendLastWeather();
        }
    };
    req.send(null);
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

function locationSuccess(pos)
{
    var coordinates = pos.coords;
    getWeatherFromLatLong(coordinates.latitude, coordinates.longitude);
}

function locationError(err)
{
    console.warn('location error (' + err.code + '): ' + err.message);
    Pebble.sendAppMessage({
        weather: watchConfig.weather? 1: 0,
        icon: WEATHER_NA_ICON_ID,
        temperature: ""
    });
}

function updateWeather()
{
    if (!watchConfig.weather)
    {
        return;
    }
    if (watchConfig.gps)
    {
        window.navigator.geolocation.getCurrentPosition(locationSuccess,
            locationError,
            locationOptions);
    }
    else
    {
        getWeatherFromLocation(watchConfig.location);
    }
}

//
// Config stuff
//

//Load saved config from localStorage
function loadConfig()
{
    var decode = parseInt(localStorage.getItem(KEY_OPTIONS_DECODE), 10),
        lang = parseInt(localStorage.getItem(KEY_OPTIONS_LANG), 10),
        vibes = parseInt(localStorage.getItem(KEY_OPTIONS_VIBES), 10),
        data = localStorage.getItem(KEY_OPTIONS_WEATHER);
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
    if (data !== undefined)
    {
        data = JSON.parse(data);
        watchConfig.weather = data.weather;
        watchConfig.gps = data.gps;
        watchConfig.location = data.location;
        watchConfig.units = data.units;
        watchConfig.interval = data.interval;
    }
    else
    {
        watchConfig.weather = 0;
        watchConfig.gps = true;
        watchConfig.location = '';
        watchConfig.units = 'c';
        watchConfig.interval = interval; // 30 minutes
    }
}

//Save config to localStorage
function saveConfig()
{
    localStorage.setItem(KEY_OPTIONS_DECODE, watchConfig.KEY_DECODE);
    localStorage.setItem(KEY_OPTIONS_LANG, watchConfig.KEY_LANG);
    localStorage.setItem(KEY_OPTIONS_VIBES, watchConfig.KEY_VIBES);
    localStorage.setItem(KEY_OPTIONS_WEATHER, JSON.stringify(watchConfig));
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
        if (!options)
        {
            return;
        }
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
        if (options.weather !== undefined)
        {
            watchConfig.weather = options.weather;
            watchConfig.gps = options.gps;
            watchConfig.location = options.location;
            watchConfig.units = options.units;
            watchConfig.vibes = parseInt(options.vibes, 10);
            watchConfig.interval = options.interval;
            noOptions = false;
    }
        if (noOptions)
        {
            return;
        }

        sendOptions(watchConfig);
        saveConfig();

        setTimeout(function() { //delay to avoid APP_MSG_BUSY Error 64, as it comes too close to config update msg
            updateWeather();
        }, 1000);

        if (!watchConfig.weather)
        {
            if (timer !== undefined)
            {
                clearInterval(timer);
                timer = undefined;
            }
        }
        else if (!timer)
        {
            timer = setInterval(function() {
                //console.log("timer fired");
                updateWeather();
            }, watchConfig.interval);
        }
    }
);


Pebble.addEventListener('showConfiguration',
    function(e) {
        try {
            var url = 'http://yunharla.altervista.org/pebble/config-rosetta.html?ver=' + VERSION + '&lang=';
            //var url = 'https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/config/config.html'; //shows as text! as GitHub returns mime type as plain text.
            //var url = 'https://cdn.rawgit.com/sdneon/Enigma-Klingon/master/config/config.html';
            //Send/show current config in config page:
            url += watchConfig.KEY_LANG + '&decode=' + watchConfig.KEY_DECODE
            + '&vibes=0x' + watchConfig.KEY_VIBES.toString(16)
                + '&weather=' + !!watchConfig.weather
                + '&gps=' + !!watchConfig.gps
                + '&location=' + watchConfig.location
                + '&units=' + watchConfig.units
                + '&interval=' + watchConfig.interval;

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
        if (watchConfig.weather)
        {
            updateWeather();
            timer = setInterval(function() {
                //console.log("timer fired");
                updateWeather();
            }, watchConfig.interval);
        }
    });
