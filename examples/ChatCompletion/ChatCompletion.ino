#include <WiFi.h>
#include <OpenAI.h>

const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";
const char* api_key = "your-OPENAI_API_KEY";

OpenAI openai(api_key);
OpenAI_ChatCompletion chat(openai);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  chat.setModel("gpt-3.5-turbo");   //Model to use for completion. Default is gpt-3.5-turbo
  chat.setSystem("Code geek");      //Description of the required assistant
  chat.setMaxTokens(1000);          //The maximum number of tokens to generate in the completion.
  chat.setTemperature(0.2);         //float between 0 and 2. Higher value gives more random results.
  chat.setStop("\r");               //Up to 4 sequences where the API will stop generating further tokens.
  chat.setPresencePenalty(0);       //float between -2.0 and 2.0. Positive values increase the model's likelihood to talk about new topics.
  chat.setFrequencyPenalty(0);      //float between -2.0 and 2.0. Positive values decrease the model's likelihood to repeat the same line verbatim.
  chat.setUser("OpenAI-ESP32");     //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

  Serial.println("You can now send chat message to OpenAI by typing in the Arduino IDE Serial Monitor.");
  Serial.println("Each line will be interpreted as one message and processed.");
  Serial.println("You can restart the conversation by typing \"clear\"\n");
}

void loop() {
  String line = Serial.readStringUntil('\n');
  if(line.length() == 0){
    return;
  }
  if(line == "clear" || line == "clear\r"){
    chat.clearConversation();
    Serial.println("Conversation cleared!");
    return;
  }
  Serial.println();
  Serial.println(line);
  Serial.println("Processing...");
  OpenAI_StringResponse result = chat.message(line);
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
