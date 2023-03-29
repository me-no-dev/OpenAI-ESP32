#pragma once
#include "Arduino.h"
#include "cJSON.h"

class OpenAI_Completion;
class OpenAI_ChatCompletion;
class OpenAI_Edit;
class OpenAI_ImageGeneration;
class OpenAI_ImageVariation;
class OpenAI_ImageEdit;
class OpenAI_AudioTranscription;
class OpenAI_AudioTranslation;

typedef enum {
  OPENAI_IMAGE_SIZE_1024x1024,
  OPENAI_IMAGE_SIZE_512x512,
  OPENAI_IMAGE_SIZE_256x256
} OpenAI_Image_Size;

typedef enum {
  OPENAI_IMAGE_RESPONSE_FORMAT_URL,
  OPENAI_IMAGE_RESPONSE_FORMAT_B64_JSON
} OpenAI_Image_Response_Format;

typedef enum {
  OPENAI_AUDIO_RESPONSE_FORMAT_JSON,
  OPENAI_AUDIO_RESPONSE_FORMAT_TEXT,
  OPENAI_AUDIO_RESPONSE_FORMAT_SRT,
  OPENAI_AUDIO_RESPONSE_FORMAT_VERBOSE_JSON,
  OPENAI_AUDIO_RESPONSE_FORMAT_VTT
} OpenAI_Audio_Response_Format;

typedef enum {
  OPENAI_AUDIO_INPUT_FORMAT_MP3,
  OPENAI_AUDIO_INPUT_FORMAT_MP4,
  OPENAI_AUDIO_INPUT_FORMAT_MPEG,
  OPENAI_AUDIO_INPUT_FORMAT_MPGA,
  OPENAI_AUDIO_INPUT_FORMAT_M4A,
  OPENAI_AUDIO_INPUT_FORMAT_WAV,
  OPENAI_AUDIO_INPUT_FORMAT_WEBM
} OpenAI_Audio_Input_Format;

typedef struct {
    unsigned int len;
    double * data;
} OpenAI_EmbeddingData;

class OpenAI_EmbeddingResponse {
  private:
    unsigned int usage;
    unsigned int len;
    OpenAI_EmbeddingData * data;
    char * error_str;

  public:
    OpenAI_EmbeddingResponse(const char * payload);
    ~OpenAI_EmbeddingResponse();

    unsigned int tokens(){
      return usage;
    }
    unsigned int length(){
      return len;
    }
    OpenAI_EmbeddingData * getAt(unsigned int index){
      if(index < len){
        return &data[index];
      }
      return NULL;
    }
    const char * error(){
      return error_str;
    }
};

class OpenAI_ModerationResponse {
  private:
    unsigned int len;
    bool * data;
    char * error_str;

  public:
    OpenAI_ModerationResponse(const char * payload);
    ~OpenAI_ModerationResponse();

    unsigned int length(){
      return len;
    }
    bool getAt(unsigned int index){
      if(index < len){
        return data[index];
      }
      return false;
    }
    const char * error(){
      return error_str;
    }
};

class OpenAI_ImageResponse {
  private:
    unsigned int len;
    char ** data;
    char * error_str;

  public:
    OpenAI_ImageResponse(const char * payload);
    ~OpenAI_ImageResponse();

    unsigned int length(){
      return len;
    }
    const char * getAt(unsigned int index){
      if(index < len){
        return data[index];
      }
      return "";
    }
    const char * error(){
      return error_str;
    }
};

class OpenAI_StringResponse {
  private:
    unsigned int usage;
    unsigned int len;
    char ** data;
    char * error_str;

  public:
    OpenAI_StringResponse(const char * payload);
    ~OpenAI_StringResponse();

    unsigned int tokens(){
      return usage;
    }
    unsigned int length(){
      return len;
    }
    const char * getAt(unsigned int index){
      if(index < len){
        return data[index];
      }
      return "";
    }
    const char * error(){
      return error_str;
    }
};

class OpenAI {
  private:
    String api_key;

  protected:

  public:
    OpenAI(const char *openai_api_key);
    ~OpenAI();

    OpenAI_EmbeddingResponse embedding(String input, const char * model=NULL, const char * user=NULL);  //Creates an embedding vector representing the input text.
    OpenAI_ModerationResponse moderation(String input, const char * model=NULL);   //Classifies if text violates OpenAI's Content Policy

    OpenAI_Completion completion();
    OpenAI_ChatCompletion chat();
    OpenAI_Edit edit();
    OpenAI_ImageGeneration imageGeneration();
    OpenAI_ImageVariation imageVariation();
    OpenAI_ImageEdit imageEdit();
    OpenAI_AudioTranscription audioTranscription();
    OpenAI_AudioTranslation audioTranslation();

    String get(String endpoint);
    String del(String endpoint);
    String post(String endpoint, String jsonBody);
    String upload(String endpoint, String boundary, uint8_t * data, size_t len);
};

class OpenAI_Completion {
  private:
    OpenAI & oai;
    const char * model;
    unsigned int max_tokens;
    float temperature;
    float top_p;
    unsigned int n;
    bool echo;
    const char * stop;
    float presence_penalty;
    float frequency_penalty;
    unsigned int best_of;
    const char * user;

  protected:

  public:
    OpenAI_Completion(OpenAI &openai);
    ~OpenAI_Completion();

    OpenAI_Completion & setModel(const char * m);
    OpenAI_Completion & setMaxTokens(unsigned int m); //The maximum number of tokens to generate in the completion.
    OpenAI_Completion & setTemperature(float t);      //float between 0 and 2. Higher value gives more random results.
    OpenAI_Completion & setTopP(float t);             //float between 0 and 1. recommended to alter this or temperature but not both.
    OpenAI_Completion & setN(unsigned int n);         //How many completions to generate for each prompt.
    OpenAI_Completion & setEcho(bool e);              //Echo back the prompt in addition to the completion
    OpenAI_Completion & setStop(const char * s);      //Up to 4 sequences where the API will stop generating further tokens.
    OpenAI_Completion & setPresencePenalty(float p);  //float between -2.0 and 2.0. Positive values increase the model's likelihood to talk about new topics.
    OpenAI_Completion & setFrequencyPenalty(float p); //float between -2.0 and 2.0. Positive values decrease the model's likelihood to repeat the same line verbatim.
    OpenAI_Completion & setBestOf(unsigned int b);    //Generates best_of completions server-side and returns the "best". "best_of" must be greater than "n"
    OpenAI_Completion & setUser(const char * u);      //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

    OpenAI_StringResponse prompt(String p);           //Send the prompt for completion
};

class OpenAI_ChatCompletion {
  private:
    OpenAI & oai;
    cJSON * messages;
    const char * model;
    const char * description;
    unsigned int max_tokens;
    float temperature;
    float top_p;
    const char * stop;
    float presence_penalty;
    float frequency_penalty;
    const char * user;

  protected:

  public:
    OpenAI_ChatCompletion(OpenAI &openai);
    ~OpenAI_ChatCompletion();

    OpenAI_ChatCompletion & setModel(const char * m);
    OpenAI_ChatCompletion & setSystem(const char * s);    //Description of the required assistant
    OpenAI_ChatCompletion & setMaxTokens(unsigned int m); //The maximum number of tokens to generate in the completion.
    OpenAI_ChatCompletion & setTemperature(float t);      //float between 0 and 2. Higher value gives more random results.
    OpenAI_ChatCompletion & setTopP(float t);             //float between 0 and 1. recommended to alter this or temperature but not both.
    OpenAI_ChatCompletion & setStop(const char * s);      //Up to 4 sequences where the API will stop generating further tokens.
    OpenAI_ChatCompletion & setPresencePenalty(float p);  //float between -2.0 and 2.0. Positive values increase the model's likelihood to talk about new topics.
    OpenAI_ChatCompletion & setFrequencyPenalty(float p); //float between -2.0 and 2.0. Positive values decrease the model's likelihood to repeat the same line verbatim.
    OpenAI_ChatCompletion & setUser(const char * u);      //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.
    OpenAI_ChatCompletion & clearConversation();          //clears the accumulated conversation

    OpenAI_StringResponse message(String m, bool save=true);//Send the message for completion. Save it with the first response if selected
};

class OpenAI_Edit {
  private:
    OpenAI & oai;
    const char * model;
    float temperature;
    float top_p;
    unsigned int n;

  protected:

  public:
    OpenAI_Edit(OpenAI &openai);
    ~OpenAI_Edit();

    OpenAI_Edit & setModel(const char * m);
    OpenAI_Edit & setTemperature(float t);      //float between 0 and 2. Higher value gives more random results.
    OpenAI_Edit & setTopP(float t);             //float between 0 and 1. recommended to alter this or temperature but not both.
    OpenAI_Edit & setN(unsigned int n);         //How many edits to generate for the input and instruction.

    OpenAI_StringResponse process(String instruction, String input=String()); //Creates a new edit for the provided input, instruction, and parameters.
};

class OpenAI_ImageGeneration {
  private:
    OpenAI & oai;
    OpenAI_Image_Size size;
    OpenAI_Image_Response_Format response_format;
    unsigned int n;
    const char * user;

  protected:

  public:
    OpenAI_ImageGeneration(OpenAI &openai);
    ~OpenAI_ImageGeneration();

    OpenAI_ImageGeneration & setSize(OpenAI_Image_Size s);                      //The size of the generated images.
    OpenAI_ImageGeneration & setResponseFormat(OpenAI_Image_Response_Format f); //The format in which the generated images are returned.
    OpenAI_ImageGeneration & setN(unsigned int n);                              //The number of images to generate. Must be between 1 and 10.
    OpenAI_ImageGeneration & setUser(const char * u);                           //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

    OpenAI_ImageResponse prompt(String p);                                      //Creates image/images from given a prompt.
};

class OpenAI_ImageVariation {
  private:
    OpenAI & oai;
    OpenAI_Image_Size size;
    OpenAI_Image_Response_Format response_format;
    unsigned int n;
    const char * user;

  protected:

  public:
    OpenAI_ImageVariation(OpenAI &openai);
    ~OpenAI_ImageVariation();

    OpenAI_ImageVariation & setSize(OpenAI_Image_Size s);                      //The size of the generated images.
    OpenAI_ImageVariation & setResponseFormat(OpenAI_Image_Response_Format f); //The format in which the generated images are returned.
    OpenAI_ImageVariation & setN(unsigned int n);                              //The number of images to generate. Must be between 1 and 10.
    OpenAI_ImageVariation & setUser(const char * u);                           //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

    OpenAI_ImageResponse image(uint8_t * data, size_t len);                                  //Creates an image given a prompt.
};

class OpenAI_ImageEdit {
  private:
    OpenAI & oai;
    const char * prompt;
    OpenAI_Image_Size size;
    OpenAI_Image_Response_Format response_format;
    unsigned int n;
    const char * user;

  protected:

  public:
    OpenAI_ImageEdit(OpenAI &openai);
    ~OpenAI_ImageEdit();

    OpenAI_ImageEdit & setPrompt(const char * p);                         //A text description of the desired image(s). The maximum length is 1000 characters.
    OpenAI_ImageEdit & setSize(OpenAI_Image_Size s);                      //The size of the generated images.
    OpenAI_ImageEdit & setResponseFormat(OpenAI_Image_Response_Format f); //The format in which the generated images are returned.
    OpenAI_ImageEdit & setN(unsigned int n);                              //The number of images to generate. Must be between 1 and 10.
    OpenAI_ImageEdit & setUser(const char * u);                           //A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

    OpenAI_ImageResponse image(uint8_t * data, size_t len, uint8_t * mask_data=NULL, size_t mask_len=0); //Creates an edited or extended image given an original image and a prompt.
};

class OpenAI_AudioTranscription {
  private:
    OpenAI & oai;
    const char * prompt;
    OpenAI_Audio_Response_Format response_format;
    float temperature;
    const char * language;

  protected:

  public:
    OpenAI_AudioTranscription(OpenAI &openai);
    ~OpenAI_AudioTranscription();

    OpenAI_AudioTranscription & setPrompt(const char * p);                          //An optional text to guide the model's style or continue a previous audio segment. The prompt should match the audio language.
    OpenAI_AudioTranscription & setResponseFormat(OpenAI_Audio_Response_Format f);  //The format of the transcript output
    OpenAI_AudioTranscription & setTemperature(float t);                            //float between 0 and 2
    OpenAI_AudioTranscription & setLanguage(const char * l);                        //The language in ISO-639-1 format of the input audio. NULL for Auto

    String file(uint8_t * data, size_t len, OpenAI_Audio_Input_Format f);           //Transcribe an audio file
};

class OpenAI_AudioTranslation {
  private:
    OpenAI & oai;
    const char * prompt;
    OpenAI_Audio_Response_Format response_format;
    float temperature;

  protected:

  public:
    OpenAI_AudioTranslation(OpenAI &openai);
    ~OpenAI_AudioTranslation();

    OpenAI_AudioTranslation & setPrompt(const char * p);                          //An optional text to guide the model's style or continue a previous audio segment. The prompt should be in English.
    OpenAI_AudioTranslation & setResponseFormat(OpenAI_Audio_Response_Format f);  //The format of the transcript output
    OpenAI_AudioTranslation & setTemperature(float t);                            //float between 0 and 2

    String file(uint8_t * data, size_t len, OpenAI_Audio_Input_Format f);         //Transcribe an audio file
};

