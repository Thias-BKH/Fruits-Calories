#include <esp_system.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Image_classification_Fruits__inferencing.h>

// ==================================================
// WIFI
// ==================================================

const char* ssid = "FruitClassifier";
const char* password = "fruit1234";

WebServer server(80);


// ==================================================
// IMAGE SETTINGS
// ==================================================

#define IMAGE_WIDTH  96
#define IMAGE_HEIGHT 96
#define IMAGE_SIZE   (IMAGE_WIDTH * IMAGE_HEIGHT)

// Each pixel is stored as 32-bit packed RGB: 0xRRGGBB
#define IMAGE_BYTES (IMAGE_SIZE * sizeof(uint32_t))

uint32_t imageBuffer[IMAGE_SIZE];

size_t receivedBytes = 0;
bool uploadValid = true;


// ==================================================
// EDGE IMPULSE: IMAGE SIGNAL
// ==================================================

int get_signal_data(size_t offset, size_t length, float *out_ptr) {

    if (offset + length > IMAGE_SIZE) {
        return -1;
    }

    for (size_t i = 0; i < length; i++) {

        // Each sample represents one packed RGB pixel.
        out_ptr[i] = (float)imageBuffer[offset + i];

    }

    return 0;
}


// ==================================================
// RUN CLASSIFICATION
// ==================================================

String classifyImage() {

    Serial.println("\n========== RGB CLASSIFICATION ==========");

    Serial.printf(
        "Free heap: %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "Largest free block: %u bytes\n",
        ESP.getMaxAllocHeap()
    );

    Serial.flush();


    // Prepare Edge Impulse signal

    signal_t signal;

    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &get_signal_data;

    ei_impulse_result_t result = { 0 };


    Serial.println("DEBUG 1: RGB signal prepared");
    Serial.flush();


    Serial.println("DEBUG 2: Calling run_classifier()");
    Serial.flush();


    EI_IMPULSE_ERROR error = run_classifier(
        &signal,
        &result,
        false
    );


    Serial.println("DEBUG 3: Classifier returned");

    Serial.printf(
        "Error code: %d\n",
        error
    );

    Serial.printf(
        "Free heap after inference: %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.flush();


    if (error != EI_IMPULSE_OK) {

        Serial.printf(
            "Classification failed: %d\n",
            error
        );

        return "{\"error\":\"Classification failed\"}";
    }


    // Print classification results

    Serial.println("\nClassification results:");

    int bestIndex = 0;
    float bestConfidence = 0;


    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {

        float confidence = result.classification[i].value;

        Serial.printf(
            "%s: %.2f%%\n",
            result.classification[i].label,
            confidence * 100
        );

        if (confidence > bestConfidence) {

            bestConfidence = confidence;
            bestIndex = i;

        }
    }


    // Get best prediction

    const char* label = result.classification[bestIndex].label;


    Serial.printf(
        "\nPrediction: %s (%.2f%%)\n",
        label,
        bestConfidence * 100
    );


    // Construct JSON response

    String response = "{";

    response += "\"label\":\"";
    response += label;

    response += "\",";

    response += "\"confidence\":";
    response += String(bestConfidence, 4);

    response += ",\"timing_ms\":";
    response += String(result.timing.classification);

    response += "}";


    Serial.println("========== CLASSIFICATION COMPLETE ==========");

    return response;
}


// ==================================================
// WEBPAGE
// ==================================================

const char webpage[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>RGB Fruit Classifier</title>

<style>

body {
    font-family: Arial, sans-serif;
    text-align: center;
    padding: 30px 15px;
    background: #f4f4f4;
}

button, input {
    margin: 15px;
    padding: 12px;
    font-size: 16px;
}

canvas {
    width: 192px;
    height: 192px;
    image-rendering: pixelated;
    border: 2px solid #333;
    margin-top: 15px;
}

#status {
    margin-top: 20px;
    font-weight: bold;
}

#result {
    font-size: 28px;
    margin-top: 20px;
}

</style>

</head>


<body>

<h1>RGB Fruit Classifier</h1>

<p>Take a photo of a fruit.</p>


<input
    type="file"
    id="photo"
    accept="image/*"
    capture="environment"
>

<br>


<canvas
    id="canvas"
    width="96"
    height="96">
</canvas>

<br>


<button onclick="sendImage()">
    Classify fruit
</button>


<p id="status">
    Waiting for image...
</p>


<div id="result"></div>


<script>


// ==================================================
// PROCESS AND SEND IMAGE
// ==================================================

async function sendImage() {

    const input = document.getElementById("photo");

    const status = document.getElementById("status");

    const result = document.getElementById("result");


    if (!input.files || input.files.length === 0) {

        status.textContent = "Please take a photo first.";

        return;

    }


    status.textContent = "Processing RGB image...";

    result.textContent = "";


    const file = input.files[0];

    const img = new Image();

    const imageURL = URL.createObjectURL(file);

    img.src = imageURL;


    try {

        await img.decode();


        // ------------------------------------------
        // RESIZE IMAGE
        // ------------------------------------------

        const canvas = document.getElementById("canvas");

        const ctx = canvas.getContext("2d", {
            willReadFrequently: true
        });


        // Center crop to square

        const side = Math.min(img.width, img.height);

        const sx = (img.width - side) / 2;

        const sy = (img.height - side) / 2;


        ctx.drawImage(
            img,
            sx, sy, side, side,
            0, 0, 96, 96
        );


        // ------------------------------------------
        // EXTRACT RGB PIXELS
        // ------------------------------------------

        const imageData = ctx.getImageData(
            0,
            0,
            96,
            96
        );

        const pixels = imageData.data;


        // One 32-bit packed RGB value per pixel.
        // Format: 0xRRGGBB

        const rgbPixels = new Uint32Array(96 * 96);


        for (let i = 0; i < rgbPixels.length; i++) {

            const r = pixels[i * 4];

            const g = pixels[i * 4 + 1];

            const b = pixels[i * 4 + 2];


            rgbPixels[i] =
                (r << 16) |
                (g << 8) |
                b;

        }


        URL.revokeObjectURL(imageURL);


        // ------------------------------------------
        // SEND RGB IMAGE
        // ------------------------------------------

        status.textContent = "Sending RGB image...";


        const formData = new FormData();


        const blob = new Blob(
            [rgbPixels.buffer],
            {
                type: "application/octet-stream"
            }
        );


        formData.append(
            "image",
            blob,
            "image.rgb"
        );


        const response = await fetch("/upload", {

            method: "POST",

            body: formData

        });


        const data = await response.json();


        if (!response.ok) {

            throw new Error(
                data.error || "Classification failed"
            );

        }


        if (data.error) {

            throw new Error(data.error);

        }


        // ------------------------------------------
        // DISPLAY RESULT
        // ------------------------------------------

        status.textContent = "Classification complete!";


        result.innerHTML = `

            <strong>${data.label}</strong>

            <br>

            ${(data.confidence * 100).toFixed(1)}%

        `;


    }

    catch (error) {

        status.textContent = "Error: " + error.message;

    }

}


</script>

</body>

</html>

)rawliteral";


// ==================================================
// SERVE WEBPAGE
// ==================================================

void handleRoot() {

    server.send(
        200,
        "text/html",
        webpage
    );

}


// ==================================================
// RECEIVE IMAGE UPLOAD
// ==================================================

void handleUploadData() {

    HTTPUpload& upload = server.upload();


    // ----------------------------------------------
    // UPLOAD START
    // ----------------------------------------------

    if (upload.status == UPLOAD_FILE_START) {

        receivedBytes = 0;

        uploadValid = true;

        Serial.println("\nRGB image upload started");

    }


    // ----------------------------------------------
    // RECEIVE DATA
    // ----------------------------------------------

    else if (upload.status == UPLOAD_FILE_WRITE) {

        if (
            receivedBytes + upload.currentSize
            <= IMAGE_BYTES
        ) {

            memcpy(

                ((uint8_t*)imageBuffer) + receivedBytes,

                upload.buf,

                upload.currentSize

            );

            receivedBytes += upload.currentSize;

        }

        else {

            uploadValid = false;

            Serial.println("ERROR: RGB image too large");

        }

    }


    // ----------------------------------------------
    // UPLOAD FINISHED
    // ----------------------------------------------

    else if (upload.status == UPLOAD_FILE_END) {

        Serial.println("RGB image upload finished");

        Serial.printf(
            "Received bytes: %u\n",
            (unsigned int)receivedBytes
        );

    }

}


// ==================================================
// HANDLE COMPLETED UPLOAD
// ==================================================

void handleUploadComplete() {


    // ----------------------------------------------
    // CHECK UPLOAD SIZE
    // ----------------------------------------------

    if (!uploadValid) {

        server.send(
            413,
            "application/json",
            "{\"error\":\"Image too large\"}"
        );

        return;

    }


    if (receivedBytes != IMAGE_BYTES) {

        Serial.printf(
            "Incorrect RGB image size: %u\n",
            (unsigned int)receivedBytes
        );

        server.send(
            400,
            "application/json",
            "{\"error\":\"Incorrect RGB image size\"}"
        );

        return;

    }


    Serial.println("\nRGB image received successfully");

    Serial.printf(
        "Pixels: %d\n",
        IMAGE_SIZE
    );

    Serial.printf(
        "Bytes: %u\n",
        (unsigned int)receivedBytes
    );


    Serial.println("DEBUG: Preparing RGB classification");

    Serial.printf(
        "Free heap: %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "Largest free block: %u bytes\n",
        ESP.getMaxAllocHeap()
    );

    Serial.flush();


    // ----------------------------------------------
    // CLASSIFY
    // ----------------------------------------------

    String response = classifyImage();


    // ----------------------------------------------
    // SEND RESULT TO PHONE
    // ----------------------------------------------

    server.send(
        200,
        "application/json",
        response
    );

}


// ==================================================
// SETUP
// ==================================================

void setup() {

    Serial.begin(115200);

    delay(1500);


    Serial.println("\nStarting RGB Fruit Classifier");

    Serial.printf(
        "Reset reason: %d\n",
        (int)esp_reset_reason()
    );


    // ----------------------------------------------
    // START WIFI ACCESS POINT
    // ----------------------------------------------

    WiFi.mode(WIFI_AP);

    WiFi.softAP(
        ssid,
        password
    );


    Serial.print("Access point IP: ");

    Serial.println(WiFi.softAPIP());


    // ----------------------------------------------
    // REGISTER ROUTES
    // ----------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        handleRoot
    );


    server.on(
        "/upload",
        HTTP_POST,
        handleUploadComplete,
        handleUploadData
    );


    // ----------------------------------------------
    // START SERVER
    // ----------------------------------------------

    server.begin();


    // ----------------------------------------------
    // MEMORY DEBUG
    // ----------------------------------------------

    Serial.println("\n--- MEMORY DEBUG ---");

    Serial.printf(
        "Free heap: %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "Largest free block: %u bytes\n",
        ESP.getMaxAllocHeap()
    );

    Serial.printf(
        "PSRAM available: %u bytes\n",
        ESP.getFreePsram()
    );

    Serial.printf(
        "RGB image buffer: %u bytes\n",
        (unsigned int)sizeof(imageBuffer)
    );

    Serial.println("--------------------\n");


    Serial.println("RGB web server started");

}


// ==================================================
// MAIN LOOP
// ==================================================

void loop() {

    server.handleClient();

}