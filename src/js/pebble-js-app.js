//JSLint static code analysis options
/*jslint browser:true, unparam:true, sloppy:true, plusplus:true, indent:4, white:true */
/*global Pebble, console*/

Pebble.addEventListener('showConfiguration',
    function(e) {
        // Show config page
        //Pebble.openURL('https://raw.githubusercontent.com/sdneon/Enigma-Klingon/master/config/config.html');
        Pebble.openURL('http://yunharla.altervista.org/pebble/config.html');
    }

);

Pebble.addEventListener('webviewclosed',
    function(e) {
        if (!e.response)
        {
            return;
        }
        var options = JSON.parse(e.response),
            transactionId;
        if (!options.decode)
        {
            return;
        }
        options.decode = parseInt(options.decode, 10);
        //console.log('Configuration window returned: ' + e.response + ' ' + options.decode);

        transactionId = Pebble.sendAppMessage( { 'KEY_DECODE': options.decode },
            function(e) {
                console.log('Successfully delivered message with transactionId='
                    + e.options.transactionId);
            },
            function(e) {
                console.log('Unable to deliver message with transactionId='
                    + e.options.transactionId
                    + ' Error is: ' + e.error.message);
            }
        );
        console.log('transaction id=' + transactionId);
    }
);

