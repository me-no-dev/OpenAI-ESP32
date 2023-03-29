/*
  ToDo:
    - Support FS::File as input
    - Look into supporting "stream" responses?
    - Thread-Safe API?
*/

#include "OpenAI.h"
#include "HTTPClient.h"

// Macros for building the request
#define reqAddString(var,val) \
  if(cJSON_AddStringToObject(req, var, val) == NULL){ \
    cJSON_Delete(req); \
    log_e("cJSON_AddStringToObject failed!"); \
    return result; \
  }

#define reqAddNumber(var,val) \
  if(cJSON_AddNumberToObject(req, var, val) == NULL){ \
    cJSON_Delete(req); \
    log_e("cJSON_AddNumberToObject failed!"); \
    return result; \
  }

#define reqAddBool(var,val) \
  if(cJSON_AddBoolToObject(req, var, val) == NULL){ \
    cJSON_Delete(req); \
    log_e("cJSON_AddBoolToObject failed!"); \
    return result; \
  }

#define reqAddItem(var,val) \
  if(!cJSON_AddItemToObject(req, var, val)){ \
    cJSON_Delete(req); \
    cJSON_Delete(val); \
    log_e("cJSON_AddItemToObject failed!"); \
    return result; \
  }

static String getJsonError(cJSON * json){
  if(json == NULL){
    return String("cJSON_Parse failed!");
  }
  if(!cJSON_IsObject(json)){
    return String("Response is not an object! " + String(cJSON_Print(json)));
  }
  if(cJSON_HasObjectItem(json, "error")){
    cJSON * error = cJSON_GetObjectItem(json, "error");
    if(!cJSON_IsObject(error)){
      return String("Error is not an object! " + String(cJSON_Print(error)));
    }
    if(!cJSON_HasObjectItem(error, "message")){
      return String("Error does not contain message! " + String(cJSON_Print(error)));
    }
    cJSON * error_message = cJSON_GetObjectItem(error, "message");
    return String(cJSON_GetStringValue(error_message));
  }
  return String();
}

//
// OpenAI_EmbeddingResponse
//

OpenAI_EmbeddingResponse::OpenAI_EmbeddingResponse(const char * payload){
  usage = 0;
  len = 0;
  data = NULL;
  error_str = NULL;
  cJSON * u, *tokens, *d;
  int dl;

  if(payload == NULL){
    return;
  }

  // Parse payload
  cJSON * json = cJSON_Parse(payload);

  // Check for error
  String error = getJsonError(json);
  if(error.length()){
    error_str = strdup(error.c_str());
    if(json != NULL){
      cJSON_Delete(json);
    }
    log_e("%s",error.c_str());
    return;
  }

  // Get total_tokens
  if(!cJSON_HasObjectItem(json, "usage")){
    log_e("Usage was not found");
    goto end;
  }
  u = cJSON_GetObjectItem(json, "usage");
  if(u == NULL || !cJSON_IsObject(u) || !cJSON_HasObjectItem(u, "total_tokens")){
    log_e("Total tokens were not found");
    goto end;
  }
  tokens = cJSON_GetObjectItem(u, "total_tokens");
  if(tokens == NULL){
    log_e("Total tokens could not be read");
    goto end;
  }
  usage = cJSON_GetNumberValue(tokens);

  // Parse data
  if(!cJSON_HasObjectItem(json, "data")){
    log_e("Data was not found");
    goto end;
  }
  d = cJSON_GetObjectItem(json, "data");
  if(d == NULL || !cJSON_IsArray(d)){
    log_e("Data is not array");
    goto end;
  }
  dl = cJSON_GetArraySize(d);
  if(dl <= 0){
    log_e("Data is empty");
    goto end;
  }
  data = (OpenAI_EmbeddingData*)malloc(dl * sizeof(OpenAI_EmbeddingData));
  if(data == NULL){
    log_e("Data could not be allocated");
    goto end;
  }
  for (int di = 0; di < dl; di++){
    cJSON * ditem = cJSON_GetArrayItem(d, di);
    if(ditem == NULL || !cJSON_IsObject(ditem) || !cJSON_HasObjectItem(ditem, "embedding")){
      log_e("Embedding was not found");
      goto end;
    }
    cJSON * numberArray = cJSON_GetObjectItem(ditem, "embedding");
    if(numberArray == NULL || !cJSON_IsArray(numberArray)){
      log_e("Embedding is not array");
      goto end;
    }
    int l = cJSON_GetArraySize(numberArray);
    if(l <= 0){
      log_e("Embedding is empty");
      goto end;
    }
    data[di].data = (double*)malloc(l * sizeof(double));
    if(data[di].data == NULL){
      log_e("Embedding could not be allocated");
      goto end;
    }
    len++;
    data[di].len = l;
    for (int i = 0; i < l; i++){
      cJSON * item = cJSON_GetArrayItem(numberArray, i);
      if(item == NULL){
        log_e("Embedding item could not be read");
        goto end;
      }
      data[di].data[i] = cJSON_GetNumberValue(item);
    }
  }
end:
  cJSON_Delete(json);
}

OpenAI_EmbeddingResponse::~OpenAI_EmbeddingResponse(){
  if(data){
    for (unsigned int i = 0; i < len; i++){
      free(data[i].data);
    }
    free(data);
  }
  if(error_str != NULL){
    free(error_str);
  }
}

//
// OpenAI_ModerationResponse
//

OpenAI_ModerationResponse::OpenAI_ModerationResponse(const char * payload){
  len = 0;
  data = NULL;
  error_str = NULL;
  cJSON *d;
  int dl;

  if(payload == NULL){
    return;
  }

  // Parse payload
  cJSON * json = cJSON_Parse(payload);

  // Check for error
  String error = getJsonError(json);
  if(error.length()){
    error_str = strdup(error.c_str());
    if(json != NULL){
      cJSON_Delete(json);
    }
    log_e("%s",error.c_str());
    return;
  }

  // Parse data
  if(!cJSON_HasObjectItem(json, "results")){
    log_e("Results was not found");
    goto end;
  }
  d = cJSON_GetObjectItem(json, "results");
  if(d == NULL || !cJSON_IsArray(d)){
    log_e("Results is not array");
    goto end;
  }
  dl = cJSON_GetArraySize(d);
  if(dl <= 0){
    log_e("Results is empty");
    goto end;
  }
  data = (bool*)malloc(dl * sizeof(bool));
  if(data == NULL){
    log_e("Data could not be allocated");
    goto end;
  }
  len = dl;
  for (int di = 0; di < dl; di++){
    cJSON * ditem = cJSON_GetArrayItem(d, di);
    if(ditem == NULL || !cJSON_IsObject(ditem) || !cJSON_HasObjectItem(ditem, "flagged")){
      log_e("Flagged was not found");
      goto end;
    }
    cJSON * flagged = cJSON_GetObjectItem(ditem, "flagged");
    if(flagged == NULL || !cJSON_IsBool(flagged)){
      log_e("Flagged is not bool");
      goto end;
    }
    data[di] = cJSON_IsTrue(flagged);
  }
end:
  cJSON_Delete(json);
}

OpenAI_ModerationResponse::~OpenAI_ModerationResponse(){
  if(data){
    free(data);
  }
  if(error_str != NULL){
    free(error_str);
  }
}

//
// OpenAI_ImageResponse
//

OpenAI_ImageResponse::OpenAI_ImageResponse(const char * payload){
  len = 0;
  data = NULL;
  error_str = NULL;
  cJSON *d;
  int dl;

  if(payload == NULL){
    return;
  }

  // Parse payload
  cJSON * json = cJSON_Parse(payload);

  // Check for error
  String error = getJsonError(json);
  if(error.length()){
    error_str = strdup(error.c_str());
    if(json != NULL){
      cJSON_Delete(json);
    }
    log_e("%s",error.c_str());
    return;
  }

  // Parse data
  if(!cJSON_HasObjectItem(json, "data")){
    log_e("Data was not found");
    goto end;
  }
  d = cJSON_GetObjectItem(json, "data");
  if(d == NULL || !cJSON_IsArray(d)){
    log_e("Data is not array");
    goto end;
  }
  dl = cJSON_GetArraySize(d);
  if(dl <= 0){
    log_e("Data is empty");
    goto end;
  }
  data = (char**)malloc(dl * sizeof(char*));
  if(data == NULL){
    log_e("Data could not be allocated");
    goto end;
  }
  
  for (int di = 0; di < dl; di++){
    cJSON * item = cJSON_GetArrayItem(d, di);
    if(item == NULL || !cJSON_IsObject(item) || (!cJSON_HasObjectItem(item, "url") && !cJSON_HasObjectItem(item, "b64_json"))){
      log_e("Image was not found");
      goto end;
    }
    if(cJSON_HasObjectItem(item, "url")){
      cJSON * url = cJSON_GetObjectItem(item, "url");
      if(url == NULL || !cJSON_IsString(url)){
        log_e("Image url could not be read");
        goto end;
      }
      data[di] = strdup(cJSON_GetStringValue(url));
      if(data[di] == NULL){
        log_e("Image url could not be copied");
        goto end;
      }
      len++;
    } else if(cJSON_HasObjectItem(item, "b64_json")){
      cJSON * b64_json = cJSON_GetObjectItem(item, "b64_json");
      if(b64_json == NULL || !cJSON_IsString(b64_json)){
        log_e("Image b64_json could not be read");
        goto end;
      }
      data[di] = strdup(cJSON_GetStringValue(b64_json));
      if(data[di] == NULL){
        log_e("Image b64_json could not be copied");
        goto end;
      }
      len++;
    }
  }
end:
  cJSON_Delete(json);
}

OpenAI_ImageResponse::~OpenAI_ImageResponse(){
  if(data){
    for (unsigned int i = 0; i < len; i++){
      free(data[i]);
    }
    free(data);
  }
  if(error_str != NULL){
    free(error_str);
  }
}

//
// OpenAI_StringResponse
//

OpenAI_StringResponse::OpenAI_StringResponse(const char * payload){
  usage = 0;
  len = 0;
  data = NULL;
  error_str = NULL;
  cJSON * u, *tokens, *d;
  int dl;

  if(payload == NULL){
    return;
  }

  // Parse payload
  cJSON * json = cJSON_Parse(payload);

  // Check for error
  String error = getJsonError(json);
  if(error.length()){
    error_str = strdup(error.c_str());
    if(json != NULL){
      cJSON_Delete(json);
    }
    log_e("%s",error.c_str());
    return;
  }

  // Get total_tokens
  if(!cJSON_HasObjectItem(json, "usage")){
    log_e("Usage was not found");
    goto end;
  }
  u = cJSON_GetObjectItem(json, "usage");
  if(u == NULL || !cJSON_IsObject(u) || !cJSON_HasObjectItem(u, "total_tokens")){
    log_e("Total tokens were not found");
    goto end;
  }
  tokens = cJSON_GetObjectItem(u, "total_tokens");
  if(tokens == NULL){
    log_e("Total tokens could not be read");
    goto end;
  }
  usage = cJSON_GetNumberValue(tokens);

  // Parse data
  if(!cJSON_HasObjectItem(json, "choices")){
    log_e("Choices was not found");
    goto end;
  }
  d = cJSON_GetObjectItem(json, "choices");
  if(d == NULL || !cJSON_IsArray(d)){
    log_e("Choices is not array");
    goto end;
  }
  dl = cJSON_GetArraySize(d);
  if(dl <= 0){
    log_e("Choices is empty");
    goto end;
  }
  data = (char**)malloc(dl * sizeof(char*));
  if(data == NULL){
    log_e("Data could not be allocated");
    goto end;
  }
  
  for (int di = 0; di < dl; di++){
    cJSON * item = cJSON_GetArrayItem(d, di);
    if(item == NULL || !cJSON_IsObject(item) || (!cJSON_HasObjectItem(item, "text") && !cJSON_HasObjectItem(item, "message"))){
      log_e("Message was not found");
      goto end;
    }
    if(cJSON_HasObjectItem(item, "text")){
      cJSON * text = cJSON_GetObjectItem(item, "text");
      if(text == NULL || !cJSON_IsString(text)){
        log_e("Text could not be read");
        goto end;
      }
      data[di] = strdup(cJSON_GetStringValue(text));
      if(data[di] == NULL){
        log_e("Text could not be copied");
        goto end;
      }
      len++;
    } else if(cJSON_HasObjectItem(item, "message")){
      cJSON * message = cJSON_GetObjectItem(item, "message");
      if(message == NULL || !cJSON_IsObject(message) || !cJSON_HasObjectItem(message, "content")){
        log_e("Message is not object");
        goto end;
      }
      cJSON * mesg = cJSON_GetObjectItem(message, "content");
      if(mesg == NULL || !cJSON_IsString(mesg)){
        log_e("Message could not be read");
        goto end;
      }
      data[di] = strdup(cJSON_GetStringValue(mesg));
      if(data[di] == NULL){
        log_e("Message could not be copied");
        goto end;
      }
      len = di+1;
    }
  }
end:
  cJSON_Delete(json);
}

OpenAI_StringResponse::~OpenAI_StringResponse(){
  if(data != NULL){
    for (unsigned int i = 0; i < len; i++){
      free(data[i]);
    }
    free(data);
  }
  if(error_str != NULL){
    free(error_str);
  }
}

//
// OpenAI
//

OpenAI::OpenAI(const char *openai_api_key)
    : api_key(openai_api_key)
{

}

OpenAI::~OpenAI(){

}

String OpenAI::upload(String endpoint, String boundary, uint8_t * data, size_t len) {
  log_d("\"%s\": boundary=%s, len=%u", endpoint.c_str(), boundary.c_str(), len);
  HTTPClient http;
  http.setTimeout(20000);
  http.begin("https://api.openai.com/v1/" + endpoint);
  http.addHeader("Content-Type", "multipart/form-data; boundary="+boundary);
  http.addHeader("Authorization", "Bearer " + api_key);
  int httpCode = http.sendRequest("POST", data, len);
  if (httpCode != HTTP_CODE_OK) {
    log_e("HTTP_ERROR: %d", httpCode);
  }
  String response = http.getString();
  http.end();
  log_d("%s", response.c_str());
  return response;
}

String OpenAI::post(String endpoint, String jsonBody) {
  log_d("\"%s\": %s", endpoint.c_str(), jsonBody.c_str());
  HTTPClient http;
  http.setTimeout(60000);
  http.begin("https://api.openai.com/v1/" + endpoint);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + api_key);
  int httpCode = http.POST(jsonBody);
  if (httpCode != HTTP_CODE_OK) {
    log_e("HTTP_ERROR: %d", httpCode);
  }
  String response = http.getString();
  http.end();
  log_d("%s", response.c_str());
  return response;
}

String OpenAI::get(String endpoint) {
  log_d("\"%s\"", endpoint.c_str());
  HTTPClient http;
  http.begin("https://api.openai.com/v1/" + endpoint);
  http.addHeader("Authorization", "Bearer " + api_key);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    log_e("HTTP_ERROR: %d", httpCode);
  }
  String response = http.getString();
  http.end();
  log_d("%s", response.c_str());
  return response;
}

String OpenAI::del(String endpoint) {
  log_d("\"%s\"", endpoint.c_str());
  HTTPClient http;
  http.begin("https://api.openai.com/v1/" + endpoint);
  http.addHeader("Authorization", "Bearer " + api_key);
  int httpCode = http.sendRequest("DELETE");
  if (httpCode != HTTP_CODE_OK) {
    log_e("HTTP_ERROR: %d", httpCode);
  }
  String response = http.getString();
  http.end();
  log_d("%s", response.c_str());
  return response;
}

OpenAI_Completion OpenAI::completion(){
  return OpenAI_Completion(*this);
}

OpenAI_ChatCompletion OpenAI::chat(){
  return OpenAI_ChatCompletion(*this);
}

OpenAI_Edit OpenAI::edit(){
  return OpenAI_Edit(*this);
}

OpenAI_ImageGeneration OpenAI::imageGeneration(){
  return OpenAI_ImageGeneration(*this);
}

OpenAI_ImageVariation OpenAI::imageVariation(){
  return OpenAI_ImageVariation(*this);
}

OpenAI_ImageEdit OpenAI::imageEdit(){
  return OpenAI_ImageEdit(*this);
}

OpenAI_AudioTranscription OpenAI::audioTranscription(){
  return OpenAI_AudioTranscription(*this);
}

OpenAI_AudioTranslation OpenAI::audioTranslation(){
  return OpenAI_AudioTranslation(*this);
}

// embeddings { //Creates an embedding vector representing the input text.
//   "model": "text-embedding-ada-002",//required
//   "input": "The food was delicious and the waiter...",//required string or array. Input text to get embeddings for, encoded as a string or array of tokens. To get embeddings for multiple inputs in a single request, pass an array of strings or array of token arrays. Each input must not exceed 8192 tokens in length.
//   "user": null//string. A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.
// }

OpenAI_EmbeddingResponse OpenAI::embedding(String input, const char * model, const char * user){
  String endpoint = "embeddings";

  OpenAI_EmbeddingResponse result = OpenAI_EmbeddingResponse(NULL);
  cJSON * req = cJSON_CreateObject();
  if(req == NULL){
    log_e("cJSON_CreateObject failed!");
    return result;
  }
  reqAddString("model", (model == NULL)?"text-embedding-ada-002":model);
  if(input.startsWith("[")){
    cJSON * in = cJSON_Parse(input.c_str());
    if(in == NULL || !cJSON_IsArray(in)){
      log_e("Input not JSON Array!");
      cJSON_Delete(req);
      return result;
    }
    reqAddItem("input", in);
  } else {
    reqAddString("input", input.c_str());
  }
  if(user != NULL){
    reqAddString("user", user);
  }
  String jsonBody = String(cJSON_Print(req));
  cJSON_Delete(req);
  String response = post(endpoint, jsonBody);

  if(!response.length()){
    log_e("Empty response!");
    return result;
  }
  return OpenAI_EmbeddingResponse(response.c_str());
}

// moderations { //Classifies if text violates OpenAI's Content Policy
//   "input": "I want to kill them.",//required string or array
//   "model": "text-moderation-latest"//optional. Two content moderations models are available: text-moderation-stable and text-moderation-latest.
// }

OpenAI_ModerationResponse OpenAI::moderation(String input, const char * model){
  String endpoint = "moderations";

  OpenAI_ModerationResponse result = OpenAI_ModerationResponse(NULL);
  String res = "";
  cJSON * req = cJSON_CreateObject();
  if(req == NULL){
    log_e("cJSON_CreateObject failed!");
    return result;
  }
  if(input.startsWith("[")){
    cJSON * in = cJSON_Parse(input.c_str());
    if(in == NULL || !cJSON_IsArray(in)){
      log_e("Input not JSON Array!");
      cJSON_Delete(req);
      return result;
    }
    reqAddItem("input", in);
  } else {
    reqAddString("input", input.c_str());
  }
  if(model != NULL){
    reqAddString("model", model);
  }
  String jsonBody = String(cJSON_Print(req));
  cJSON_Delete(req);
  res = post(endpoint, jsonBody);

  if(!res.length()){
    log_e("Empty result!");
    return result;
  }
  return OpenAI_ModerationResponse(res.c_str());
}

// completions { //Creates a completion for the provided prompt and parameters
//   "model": "text-davinci-003",//required
//   "prompt": "<|endoftext|>",//string, array of strings, array of tokens, or array of token arrays.
//   "max_tokens": 16,//integer. The maximum number of tokens to generate in the completion.
//   "temperature": 1,//float between 0 and 2
//   "top_p": 1,//float between 0 and 1. recommended to alter this or temperature but not both.
//   "n": 1,//integer. How many completions to generate for each prompt.
//   "stream": false,//boolean. Whether to stream back partial progress. keep false
//   "logprobs": null,//integer. Include the log probabilities on the logprobs most likely tokens, as well the chosen tokens.
//   "echo": false,//boolean. Echo back the prompt in addition to the completion
//   "stop": null,//string or array. Up to 4 sequences where the API will stop generating further tokens. 
//   "presence_penalty": 0,//float between -2.0 and 2.0. Positive values penalize new tokens based on whether they appear in the text so far, increasing the model's likelihood to talk about new topics.
//   "frequency_penalty": 0,//float between -2.0 and 2.0. Positive values penalize new tokens based on their existing frequency in the text so far, decreasing the model's likelihood to repeat the same line verbatim.
//   "best_of": 1,//integer. Generates best_of completions server-side and returns the "best". best_of must be greater than n
//   "logit_bias": null,//map. Modify the likelihood of specified tokens appearing in the completion. 
//   "user": null//string. A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.
// }

OpenAI_Completion::OpenAI_Completion(OpenAI &openai)
  : oai(openai)
  , model(NULL)
  , max_tokens(0)
  , temperature(1)
  , top_p(1)
  , n(1)
  , echo(false)
  , stop(NULL)
  , presence_penalty(0)
  , frequency_penalty(0)
  , best_of(1)
  , user(NULL)
{}

OpenAI_Completion::~OpenAI_Completion(){
  if(model != NULL){
    free((void*)model);
  }
  if(stop != NULL){
    free((void*)stop);
  }
  if(user != NULL){
    free((void*)user);
  }
}

OpenAI_Completion & OpenAI_Completion::setModel(const char * m){
  if(model != NULL){
    free((void*)model);
  }
  model = strdup(m);
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setMaxTokens(unsigned int m){
  if(m > 0){
    max_tokens = m;
  }
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setTemperature(float t){
  if(t >= 0 && t <= 2.0){
    temperature = t;
  }
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setTopP(float t){
  if(t >= 0 && t <= 1.0){
    top_p = t;
  }
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setN(unsigned int _n){
  if(n > 0){
    n = _n;
  }
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setEcho(bool e){
  echo = e;
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setStop(const char * s){
  if(stop != NULL){
    free((void*)stop);
  }
  stop = strdup(s);
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setPresencePenalty(float p){
  if(p >= -2.0 && p <= 2.0){
    presence_penalty = p;
  }
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setFrequencyPenalty(float p){
  if(p >= -2.0 && p <= 2.0){
    frequency_penalty = p;
  }
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setBestOf(unsigned int b){
  if(b >= n){
    best_of = b;
  }
  return *this;
}

OpenAI_Completion & OpenAI_Completion::setUser(const char * u){
  if(user != NULL){
    free((void*)user);
  }
  user = strdup(u);
  return *this;
}

OpenAI_StringResponse OpenAI_Completion::prompt(String p){
  String endpoint = "completions";

  OpenAI_StringResponse result = OpenAI_StringResponse(NULL);
  cJSON * req = cJSON_CreateObject();
  if(req == NULL){
    log_e("cJSON_CreateObject failed!");
    return result;
  }
  reqAddString("model", (model == NULL)?"text-davinci-003":model);
  if(p.startsWith("[")){
    cJSON * in = cJSON_Parse(p.c_str());
    if(in == NULL || !cJSON_IsArray(in)){
      log_e("Input not JSON Array!");
      cJSON_Delete(req);
      return result;
    }
    reqAddItem("prompt", in);
  } else {
    reqAddString("prompt", p.c_str());
  }
  if(max_tokens){
    reqAddNumber("max_tokens", max_tokens);
  }
  if(temperature != 1){
    reqAddNumber("temperature", temperature);
  }
  if(top_p != 1){
    reqAddNumber("top_p", top_p);
  }
  if(n != 1){
    reqAddNumber("n", n);
  }
  if(echo){
    reqAddBool("echo", true);
  }
  if(stop != NULL){
    reqAddString("stop", stop);
  }
  if(presence_penalty != 0){
    reqAddNumber("presence_penalty", presence_penalty);
  }
  if(frequency_penalty != 0){
    reqAddNumber("frequency_penalty", frequency_penalty);
  }
  if(best_of != 1){
    reqAddNumber("best_of", best_of);
  }
  if(user != NULL){
    reqAddString("user", user);
  }
  String jsonBody = String(cJSON_Print(req));
  cJSON_Delete(req);
  String res = oai.post(endpoint, jsonBody);

  if(!res.length()){
    log_e("Empty result!");
    return result;
  }
  return OpenAI_StringResponse(res.c_str());
}

// chat/completions { //Given a chat conversation, the model will return a chat completion response.
//   "model": "gpt-3.5-turbo",//required
//   "messages": [//required array
//     {"role": "system", "content": "Description of the required assistant"},
//     {"role": "user", "content": "First question from the user"},
//     {"role": "assistant", "content": "Response from the assistant"},
//     {"role": "user", "content": "Next question from the user to be answered"}
//   ],
//   "temperature": 1,//float between 0 and 2
//   "top_p": 1,//float between 0 and 1. recommended to alter this or temperature but not both.
//   "stream": false,//boolean. Whether to stream back partial progress. keep false
//   "stop": null,//string or array. Up to 4 sequences where the API will stop generating further tokens. 
//   "max_tokens": 16,//integer. The maximum number of tokens to generate in the completion.
//   "presence_penalty": 0,//float between -2.0 and 2.0. Positive values penalize new tokens based on whether they appear in the text so far, increasing the model's likelihood to talk about new topics.
//   "frequency_penalty": 0,//float between -2.0 and 2.0. Positive values penalize new tokens based on their existing frequency in the text so far, decreasing the model's likelihood to repeat the same line verbatim.
//   "logit_bias": null,//map. Modify the likelihood of specified tokens appearing in the completion. 
//   "user": null//string. A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.
// }

OpenAI_ChatCompletion::OpenAI_ChatCompletion(OpenAI &openai)
  : oai(openai)
  , model(NULL)
  , description(NULL)
  , max_tokens(0)
  , temperature(1)
  , top_p(1)
  , stop(NULL)
  , presence_penalty(0)
  , frequency_penalty(0)
  , user(NULL)
{
  messages = cJSON_CreateArray();
}

OpenAI_ChatCompletion::~OpenAI_ChatCompletion(){
  if(model != NULL){
    free((void*)model);
  }
  if(description != NULL){
    free((void*)description);
  }
  if(stop != NULL){
    free((void*)stop);
  }
  if(user != NULL){
    free((void*)user);
  }
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setModel(const char * m){
  if(model != NULL){
    free((void*)model);
  }
  model = strdup(m);
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setSystem(const char * s){
  if(description != NULL){
    free((void*)description);
  }
  description = strdup(s);
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setMaxTokens(unsigned int m){
  if(m > 0){
    max_tokens = m;
  }
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setTemperature(float t){
  if(t >= 0 && t <= 2.0){
    temperature = t;
  }
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setTopP(float t){
  if(t >= 0 && t <= 1.0){
    top_p = t;
  }
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setStop(const char * s){
  if(stop != NULL){
    free((void*)stop);
  }
  stop = strdup(s);
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setPresencePenalty(float p){
  if(p >= -2.0 && p <= 2.0){
    presence_penalty = p;
  }
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setFrequencyPenalty(float p){
  if(p >= -2.0 && p <= 2.0){
    frequency_penalty = p;
  }
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::setUser(const char * u){
  if(user != NULL){
    free((void*)user);
  }
  user = strdup(u);
  return *this;
}

OpenAI_ChatCompletion & OpenAI_ChatCompletion::clearConversation(){
  if(messages != NULL){
    cJSON_Delete(messages);
    messages = cJSON_CreateArray();
  }
  return *this;
}

static cJSON * createChatMessage(cJSON * messages, const char * role, const char * content){
  cJSON * message = cJSON_CreateObject();
  if(message == NULL){
    log_e("cJSON_CreateObject failed!");
    return NULL;
  }
  if(cJSON_AddStringToObject(message, "role", role) == NULL){
    cJSON_Delete(message);
    log_e("cJSON_AddStringToObject failed!");
    return NULL;
  }
  if(cJSON_AddStringToObject(message, "content", content) == NULL){
    cJSON_Delete(message);
    log_e("cJSON_AddStringToObject failed!");
    return NULL;
  }
  if(!cJSON_AddItemToArray(messages, message)){
    cJSON_Delete(message);
    log_e("cJSON_AddItemToArray failed!");
    return NULL;
  }
  return message;
}

OpenAI_StringResponse OpenAI_ChatCompletion::message(String p, bool save){
  String endpoint = "chat/completions";

  OpenAI_StringResponse result = OpenAI_StringResponse(NULL);
  cJSON * req = cJSON_CreateObject();
  if(req == NULL){
    log_e("cJSON_CreateObject failed!");
    return result;
  }
  reqAddString("model", (model == NULL)?"gpt-3.5-turbo":model);

  cJSON * _messages = cJSON_CreateArray();
  if(_messages == NULL){
    cJSON_Delete(req);
    log_e("cJSON_CreateArray failed!");
    return result;
  }
  if(description != NULL){
    if(createChatMessage(_messages, "system", description) == NULL){
      cJSON_Delete(req);
      cJSON_Delete(_messages);
      log_e("createChatMessage failed!");
      return result;
    }
  }
  if(messages != NULL && cJSON_IsArray(messages)){
    int mlen = cJSON_GetArraySize(messages);
    for(int i = 0; i < mlen; ++i){
      cJSON * item = cJSON_GetArrayItem(messages, i);
      if(item != NULL && cJSON_IsObject(item)){
        if(!cJSON_AddItemReferenceToArray(_messages, item)){
          cJSON_Delete(req);
          cJSON_Delete(_messages);
          log_e("cJSON_AddItemReferenceToArray failed!");
          return result;
        }
      }
    }
  }
  if(createChatMessage(_messages, "user", p.c_str()) == NULL){
    cJSON_Delete(req);
    cJSON_Delete(_messages);
    log_e("createChatMessage failed!");
    return result;
  }

  reqAddItem("messages", _messages);
  if(max_tokens){
    reqAddNumber("max_tokens", max_tokens);
  }
  if(temperature != 1){
    reqAddNumber("temperature", temperature);
  }
  if(top_p != 1){
    reqAddNumber("top_p", top_p);
  }
  if(stop != NULL){
    reqAddString("stop", stop);
  }
  if(presence_penalty != 0){
    reqAddNumber("presence_penalty", presence_penalty);
  }
  if(frequency_penalty != 0){
    reqAddNumber("frequency_penalty", frequency_penalty);
  }
  if(user != NULL){
    reqAddString("user", user);
  }
  String jsonBody = String(cJSON_Print(req));
  cJSON_Delete(req);

  String res = oai.post(endpoint, jsonBody);

  if(!res.length()){
    log_e("Empty result!");
    return result;
  }
  if(save){
    //add the responses to the messages here
    //double parsing is here as workaround
    OpenAI_StringResponse r = OpenAI_StringResponse(res.c_str());
    if(r.length()){
      if(createChatMessage(messages, "user", p.c_str()) == NULL){
        log_e("createChatMessage failed!");
      }
      if(createChatMessage(messages, "assistant", r.getAt(0)) == NULL){
        log_e("createChatMessage failed!");
      }
    }
  }
  return OpenAI_StringResponse(res.c_str());
}

// edits { //Creates a new edit for the provided input, instruction, and parameters.
//   "model": "text-davinci-edit-001",//required
//   "input": "",//string. The input text to use as a starting point for the edit.
//   "instruction": "Fix the spelling mistakes",//required string. The instruction that tells the model how to edit the prompt.
//   "n": 1,//integer. How many edits to generate for the input and instruction.
//   "temperature": 1,//float between 0 and 2
//   "top_p": 1//float between 0 and 1. recommended to alter this or temperature but not both.
// }

OpenAI_Edit::OpenAI_Edit(OpenAI &openai)
  : oai(openai)
  , model(NULL)
  , temperature(1)
  , top_p(1)
  , n(1)
{}

OpenAI_Edit::~OpenAI_Edit(){
  if(model != NULL){
    free((void*)model);
  }
}

OpenAI_Edit & OpenAI_Edit::setModel(const char * m){
  if(model != NULL){
    free((void*)model);
  }
  model = strdup(m);
  return *this;
}

OpenAI_Edit & OpenAI_Edit::setTemperature(float t){
  if(t >= 0 && t <= 2.0){
    temperature = t;
  }
  return *this;
}

OpenAI_Edit & OpenAI_Edit::setTopP(float t){
  if(t >= 0 && t <= 1.0){
    top_p = t;
  }
  return *this;
}

OpenAI_Edit & OpenAI_Edit::setN(unsigned int _n){
  if(n > 0){
    n = _n;
  }
  return *this;
}

OpenAI_StringResponse OpenAI_Edit::process(String instruction, String input){
  String endpoint = "edits";

  OpenAI_StringResponse result = OpenAI_StringResponse(NULL);
  cJSON * req = cJSON_CreateObject();
  if(req == NULL){
    log_e("cJSON_CreateObject failed!");
    return result;
  }
  reqAddString("model", (model == NULL)?"text-davinci-edit-001":model);
  reqAddString("instruction", instruction.c_str());
  if(input){
    reqAddString("input", input.c_str());
  }
  if(temperature != 1){
    reqAddNumber("temperature", temperature);
  }
  if(top_p != 1){
    reqAddNumber("top_p", top_p);
  }
  if(n != 1){
    reqAddNumber("n", n);
  }
  String jsonBody = String(cJSON_Print(req));
  cJSON_Delete(req);
  
  String res = oai.post(endpoint, jsonBody);

  if(!res.length()){
    log_e("Empty result!");
    return result;
  }
  return OpenAI_StringResponse(res.c_str());
}

//
// Images
//

static const char * image_sizes[] = {"1024x1024","512x512","256x256"};
static const char * image_response_formats[] = {"url","b64_json"};

// images/generations { //Creates an image given a prompt.
//   "prompt": "A cute baby sea otter",//required
//   "n": 1,//integer. The number of images to generate. Must be between 1 and 10.
//   "size": "1024x1024",//string. The size of the generated images. Must be one of "256x256", "512x512", or "1024x1024"
//   "response_format": "url",//string. The format in which the generated images are returned. Must be one of "url" or "b64_json".
//   "user": null//string. A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.
// }

OpenAI_ImageGeneration::OpenAI_ImageGeneration(OpenAI &openai)
  : oai(openai)
  , size(OPENAI_IMAGE_SIZE_1024x1024)
  , response_format(OPENAI_IMAGE_RESPONSE_FORMAT_URL)
  , n(1)
  , user(NULL)
{}

OpenAI_ImageGeneration::~OpenAI_ImageGeneration(){
  if(user != NULL){
    free((void*)user);
  }
}

OpenAI_ImageGeneration & OpenAI_ImageGeneration::setSize(OpenAI_Image_Size s){
  if(s >= OPENAI_IMAGE_SIZE_1024x1024 && s <= OPENAI_IMAGE_SIZE_256x256){
    size = s;
  }
  return *this;
}

OpenAI_ImageGeneration & OpenAI_ImageGeneration::setResponseFormat(OpenAI_Image_Response_Format f){
  if(f >= OPENAI_IMAGE_RESPONSE_FORMAT_URL && f <= OPENAI_IMAGE_RESPONSE_FORMAT_B64_JSON){
    response_format = f;
  }
  return *this;
}

OpenAI_ImageGeneration & OpenAI_ImageGeneration::setN(unsigned int _n){
  if(n > 0 && n <= 10){
    n = _n;
  }
  return *this;
}

OpenAI_ImageGeneration & OpenAI_ImageGeneration::setUser(const char * u){
  if(user != NULL){
    free((void*)user);
  }
  user = strdup(u);
  return *this;
}

OpenAI_ImageResponse OpenAI_ImageGeneration::prompt(String p){
  String endpoint = "images/generations";

  OpenAI_ImageResponse result = OpenAI_ImageResponse(NULL);
  cJSON * req = cJSON_CreateObject();
  if(req == NULL){
    log_e("cJSON_CreateObject failed!");
    return result;
  }
  reqAddString("prompt", p.c_str());
  if(size != OPENAI_IMAGE_SIZE_1024x1024){
    reqAddString("size", image_sizes[size]);
  }
  if(response_format != OPENAI_IMAGE_RESPONSE_FORMAT_URL){
    reqAddString("response_format", image_response_formats[response_format]);
  }
  if(n != 1){
    reqAddNumber("n", n);
  }
  if(user != NULL){
    reqAddString("user", user);
  }
  String jsonBody = String(cJSON_Print(req));
  cJSON_Delete(req);
  String res = oai.post(endpoint, jsonBody);
  if(!res.length()){
    log_e("Empty result!");
    return result;
  }
  return OpenAI_ImageResponse(res.c_str());
}

// images/variations { //Creates a variation of a given image.
//   "image": "",//required string. The image to edit. Must be a valid PNG file, less than 4MB, and square.
//   "n": 1,//integer. The number of images to generate. Must be between 1 and 10.
//   "size": "1024x1024",//string. The size of the generated images. Must be one of "256x256", "512x512", or "1024x1024"
//   "response_format": "url",//string. The format in which the generated images are returned. Must be one of "url" or "b64_json".
//   "user": null//string. A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.
// }

OpenAI_ImageVariation::OpenAI_ImageVariation(OpenAI &openai)
  : oai(openai)
  , size(OPENAI_IMAGE_SIZE_1024x1024)
  , response_format(OPENAI_IMAGE_RESPONSE_FORMAT_URL)
  , n(1)
  , user(NULL)
{}

OpenAI_ImageVariation::~OpenAI_ImageVariation(){
  if(user != NULL){
    free((void*)user);
  }
}

OpenAI_ImageVariation & OpenAI_ImageVariation::setSize(OpenAI_Image_Size s){
  if(s >= OPENAI_IMAGE_SIZE_1024x1024 && s <= OPENAI_IMAGE_SIZE_256x256){
    size = s;
  }
  return *this;
}

OpenAI_ImageVariation & OpenAI_ImageVariation::setResponseFormat(OpenAI_Image_Response_Format f){
  if(f >= OPENAI_IMAGE_RESPONSE_FORMAT_URL && f <= OPENAI_IMAGE_RESPONSE_FORMAT_B64_JSON){
    response_format = f;
  }
  return *this;
}

OpenAI_ImageVariation & OpenAI_ImageVariation::setN(unsigned int _n){
  if(n > 0 && n <= 10){
    n = _n;
  }
  return *this;
}

OpenAI_ImageVariation & OpenAI_ImageVariation::setUser(const char * u){
  if(user != NULL){
    free((void*)user);
  }
  user = strdup(u);
  return *this;
}

OpenAI_ImageResponse OpenAI_ImageVariation::image(uint8_t * img_data, size_t img_len){
  String endpoint = "images/variations";
  String boundary = "----WebKitFormBoundaryb9v538xFWfzLzRO3";
  String itemPrefix = "--" +boundary+ "\r\nContent-Disposition: form-data; name=";
  uint8_t * data = NULL;
  size_t len = 0;
  OpenAI_ImageResponse result = OpenAI_ImageResponse(NULL);

  String reqBody = "";
  if(size != OPENAI_IMAGE_SIZE_1024x1024){
    reqBody += itemPrefix+"\"size\"\r\n\r\n"+String(image_sizes[size])+"\r\n";
  }
  if(response_format != OPENAI_IMAGE_RESPONSE_FORMAT_URL){
    reqBody += itemPrefix+"\"response_format\"\r\n\r\n"+String(image_response_formats[response_format])+"\r\n";
  }
  if(n != 1){
    reqBody += itemPrefix+"\"n\"\r\n\r\n"+String(n)+"\r\n";
  }
  if(user != NULL){
    reqBody += itemPrefix+"\"user\"\r\n\r\n"+String(user)+"\r\n";
  }
  reqBody += itemPrefix+"\"image\"; filename=\"image.png\"\r\nContent-Type: image/png\r\n\r\n";

  String reqEndBody = "\r\n--" +boundary+ "--\r\n";

  len = reqBody.length() + reqEndBody.length() + img_len;

  data = (uint8_t*)malloc(len + 1);
  if(data == NULL){
    log_e("Failed to allocate request buffer! Len: %u", len);
    return result;
  }
  uint8_t * d = data;
  memcpy(d, reqBody.c_str(), reqBody.length());
  d += reqBody.length();
  memcpy(d, img_data, img_len);
  d += img_len;
  memcpy(d, reqEndBody.c_str(), reqEndBody.length());
  d += reqEndBody.length();
  *d = 0;

  String res = oai.upload(endpoint, boundary, data, len);
  free(data);
  if(!res.length()){
    log_e("Empty result!");
    return result;
  }
  return OpenAI_ImageResponse(res.c_str());
}

// images/edits { //Creates an edited or extended image given an original image and a prompt.
//   "image": "",//required string. The image to edit. Must be a valid PNG file, less than 4MB, and square. If mask is not provided, image must have transparency, which will be used as the mask.
//   "mask": "",//optional string. An additional image whose fully transparent areas (e.g. where alpha is zero) indicate where image should be edited. Must be a valid PNG file, less than 4MB, and have the same dimensions as image.
//   "prompt": "A cute baby sea otter",//required. A text description of the desired image(s). The maximum length is 1000 characters.
//   "n": 1,//integer. The number of images to generate. Must be between 1 and 10.
//   "size": "1024x1024",//string. The size of the generated images. Must be one of "256x256", "512x512", or "1024x1024"
//   "response_format": "url",//string. The format in which the generated images are returned. Must be one of "url" or "b64_json".
//   "user": null//string. A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.
// }

OpenAI_ImageEdit::OpenAI_ImageEdit(OpenAI &openai)
  : oai(openai)
  , prompt(NULL)
  , size(OPENAI_IMAGE_SIZE_1024x1024)
  , response_format(OPENAI_IMAGE_RESPONSE_FORMAT_URL)
  , n(1)
  , user(NULL)
{}

OpenAI_ImageEdit::~OpenAI_ImageEdit(){
  if(prompt != NULL){
    free((void*)prompt);
  }
  if(user != NULL){
    free((void*)user);
  }
}

OpenAI_ImageEdit & OpenAI_ImageEdit::setPrompt(const char * p){
  if(prompt != NULL){
    free((void*)prompt);
    prompt = NULL;
  }
  if(p != NULL){
    prompt = strdup(p);
  }
  return *this;
}

OpenAI_ImageEdit & OpenAI_ImageEdit::setSize(OpenAI_Image_Size s){
  if(s >= OPENAI_IMAGE_SIZE_1024x1024 && s <= OPENAI_IMAGE_SIZE_256x256){
    size = s;
  }
  return *this;
}

OpenAI_ImageEdit & OpenAI_ImageEdit::setResponseFormat(OpenAI_Image_Response_Format f){
  if(f >= OPENAI_IMAGE_RESPONSE_FORMAT_URL && f <= OPENAI_IMAGE_RESPONSE_FORMAT_B64_JSON){
    response_format = f;
  }
  return *this;
}

OpenAI_ImageEdit & OpenAI_ImageEdit::setN(unsigned int _n){
  if(n > 0 && n <= 10){
    n = _n;
  }
  return *this;
}

OpenAI_ImageEdit & OpenAI_ImageEdit::setUser(const char * u){
  if(user != NULL){
    free((void*)user);
  }
  user = strdup(u);
  return *this;
}

OpenAI_ImageResponse OpenAI_ImageEdit::image(uint8_t * img_data, size_t img_len, uint8_t * mask_data, size_t mask_len){
  String endpoint = "images/edits";
  String boundary = "----WebKitFormBoundaryb9v538xFWfzLzRO3";
  String itemPrefix = "--" +boundary+ "\r\nContent-Disposition: form-data; name=";
  uint8_t * data = NULL;
  size_t len = 0;
  OpenAI_ImageResponse result = OpenAI_ImageResponse(NULL);

  String reqBody = "";
  if(prompt != NULL){
    reqBody += itemPrefix+"\"prompt\"\r\n\r\n"+String(prompt)+"\r\n";
  }
  if(size != OPENAI_IMAGE_SIZE_1024x1024){
    reqBody += itemPrefix+"\"size\"\r\n\r\n"+String(image_sizes[size])+"\r\n";
  }
  if(response_format != OPENAI_IMAGE_RESPONSE_FORMAT_URL){
    reqBody += itemPrefix+"\"response_format\"\r\n\r\n"+String(image_response_formats[response_format])+"\r\n";
  }
  if(n != 1){
    reqBody += itemPrefix+"\"n\"\r\n\r\n"+String(n)+"\r\n";
  }
  if(user != NULL){
    reqBody += itemPrefix+"\"user\"\r\n\r\n"+String(user)+"\r\n";
  }
  reqBody += itemPrefix+"\"image\"; filename=\"image.png\"\r\nContent-Type: image/png\r\n\r\n";

  String reqEndBody = "\r\n--" +boundary+ "--\r\n";

  len = reqBody.length() + reqEndBody.length() + img_len;

  String maskBody = "";
  if(mask_data != NULL && mask_len > 0){
    maskBody += "\r\n"+itemPrefix+"\"mask\"; filename=\"mask.png\"\r\nContent-Type: image/png\r\n\r\n";
    len += maskBody.length() + mask_len;
  }

  data = (uint8_t*)malloc(len + 1);
  if(data == NULL){
    log_e("Failed to allocate request buffer! Len: %u", len);
    return result;
  }
  uint8_t * d = data;
  memcpy(d, reqBody.c_str(), reqBody.length());
  d += reqBody.length();
  memcpy(d, img_data, img_len);
  d += img_len;
  if(mask_data != NULL && mask_len > 0){
    memcpy(d, maskBody.c_str(), maskBody.length());
    d += maskBody.length();
    memcpy(d, mask_data, mask_len);
    d += mask_len;
  }
  memcpy(d, reqEndBody.c_str(), reqEndBody.length());
  d += reqEndBody.length();
  *d = 0;

  String res = oai.upload(endpoint, boundary, data, len);
  free(data);
  if(!res.length()){
    log_e("Empty result!");
    return result;
  }
  return OpenAI_ImageResponse(res.c_str());
}

// audio/transcriptions { //Transcribes audio into the input language.
//   "file": "audio.mp3",//required. The audio file to transcribe, in one of these formats: mp3, mp4, mpeg, mpga, m4a, wav, or webm.
//   "model": "whisper-1",//required. ID of the model to use. Only whisper-1 is currently available.
//   "prompt": "A cute baby sea otter",//An optional text to guide the model's style or continue a previous audio segment. The prompt should match the audio language.
//   "response_format": "json",//string. The format of the transcript output, in one of these options: "json", "text", "srt", "verbose_json", or "vtt".
//   "temperature": 1,//float between 0 and 2
//   "language": null//string. The language of the input audio. Supplying the input language in ISO-639-1 format will improve accuracy and latency.
// }

static const char * audio_input_formats[] = {
  "mp3", 
  "mp4", 
  "mpeg", 
  "mpga", 
  "m4a", 
  "wav", 
  "webm"
};
static const char * audio_input_mime[] = {
  "audio/mpeg", 
  "audio/mp4", 
  "audio/mpeg", 
  "audio/mpeg", 
  "audio/x-m4a", 
  "audio/x-wav", 
  "audio/webm"
};
static const char * audio_response_formats[] = {"json", "text", "srt", "verbose_json", "vtt"};

    const char * prompt;
    OpenAI_Audio_Response_Format response_format;
    float temperature;
    const char * language;

OpenAI_AudioTranscription::OpenAI_AudioTranscription(OpenAI &openai)
  : oai(openai)
  , prompt(NULL)
  , response_format(OPENAI_AUDIO_RESPONSE_FORMAT_JSON)
  , temperature(0)
  , language(NULL)
{}

OpenAI_AudioTranscription::~OpenAI_AudioTranscription(){
  if(prompt != NULL){
    free((void*)prompt);
  }
  if(language != NULL){
    free((void*)language);
  }
}

OpenAI_AudioTranscription & OpenAI_AudioTranscription::setPrompt(const char * p){
  if(prompt != NULL){
    free((void*)prompt);
    prompt = NULL;
  }
  if(p != NULL){
    prompt = strdup(p);
  }
  return *this;
}

OpenAI_AudioTranscription & OpenAI_AudioTranscription::setResponseFormat(OpenAI_Audio_Response_Format f){
  if(f >= OPENAI_AUDIO_RESPONSE_FORMAT_JSON && f <= OPENAI_AUDIO_RESPONSE_FORMAT_VTT){
    response_format = f;
  }
  return *this;
}

OpenAI_AudioTranscription & OpenAI_AudioTranscription::setTemperature(float t){
  if(t >= 0 && t <= 2.0){
    temperature = t;
  }
  return *this;
}

OpenAI_AudioTranscription & OpenAI_AudioTranscription::setLanguage(const char * l){
  if(language != NULL){
    free((void*)language);
    language = NULL;
  }
  if(l != NULL){
    language = strdup(l);
  }
  return *this;
}

String OpenAI_AudioTranscription::file(uint8_t * audio_data, size_t audio_len, OpenAI_Audio_Input_Format f){
  String endpoint = "audio/transcriptions";
  String boundary = "----WebKitFormBoundary9HKFexBRLrf9dcpY";
  String itemPrefix = "--" +boundary+ "\r\nContent-Disposition: form-data; name=";
  uint8_t * data = NULL;
  size_t len = 0;

  String reqBody = itemPrefix+"\"model\"\r\n\r\nwhisper-1\r\n";
  if(prompt != NULL){
    reqBody += itemPrefix+"\"prompt\"\r\n\r\n"+String(prompt)+"\r\n";
  }
  if(response_format != OPENAI_AUDIO_RESPONSE_FORMAT_JSON){
    reqBody += itemPrefix+"\"response_format\"\r\n\r\n"+String(audio_response_formats[response_format])+"\r\n";
  }
  if(temperature != 0){
    reqBody += itemPrefix+"\"temperature\"\r\n\r\n"+String(temperature)+"\r\n";
  }
  if(language != NULL){
    reqBody += itemPrefix+"\"language\"\r\n\r\n"+String(language)+"\r\n";
  }
  reqBody += itemPrefix+"\"file\"; filename=\"audio."+String(audio_input_formats[f])+"\"\r\nContent-Type: "+String(audio_input_mime[f])+"\r\n\r\n";

  String reqEndBody = "\r\n--" +boundary+ "--\r\n";

  len = reqBody.length() + reqEndBody.length() + audio_len;
  data = (uint8_t*)malloc(len + 1);
  if(data == NULL){
    log_e("Failed to allocate request buffer! Len: %u", len);
    return String();
  }
  uint8_t * d = data;
  memcpy(d, reqBody.c_str(), reqBody.length());
  d += reqBody.length();
  memcpy(d, audio_data, audio_len);
  d += audio_len;
  memcpy(d, reqEndBody.c_str(), reqEndBody.length());
  d += reqEndBody.length();
  *d = 0;

  String result = oai.upload(endpoint, boundary, data, len);
  free(data);
  if(!result.length()){
    log_e("Empty result!");
    return result;
  }
  cJSON * json = cJSON_Parse(result.c_str());
  String error = getJsonError(json);
  result = "";
  if(error.length()){
    log_e("%s",error.c_str());
  } else {
    if(cJSON_HasObjectItem(json, "text")){
      cJSON * text = cJSON_GetObjectItem(json, "text");
      result = String(cJSON_GetStringValue(text));
    }
  }
  cJSON_Delete(json);
  return result;
}

// audio/translations { //Translates audio into into English.
//   "file": "german.m4a",//required. The audio file to translate, in one of these formats: mp3, mp4, mpeg, mpga, m4a, wav, or webm.
//   "model": "whisper-1",//required. ID of the model to use. Only whisper-1 is currently available.
//   "prompt": "A cute baby sea otter",//An optional text to guide the model's style or continue a previous audio segment. The prompt should match the audio language.
//   "response_format": "json",//string. The format of the transcript output, in one of these options: "json", "text", "srt", "verbose_json", or "vtt".
//   "temperature": 1//float between 0 and 2
// }

OpenAI_AudioTranslation::OpenAI_AudioTranslation(OpenAI &openai)
  : oai(openai)
  , prompt(NULL)
  , response_format(OPENAI_AUDIO_RESPONSE_FORMAT_JSON)
  , temperature(0)
{}

OpenAI_AudioTranslation::~OpenAI_AudioTranslation(){
  if(prompt != NULL){
    free((void*)prompt);
  }
}

OpenAI_AudioTranslation & OpenAI_AudioTranslation::setPrompt(const char * p){
  if(prompt != NULL){
    free((void*)prompt);
    prompt = NULL;
  }
  if(p != NULL){
    prompt = strdup(p);
  }
  return *this;
}

OpenAI_AudioTranslation & OpenAI_AudioTranslation::setResponseFormat(OpenAI_Audio_Response_Format f){
  if(f >= OPENAI_AUDIO_RESPONSE_FORMAT_JSON && f <= OPENAI_AUDIO_RESPONSE_FORMAT_VTT){
    response_format = f;
  }
  return *this;
}

OpenAI_AudioTranslation & OpenAI_AudioTranslation::setTemperature(float t){
  if(t >= 0 && t <= 2.0){
    temperature = t;
  }
  return *this;
}

String OpenAI_AudioTranslation::file(uint8_t * audio_data, size_t audio_len, OpenAI_Audio_Input_Format f){
  String endpoint = "audio/translations";
  String boundary = "----WebKitFormBoundary9HKFexBRLrf9dcpY";
  String itemPrefix = "--" +boundary+ "\r\nContent-Disposition: form-data; name=";
  uint8_t * data = NULL;
  size_t len = 0;

  String reqBody = itemPrefix+"\"model\"\r\n\r\nwhisper-1\r\n";
  if(prompt != NULL){
    reqBody += itemPrefix+"\"prompt\"\r\n\r\n"+String(prompt)+"\r\n";
  }
  if(response_format != OPENAI_AUDIO_RESPONSE_FORMAT_JSON){
    reqBody += itemPrefix+"\"response_format\"\r\n\r\n"+String(audio_response_formats[response_format])+"\r\n";
  }
  if(temperature != 0){
    reqBody += itemPrefix+"\"temperature\"\r\n\r\n"+String(temperature)+"\r\n";
  }
  reqBody += itemPrefix+"\"file\"; filename=\"audio."+String(audio_input_formats[f])+"\"\r\nContent-Type: "+String(audio_input_mime[f])+"\r\n\r\n";

  String reqEndBody = "\r\n--" +boundary+ "--\r\n";

  len = reqBody.length() + reqEndBody.length() + audio_len;
  data = (uint8_t*)malloc(len + 1);
  if(data == NULL){
    log_e("Failed to allocate request buffer! Len: %u", len);
    return String();
  }
  uint8_t * d = data;
  memcpy(d, reqBody.c_str(), reqBody.length());
  d += reqBody.length();
  memcpy(d, audio_data, audio_len);
  d += audio_len;
  memcpy(d, reqEndBody.c_str(), reqEndBody.length());
  d += reqEndBody.length();
  *d = 0;

  String result = oai.upload(endpoint, boundary, data, len);
  free(data);
  if(!result.length()){
    log_e("Empty result!");
    return result;
  }
  cJSON * json = cJSON_Parse(result.c_str());
  String error = getJsonError(json);
  result = "";
  if(error.length()){
    log_e("%s",error.c_str());
  } else {
    if(cJSON_HasObjectItem(json, "text")){
      cJSON * text = cJSON_GetObjectItem(json, "text");
      result = String(cJSON_GetStringValue(text));
    }
  }
  cJSON_Delete(json);
  return result;
}


// files { //Upload a file that contains document(s) to be used across various endpoints/features.
//   "file": "mydata.jsonl",//required. Name of the JSON Lines file to be uploaded. If the purpose is set to "fine-tune", each line is a JSON record with "prompt" and "completion" fields representing your training examples.
//   "purpose": "fine-tune"//required. The intended purpose of the uploaded documents. Use "fine-tune" for Fine-tuning. This allows us to validate the format of the uploaded file.
// }

// GET files //Returns a list of files that belong to the user's organization.
// DELETE files/{file_id} //Delete a file.
// GET files/{file_id} //Returns information about a specific file.
// GET files/{file_id}/content //Returns the contents of the specified file

