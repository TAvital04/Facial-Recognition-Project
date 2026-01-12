#include <string.h>

// Interface
  #define BUTTON_UPLOAD    13
  #define BUTTON_VALIDATE  14
  #define BUTTON_DELETE     2

// Camera
  #include "esp_camera.h"

  // Pins that perform power/reset operations
  #define CAM_PIN_PWDN    32  // Power down pin
  #define CAM_PIN_RESET   -1  // Reset is performed through software

  // Pin that drives the camera's internal clock
  #define CAM_PIN_XCLK    0

  // Pins that send config messages
  #define CAM_PIN_SIOD    26  // Data wire used to send the messages
  #define CAM_PIN_SIOC    27  // Clock wire used to synchronize message timing

  // Mega highway of pins for sending the images
    // Each digit of each 8-bit word is moved by each pin, 1 pulse at a time
    #define CAM_PIN_D7      35
    #define CAM_PIN_D6      34
    #define CAM_PIN_D5      39
    #define CAM_PIN_D4      36
    #define CAM_PIN_D3      21
    #define CAM_PIN_D2      19
    #define CAM_PIN_D1      18
    #define CAM_PIN_D0       5

    // Clock wires
    #define CAM_PIN_VSYNC   25  // Marks the start of an image
    #define CAM_PIN_HREF    23  // On high while a row is being sent
    #define CAM_PIN_PCLK    22  // On high when a new word should be read

  // Camera config data structure
  static camera_config_t camera_config = {
    // Pin assignment
    .pin_pwdn      = CAM_PIN_PWDN,
    .pin_reset     = CAM_PIN_RESET,
    .pin_xclk      = CAM_PIN_XCLK,
    .pin_sccb_sda  = CAM_PIN_SIOD,
    .pin_sccb_scl  = CAM_PIN_SIOC,

    .pin_d7        = CAM_PIN_D7,
    .pin_d6        = CAM_PIN_D6,
    .pin_d5        = CAM_PIN_D5,
    .pin_d4        = CAM_PIN_D4,
    .pin_d3        = CAM_PIN_D3,
    .pin_d2        = CAM_PIN_D2,
    .pin_d1        = CAM_PIN_D1,
    .pin_d0        = CAM_PIN_D0,
    .pin_vsync     = CAM_PIN_VSYNC,
    .pin_href      = CAM_PIN_HREF,
    .pin_pclk      = CAM_PIN_PCLK,

    // Camera's internal clock
    .xclk_freq_hz  = 20000000,               // Frequency

    .ledc_timer    = LEDC_TIMER_0,           // Internal clock mechanism
    .ledc_channel  = LEDC_CHANNEL_0,         // Internal clock output

    // Image configuration
    .pixel_format  = PIXFORMAT_JPEG,         // Image format
    .frame_size    = FRAMESIZE_SVGA,         // Resolution
    .jpeg_quality  = 15,                     // Compression level (lower number = higher quality)
    .fb_count      = 1,                      // Number of frame buffers
    .grab_mode     = CAMERA_GRAB_WHEN_EMPTY  // Only write into a buffer when it's empty
  };

// HTTP Request
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
  #include <HTTPClient.h>

  static const char *WIFI_SSID = "Avital Wi-Fi";
  static const char *WIFI_PASS = "asdfghjk";

  static const char *LAMBDA_URL = "https://z7tejhg75732zty5kv7fnodile0mgdjh.lambda-url.us-east-2.on.aws/";

  static const char *DEVICE_TOKEN = "SPAGHETTI";

  static const char *LAMBDA_OPERATION_UPLOAD = "upload";
  static const char *LAMBDA_OPERATION_VALIDATE = "validate";
  static const char *LAMBDA_OPERATION_DELETE = "delete";

// Functions
  void take_picture(camera_fb_t **image_buffer) {
    Serial.printf("Taking a picture.\n");
    *image_buffer = esp_camera_fb_get();

    if(!*image_buffer) {
      Serial.printf("Failed to take a picture.\n");
    }
  }

  int send_image(camera_fb_t **image_buffer, const char *operation) {
    // Declare variables
    WiFiClientSecure client;
    HTTPClient http;
    int code;
    String response;
    const char *response_cstr;
    int result = 1;

    // Send the picture to AWS
    Serial.printf("Establishing a connection with the AWS Lambda function.\n");
    client.setInsecure();
    http.setConnectTimeout(60000);
    http.setTimeout(60000);

    if(!http.begin(client, LAMBDA_URL)) {
      Serial.printf("Failed to establish a connection with the AWS Lambda function.\n");
      goto cleanup;
    }

    Serial.printf("Sending the picture to the AWS Lambda function.\n");
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("device-token", DEVICE_TOKEN);
    http.addHeader("operation", operation);

    code = http.sendRequest("POST", (*image_buffer)->buf, (*image_buffer)->len);

    Serial.printf("Getting the response from the AWS Lambda function.\n");
    response = http.getString();
    response_cstr = response.c_str();

    if(strcmp(response_cstr, "true") == 0) {
      result = 1;
    } else {
      result = 0;
    }
    
    Serial.printf("%s\n%d\n", response.c_str(), code);

  cleanup:
    http.end();

    if(*image_buffer) {
      esp_camera_fb_return(*image_buffer);
      *image_buffer = nullptr;
    }

    return result;
  }

// Setup
  void setup() {
    Serial.begin(115200);
    delay(300);

    // Initialize the pins
    pinMode(BUTTON_UPLOAD, INPUT_PULLUP);
    pinMode(BUTTON_VALIDATE, INPUT_PULLUP);
    pinMode(BUTTON_DELETE, INPUT_PULLUP);

    // Initialize the camera
    Serial.printf("Initializing the camera.\n");
    esp_err_t status = esp_camera_init(&camera_config);

    if(status != ESP_OK) {
      Serial.printf("Failed to initialize camera: 0x%x.\n", status);
      return;
    }

    // Connect to the WIFI
    Serial.printf("Connecting to the WiFi.\n");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while(WiFi.status() != WL_CONNECTED) {
      delay(200);
    }

    Serial.printf("Setup complete.\n");
  }

  void loop() {
    if(!(digitalRead(BUTTON_UPLOAD) && digitalRead(BUTTON_VALIDATE) && digitalRead(BUTTON_DELETE))) {
      // Declare variables
      camera_fb_t *buffer = nullptr;
      int result = 0;

      // Perform operation
      take_picture(&buffer);

      if(!digitalRead(BUTTON_UPLOAD)) {
        Serial.printf("Operation selected: upload a face.\n");        
        if(buffer) {
          result = send_image(&buffer, LAMBDA_OPERATION_UPLOAD);
        }
      } else if(!digitalRead(BUTTON_VALIDATE)) {
        Serial.printf("Operation selected: validate a face.\n");        
        if(buffer) {
          send_image(&buffer, LAMBDA_OPERATION_VALIDATE);
        }
      } else if(!digitalRead(BUTTON_DELETE)) {
        Serial.printf("Operation selected: delete a face.\n");
        if(buffer) {
          send_image(&buffer, LAMBDA_OPERATION_DELETE);
        }
      }

      Serial.printf("Ready to take another picture.\n");
    }
  }