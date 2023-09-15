<?php
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $data = [
        'name' => $_POST['name'],
        'email' => $_POST['email']
    ];

    // Convert the data array to JSON format
    $json_data = json_encode($data);

    // Specify the path where you want to save the JSON file
    $file_path = 'userdata.json';

    // Save the JSON data to the file
    file_put_contents($file_path, $json_data);

    echo "Data saved as JSON successfully.";
}
?>