#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi access point
const char* ssid = "FruitClassifier";
const char* password = "fruit1234";

// Web server
WebServer server(80);

// Edge Impulse model input: 96 x 96 grayscale
const int IMAGE_WIDTH = 96;
const int IMAGE_HEIGHT = 96;
const int IMAGE_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT;

uint8_t imageBuffer[IMAGE_SIZE];

size_t receivedBytes = 0;
bool uploadValid = true;

// --------------------------------------------------
// Webpage served to your phone
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

    <button onclick="sendImage()">Send image</button>

    <p id="status">Waiting for image...</p>

<script>

async function sendImage() {

    const input = document.getElementById("photo");
    const status = document.getElementById("status");

    if (!input.files || input.files.length === 0) {
        status.textContent = "Please take a photo first.";
        return;
    }

    status.textContent = "Processing image...";

    const file = input.files[0];

    const img = new Image();
    const imageURL = URL.createObjectURL(file);

    img.src = imageURL;

    await img.decode();

    // Resize and center-crop to 96x96
    const canvas = document.getElementById("canvas");
    const ctx = canvas.getContext("2d", {
        willReadFrequently: true
    });

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

    status.textContent = "Uploading image...";

    // Send binary grayscale image
    const formData = new FormData();

    const blob = new Blob(
        [grayscale],
        { type: "application/octet-stream" }
    );

    formData.append("image", blob, "image.raw");

    try {

        const response = await fetch("/upload", {
            method: "POST",
            body: formData
        });

        const result = await response.json();

        if (!response.ok) {
            throw new Error(result.error || "Upload failed");
        }

        status.textContent =
            "Success! Received " + result.received + " pixels.";

    } catch (error) {

        status.textContent = "Error: " + error.message;

    }
}

</script>

</body>
</html>
)rawliteral";


// --------------------------------------------------
// Handle webpage request
// --------------------------------------------------

void handleRoot() {
    server.send(200, "text/html", webpage);
}


// --------------------------------------------------
// Receive uploaded image
// --------------------------------------------------

void handleUploadData() {

    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {

        receivedBytes = 0;
        uploadValid = true;

        Serial.println("Image upload started");

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
// Respond after upload completes
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
            "Incorrect size: %d bytes\n",
            (int)receivedBytes
        );

        return;
    }

    Serial.println("Image received successfully!");
    Serial.printf("Received: %d bytes\n", (int)receivedBytes);

    // Print a few pixel values for testing
    Serial.print("First 10 pixels: ");

    for (int i = 0; i < 10; i++) {
        Serial.print(imageBuffer[i]);
        Serial.print(" ");
    }

    Serial.println();

    server.send(
        200,
        "application/json",
        "{\"status\":\"ok\",\"received\":9216}"
    );
}


// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("Starting Fruit Classifier...");

    WiFi.mode(WIFI_AP);

    WiFi.softAP(ssid, password);

    Serial.println("Wi-Fi access point started");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);

    server.on(
        "/upload",
        HTTP_POST,
        handleUploadComplete,
        handleUploadData
    );

    server.begin();

    Serial.println("Web server started");
}


// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

    server.handleClient();

}