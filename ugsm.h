#ifndef UGSM_H_
#define UGSM_H_

// mini library to work with sim900a specially
// created by Ahmed Alkabir
#include <Arduino.h>
#include <string.h>
#include <stdint.h>

class uGsm
{
public:
  uGsm(){}
  void begin(Stream *_serial);

  // to check if the arduino is connected to module
  bool isPoweredUp(uint16_t time_out);

  // to check if the module is registered to network
  bool isRegistered(uint16_t time_out);

  // to send message to specifc destenation
  bool sendSMS(const char *dst, const char *msg);
  bool sendSMS(const __FlashStringHelper *dst, const __FlashStringHelper *msg);
  // return true if there's income message to read
  bool messageToRead();
  // return 1 if readSMS read message successfully otherwise return 0, and return -1
  // for invalid input
  int readLastSMS(char *phone_number, char *received_message);
  // TODO : prevent from entering unreasonable number
  // return 1 if readSMS read message successfully otherwise return 0, and return -1
  // for invalid input
  int readSMS(uint8_t index_m, char *phone_number, char *received_message);
  void doCommand(const char *cmd, void (*cb)());
  void doCommand(const __FlashStringHelper *cmd, void (*cb)());
  
  //
  bool deleteSMS(uint8_t index_m);
  //
  bool deleteAllSMS();

  void disableEcho();
  void enableEcho();

  void test_responed_function();

private:
  Stream *_serialGSM = nullptr;
  char buffer[203];
  char last_message_index[3];
  // private methods
  inline void write_at_command(const char *cmd);
  inline void write_at_command(const __FlashStringHelper *cmd);
  void flush_the_serial_and_buffer();
  bool wait_for_response(const char *rsp, uint16_t time_out);
  bool wait_for_response(const __FlashStringHelper *rsp, uint16_t time_out);
  char* read_buffer();
  inline bool is_contain_response(const char *rsp);
  inline bool is_contain_response(const __FlashStringHelper *rsp);
};

// implementation of private methods
char* uGsm::read_buffer(){
  if(_serialGSM->available() > 0){
    char ch;
    char *ptr = buffer;
    while(_serialGSM->available() > 0){
      ch = _serialGSM->read();
      *ptr++ = ch;
    }
    *ptr = '\0';
  } 
  // let's make a few delay between command
  // Serial.println(buffer);
  delay(20);
  return buffer;
}

void uGsm::write_at_command(const char *cmd){
  flush_the_serial_and_buffer();
  uint8_t cmd_len = strlen(cmd);
  for(uint8_t i = 0; i < cmd_len; i++){
    _serialGSM->write(cmd[i]);
  }
  delay(500);
}

void uGsm::write_at_command(const __FlashStringHelper *cmd){
  char cmdR[strlen_P(reinterpret_cast<const char *>(cmd)) + 1];
  strcpy_P(cmdR, reinterpret_cast<const char *>(cmd));
  write_at_command(cmdR);
}

bool uGsm::wait_for_response(const char *response, uint16_t time_out = 1000){
  uint16_t last_time = millis();
  while(1){
    const char *pBuffer = read_buffer();
    if(is_contain_response(response)){
      flush_the_serial_and_buffer();
      return true;
    }

    if(millis() - last_time > time_out){
      flush_the_serial_and_buffer();
      return false;
    }
  }
  return false;
}

bool uGsm::wait_for_response(const __FlashStringHelper *response, uint16_t time_out){
  char responseR[strlen_P(reinterpret_cast<const char *>(response)) + 1];
  strcpy_P(responseR, reinterpret_cast<const char *>(response));
  return wait_for_response(responseR, time_out);
}

void uGsm::flush_the_serial_and_buffer(){
  while(_serialGSM->available() > 0)
    _serialGSM->read();
  memset(buffer, '\0', 203);
}

bool uGsm::is_contain_response(const char *rsp){
  if(strstr(buffer, rsp) == NULL)
    return false;
  return true;
}

bool uGsm::is_contain_response(const __FlashStringHelper *rsp){
  char rsp_r[strlen_P(reinterpret_cast<const char *>(rsp)) + 1];
  strcpy_P(rsp_r, reinterpret_cast<const char *>(rsp));
  return is_contain_response(rsp_r);
}

// implementation of public methods
void uGsm::begin(Stream *_serial){
  _serialGSM = _serial;
  enableEcho();
}

void uGsm::disableEcho(){
  write_at_command(F("ATE0\r"));
  flush_the_serial_and_buffer();
}

void uGsm::enableEcho(){
  write_at_command(F("ATE1\r"));
  flush_the_serial_and_buffer();
}

bool uGsm::isPoweredUp(uint16_t time_out){
  write_at_command(F("AT\r"));
  return wait_for_response(F("OK"), time_out);
}

bool uGsm::isRegistered(uint16_t time_out){
  write_at_command(F("AT+COPS?\r"));
  return wait_for_response(F("+COPS: 0,0"), time_out);
}

bool uGsm::sendSMS(const char *dst, const char *msg){
  // let's set text mode for sending message
  flush_the_serial_and_buffer();
  write_at_command(F("AT+CMGF=1\r"));
  if(!wait_for_response(F("OK"), 3000))
    return false;
  delay(20);
  // let's send message
  char sms_dst[20];
  sprintf_P(sms_dst, PSTR("AT+CMGS=\"%s\"\r"), dst);
  write_at_command(sms_dst);
  delay(20);
  if(wait_for_response(F(">"), 3000)){
    // DEBUG
    // Serial.println("DEBUG -- SEND MESSAGE");
    // write message 
    const char *pChar = msg;
    while(*pChar != '\0'){
      _serialGSM->write(*pChar++);
    }

    // after that write end marker
    _serialGSM->write((char)26);
    delay(200);
    flush_the_serial_and_buffer();
    while(_serialGSM->available() == 0);
    return wait_for_response(F("+CMG"), 5000);
  }else{
    return false;
  }
}

bool uGsm::sendSMS(const __FlashStringHelper *dst, const __FlashStringHelper *msg){
  char dst_r[strlen_P(reinterpret_cast<const char *>(dst)) + 1];
  char msg_r[strlen_P(reinterpret_cast<const char *>(msg)) + 1];

  strcpy_P(dst_r, reinterpret_cast<const char *>(dst));
  strcpy_P(msg_r, reinterpret_cast<const char *>(msg));

  return sendSMS(dst_r, msg_r);
}

bool uGsm::messageToRead(){
  if(_serialGSM->available() > 0){
    const char *expct_rsp = "+CMTI: \"SM\",";
    const char *pBuffer = read_buffer();
    uint8_t len_rsp = strlen(expct_rsp);
    uint8_t incrChar = 0;

    char *pLastIndex = last_message_index;
    while(*pBuffer != '\0'){
      (*pBuffer++ == expct_rsp[incrChar]) ? incrChar++ : 0;
      if(incrChar == len_rsp){
        // here i'm going to read the index of received message
        for(uint8_t i=0; i < strlen(pBuffer); i++){
          *pLastIndex++ = *pBuffer++;
        }
        *pLastIndex = '\0';
        return true;
      }
    }
  }
  return false;
}

int uGsm::readLastSMS(char *phone_number, char *received_message){
  return readSMS(atoi(last_message_index), phone_number, received_message);
}

int uGsm::readSMS(uint8_t index_m, char *phone_number, char *received_message){
  char c_index_m[3];
  char cmd[12];
  itoa(index_m, c_index_m, 10);
  // uint16_t last_time = millis();
  sprintf_P(cmd,PSTR("AT+CMGR=%s\r"), c_index_m);
  flush_the_serial_and_buffer();
  write_at_command(cmd);
  char *pBuffer = read_buffer();
  // flush_the_serial_and_buffer();
  // Serial.println(pBuffer);
  char *pPhone_number = phone_number;
  char *pReceived_message = received_message;
  bool phoneFetchedDone = false;
  while(*pBuffer != '\0'){
    // char ch = *pBuffer++;
    // start reading phone number
    if(*pBuffer++ == ',' && !phoneFetchedDone){
      // ignore "
      *pBuffer++;
      while(*pBuffer != '"'){
        *pPhone_number++ = *pBuffer++;
      }
      *pPhone_number = '\0';
      phoneFetchedDone = true;
    }
    // let's wait for <CR>
    if(*pBuffer == '\r' && phoneFetchedDone){
      // ignore <LF>
      *pBuffer++;
      *pBuffer++;
      while(*pBuffer != '\0'){
        // Serial.write(*pBuffer++);
        *pReceived_message++ = *pBuffer++;
        // break current loop
        if(*pBuffer == '\r')
          break;
      }
      // add the null character to state it as string literals 
      *pReceived_message = '\0';
      // after we got the contents of message we stop the loop 
      return 1;
    }
  }
  return 0;
}

void uGsm::doCommand(const char *cmd, void (*cb)()){
  // well, we're going to read last message as suppoesd to be a command to do something special
  // let's flush the serial buffer
  flush_the_serial_and_buffer();
  char cmd_at[10];
  sprintf_P(cmd_at, PSTR("AT+CMGR=%s\r"), last_message_index);
  write_at_command(cmd_at);
  if(wait_for_response(cmd, 500)){
    cb();
  }
}

void uGsm::doCommand(const __FlashStringHelper *cmd, void (*cb)()){
  char cmd_d[strlen_P(reinterpret_cast<const char *>(cmd)) + 1];
  strcpy_P(cmd_d, reinterpret_cast<const char *>(cmd));

  doCommand(cmd_d, cb);
}


bool uGsm::deleteAllSMS(){
  write_at_command(F("AT+CMGD=0,4\r"));
  return wait_for_response(F("OK"), 3000);
}
void uGsm::test_responed_function(){
  write_at_command("AT+COPS?\r");
  Serial.println(read_buffer());
}
#endif