# Facial Recognition Project Verson 2
This is the start of version 2 of Face Recognition Project. There are two main goals in this version. The first goal is to update the monolithic firmware file to a proper RTOS using the ESP IDF. The second goal is to create the first real prototype of the product with a custom designed and hand-soldered PCB and a custom designed, 3D printed, and assembled weatherproof case.

## DEVS

### Enter the Docker Container (Linux)
```bash
docker build -t esp-idf .
docker run -it --rm \
    --device=/dev/ttyUSB0 \
    -v $(pwd)/facial-recognition-firmware:/app \
    -w /app \
    esp-idf:latest /bin/bash
```
