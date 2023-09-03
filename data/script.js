// Retrieve JSON data from the JSON file in SPIFFS
fetch('igate_conf.json')
    .then(response => response.json())
    .then(data => {
        // Process the JSON data
        //displayJSONData(data);
        
        const jsonDataDiv = document.getElementById('json-container');
        jsonDataDiv.innerHTML = ''; // Clear the previous content

        if (Array.isArray(data)) {
            // If data is an array, iterate through it
            data.forEach(item => {
                const itemDiv = document.createElement('div');
                itemDiv.textContent = JSON.stringify(item, null, 2);
                jsonDataDiv.appendChild(itemDiv);
            });
        } else {
            // If data is not an array, display it directly
            const itemDiv = document.createElement('div');
            itemDiv.textContent = JSON.stringify(data, null, 2);
            jsonDataDiv.appendChild(itemDiv);
        }
    })
    .catch(error => {
        console.error('There was a problem fetching the JSON data:', error);
    });

// Display JSON data in the HTML
function displayJSONData(data) {
    //const jsonContainer = document.getElementById('json-container');
    //jsonContainer.innerHTML = JSON.stringify(data, null, 2);
    
}
