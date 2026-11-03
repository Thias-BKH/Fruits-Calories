#include <esp_system.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Image_classification_Fruits__inferencing.h>

// --------------------------------------------------
// Wi-Fi
// --------------------------------------------------

const char* ssid = "FruitClassifier";
const char* password = "fruit1234";

WebServer server(80);

// --------------------------------------------------
// Image buffer
// --------------------------------------------------

#define IMAGE_WIDTH 96
#define IMAGE_HEIGHT 96
#define IMAGE_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT)

uint8_t imageBuffer[IMAGE_SIZE];

size_t receivedBytes = 0;
bool uploadValid = true;


// --------------------------------------------------
// Edge Impulse: Read image data
// --------------------------------------------------

int get_signal_data(size_t offset, size_t length, float *out_ptr) {

    if (offset + length > IMAGE_SIZE) {
        return -1;
    }

    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = imageBuffer[offset + i];
    }

    return 0;
}


// --------------------------------------------------
// Run classification
// --------------------------------------------------

String classifyImage() {

    Serial.println("\n========== CLASSIFICATION ==========");
    Serial.printf("Free heap before inference: %u\n", ESP.getFreeHeap());
    Serial.printf("Largest block before inference: %u\n", ESP.getMaxAllocHeap());
    Serial.flush();

    signal_t signal;

    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &get_signal_data;

    ei_impulse_result_t result = { 0 };

    Serial.println("DEBUG 1: Signal prepared");
    Serial.flush();

    Serial.println("DEBUG 2: Calling run_classifier()");
    Serial.flush();

    EI_IMPULSE_ERROR error = run_classifier(
        &signal,
        &result,
        false
    );

    Serial.println("DEBUG 3: run_classifier() returned");
    Serial.printf("Error code: %d\n", error);
    Serial.printf("Free heap after inference: %u\n", ESP.getFreeHeap());
    Serial.flush();

    if (error != EI_IMPULSE_OK) {

        Serial.printf("Classification failed: %d\n", error);

        return "{\"error\":\"Classification failed\"}";
    }

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

    const char* label = result.classification[bestIndex].label;

    Serial.printf(
        "\nPrediction: %s (%.2f%%)\n",
        label,
        bestConfidence * 100
    );

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

// --------------------------------------------------
// Webpage
// --------------------------------------------------

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>Fruit Classifier</title>

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

<h1>Fruit Classifier</h1>

<p>Take a photo of a fruit.</p>

<input
    type="file"
    id="photo"
    accept="image/*"
    capture="environment"
>

<br>

<canvas id="canvas" width="96" height="96"></canvas>

<br>

<button onclick="sendImage()">Classify fruit</button>

<p id="status">Waiting for image...</p>

<div id="result"></div>

<script>

async function sendImage() {

    const input = document.getElementById("photo");
    const status = document.getElementById("status");
    const result = document.getElementById("result");

    if (!input.files || input.files.length === 0) {
        status.textContent = "Please take a photo first.";
        return;
    }

    status.textContent = "Processing image...";
    result.textContent = "";

    const file = input.files[0];

    const img = new Image();
    const imageURL = URL.createObjectURL(file);

    img.src = imageURL;

    try {

        await img.decode();

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

        // Convert RGB to grayscale
        const imageData = ctx.getImageData(0, 0, 96, 96);
        const pixels = imageData.data;

        const grayscale = new Uint8Array(96 * 96);

        for (let i = 0; i < grayscale.length; i++) {

            const r = pixels[i * 4];
            const g = pixels[i * 4 + 1];
            const b = pixels[i * 4 + 2];

            grayscale[i] = Math.round(
                0.299 * r +
                0.587 * g +
                0.114 * b
            );
        }

        URL.revokeObjectURL(imageURL);

        status.textContent = "Sending image and classifying...";

        const formData = new FormData();

        const blob = new Blob(
            [grayscale],
            { type: "application/octet-stream" }
        );

        formData.append("image", blob, "image.raw");

        const response = await fetch("/upload", {
            method: "POST",
            body: formData
        });

        const data = await response.json();

        if (!response.ok) {
            throw new Error(data.error || "Classification failed");
        }

        status.textContent = "Classification complete!";

        result.innerHTML = `
            <strong>${data.label}</strong>
            <br>
            ${(data.confidence * 100).toFixed(1)}%
        `;

    } catch (error) {

        status.textContent = "Error: " + error.message;

    }

}

</script>

</body>
</html>
)rawliteral";


// --------------------------------------------------
// Serve webpage
// --------------------------------------------------

void handleRoot() {

    server.send(
        200,
        "text/html",
        webpage
    );

}


// --------------------------------------------------
// Receive uploaded image
// --------------------------------------------------

void handleUploadData() {

    HTTPUpload& upload = server.upload();


    if (upload.status == UPLOAD_FILE_START) {


        receivedBytes = 0;
        uploadValid = true;

        Serial.println("\nImage upload started");

    }

    else if (upload.status == UPLOAD_FILE_WRITE) {

        if (receivedBytes + upload.currentSize <= IMAGE_SIZE) {

            memcpy(
                imageBuffer + receivedBytes,
                upload.buf,
                upload.currentSize
            );


            receivedBytes += upload.currentSize;

        } else {

            uploadValid = false;

        }

    }

    else if (upload.status == UPLOAD_FILE_END) {

        Serial.println("Image upload finished");

    }

}


// --------------------------------------------------
// Handle completed upload
// --------------------------------------------------

void handleUploadComplete() {
    
    if (!uploadValid) {

        server.send(
            413,
            "application/json",
            "{\"error\":\"Image too large\"}"
        );

        return;
    }

    if (receivedBytes != IMAGE_SIZE) {

        server.send(
            400,
            "application/json",
            "{\"error\":\"Incorrect image size\"}"
        );

        Serial.printf(
            "Incorrect image size: %d\n",
            (int)receivedBytes
        );

        return;
    }

    Serial.printf(
        "Received %d image pixels\n",
        (int)receivedBytes
    );

    Serial.println("DEBUG: Upload complete, starting classification");
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("Largest free block: %u\n", ESP.getMaxAllocHeap());
    Serial.flush();
    
    String response = classifyImage();

    server.send(
        200,
        "application/json",
        response
    );

}


// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

    Serial.begin(115200);

    Serial.printf("Reset reason: %d\n", (int)esp_reset_reason());


    delay(1500);

    Serial.println("\nStarting Fruit Classifier");

    WiFi.mode(WIFI_AP);

    WiFi.softAP(ssid, password);

    Serial.print("Access point IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);

    server.on(
        "/upload",
        HTTP_POST,
        handleUploadComplete,
        handleUploadData
    );

    server.begin();
    
    Serial.println("\n--- MEMORY DEBUG ---");
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Largest free block: %u bytes\n", ESP.getMaxAllocHeap());
    Serial.printf("PSRAM available: %u bytes\n", ESP.getFreePsram());
    Serial.println("--------------------\n");

    Serial.println("Web server started");

}


// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

    server.handleClient();

}