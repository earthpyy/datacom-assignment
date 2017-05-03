#include <stdio.h>
#include <tchar.h>
#include "Serial.h"
#include <iostream>
#include <cmath>
#include <string>

#define COM_PORT_S "\\\\.\\COM5"
#define COM_PORT_R "\\\\.\\COM3"
#define COM_PORT_A "\\\\.\\COM10"
#define TIME_OUT 15																// 1/10 second
#define FRAME_LENGTH 19
#define TEST_SINGLE false

using namespace std;

bool isValidData(string);
bool isValidFrameNo(char);
bool isValidMyFrameNo(char);
bool isACK(string);
bool isControl(string);
bool isStartCmd(string);
bool isStopCmd(string);
void sendACK();
int getCamData();
void sendData(string);
string extractedData(string);
string encodeCRC(string);
bool decodeCRC(string);
string makeCRC(string);
string intToBin8(int);
int binToInt6(string);
void camStart();
void camReady();
void camControl(int, int);

bool isDebug = true;

int frameNo = 0, myFrameNo;
char incomingData[256] = "";
unsigned int dataLength = FRAME_LENGTH;
int readResult = 0;
bool suc = false;

Serial *SPS, *SPR, *SPA;

int main(int argc, char* argv[])
{
	string outgoingData, tmp;
	string frameType, frameData;
	unsigned int frameAngle, ccAmount;
	int found, tmpi;

	string angleName[3] = {"-45", "0", "45"};
	int color[3] = {-1, -1, -1};
	int x, y, colorLevel;

	cout << "Connecting to " << COM_PORT_S << ".." << endl;
	SPS = new Serial(COM_PORT_S);

	if (SPS->IsConnected())
	{
		cout << "We're connected!" << endl << endl;
	}

	if (TEST_SINGLE)
	{
		SPR = SPS;
	}
	else
	{
		cout << "Connecting to " << COM_PORT_R << ".." << endl;
		SPR = new Serial(COM_PORT_R);

		// TODO: connect to third COM port with ARDUINO

		if (SPR->IsConnected())
		{
			cout << "We're connected!" << endl << endl;
		}
	}

	cout << "Connecting to " << COM_PORT_A << ".." << endl;
	SPA = new Serial(COM_PORT_A);

	if (SPA->IsConnected())
	{
		cout << "We're connected!" << endl << endl;
	}

	camReady();

	while(SPS->IsConnected() && SPR->IsConnected() && SPA->IsConnected())
	{
		readResult = 0;
		readResult = SPR->ReadData(incomingData,dataLength);
		Sleep(100);
		if (readResult != 0)
		{
			incomingData[readResult] = 0;
			if (isDebug)
				cout << "Received data. [" << incomingData << "]" << endl;

			if (isValidData(incomingData) && (isStartCmd(incomingData) || isStopCmd(incomingData) || isControl(incomingData)))
			{
				if (isValidFrameNo(incomingData[0]))
				{
					frameNo = (frameNo == 0 ? 1 : 0);
					// myFrameNo = 0;
					if (isStartCmd(incomingData))
					{
						if (isDebug)
							cout << "Received START command [" << incomingData[0] << "]! Sending ACK" << frameNo << ".." << endl;
						sendACK();
						cout << "Getting data from camera.." << endl;
						camStart();
						// TODO: while (outputFileIsOK);

						frameType = "01";
						found = getCamData();
						tmp = incomingData;
						if (isDebug)
							cout << "Received camera data [" << tmp << "]" << endl;
						found = tmp.find(' ');
						color[2] = stoi(tmp.substr(0, found));
						color[1] = stoi(tmp.substr(found + 1, tmp.find(' ', found + 1)));
						color[0] = stoi(tmp.substr(tmp.find(' ', found + 1) + 1));

						// if (isDebug)
							// cout << color[0] << " | " << color[1] << " | " << color[2] << " | " << endl;

						for (int i = 0; i < 3; i++)
						{
							// found = tmp.find(' ');
							// color[i] = tmp.substr(0, tmp.length() - found);
							frameData = intToBin8(color[i]);
							cout << endl << "Sending average color level (" << (i == 0 ? "-45" : i == 1 ? "0" : "45") << ") [Value: " << color[i] << "]" << endl;

							outgoingData = encodeCRC(to_string(myFrameNo) + frameType + frameData);
							// cout << "[" << frameData << " | " << outgoingData << "]" << endl;
							sendData(outgoingData);

							// myFrameNo = (myFrameNo == 0 ? 1 : 0);
							Sleep(300);
						}

					}
					else if (isStopCmd(incomingData))
					{
						if (isDebug)
							cout << "Received STOP command [" << incomingData[0] << "]! Sending ACK" << frameNo << ".." << endl;
						sendACK();
						cout << "Stopped all service." << endl;
					}
					else if (isControl(incomingData))
					{
						if (isDebug)
							cout << "Received CONTROL command [" << incomingData[0] << "]! Sending ACK" << frameNo << ".." << endl;

						tmp = extractedData(incomingData);
						sendACK();

						if (!tmp.substr(0, 2).compare("01"))
						{
							frameAngle = 0;
						}
						else if (!tmp.substr(0, 2).compare("10"))
						{
							frameAngle = 1;
						}
						else if (!tmp.substr(0, 2).compare("11"))
						{
							frameAngle = 2;
						}
						ccAmount = binToInt6(tmp.substr(2, 6));

						cout << "Getting " << ccAmount << " data from camera (" << angleName[frameAngle] << " degree).." << endl;
						camControl(frameAngle, ccAmount);
						// TODO: while (outputFileIsOK);

						found = getCamData();
						tmp = incomingData;
						if (isDebug)
							cout << "Received camera data [" << tmp << "]" << endl;

						frameType = "01";
						int p = 0, q = 0;
						for (int i = 0; i < ccAmount; i++)
						{
							// found = getCamData();
							// tmp = incomingData;
							// if (isDebug)
							// 	cout << "Received camera data [" << tmp << "]" << endl;
							// found = tmp.find(' ');
							// x = stoi(tmp.substr(0, found));
							// y = stoi(tmp.substr(found + 1, tmp.find(' ', found + 1)));
							// colorLevel = stoi(tmp.substr(tmp.find(' ', found + 1) + 1));

							for (int j = 0; j < 3; j++)
							{

								// tmpi = stoi(tmp.substr((i == 0 && j == 0 ? 0 : tmp.find(' '))));
								// found = tmp.find(' ');

								tmpi = stoi(tmp.substr(p+(p!=0), (q=tmp.find(' ', p+1))-p-(p!=0)));
								p = q;

								//if (isDebug)
								//	cout << "[" << tmpi << "]";

								if (j == 0)
								{
									x = tmpi;
									frameData = intToBin8(x);
									cout << endl << "Sending x (" << (frameAngle == 0 ? "-45" : frameAngle == 1 ? "0" : "45") << ") [Value: " << x << "]" << endl;
								}
								else if (j == 1)
								{
									y = tmpi;
									frameData = intToBin8(y);
									cout << endl << "Sending y (" << (frameAngle == 0 ? "-45" : frameAngle == 1 ? "0" : "45") << ") [Value: " << y << "]" << endl;
								}
								else if (j == 2)
								{
									colorLevel = tmpi;
									frameData = intToBin8(colorLevel);
									cout << endl << "Sending color level (" << (frameAngle == 0 ? "-45" : frameAngle == 1 ? "0" : "45") << ") [Value: " << colorLevel << "]" << endl;
								}

								outgoingData = encodeCRC(to_string(myFrameNo) + frameType + frameData);
								sendData(outgoingData);

								// camStart();
								Sleep(300);
							}
						}
					}
					else
					{
						if (isDebug)
							cout << "Rejected invalid command!" << endl;
						continue;
					}
				}
				else
				{
					if (isDebug)
						cout << "Invalid Frame No. [" << incomingData[0] << "]. Sending ACK" << frameNo << " again.." << endl;
					sendACK();
					continue;
				}
				cout << "Done!" << endl << endl;
				Sleep(300);
			}
			else
			{
				cout << "Invalid command [" << incomingData[0] << "]!" << endl;
				continue;
			}
		}
	}
	return 0;
}

string intToBin8(int num)
{
	string result = "00000000";
	int n = 0;
    for (int i = 128; i >= 1; i /= 2)
	{
		result[n++] = (num / i == 1 ? '1' : '0');
		num %= i;
	}
	result[n] = 0;
	return result;
}

int binToInt6(string data)
{
	int result = 0;
	for (int i = 0; i < 6; i++)
	{
		result += (data[i] - '0') * pow(2, 5 - i);
	}
	return result;
}

// TODO: change to bool and if send fail 3 times then error return false
void sendData(string outgoingData)
{
	bool sendAgain;

	do
	{
		suc = SPS->WriteData(outgoingData.c_str(), outgoingData.length());

		if (suc)																// send success
		{
			if (isDebug)
				cout << "Sended data! Wait for ACK.." << endl;
			// Sleep(TIME_OUT);
			readResult = 0;

			for (int i = 0; i < TIME_OUT; i++)
			{
				Sleep(300);
				readResult = SPR->ReadData(incomingData,dataLength);
				// Sleep(300);
				if (readResult != 0)
				{
					break;
				}
			}

			if (readResult != 0)												// receiving ACK
			{
				incomingData[readResult] = 0;
				if (isDebug)
					cout << "Received data. [" << incomingData << "]" << endl;
				if (isValidData(incomingData) && isACK(incomingData) && isValidMyFrameNo(incomingData[0] == '0' ? '1' : '0'))	// isACK and CRC is right
				{
					cout << "Received ACK" << incomingData[0] << "!" << endl;

					sendAgain = false;
					myFrameNo = (myFrameNo == 0 ? 1 : 0);						// toggle frameNo
				}
				else
				{
					if (isDebug)
						cout << "Invalid ACK! Try sending again.." << endl;
					sendAgain = true;
				}
			}
			else																// no ACK received
			{
				if (isDebug)
					cout << "Time out! No data received. Try sending again.." << endl;
				sendAgain = true;
			}
		}
		else	// TODO: sendAgain = true and try again 3 times then sendAgain = false
		{
			cout << "Failed to send data!" << endl;
			sendAgain = false;
		}
		Sleep(300);
	}
	while (sendAgain);
}

string encodeCRC(string data)
{
	return data + makeCRC(data);
}

bool decodeCRC(string data)
{
	return !makeCRC(data).compare("00000000");
}

string makeCRC(string BitString)
{
    string Res = "00000000";                                					 // CRC Result
    int CRC[8] = {0}, DoInvert;

    for (int i = 0; i < BitString.length(); i++)
    {
    	DoInvert = (BitString[i] == '1' ? 1 : 0) ^ CRC[7];

	    CRC[7] = CRC[6];
	    CRC[6] = CRC[5];
	    CRC[5] = CRC[4];
	    CRC[4] = CRC[3];
	    CRC[3] = CRC[2];
	    CRC[2] = CRC[1] ^ DoInvert;
	    CRC[1] = CRC[0] ^ DoInvert;
	    CRC[0] = DoInvert;
      }

	 for (int i = 0; i < 8; i++)
	 {
		 Res[7-i] = (CRC[i] ? '1' : '0');										// Convert binary to ASCII
	 }
     Res[8] = 0;                                        						// Set string terminator

     return Res;
}

bool isValidData(string data)
{
	return (decodeCRC(data) && data.length() == 19);							// [ frameNo ][ xx ][ xxxxxxxx ][ valid CRC ]
}

bool isValidFrameNo(char frameNoInput)
{
	return (frameNo == (frameNoInput - '0'));
}

bool isValidMyFrameNo(char frameNoInput)
{
	return (myFrameNo == (frameNoInput - '0'));
}

bool isACK(string data)
{
	return (!data.substr(1, 2).compare("11") && !extractedData(data).compare("01000001"));			// [ x ][ 11 ][ xxxxxxxx ][ xxxxxxxx ]
}

bool isControl(string data)
{
	return (!data.substr(1, 2).compare("10"));
}

bool isStartCmd(string data)
{
	return (!data.substr(1, 2).compare("00") && !extractedData(data).compare("00000000"));
}

bool isStopCmd(string data)
{
	return (!data.substr(1, 2).compare("00") && !extractedData(data).compare("00111111"));
}

void sendACK()
{
	string outgoingData = encodeCRC(to_string(frameNo) + "1101000001");			// [ frameNo ][ 11 ][ 01000001 ][ xxxxxxxx ]
	// SPS->WriteData(outgoingData.c_str(), outgoingData.length());
	// if (isDebug)
	// 	cout << "Sended ACK" << frameNo << "!";
	cout << "--- Sending ACK" << frameNo << " ---" << endl;
	sendData(outgoingData);
	cout << "--- Sended ACK ---" << endl;
}

string extractedData(string data)
{
	return data.substr(3, 8);
}

int getCamData()
{
	readResult = 0;
	while (true)
	{
		readResult = SPA->ReadData(incomingData, 255);
		Sleep(100);
		if (readResult != 0)
		{
			incomingData[readResult] = 0;
			if (isDebug)
				cout << "Received data. [" << incomingData << "]" << endl;
			break;
		}
		if (isDebug)
			cout << ".";
	}
	return 0;
}

void camReady()
{
	string outgoingData = "*TxREADY*";
	suc = SPA->WriteData(outgoingData.c_str(), outgoingData.length());
	if (isDebug)
	{
		if (suc)
			cout << "Send TxREADY to Arduino successful!" << endl;
		else
			cout << "Send TxREADY to Arduino unsuccessful :(" << endl;
	}
}

void camStart()
{
	string outgoingData = "3";
	suc = SPA->WriteData(outgoingData.c_str(), outgoingData.length());
	if (isDebug)
	{
		if (suc)
			cout << "Send command to Arduino successful!" << endl;
		else
			cout << "Send command to Arduino unsuccessful :(" << endl;
	}
}

void camControl(int angle, int amount)
{
	string outgoingData = to_string(angle) + " " + to_string(amount) + "#";
	suc = SPA->WriteData(outgoingData.c_str(), outgoingData.length());
	if (isDebug)
	{
		if (suc)
			cout << "Send CONTROL [" << outgoingData << "] to Arduino successful!" << endl;
		else
			cout << "Send CONTROL [" << outgoingData << "] to Arduino unsuccessful :(" << endl;
	}
}
