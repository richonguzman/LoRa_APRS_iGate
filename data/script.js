// Retrieve JSON data from the JSON file in SPIFFS
fetch('igate_conf.json')
    .then(response => response.json())
    .then(data => {
        // Process the JSON data
        displayJSONData(data);
    })
    .catch(error => {
        console.error('Error fetching JSON:', error);
    });

// Display JSON data in the HTML
function displayJSONData(data) {
    const jsonContainer = document.getElementById('json-container');

    var jsonData = jsonContainer.textContent;
        
    // Parse the JSON data into a JavaScript object
    try {
        var parsedData = JSON.parse(jsonData);
            
        // Access the data in the JavaScript object
        var callsign = parsedData.callsign;
        //var age = parsedData.stationMode;
        //var city = parsedData.city;
            
        // Display the parsed data
        console.log("Callsign : " + callsign);
        //console.log("Age: " + age);
        //console.log("City: " + city);
    } catch (error) {
        console.error("Error parsing JSON: " + error.message);
    }
//    jsonContainer.innerHTML = JSON.stringify(data, null, 2);
}
