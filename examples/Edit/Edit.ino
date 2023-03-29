#include <WiFi.h>
#include <OpenAI.h>

const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";
const char* api_key = "your-OPENAI_API_KEY";

OpenAI openai(api_key);
OpenAI_Edit edit(openai);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  edit.setModel("code-davinci-edit-001"); //Model to use for completion. Default is text-davinci-edit-001
  edit.setTemperature(0);                 //float between 0 and 2. Higher value gives more random results.
  edit.setN(1);                           //How many edits to generate for the input and instruction.

  Serial.println("You can now send an instruction on how to edit an input to OpenAI by typing in the Arduino IDE Serial Monitor.");
  Serial.println("Each line will be interpreted as new instruction and processed against the last edit.");
  Serial.println("You can clear the old input by typing \"clear\"\n");
}

// Will hold the last edit from OpenAI
String input = "";

void loop() {
  String line = Serial.readStringUntil('\n');
  if(line.length() == 0){
    return;
  }
  if(line == "clear" || line == "clear\r"){
    input = "";
    Serial.println("Input cleared!");
    return;
  }
  Serial.println();
  Serial.println(line);
  Serial.println("Processing...");
  OpenAI_StringResponse result = edit.process(line, input.length()?input.c_str():NULL);
  if(result.length() == 1){
    Serial.printf("Received edit. Tokens: %u\n", result.tokens());
    input = result.getAt(0);
    input.trim();
    Serial.println(input);
  } else if(result.length() > 1){
    Serial.printf("Received %u edits. Tokens: %u\n", result.length(), result.tokens());
    for (unsigned int i = 0; i < result.length(); ++i){
      String response = result.getAt(i);
      response.trim();
      Serial.printf("Edit[%u]:\n%s\n", i, response.c_str());
    }
  } else if(result.error()){
    Serial.print("Error! ");
    Serial.println(result.error());
  } else {
    Serial.println("Unknown error!");
  }
}
