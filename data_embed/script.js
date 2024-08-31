// Custom scripts

let currentSettings = null;

function backupSettings() {
    const data =
        "data:text/json;charset=utf-8," +
        encodeURIComponent(JSON.stringify(currentSettings));
    const a = document.createElement("a");
    a.setAttribute("href", data);
    a.setAttribute("download", "iGateConfigurationBackup.json");
    a.click();
}

document.getElementById("backup").onclick = backupSettings;

document.getElementById("restore").onclick = function (e) {
    e.preventDefault();

    document.querySelector("input[type=file]").click();
};

document.querySelector("input[type=file]").onchange = function () {
    const files = document.querySelector("input[type=file]").files;

    if (!files.length) return;

    const file = files.item(0);

    const reader = new FileReader();
    reader.readAsText(file);
    reader.onload = () => {
        const data = JSON.parse(reader.result);

        loadSettings(data);
    };
};

function fetchSettings() {
    fetch("/configuration.json")
        .then((response) => response.json())
        .then((settings) => {
            loadSettings(settings);
        })
        .catch((err) => {
            console.error(err);

            alert(`Failed to load configuration`);
        });
}

const alwaysOnCheckbox = document.querySelector(
    'input[name="display.alwaysOn"]'
);
const timeoutInput = document.querySelector('input[name="display.timeout"]');

alwaysOnCheckbox.addEventListener("change", function () {
    timeoutInput.disabled = this.checked;
});

// timeoutInput.addEventListener("change", function () {
//     alwaysOnCheckbox.disabled = this.value !== "";
// });

const logCheckbox = document.querySelector('input[name="syslog.active"]');
const serverField = document.querySelector('input[name="syslog.server"]');
const portField = document.querySelector('input[name="syslog.port"]');

logCheckbox.addEventListener("change", function () {
    serverField.disabled = !this.checked;
    portField.disabled = !this.checked;
});

function loadSettings(settings) {
    currentSettings = settings;
    // General
    document.getElementById("callsign").value                           = settings.callsign;
    document.getElementById("beacon.comment").value                     = settings.beacon.comment;
    document.getElementById("beacon.path").value                        = settings.beacon.path;
    document.getElementById("beacon.symbol").value                      = settings.beacon.symbol;
    document.getElementById("beacon.overlay").value                     = settings.beacon.overlay;
    document.getElementById("personalNote").value                       = settings.personalNote;
    document.getElementById("action.symbol").value                      = settings.beacon.overlay + settings.beacon.symbol;

    document.querySelector(".list-networks").innerHTML                  = "";

    // Networks
    const wifiNetworks = settings.wifi.AP || [];
    const networksContainer = document.querySelector(".list-networks");

    let networkCount = 0;

    wifiNetworks.forEach((network) => {
        const networkElement = document.createElement("div");
        networkElement.classList.add("row", "network", "border-bottom", "py-2");

        // Increment the name, id, and for attributes
        const attributeName = `wifi.AP.${networkCount}`;
        networkElement.innerHTML = `
                  <div class="form-floating col-5 px-1 mb-2">
                    <input type="text" class="form-control form-control-sm" name="${attributeName}.ssid" id="${attributeName}.ssid" value="${network.ssid}">
                    <label for="${attributeName}.ssid">SSID</label>
                  </div>
                  <div class="form-floating col-5 px-1 mb-2">
                    <input type="password" class="form-control form-control-sm" name="${attributeName}.password" id="${attributeName}.password" value="${network.password}">
                    <label for="${attributeName}.password">Passphrase</label>
                  </div>
                  <div class="col-2 d-flex align-items-center justify-content-end">
                    <div class="btn-group" role="group">
                      <button type="button" class="btn btn-sm btn-danger" title="Delete" onclick="return this.parentNode.parentNode.parentNode.remove();"><svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-trash3-fill" viewBox="0 0 16 16">
              <path d="M11 1.5v1h3.5a.5.5 0 0 1 0 1h-.538l-.853 10.66A2 2 0 0 1 11.115 16h-6.23a2 2 0 0 1-1.994-1.84L2.038 3.5H1.5a.5.5 0 0 1 0-1H5v-1A1.5 1.5 0 0 1 6.5 0h3A1.5 1.5 0 0 1 11 1.5m-5 0v1h4v-1a.5.5 0 0 0-.5-.5h-3a.5.5 0 0 0-.5.5M4.5 5.029l.5 8.5a.5.5 0 1 0 .998-.06l-.5-8.5a.5.5 0 1 0-.998.06m6.53-.528a.5.5 0 0 0-.528.47l-.5 8.5a.5.5 0 0 0 .998.058l.5-8.5a.5.5 0 0 0-.47-.528M8 4.5a.5.5 0 0 0-.5.5v8.5a.5.5 0 0 0 1 0V5a.5.5 0 0 0-.5-.5"/>
            </svg><span class="visually-hidden">Delete</span></button>
                    </div>
                  </div>
                `;
        networksContainer.appendChild(networkElement);
        networkCount++;
    });

    // APRS-IS
    document.getElementById("aprs_is.active").checked                   = settings.aprs_is.active;
    document.getElementById("aprs_is.messagesToRF").checked             = settings.aprs_is.messagesToRF;
    document.getElementById("aprs_is.objectsToRF").checked              = settings.aprs_is.objectsToRF;
    document.getElementById("aprs_is.server").value                     = settings.aprs_is.server;
    document.getElementById("aprs_is.port").value                       = settings.aprs_is.port;
    document.getElementById("aprs_is.filter").value                     = settings.aprs_is.filter;
    document.getElementById("aprs_is.passcode").value                   = settings.aprs_is.passcode;

    // Beacon
    document.getElementById("beacon.latitude").value                    = settings.beacon.latitude;
    document.getElementById("beacon.longitude").value                   = settings.beacon.longitude;
    document.getElementById("beacon.interval").value                    = settings.beacon.interval;
    document.getElementById("other.rememberStationTime").value          = settings.other.rememberStationTime;   
    document.getElementById("beacon.sendViaAPRSIS").checked             = settings.beacon.sendViaAPRSIS;
    document.getElementById("beacon.sendViaRF").checked                 = settings.beacon.sendViaRF;

    // Digi
    document.getElementById("digi.mode").value                          = settings.digi.mode;

    // LoRa
    document.getElementById("lora.txFreq").value                        = settings.lora.txFreq;
    document.getElementById("lora.rxFreq").value                        = settings.lora.rxFreq;
    document.getElementById("lora.txActive").checked                    = settings.lora.txActive;
    document.getElementById("lora.rxActive").checked                    = settings.lora.rxActive;
    document.getElementById("lora.spreadingFactor").value               = settings.lora.spreadingFactor;
    document.getElementById("lora.signalBandwidth").value               = settings.lora.signalBandwidth;
    document.getElementById("lora.codingRate4").value                   = settings.lora.codingRate4;
    document.getElementById("lora.power").value                         = settings.lora.power;

    // Display
    document.getElementById("display.alwaysOn").checked                 = settings.display.alwaysOn;
    document.getElementById("display.turn180").checked                  = settings.display.turn180;
    document.getElementById("display.timeout").value                    = settings.display.timeout;

    if (settings.display.alwaysOn) {
        timeoutInput.disabled = true;
    }

    // BATTERY
    document.getElementById("battery.sendInternalVoltage").checked      = settings.battery.sendInternalVoltage;
    document.getElementById("battery.monitorInternalVoltage").checked   = settings.battery.monitorInternalVoltage;
    document.getElementById("battery.internalSleepVoltage").value       = settings.battery.internalSleepVoltage.toFixed(1);
    document.getElementById("battery.sendExternalVoltage").checked      = settings.battery.sendExternalVoltage;
    document.getElementById("battery.externalVoltagePin").value         = settings.battery.externalVoltagePin;
    document.getElementById("battery.voltageDividerR1").value                 = settings.battery.voltageDividerR1.toFixed(1);
    document.getElementById("battery.voltageDividerR2").value                 = settings.battery.voltageDividerR2.toFixed(1);
    document.getElementById("battery.monitorExternalVoltage").checked   = settings.battery.monitorExternalVoltage;
    document.getElementById("battery.externalSleepVoltage").value       = settings.battery.externalSleepVoltage.toFixed(1);
    
    // TELEMETRY BME/WX
    document.getElementById("bme.active").checked                       = settings.bme.active;
    document.getElementById("bme.heightCorrection").value               = settings.bme.heightCorrection;
    document.getElementById("bme.temperatureCorrection").value          = settings.bme.temperatureCorrection.toFixed(1);
    
    // SYSLOG
    document.getElementById("syslog.active").checked                    = settings.syslog.active;
    document.getElementById("syslog.server").value                      = settings.syslog.server;
    document.getElementById("syslog.port").value                        = settings.syslog.port;

    if (settings.syslog.active) {
        serverField.disabled = false;
        portField.disabled = false;
    }

    // TNC
    if (settings.tnc) {
        document.getElementById("tnc.enableServer").checked             = settings.tnc.enableServer;
        document.getElementById("tnc.enableSerial").checked             = settings.tnc.enableSerial;
        document.getElementById("tnc.acceptOwn").checked                = settings.tnc.acceptOwn;
    }

    // Reboot
    document.getElementById("other.rebootMode").checked                 = settings.other.rebootMode;
    document.getElementById("other.rebootModeTime").value               = settings.other.rebootModeTime;

    // WiFi Auto AP
    document.getElementById("wifi.autoAP.password").value               = settings.wifi.autoAP.password;
    document.getElementById("wifi.autoAP.powerOff").value               = settings.wifi.autoAP.powerOff;

    // OTA
    document.getElementById("ota.username").value                       = settings.ota.username;
    document.getElementById("ota.password").value                       = settings.ota.password;

    // Webadmin
    document.getElementById("webadmin.active").checked                  = settings.webadmin.active;
    document.getElementById("webadmin.username").value                  = settings.webadmin.username;
    document.getElementById("webadmin.password").value                  = settings.webadmin.password;

    // Experimental
    document.getElementById("other.backupDigiMode").checked             = settings.other.backupDigiMode;

    document.getElementById("other.lowPowerMode").checked               = settings.other.lowPowerMode;
    document.getElementById("other.lowVoltageCutOff").value             = settings.other.lowVoltageCutOff || 0

    updateImage();
    refreshSpeedStandard();
    toggleFields();
}

function showToast(message) {
    const el = document.querySelector('#toast');

    el.querySelector('.toast-body').innerHTML = message;

    (new bootstrap.Toast(el)).show();
}

document.getElementById('send-beacon').addEventListener('click', function (e) {
    e.preventDefault();

    fetch("/action?type=send-beacon", { method: "POST" });

    showToast("Your beacon will be sent in a moment. <br> <u>This action will be ignored if you have APRSIS and LoRa TX disabled!</u>");
});

document.getElementById('reboot').addEventListener('click', function (e) {
    e.preventDefault();

    fetch("/action?type=reboot", { method: "POST" });

    showToast("Your device will be rebooted in a while");
});

const bmeCheckbox = document.querySelector("input[name='bme.active']");

const stationModeSelect = document.querySelector("select[name='stationMode']");

function updateImage() {
    const value = document.getElementById("beacon.overlay").value + document.getElementById("beacon.symbol").value;

    const image = document.querySelector("img");

    switch (value) {
        case "L&":
            image.src =
                "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAHgAAAB4CAYAAAA5ZDbSAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAQ6SURBVHgB7d0/axRBGMfx50IMWPoKkpA02gUsLfZdqOCLOPBFWAliHbCUVNpaxQNrIY0WVjZaJlUsIsZ5uDu9bGb/zMwzs/PM/L7wIG4Ocflwl7ndvVsihBBCmTajunpk5uHG33+aOSGkvhdmLs1cW+bKzDEhle2a+UZ22Pacrx6PlMRYXc/arrkkIKvIBxfISgrBBXLmSeACOdMkcYGcWTFwgZxJMXGBPHEpcIE8USlxgSzcbGD2KD0ukIXKGRfIAuWOC+TANOACOSAtuEAemQ10a2P2KV9cIA+0ZZltMzurOaT8cYHcUx+wJlwgd9QFrBEXyJZswAekFxfIrdq4GhZUQO7Itlq+QzoXVEC21AdcIm51yF3AJeNWhWwDLmFBBeRVmg4/Atmj2nGLR/bCbZrm2qWx/+7Ekwx5i9LGO/XFzF2qO97/r5QAOSUwcG+WBDkVMHDtRUdOAcz/ed4J4NqLihwbGLjjioYcExi4bkVBjgUMXL/EkWMAAzcsUWRpYODKJIYsCQxc2USQpYCBG6dgZAlg4MYtCDkUGLhp8kYOAQZu2ryQfYGT4S7PAKJVzsg+wEmfubNZbd+XOpgTsiswXpbzaDSyCzBw82oU8lhg4ObZIPIYYODmXS/yEDBwdcQ+72w/6AMGrq6OaHnLghv1AQNXX6/bG7qAPxNwNbbf3mAD5qf5EaEisgG/IVRMNuA9QsVkA94mpLWP7Q2pP5uE4vayvQHA5bQw86m90Qb8i5C22Kyx/cAG/IGQphj3ftcPbcBzM78JaWiN+73rATZgfvAzWn4SHeXbIC7Xtcg6MfOUgJxro3C5vlU0kPNsNC439DYJyHnlhMuNeR8M5DxyxuXGHugA8rR54XIuR7KAPE3euJzroUogpy0Il/M5Fg3kNAXjcr4nG4AcNxFcLuTc78nqz7e0/A7KLDo9PbV+nkn6Q2xnZ2c0n88pQmK4XOjJ/eyQm6YhxYnichLng/FyLZM4Lid1wh/IYUXB5SSv6ACyX9FwOelLdoDsVlRcLsY1WUAeV3RcLtZFd0Duj3EfUGRcLuZVlUC2lwyXi32Re/D75IuLC1osFtYDFXxAo73dts21MV/8wgc6PEqKm7LHZv5QnndASTV8p5U9unl7oaJ6QvUi23CLA+YdqhGZcQ/Jfmu/olrvVE3Ia9wdqgB4sxp+JzPuPv2/+XV1lYzcxq32Q34lvlzbcKsF5h0vCZlxD+g2btXApSCvF1TbBOB/ba4oNSOvn7lr3KpWzC5pXHgVf4RKOk3P5CqOUEmn5eW6CxfAA2n4ndyHC+AR5YwMXOFyWngx7i4h8XJABm7kpkQGbqKmQAZu4lIiA3eiUiADd+JiIgM3k2IgAzezJJGBm2kSyMDNvBBk4CrJBxm4ynJB/kHAVRnf3OucumGvzBxTBZV+eouhn5u5t7HtvZlXhBBCCCEUsb+qGDWF0d8l5AAAAABJRU5ErkJggg==";
            break;
        case "L#":
            image.src =
                "data:image/png;base64,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";
            break;
        case "L_":
            image.src =
                "data:image/png;base64,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";
            break;
        case "La":
            image.src =
                "data:image/png;base64,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";
            break;
        case "I#":
            image.src = 
                "data:image/png;base64,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";
            break;
        case "/-":
            image.src = 
                "data:image/png;base64,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";
            break;
        default:
            image.src =
                "data:image/svg+xml;charset=utf-8,<svg version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"800px\" height=\"800px\" viewBox=\"0 0 395.001 395\" xml:space=\"preserve\"><g><g><path d=\"M322.852,0H72.15C32.366,0,0,32.367,0,72.15v250.7C0,362.634,32.367,395,72.15,395h250.701c39.784,0,72.15-32.366,72.15-72.15V72.15C395.002,32.367,362.635,0,322.852,0z M370.002,322.85c0,25.999-21.151,47.15-47.15,47.15H72.15C46.151,370,25,348.849,25,322.85V72.15C25,46.151,46.151,25,72.15,25h250.701c25.999,0,47.15,21.151,47.15,47.15L370.002,322.85L370.002,322.85z\"/><path d=\"M197.501,79.908c-33.775,0-61.253,27.479-61.253,61.254c0,6.903,5.596,12.5,12.5,12.5c6.904,0,12.5-5.597,12.5-12.5c0-19.99,16.263-36.254,36.253-36.254s36.253,16.264,36.253,36.254c0,11.497-8.314,19.183-22.01,30.474c-12.536,10.334-26.743,22.048-26.743,40.67v40.104c0,6.902,5.597,12.5,12.5,12.5c6.903,0,12.5-5.598,12.5-12.5v-40.104c0-6.832,8.179-13.574,17.646-21.381c13.859-11.426,31.106-25.646,31.106-49.763C258.754,107.386,231.275,79.908,197.501,79.908z\"/><path d=\"M197.501,283.024c-8.842,0-16.034,7.193-16.034,16.035c0,8.84,7.192,16.033,16.034,16.033c8.841,0,16.034-7.193,16.034-16.033C213.535,290.217,206.342,283.024,197.501,283.024z\"/></g></g></svg>";
            break;
    }
}

function toggleFields() {
    const sendExternalVoltageCheckbox = document.querySelector(
        'input[name="battery.sendExternalVoltage"]'
    );
    const externalVoltagePinInput = document.querySelector(
        'input[name="battery.externalVoltagePin"]'
    );

    externalVoltagePinInput.disabled = !sendExternalVoltageCheckbox.checked;
    voltageDividerR1.disabled = !sendExternalVoltageCheckbox.checked;
    voltageDividerR2.disabled = !sendExternalVoltageCheckbox.checked;

    const WebadminCheckbox = document.querySelector(
        'input[name="webadmin.active"]'
    );

    const WebadminUsername = document.querySelector(
        'input[name="webadmin.username"]'
    );

    const WebadminPassword = document.querySelector(
        'input[name="webadmin.password"]'
    );
    WebadminUsername.disabled = !WebadminCheckbox.checked;
    WebadminPassword.disabled = !WebadminCheckbox.checked;
}

const sendExternalVoltageCheckbox = document.querySelector(
    'input[name="battery.sendExternalVoltage"]'
);
const externalVoltagePinInput = document.querySelector(
    'input[name="battery.externalVoltagePin"]'
);

const voltageDividerR1 = document.querySelector(
    'input[name="battery.voltageDividerR1"]'
);

const voltageDividerR2 = document.querySelector(
    'input[name="battery.voltageDividerR2"]'
);

sendExternalVoltageCheckbox.addEventListener("change", function () {
    externalVoltagePinInput.disabled = !this.checked;
    voltageDividerR1.disabled = !this.checked;
    voltageDividerR2.disabled = !this.checked;
});

const WebadminCheckbox = document.querySelector(
    'input[name="webadmin.active"]'
);

const WebadminUsername = document.querySelector(
    'input[name="webadmin.username"]'
);

const WebadminPassword = document.querySelector(
    'input[name="webadmin.password"]'
);
WebadminCheckbox.addEventListener("change", function () {
    WebadminUsername.disabled = !this.checked;
    WebadminPassword.disabled = !this.checked;
});

document.querySelector(".new button").addEventListener("click", function () {
    const networksContainer = document.querySelector(".list-networks");

    let networkCount = document.querySelectorAll(".network").length;

    const networkElement = document.createElement("div");

    networkElement.classList.add("row", "network", "border-bottom", "py-2");

    // Increment the name, id, and for attributes
    const attributeName = `wifi.AP.${networkCount}`;
    networkElement.innerHTML = `
                <div class="form-floating col-6 col-md-5 px-1 mb-2">
                <input type="text" class="form-control form-control-sm" name="${attributeName}.ssid" id="${attributeName}.ssid" placeholder="" >
                <label for="${attributeName}.ssid">SSID</label>
                </div>
                <div class="form-floating col-6 col-md-5 px-1 mb-2">
                <input type="password" class="form-control form-control-sm" name="${attributeName}.password" id="${attributeName}.password" placeholder="">
                <label for="${attributeName}.password">Passphrase</label>
                </div>
                <div class="col-4 col-md-2 d-flex align-items-center justify-content-end">
                <div class="btn-group" role="group">
                    <button type="button" class="btn btn-sm btn-danger" title="Delete" onclick="return this.parentNode.parentNode.parentNode.remove();"><svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-trash3-fill" viewBox="0 0 16 16">
            <path d="M11 1.5v1h3.5a.5.5 0 0 1 0 1h-.538l-.853 10.66A2 2 0 0 1 11.115 16h-6.23a2 2 0 0 1-1.994-1.84L2.038 3.5H1.5a.5.5 0 0 1 0-1H5v-1A1.5 1.5 0 0 1 6.5 0h3A1.5 1.5 0 0 1 11 1.5m-5 0v1h4v-1a.5.5 0 0 0-.5-.5h-3a.5.5 0 0 0-.5.5M4.5 5.029l.5 8.5a.5.5 0 1 0 .998-.06l-.5-8.5a.5.5 0 1 0-.998.06m6.53-.528a.5.5 0 0 0-.528.47l-.5 8.5a.5.5 0 0 0 .998.058l.5-8.5a.5.5 0 0 0-.47-.528M8 4.5a.5.5 0 0 0-.5.5v8.5a.5.5 0 0 0 1 0V5a.5.5 0 0 0-.5-.5"/>
        </svg><span class="visually-hidden">Delete</span></button>
                </div>
                </div>
            `;
    networksContainer.appendChild(networkElement);

    networkCount++;

    // Add the new network element to the end of the document
    document.querySelector(".new").before(networkElement);
});

document
    .getElementById("action.symbol")
    .addEventListener("change", function () {
        const value = document.getElementById("action.symbol").value;

        document.getElementById("beacon.overlay").value = value[0];
        document.getElementById("beacon.symbol").value = value[1];

        updateImage();
    });

const speedStandards = {
    300: [125, 5, 12],
    244: [125, 6, 12],
    209: [125, 7, 12],
    183: [125, 8, 12],
    610: [125, 8, 10],
    1200: [125, 7, 9],
};

function refreshSpeedStandard() {
    const bw = Number(document.getElementById("lora.signalBandwidth").value);
    const cr4 = Number(document.getElementById("lora.codingRate4").value);
    const sf = Number(document.getElementById("lora.spreadingFactor").value);

    let found = false;

    for (const speed in speedStandards) {
        const standard = speedStandards[speed];

        if (standard[0] !== bw / 1000) continue;
        if (standard[1] !== cr4) continue;
        if (standard[2] !== sf) continue;

        document.getElementById("action.speed").value = speed;
        found = true;

        break;
    }

    if (!found) {
        document.getElementById("action.speed").value = "";
    }
}

document
    .getElementById("lora.signalBandwidth")
    .addEventListener("focusout", refreshSpeedStandard);
document
    .getElementById("lora.codingRate4")
    .addEventListener("focusout", refreshSpeedStandard);
document
    .getElementById("lora.spreadingFactor")
    .addEventListener("focusout", refreshSpeedStandard);

document.getElementById("action.speed").addEventListener("change", function () {
    const speed = document.getElementById("action.speed").value;

    if (speed !== "") {
        const value = speedStandards[Number(speed)];

        const bw = value[0];
        const cr4 = value[1];
        const sf = value[2];

        document.getElementById("lora.signalBandwidth").value = bw * 1000;
        document.getElementById("lora.codingRate4").value = cr4;
        document.getElementById("lora.spreadingFactor").value = sf;
    }
});

const form = document.querySelector("form");

const saveModal = new bootstrap.Modal(document.getElementById("saveModal"), {
    backdrop: "static",
    keyboard: false,
});

const savedModal = new bootstrap.Modal(
    document.getElementById("savedModal"),
    {}
);

function checkConnection() {
    const controller = new AbortController();

    setTimeout(() => controller.abort(), 2000);

    fetch("/status?_t=" + Date.now(), { signal: controller.signal })
        .then(() => {
            saveModal.hide();

            savedModal.show();

            setTimeout(function () {
                savedModal.hide();
            }, 3000);

            fetchSettings();
        })
        .catch((err) => {
            setTimeout(checkConnection, 0);
        });
}

form.addEventListener("submit", async (event) => {
    event.preventDefault();

    document.getElementById("wifi.APs").value =
        document.querySelectorAll(".network").length;

    fetch(form.action, {
        method: form.method,
        body: new FormData(form),
    });

    saveModal.show();

    setTimeout(checkConnection, 2000);
});

fetchSettings();

function loadReceivedPackets(packets) {
    if (packets) {
        document.querySelector('#received-packets tbody').innerHTML = '';

        const container = document.querySelector("#received-packets tbody");

        container.innerHTML = '';

        const date = new Date();

        packets.forEach((packet) => {
            const element = document.createElement("tr");

            date.setTime(packet.millis);

            const p = date.toUTCString().split(' ')
        
            element.innerHTML = `
                        <td>${p[p.length-2]}</td>
                        <td>${packet.packet}</td>
                        <td>${packet.RSSI}</td>
                        <td>${packet.SNR}</td>
                    `;

            container.appendChild(element);
        })
    }

    setTimeout(fetchReceivedPackets, 15000);
}

function fetchReceivedPackets() {
    fetch("/received-packets.json")
    .then((response) => response.json())
    .then((packets) => {
        loadReceivedPackets(packets);
    })
    .catch((err) => {
        console.error(err);

        console.error(`Failed to load received packets`);
    });
}

document.querySelector('a[href="/received-packets"]').addEventListener('click', function (e) {
    e.preventDefault();

    document.getElementById('received-packets').classList.remove('d-none');
    document.getElementById('configuration').classList.add('d-none');
    
    document.querySelector('button[type=submit]').remove();

    fetchReceivedPackets();
})