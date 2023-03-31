#include <WiFi.h>
#include <OpenAI.h>

const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";
const char* api_key = "your-OPENAI_API_KEY";

OpenAI openai(api_key);
OpenAI_Completion completion(openai);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  // completion.setModel("text-ada-001");  //Model to use for completion. Default is text-davinci-003
  // completion.setMaxTokens(1000);        //The maximum number of tokens to generate in the completion.
  // completion.setTemperature(0.2);       //float between 0 and 2. Higher value gives more random results.
  // completion.setN(2);                   //How many completions to generate for each prompt.
  // completion.setEcho(true);             //Echo back the prompt in addition to the completion
  // completion.setStop("\r");             //Up to 4 sequences where the API will stop generating further tokens.
  // completion.setPresencePenalty(2.0);   //float between -2.0 and 2.0. Positive values increase the model's likelihood to talk about new topics.
  // completion.setFrequencyPenalty(2.0);  //float between -2.0 and 2.0. Positive values decrease the model's likelihood to repeat the same line verbatim.
  // completion.setBestOf(5);              //Generates best_of completions server-side and returns the "best". "best_of" must be greater than "n"
  // completion.setUser("OpenAI-ESP32");   //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

  Serial.println("You can now send prompt to OpenAI for completion by typing in the Arduino IDE Serial Monitor.");
  Serial.println("Each line will be interpreted as one prompt and processed.\n");
}

void loop() {
  String line = Serial.readStringUntil('\n');
  if(line.length() == 0){
    return;
  }
  Serial.println();
  Serial.println(line);
  Serial.println("Processing...");
  OpenAI_StringResponse result = completion.prompt(line);
  if(result.length() == 1){
    Serial.printf("Received completion. Tokens: %u\n", result.tokens());
    String response = result.getAt(0);
    response.trim();
    Serial.println(response);
  } else if(result.length() > 1){
    Serial.printf("Received %u completions. Tokens: %u\n", result.length(), result.tokens());
    for (unsigned int i = 0; i < result.length(); ++i){
      String response = result.getAt(i);
      response.trim();
      Serial.printf("Completion[%u]:\n%s\n", i, response.c_str());
    }
  } else if(result.error()){
    Serial.print("Error! ");
    Serial.println(result.error());
  } else {
    Serial.println("Unknown error!");
  }
}