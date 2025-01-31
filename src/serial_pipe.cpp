#include "serial_pipe.h"

#define QUERY_TIMEOUT 1000

#define LIBRARY_MAX_OPCODE_NUM 9
#define OPCODE_CONNECTED 5


Pipe::Pipe(Stream *serialPtr)
{
	serial = serialPtr;
}

void Pipe::setBoundary(char initiator, char terminator)
{
	_initiator = initiator;
	_terminator = terminator;
}

void Pipe::setAckTimeout(int time)
{
	_ackTimeout = time;
}

int Pipe::_timedRead()
{
	int c;
	_last_millis = millis();
	uint32_t nowMs;
	do
	{
		c = serial -> read();
		if (c > 0 )
		{
			return c;
		}
		nowMs = millis();
		nowMs = nowMs - _last_millis;
	} while (nowMs < 1000);
	return -1;
}

char * Pipe::readUntil(char *buffer,char terminator, uint16_t len)
{
	char *ptr  = buffer;
	uint8_t dCount = 0;

	int c = _timedRead();
	while(c >=0 && c != terminator && dCount<len)
	{
		*ptr = (char)c;
		ptr++;
		dCount++;
		c = _timedRead();
	}

	// Serial.print("Term : ");Serial.println((char)c);
	//Check terminator found properly or not. 
	if(c == terminator)
	{
		// Serial.println(F("Term: "));
		*ptr = '\0'; //string terminator as extra safety
		return buffer;
	}
	else
	{
		// Serial.println(F("Not Term"));
		return ptr;//sending the position of null pointer;
	}
	// Serial.print("Term : ");Serial.println((char)c);
	
	// _bufferClear();
	// return buffer;
}

void Pipe::_bufferClear()
{
	while (serial -> available())
	{
		serial -> read();
	}
}

int Pipe::_available()
{
	int len = serial -> available();
	if(len > 0)
	{
		return len;
	}
	else
	{
		return -1;
	}
	// int current_len = 0;
	// current_len = serial -> available();
	// int len_diff = current_len  - _last_len;
	// if(len_diff>0)
	// {
	// 	Serial.println(F("Increasing"));
	// 	_last_len = current_len;
	// 	_last_millis = millis();
	// }
	// else
	// {
	// 	long current_millis = millis();
	// 	if( current_millis -  _last_millis > 1000)
	// 	{
	// 		_last_millis = current_millis;
	// 		if(current_len> 0 ) return current_len;
	// 	}
	// }
	// return -1;
}

bool Pipe::_discardUntil(char terminator)
{
	int c = 0;
	do
	{
		c = _timedRead();
		if((char)c == terminator)
		{
			// Serial.print(F("Found: "));Serial.println((char)c);
			return true;
		} 
	}while(c>0);

	return false;
}

int Pipe::getOpcode()
{
	int length = _available();
	if(length > 0)
	{
		bool firstChar = _discardUntil(_initiator);
		if(firstChar)
		{
			char opStr[6];
			char *ptr = readUntil(opStr,'=',6);
			// Serial.print(F("op: "));Serial.println(ptr);
			int opcode = atoi(ptr);
			if(opcode <= LIBRARY_MAX_OPCODE_NUM)
			{
				Serial.println(F("<Pipe Internal OPC>"));
				_handleInternalOpcode(opcode);
				return 0;
			}
			return opcode;
		}
	}
	return -1;

}

void Pipe::_handleInternalOpcode(uint8_t opCode)
{
	switch(opCode)
	{
		case OPCODE_CONNECTED:
			// delay(5000);
			ack();// send ack for connected
		break;
		default:
		break;
	}
}

void Pipe::send(const char *data)
{
	serial -> print(data);
}

void Pipe::_sendHeader(uint8_t opCode)
{
	char temp[6];
	char *p = temp;
	*p++ = _initiator;
	itoa(opCode,p,10);
	uint8_t len = strlen(p);
	p = p+len;
	*p++ = '=';
	*p = '\0';
	serial -> write(temp);
}

void Pipe::send(uint8_t opCode, const char *data)
{
	// char temp[6];
	// char *p = temp;
	// *p++ = _initiator;
	// itoa(opCode,p,10);
	// uint8_t len = strlen(p);
	// p = p+len;
	// *p++ = '=';
	// *p = '\0';
	// // Serial.print("Test str: "); Serial.println(temp);
	// serial -> write(temp);
	_sendHeader(opCode);
	serial -> write(data);
	serial -> print('#');
}
void Pipe::send(uint8_t opCode, char *data, uint8_t mid)
{
	_sendHeader(opCode);
	serial -> write((char)mid);
	serial -> write(data);
	serial -> print('#');
}
void Pipe::send(uint8_t opCode, uint32_t data)
{
	_sendHeader(opCode);
	serial -> print(data);
	serial -> print('#');
}

char *Pipe::read(char *dataPtr, uint16_t len)
{
	char *ptr = readUntil(dataPtr,_terminator, len);
	return ptr;
	// return dataPtr;
}

char *Pipe::query(uint8_t opCode, char *buf, uint16_t len)
{
	// Serial.print(F("Pipe Query opCode : "));Serial.println(opCode);
	_sendHeader(opCode);
	int timeOut = QUERY_TIMEOUT;
	int retCode;
	do
	{
		retCode = getOpcode();
		delay(1);
		// Serial.print(F("Code : "));Serial.println(retCode);
	}while(!(retCode == opCode) && --timeOut);
	char *p = read(buf, len);
	return p;
}

void Pipe::sendOpcode(uint8_t opcode)
{
	_sendHeader(opcode);
}

bool Pipe::isConnected()
{
	_sendHeader(OPCODE_CONNECTED);
	return (waitForAck() == 200);
	// if(waitForAck() == 200)
	// {
	// 	Serial.println(F("---->PIPE connected"));
	// }
	// else
	// {
	// 	Serial.println(F("---->Pipe not connected"));
	// }
	
}

// void Pipe::sendAck(const __FlashStringHelper *msg)
// {
// 	serial -> print(msg);
// }

void Pipe::ack()
{
	serial -> print(OK);
}
void Pipe::noAck()
{
	serial ->print(NOK);
}

int Pipe::waitForAck()
{
	int timeOut = _ackTimeout;
	int code = 0;
	char ackData[5];
	while(--timeOut)
	{
		if(serial -> available())
		{
			char *ptr = read(ackData,5);
			code = atoi(ptr);
			// Serial.println(code);
			if(code == 200)
			{
				break;
			}
		}
		delay(1);
	}
	_bufferClear();
	if(code>0)
	{
		return code;
	}
	Serial.println(F("Pipe Wait Timeout"));
	return -1;
}


bool Pipe::sendWithAck(const char *data)
{
	serial -> print(data);
	int code = waitForAck();
}


char *Pipe::readWithAck(char *dataPtr, uint16_t len)
{
	char *ptr = dataPtr;
	readUntil(ptr,_terminator,len);
	int len2 = strlen(dataPtr);
	if(len2 >0 )
	{
		// sendAck(OK);
		ack();
		// serial -> print(F("200#"));
		// serial -> print(F("404#"));
	}
	else
	{
		noAck();
		// serial -> print(F("404#"));
	}
	Serial.println(dataPtr);
	return dataPtr;
}

