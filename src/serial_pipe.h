#ifndef SERIAL_PIPE_H
#define SERIAL_PIPE_H
#include <Arduino.h>

#define OK F("200#")
#define NOK F("404#")

class Pipe
{
public:
	Pipe(Stream *serialPtr);
	void setBoundary(char initiator, char terminator);

	void sendOpcode(uint8_t opcode);
	int getOpcode();
	void send(const char *data);
	void send(uint8_t opCode, const char *data);
	void send(uint8_t opCode, char *data, uint8_t mid);
	void send(uint8_t opCode, uint32_t data);
	char *read(char *dataPtr, uint16_t len);
	char *query(uint8_t opCode, char *buf, uint16_t len);
	

	bool sendWithAck(const char *data);
	char *readWithAck(char *dataPtr, uint16_t len);

	int waitForAck();
	void ack();
	void noAck();

	void setAckTimeout(int time);
	bool isConnected();
	
private:
	Stream *serial;
	int _ackTimeout = 1000;

	long _last_millis;
	char _initiator = '$';
	char _terminator = '#';

	int _available();
	char *readUntil(char *buffer,char terminator, uint16_t len);
	bool _discardUntil(char terminator);
	void _bufferClear();
	int _timedRead();
	void _sendHeader(uint8_t opCode);
	void _handleInternalOpcode(uint8_t opCode);
};
#endif