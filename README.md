# Facial Recognition Project
The following documents described the structure and code of the Facial Recognition Project.

## Contents 
1. Introduction
2. Hardware
3. Firmware
  
     3.1. Environment

     3.2. Code

4. Software

  4.1. Environment 
  
  4.2. Code

5. Pipeline of Information
  
## 1. Introduction 
This project serves as a device that can recognize a person's face and send a response. It contains hardware, firmware, and software implementations. This docment will demonstrate each element- how they work, and how they work with the others. 

## 2. Hardware 
Facial Recognition Project uses the FTDI YP-05 Serial Adapter to connect to an ESP32-CAM. The serial adapter is only necessary for uploading code, but is not required in order to run it. 

The serial adapter connects to a PC using a Mini-USB, but its connection to the ESP32-CAM is a bit more complicated. 
| Adapter | Microcontroller | Purpose |
|-|-|------------------------------|
| VCC (5V) | 5V | Power the ESP32-CAM. Must be 5 volts, not 3.3! |
| GND | GND | Establish a "Low" or "Zero" and close the circuit. |
| TX | RX | Send data from the adapter to the microcontroller. |
|RX | TX | Send data from the microcontroller to the adapter. |

There are also buttons that connect to GPIO pins these should be wired between a GPIO and GND pin of the microcontroller. 

> [!Warning]
> The buttons are directed to GPIO pins. Any user intending to replicate this project must remember that these pins may occasionally suit additional purposes within their microcontroller. Interfering with these operations may cause problems. Please consult the manual and documentation for your device before accessing its GPIO pins.
> The program is power-intensive and will not run on 3.3V. Send 5V and proceed with caution.
>
> Do not use this document as a complete tutorial for this operation. Consult your device and other sources. Make informed decisions!

## 3. Firmware 
This part will be about the firmware of the Facial Recognition Project.

### 3.1. Environment 
The firmware of this project is written, compiled, and uploaded to the microcontroller with the Arduino IDE. The firmware is written in C++ and is completely encapsulated within /Facial-Recognition-Project/Embedded Software/take_and_send/take_and_send.ino.
A developer may open this program in the IDE, and it would work, but they will need to make changes to the following, directly pasted from the document: 
```c++
// /Facial-Recognition-Project/Embedded Software/take_and_send/take_and_send.ino

                                         /* CHANGE THIS */
/*********************************************************
  static const char *WIFI_SSID = "*****";
  static const char *WIFI_PASS = "*****";
  static const char *LAMBDA_URL = "*****";
  static const char *DEVICE_TOKEN = "*****";
**********************************************************/
``` 

### 3.2. Code
The code for the firmware contains 3 distinct components for dictating the hardware interface, establishing a connection with the ESP32-CAM, and sending an HTTP request. 

There are buttons labeled and attached to GPIO pins. They are set to "INPUT_PULLOUT", which helps reduce the effect of noise. Pressing any button starts the sequence for capturing an image, but the specific button pressed indicates what operation will be performed by the API call.

> [!Warning]
> The buttons are directed to GPIO pins. Any user intending to replicate this project must remember that these pins may occasionally suit additional purposes within their microcontroller. Interfering with these operations may cause problems. Please consult the manual and documentation for your device before accessing its GPIO pins.

The ESP32-CAM is programmed with "esp_camera.h". It is set up using the camera_config_t structure and the esp_camera_init() function. You might find the configuration for your device [here](https://github.com/RuiSantosdotme/arduino-esp32-CameraWebServer/blob/master/CameraWebServer/CameraWebServer.ino). This device uses the configuration "CAMERA_MODEL_AI_THINKER". It captures an image who's raw JPEG binaries are sent to the API.

The program needs an internet connection to perform the API call. This is established through the <WiFi.h> library. Next, it needs to perform that call. To serve that purpose, this program uses <HTTPClient.h>, as it is robust and beginner-friendly. It also uses <WiFiClientSecure.h> for HTTPS status and to disable certificate checking, but the HTTPS status was the main focus. 

## 4. Software 
The software for this project involves AI, which is computationally heavy. Therefore, it cannot be performed in the microcontroller and must be performed elsewhere. In this case, an AWS Lambda Function is called.

### 4.1. Environment 
All the software for this project is in the Lambda Function, and the Lambda Funcion is in ./Facial Recognition Software/recognize_face.py. Also, the images to be uploaded and recognized are stored in AWS S3 buckets. 

This program should be pasted into a Lambda Function, and can be called with the API Gateway from the Lambda URL. The Lambda URL is recommended since it can accept more data and longer timeouts. The API call requires a token that matches the one in the function and an operation, whether it is "upload", "validate", or "delete" as headers called "device-token" and "operation" respectively. In the body, it expects the raw JPEG bytes of an image. If any of these are done incorrectly, the function will throw the corresponding error.

This project uses the bare minimum when it comes to security. The API call and the function are set to public and are currently disabled. The token is the only layer of security. This is necessary for the request in the firmware to work. A future version may address this issue.

The Lambda Function requires the face_recognition library, which requires CMake and dlib. This does not sound like a hastle, but it is. The binaries for the Amazon Linux-based shell were compiled using the AWS Lambda Docker Container and can be found with the proper file structure in ./Facial Recognition Software/dependencies.zip. This needs to added as a Layer. Since the zip folder is too large to be added directly, it needs to be sent through an S3 bucket. 

### 4.2. Code
The Lambda Function is given one of three operations: "upload", "validate", or "delete". 

They all start the same. The token is validated, the captured image raw JPEG binaries are converted to base64 and subsequently encoded, all other face encodings from the S3 bucket are gathered, and finally, the compare_faces() function is called. compare_faces() takes in the new encoding and the array of existing encodings, and it returns an array that represents the array of face encodings gathered from the S3 buckets, except each index is boolean- true if its corresponding image is a match and false if it isn't. 

For the "upload" and "delete" operations, all matching faces are deleted. For the "upload" operation, the new face encoding is also added to the bucket. For the "validate" operation, the function returns a success message iff at least one index in the returned array is true. Any other case will be returned as an error.

## 5. Pipeline of Information
This section demonstrates how information flows throughout this project.

The program starts with the device in its setup phase. During this phase, it connects to the WiFi and the camera. 

Once this phase is complete, the device starts to listen the the press of one of three buttons, each corresponding to an operation. These operations are "upload", "validate", and "delete". When a button is pressed, the microcontroller captures an image with the camera it is assigned to and sends an API call.

The API call is a POST request to a Lambda function URL. It has a header signifying that an image is being sent, a header that contains a token the Lambda function checks as a layer of security, and a hearder that dictates the operation corresponding to the button that was pressed. In the request body is the raw JPEG binary version of the captured image.

When the Lambda function receives the request, it starts by validating the token. 

Next, it gathers all the images uploaded previously from the S3 bucket they were stored in and places them in an array.

After that, it converts the raw JPEG binary image into base64 format 
