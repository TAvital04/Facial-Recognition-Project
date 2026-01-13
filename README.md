# Facial Recognition Project
This document describes the different components of Facial Recognition Project and how they work. It isn't a tutorial on how to replicate the project.

## Contents 
1. Hardware
2. Firmware
   
    2.1. Environment
  
    2.2. Code
  
3. Software

   3.1. Environment 

   3.2. Code
4. Pipeline of Information
  
## 1. Hardware 
Facial Recognition Project uses an FTDI YP-05 Serial Adapter and an ESP32-CAM microcontroller. The serial adapter is necessary for uploading code to the ESP32-CAM. In this project, it is also used to power the microcontroller. 

The serial adapter connects to a PC using a Mini-USB, but its connection to the ESP32-CAM is a bit more complicated. 
| Adapter | Microcontroller | Purpose |
|-|-|------------------------------|
| VCC (5V) | 5V | Power the ESP32-CAM. Must be 5 volts, not 3.3! |
| GND | GND | Establish a "Low" or "Zero" and close the circuit. |
| TX | RX | Send data from the adapter to the microcontroller. |
| RX | TX | Send data from the microcontroller to the adapter. |

There are also buttons that connect to GPIO pins. These should be wired between a GPIO and GND pin of the microcontroller. 

> [!Warning]
> The buttons are directed to GPIO pins. Any user intending to replicate this project must remember that these pins may occasionally suit additional purposes within their microcontroller. Interfering with these operations may cause problems. Please consult the manual and documentation for your device before accessing its GPIO pins.
>
> The program is power-intensive and will not run on 3.3V. Send 5V and proceed with caution.
>
>
> **Do not use this document as a complete tutorial for this operation. Consult your device and other sources. Make informed decisions when working with electrical ciruits!**

## 2. Firmware 
This part will be about the firmware of the Facial Recognition Project.

### 2.1. Environment 
The firmware of this project is written, compiled, and uploaded to the microcontroller through the Arduino IDE. The firmware is written in C++ and is completely encapsulated within ```\Facial-Recognition-Project\Embedded Software\take_and_send\take_and_send.ino.```

### 2.2. Code
The code for the firmware contains 3 distinct components for designing the hardware interface, establishing a connection with the ESP32-CAM, and sending an HTTP request. 

There are buttons labeled and attached to GPIO pins. They are set to "INPUT_PULLOUT", which helps reduce the effect of noise. Pressing any button starts the sequence for capturing an image, but the specific button pressed indicates what operation will be performed by the API call.

> [!Warning]
> The buttons are directed to GPIO pins. Any user intending to replicate this project must remember that these pins may occasionally suit additional purposes within their microcontroller. Interfering with these operations may cause problems. Please consult the manual and documentation for your device before accessing its GPIO pins.
>
> **Do not use this document as a complete tutorial for this operation. Consult your device and other sources. Make informed decisions when working with electrical ciruits!**

The ESP32-CAM is programmed with "esp_camera.h". It is set up using the camera_config_t structure and the esp_camera_init() function. The configuration for this device was found [here](https://github.com/RuiSantosdotme/arduino-esp32-CameraWebServer/blob/master/CameraWebServer/CameraWebServer.ino) as "CAMERA_MODEL_AI_THINKER". It captures an image who's raw JPEG binaries are sent to the API.

The program needs an internet connection to perform the API call. This is established through the <WiFi.h> library. Next, it performs that call with <HTTPClient.h>- a robust and beginner-friendly library built on top of <Wifi.h> specifically for performing HTTP requests. It also uses <WiFiClientSecure.h> for HTTPS status and to disable certificate checking, but the HTTPS status was the main focus. 

## 3. Software 
The software for this project involves AI, which is computationally heavy. Therefore, it cannot be performed in the microcontroller and must be performed elsewhere. In this case, an AWS Lambda Function is called.

### 3.1. Environment 
All the software in this project is in the Lambda Function, and the code in the Lambda Function can be found in ```\Facial Recognition Software\recognize_face.py```. The images to be uploaded and recognized are stored in AWS S3 buckets.

This program is called with the Lambda URL, not the API Gateway, since the Lambda URL can accept more data and longer timeouts. The API call requires a token that matches the one in the function and an operation, whether it is "upload", "validate", or "delete". These are sent as headers called "device-token" and "operation" respectively. In the body, it requires the raw JPEG bytes of an image. If any of these requires are missed or are fulfilled insufficiently, the function will throw the corresponding error.

The security of the API call is currently very weak. The Lambda function URL must be set to public, and the token is the only layer of security. This is necessary for the request in the firmware to work as it is written. The next step in security would be to validate a request with AWS IAM authentication.

The Lambda Function requires the face_recognition library, which requires CMake and dlib. These dependencies must be uploaded manually and must be compatible with the AWS Linux-based shell. The binaries for the dependencies were compiled using the AWS Lambda Docker Container and can be found with the proper file structure in ```\Facial Recognition Software\dependencies.zip```. They are implemented as an AWS Layer.

### 3.2. Code
The Lambda Function is given one of three operations: "upload", "validate", or "delete". 

They all start the same. The token is validated, the captured image raw JPEG binaries are converted to base64 and subsequently encoded, all other face encodings from the S3 bucket are gathered, and finally, the compare_faces() function is called. compare_faces() takes in the new encoding and the array of existing encodings, and it returns a new array that represents the array of face encodings gathered from the S3 buckets, where each index is boolean- true if its corresponding image is a match and false if it isn't. 

For the "upload" and "delete" operations, all matching faces are deleted. For the "upload" operation, the new face encoding is also added to the bucket. For the "validate" operation, the function returns a success message if and only if at least one index in the returned array is true. Any other case will be returned as an error.

## 4. Pipeline of Information
This section demonstrates how information flows through this project.

The program starts with the device in its setup phase. During this phase, it connects to the WiFi and camera. 

Once this phase is complete, the device starts to listen for the press of one of three buttons, each corresponding to an operation. These operations are "upload", "validate", and "delete". When a button is pressed, the microcontroller captures an image with the camera and sends an API call.

The API call is a POST request to a Lambda function URL. It has a header signifying that an image is being sent, a header that contains a token the Lambda function validates as a layer of security, and a header that dictates the operation that corresponds to the button that was pressed. In the request body is the raw JPEG binary version of the captured image.

When the Lambda function receives the request, it starts by validating the token, and then it performs the given operation.

Finally, the Lambda function returns JSON response to the microcontroller, which the microcontroller converts into a string and prints onto the Arduino IDE serial monitor.
