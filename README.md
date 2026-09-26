# Facial Recognition Project Verson 2

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
