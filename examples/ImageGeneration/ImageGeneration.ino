#include <WiFi.h>
#include <OpenAI.h>

const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";
const char* api_key = "your-OPENAI_API_KEY";

OpenAI openai(api_key);
OpenAI_ImageGeneration imageGeneration(openai);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  imageGeneration.setSize(OPENAI_IMAGE_SIZE_256x256);                   //The size of the generated images. 256, 512 or 1024 pixels square.
  imageGeneration.setResponseFormat(OPENAI_IMAGE_RESPONSE_FORMAT_URL);  //The format in which the generated images are returned. URL or B64_JSON
  imageGeneration.setN(1);                                              //The number of images to generate. Must be between 1 and 10.
  imageGeneration.setUser("OpenAI-ESP32");                              //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

  Serial.println("You can now let OpenAI to generate an image based on prompt by typing in the Arduino IDE Serial Monitor.");
  Serial.println("Each line will be interpreted as new prompt and processed.");
}

void loop() {
  String line = Serial.readStringUntil('\n');
  if(line.length() == 0){
    return;
  }
  Serial.println();
  Serial.println(line);
  Serial.println("Processing...");
  OpenAI_ImageResponse result = imageGeneration.prompt(line);
  if(result.length() == 1){
    Serial.printf("Received image.\n");
    Serial.println(result.getAt(0));
  } else if(result.length() > 1){
    Serial.printf("Received %u images.\n", result.length());
    for (unsigned int i = 0; i < result.length(); ++i){
      Serial.printf("Image[%u]:\n%s\n", i, result.getAt(i));
    }
  } else if(result.error()){
    Serial.print("Error! ");
    Serial.println(result.error());
  } else {
    Serial.println("Unknown error!");
  }
}
