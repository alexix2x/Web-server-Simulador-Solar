#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

const char* ssid = "-------";
const char* password = "------";
// Definir los pines GPIO para los MOSFETs y el driver de LEDs
/*
MOSFET_1_PIN 10
MOSFET_2_PIN 7
MOSFET_3_PIN 6
LED_DRIVER_PIN 8
// Definir los pines para los sensores LM35
TEMP_SENSOR_PIN1 2
TEMP_SENSOR_PIN2 3
TEMP_SENSOR_PIN3 4
TEMP_SENSOR_PIN4 5
*/
const int ledPins[] = {18, 19, 21, 22}; // Pines de los LEDs UV
const int tempPins[] = {34, 35, 32, 33}; // Pines de los sensores de temperatura


// Canales PWM
const int pwmChannel1 = 0;
const int pwmChannel2 = 1;
const int pwmChannel3 = 2;

// Frecuencia y resolución PWM
const int freq = 5000;
const int resolution = 8;

// Valores de PWM
int pwmValues[] = {0, 0, 0};

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

String getTemperatureJSON() {
    float temperatures[4];
    for (int i = 0; i < 4; i++) {
        /*int analogValue = analogRead(tempPins[i]);
        float voltage = analogValue * (5.0 / 4095.0); // Convertir a voltaje
        float temperatureC = voltage * 100.0; // Convertir a temperatura
        temperatures[i] = temperatureC;*/
        temperatures[i] = 50;
    }

    String json = "{";
    json += "\"temp1\": " + String(temperatures[0]) + ",";
    json += "\"temp2\": " + String(temperatures[1]) + ",";
    json += "\"temp3\": " + String(temperatures[2]) + ",";
    json += "\"temp4\": " + String(temperatures[3]);
    json += "}";
    return json;
}

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < 4; i++) {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW); // Asegurarse de que los LEDs estén apagados al inicio
    }

    // Configurar y asignar los canales PWM a los pines
    ledcSetup(pwmChannel1, freq, resolution);
    ledcSetup(pwmChannel2, freq, resolution);
    ledcSetup(pwmChannel3, freq, resolution);

    ledcAttachPin(ledPins[0], pwmChannel1);
    ledcAttachPin(ledPins[1], pwmChannel2);
    ledcAttachPin(ledPins[2], pwmChannel3);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Control de LEDs y Lectura de Temperatura</title>
            <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css">
            <script src="https://cdnjs.cloudflare.com/ajax/libs/raphael/2.2.7/raphael.min.js"></script>
            <script src="https://cdnjs.cloudflare.com/ajax/libs/justgage/1.2.9/justgage.min.js"></script>
            <style>
                @font-face {
                    font-family: 'RobotoMedium';
                    src: url('fonts/Roboto-Medium.ttf') format('truetype');
                }
                body {
                    font-family: 'RobotoMedium', sans-serif;
                    background-color: #f0f0f0;
                }
                .gauge {
                    width: 200px;
                    height: 160px;
                    margin: 0 auto;
                }
                .container {
                    margin-top: 50px;
                }
                .led {
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                    margin: 10px 0;
                }
                .led-indicator {
                    border-radius: 5px;
                    width: 10px;
                    height: 10px;
                    box-shadow: 0px 0px 3px black;
                    margin: 5px;
                    zoom: 5;
                }
                .led-indicator:after {
                    display: block;
                    content: '';
                    margin-left: 1px;
                    margin-right: 1px;
                    width: 8px;
                    height: 6px;
                    border-top-right-radius: 4px 3px;
                    border-top-left-radius: 4px 3px;
                    border-bottom-right-radius: 4px 3px;
                    border-bottom-left-radius: 4px 3px;
                    background-image: linear-gradient(to top, rgba(255,255,255,0.8) 0%, rgba(255,255,255,0.2) 100%);
                    -webkit-border-top-right-radius: 4px 3px;
                    -webkit-border-top-left-radius: 4px 3px;
                    -webkit-border-bottom-right-radius: 4px 3px;
                    -webkit-border-bottom-left-radius: 4px 3px;
                    background-image: -webkit-linear-gradient(top, rgba(255,255,255,0.8) 0%, rgba(255,255,255,0.2) 100%);
                }
                .on {
                    background-image: linear-gradient(to top, #079C42 0%, #079C42 50%, #079C42 100%);
                    background-image: -webkit-linear-gradient(top, #079C42 0%, #079C42 50%, #079C42 100%);
                }
                .off {
                    background-image: linear-gradient(to top, #0C4D17 0%, #0C4D17 50%, #0C4D17 100%);
                    background-image: -webkit-linear-gradient(top, #0C4D17 0%, #0C4D17 50%, #0C4D17 100%);
                }
            </style>
        </head>
        <body>
           <div class="container text-center">
                <h1>Control de Luz Solar UV</h1>
                <div class="row justify-content-center">
                    <div class="col-md-3">
                        <div class="led" id="led1">
                            <div class="off led-indicator"></div>
                            <h6>LED-385</h6>
                        </div>
                    </div>
                    <div class="col-md-3">
                        <div class="led" id="led2">
                            <div class="off led-indicator"></div>
                            <h6>LED-275</h6>
                        </div>
                    </div>
                    <div class="col-md-3">
                        <div class="led" id="led3">
                            <div class="off led-indicator"></div>
                            <h6>LED-405</h6>
                        </div>
                    </div>
                    <div class="col-md-3">
                        <div class="led" id="led4">
                            <div class="off led-indicator"></div>
                            <h6>LED-395</h6>
                        </div>
                    </div>
                </div>
                <div class="row justify-content-center mt-4">
                    <div class="col-md-4">
                        <div class="led" id="led5">
                            <div class="off led-indicator"></div>
                            <h6>LED-350</h6>
                        </div>
                    </div>
                    <div class="col-md-4">
                        <div class="led" id="led6">
                            <div class="off led-indicator"></div>
                            <h6>LED-650</h6>
                        </div>
                    </div>
                    <div class="col-md-4">
                        <div class="led" id="led7">
                            <div class="off led-indicator"></div>
                            <h6>LED-265</h6>
                        </div>
                    </div>
                </div>
                <div class="row justify-content-center mt-4">
                    <div class="col-md-3">
                        <div id="gauge1" class="gauge"></div>
                        <h4>Temperatura 1</h4>
                    </div>
                    <div class="col-md-3">
                        <div id="gauge2" class="gauge"></div>
                        <h4>Temperatura 2</h4>
                    </div>
                    <div class="col-md-3">
                        <div id="gauge3" class="gauge"></div>
                        <h4>Temperatura 3</h4>
                    </div>
                    <div class="col-md-3">
                        <div id="gauge4" class="gauge"></div>
                        <h4>Temperatura 4</h4>
                    </div>
                </div>
                <button id="toggleButton" class="btn btn-primary mt-4">Encender</button>
            </div>
            <script>
                document.addEventListener('DOMContentLoaded', function() {
                    const gauges = [];
                    const button = document.getElementById('toggleButton');
                    const leds = document.querySelectorAll('.led .led-indicator');

                    function createGauge(id, value) {
                        return new JustGage({
                            id: id,
                            value: value,
                            min: 0,
                            max: 70,
                            symbol: '°C',
                            pointer: true,
                            gaugeWidthScale: 0.6,
                            counter: true,
                            relativeGaugeSize: true,
                            pointerOptions: {
                                color: '#000000'
                            },
                            gaugeColor: '#E0E0E0',
                            levelColors: [
                                "#00FF00",
                                "#FFFF00",
                                "#FF0000"
                            ]
                        });
                    }

                    for (let i = 1; i <= 4; i++) {
                        gauges.push(createGauge(`gauge${i}`, 0));
                    }

                    async function fetchTemperatures() {
                        try {
                            const response = await fetch('/temperature');
                            const data = await response.json();
                            gauges[0].refresh(data.temp1);
                            gauges[1].refresh(data.temp2);
                            gauges[2].refresh(data.temp3);
                            gauges[3].refresh(data.temp4);
                        } catch (error) {
                            console.error('Error fetching temperatures:', error);
                        }
                    }

                    async function toggleLEDs() {
                        try {
                            const response = await fetch('/toggle');
                            const data = await response.json();
                            data.ledStates.forEach((state, index) => {
                                leds[index].className = state ? 'on led-indicator' : 'off led-indicator';
                            });
                            button.textContent = data.ledDriverOn ? 'Apagar' : 'Encender';
                        } catch (error) {
                            console.error('Error toggling LEDs:', error);
                        }
                    }

                    button.addEventListener('click', toggleLEDs);

                    setInterval(fetchTemperatures, 10000);

                    fetchTemperatures();
                    toggleLEDs(); // Inicializar estado de los LEDs y botón
                });
            </script>
        </body>
        </html>)rawliteral";
        request->send(200, "text/html", html);
    });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = getTemperatureJSON();
        request->send(200, "application/json", json);
    });

    server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
        static bool ledDriverOn = false;
        ledDriverOn = !ledDriverOn;
        bool ledStates[7];
        for (int i = 0; i < 7; i++) {
            if (i < 3){
                // Controlar LEDs con PWM - Mosfet
                int pwmValue = ledDriverOn ? 255 : 0;
                ledcWrite(i, pwmValue);
                pwmValues[i] = pwmValue; // Guardar el valor PWM
                Serial.print("Mosfet Activo en pin ");
                Serial.print(ledPins[i]);
                Serial.print(": ");
                Serial.println(pwmValue == 255 ? "Encendido" : "Apagado");
            } else if (i == 3) {
                digitalWrite(ledPins[i], ledDriverOn ? HIGH : LOW);   
                Serial.print("Estado del LED en el pin ");
                Serial.print(ledPins[3]);
                Serial.print(": ");
                Serial.println(ledDriverOn ? "Encendido" : "Apagado");
            }
            ledStates[i] = ledDriverOn;
        }

        String json = "{";
        json += "\"ledDriverOn\": " + String(ledDriverOn) + ",";
        json += "\"ledStates\": [";
        for (int i = 0; i < 7; i++) {
            json += String(ledStates[i]) + (i < 6 ? "," : "");
        }
        json += "]}";
        request->send(200, "application/json", json);
    });

    server.onNotFound(notFound);

    server.begin();
}

void loop() {
    // No se requiere código en el loop
}
