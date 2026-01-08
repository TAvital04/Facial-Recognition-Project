void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("HELLO FROM SETUP");
  Serial.printf("Model=%s, cores=%d\n", ESP.getChipModel(), ESP.getChipCores());
}

void loop() {
  Serial.println("tick");
  delay(1000);
}