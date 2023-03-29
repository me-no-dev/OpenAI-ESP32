#include <WiFi.h>
#include <OpenAI.h>

const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";
const char* api_key = "your-OPENAI_API_KEY";

OpenAI openai(api_key);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("You can now send a message to OpenAI and check if it's offensive by typing in the Arduino IDE Serial Monitor.");
  Serial.println("Each line will be interpreted as a single message and processed.");
}

void loop() {
  String line = Serial.readStringUntil('\n');
  if(line.length() == 0){
    return;
  }
  Serial.println(line);
  Serial.println("Processing...");
  OpenAI_ModerationResponse result = openai.moderation(line);
  if(result.length() == 1){
    Serial.println(result.getAt(0)?"Flagged":"Not Flagged");
  } else if(result.length() > 1){
    Serial.printf("Received %u moderations.\n", result.length());
    for (unsigned int i = 0; i < result.length(); ++i){
      Serial.printf("Moderation[%u]:\n%s\n", i, result.getAt(i)?"Flagged":"Not Flagged");
    }
  } else if(result.error()){
    Serial.print("Error! ");
    Serial.println(result.error());
  } else {
    Serial.println("Unknown error!");
  }
  Serial.println();
}
