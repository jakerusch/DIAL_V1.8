var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var myAPIKey = '';

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url, false);
  xhr.send();
};

// function locationSuccess(pos) {
//   // Construct URL
//   var weatherUrl = 'https://api.darksky.net/forecast/' + myAPIKey + '/' + pos.coords.latitude + ',' + pos.coords.longitude;
  
//   // get forecast through dark sky
//   xhrRequest(weatherUrl, 'GET', 
//     function(responseText) {
//       var json = JSON.parse(responseText);
      
//       // round temperature
//       var temp = Math.round(json.currently.temperature);
//       console.log("Temperature is " + temp);
      
//       // icon for weather condition
//       var icon = json.currently.icon;
//       console.log("Current icon is " + icon);
      
//       // assemble dictionary using keys
//       var dictionary = {
//         "KEY_TEMP": temp,
//         "KEY_ICON": icon,
//       };
      
//       // Send to Pebble
//       Pebble.sendAppMessage(dictionary,
//         function(e) {
//           console.log("Weather info sent to Pebble successfully!");
//         },
//         function(e) {
//           console.log("Error sending weather info to Pebble!");
//         }
//       );
//     }      
//   );
// }

function locationSuccess(pos) {
  // Construct URL
  var weatherUrl = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude + '&appid=' + myAPIKey + '&units=imperial';  

  // Send request to OpenWeatherMap.org
  xhrRequest(weatherUrl, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);
            
      // Delivered in imperial so no need for adjustment
      var temp = Math.round(json.main.temp);
      console.log("Temperature is " + temp);   
      
      var icon = json.weather[0].icon;
      console.log("Icon is " + icon);
      
      // assemble dictionary using keys
      var dictionary = {
        "KEY_TEMP": temp,
        "KEY_ICON": icon,
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);