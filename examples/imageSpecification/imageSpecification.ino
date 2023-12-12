#include <WiFi.h>
#include "lib/OpenAI-ESP32CAM-Vision-Analisis/src/OpenAI.h"
#include "camera.h"


const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";
const char* api_key = "your-OPENAI_API_KEY";

OpenAI openai(api_key);
OpenAI_ChatCompletion chat(openai);

void captureAndSendImage(){
  camera_fb_t *fb = NULL; 
  // Take Picture with Camera and return pointer to image buffer
  fb = esp_camera_fb_get();

  Serial.println("capuring image");
  delay(5);
  esp_camera_fb_return(fb);
  delay(1000);
  Serial.println("sending image to openai");

  // send the image to openai
  OpenAI_StringResponse result = chat.message("whats in this image?",fb->buf,fb->len); // enter your prompt with the image buffer and the length of the buffer

  if(result.length() == 1){
    Serial.printf("Received message. Tokens: %u\n", result.tokens());
    String response = result.getAt(0);
    response.trim();
    Serial.println(response);
  } else if(result.length() > 1){
    Serial.printf("Received %u messages. Tokens: %u\n", result.length(), result.tokens());
    for (unsigned int i = 0; i < result.length(); ++i){
      String response = result.getAt(i);
      response.trim();
      Serial.printf("Message[%u]:\n%s\n", i, response.c_str());
    }
  } else if(result.error()){
    Serial.print("Error! ");
    Serial.println(result.error());
  } else {
    Serial.println("Unknown error!");
  }
}



void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  chat.setModel("gpt-4-vision-preview");   //Model to use for completion. Default is gpt-4-vision-preview is needed for image analysis
  chat.setSystem("Code geek");      //Description of the required assistant
  chat.setMaxTokens(1000);          //The maximum number of tokens to generate in the completion.
  chat.setTemperature(0.2);         //float between 0 and 2. Higher value gives more random results.
  chat.setStop("\r");               //Up to 4 sequences where the API will stop generating further tokens.
  chat.setPresencePenalty(0);       //float between -2.0 and 2.0. Positive values increase the model's likelihood to talk about new topics.
  chat.setFrequencyPenalty(0);      //float between -2.0 and 2.0. Positive values decrease the model's likelihood to repeat the same line verbatim.
  chat.setUser("OpenAI-ESP32-cam");     //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

  initCamera();
  delay(1000);

  captureAndSendImage();
}

void loop() {
  // do nothing
}
