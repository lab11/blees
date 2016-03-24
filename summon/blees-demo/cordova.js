// select which cordova_*.js to load based on the platform
if (navigator.platform.startsWith("iP")) {
    // iOS (iPhone, iPad, iPod)
    console.log("Loading iOS-specific cordova");
    $.getScript('cordova_ios.js', load_after);
} else {
    // android or bust
    console.log("Loading android-specific cordova");
    $.getScript('cordova_android.js', load_after)
        .fail(function(jqxhr, settings, exception) {
            console.log("Failed to load");
            console.log(JSON.stringify(jqxhr));
            console.log("Settings: " + settings);
            console.log("Exception: " + exception);
        });
}

// don't load the user's index.js until cordova is loaded
function load_after (response, stats) {
    console.log("Successful load");
    console.log("Response: " + response);
    console.log("Status: " + stats);
    console.log("Loading user's index.js file");
    $.getScript('js/index.js');
}

