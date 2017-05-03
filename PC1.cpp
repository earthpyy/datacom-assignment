#include <stdio.h>
#include <tchar.h>
#include "Serial.h"
#include <iostream>
#include <cmath>
#include <string>

#define COM_PORT_S "\\\\.\\COM11"
#define COM_PORT_R "\\\\.\\COM9"
#define TIME_OUT 15															// 1/10 second
#define FRAME_LENGTH 19
#define TEST_SINGLE false

using namespace std;

bool isValidData(string);
bool isValidFrameNo(char);
bool isValidMyFrameNo(char);
bool isACK(string);
bool isImgData(string);
void sendACK();
void sendLastACK();
void sendData(string);
string extractedData(string);
string encodeCRC(string);
bool decodeCRC(string);
string makeCRC(string);
string intToBin6(int);
int binToInt8(string);

bool isDebug = true;

int frameNo = 0, myFrameNo;
int readResult = 0;
char incomingData[256] = "";
unsigned int dataLength = FRAME_LENGTH;
bool isStarted = false, isWaitForStartData = false, isWaitForControlData = false;
bool suc = false;

Serial *SPS, *SPR;

int main(int argc, char* argv[])
{
	// TODO: silent mode using argv[]

	string outgoingData, cmd, tmp = "";
	string frameType, frameAngle, frameData;
	int ccAmount, colorInput, waitingAmount;

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

		if (SPR->IsConnected())
		{
			cout << "We're connected!" << endl << endl;
		}
	}

	while(SPS->IsConnected() && SPR->IsConnected())
	{
		if (isWaitForStartData)
		{
		    sendACK();
			//myFrameNo = 0;
			waitingAmount = 3;
			while (waitingAmount > 0)
			{
				readResult = 0;

				readResult = SPR->ReadData(incomingData,dataLength);
				Sleep(100);
				if (readResult != 0)
				{
					incomingData[readResult] = 0;
					if (isDebug)
						cout << "Received data. [" << incomingData << "]" << endl;
					// cout << "[" << makeCRC(incomingData) << "]";
					if (isValidData(incomingData) && isImgData(incomingData))								// check CRC is right
					{
						if (isValidMyFrameNo(incomingData[0]))
						{
							if (isDebug)
								cout << "Valid images data [" << incomingData[0] << "] received! Saving to COLOR[" << (3 - waitingAmount) << "]" << endl;
							color[3 - waitingAmount] = binToInt8(extractedData(incomingData));
							myFrameNo = (myFrameNo == 0 ? 1 : 0);						// toggle frameNo
							waitingAmount--;
							Sleep(100);
						}
						else
						{
							if (isDebug)
								cout << "Invalid ACK No. [" << incomingData[0] << "]. Sending ACK" << myFrameNo << " again.." << endl;
						}
						sendACK();
					}
					else
					{
						if (isDebug)
							cout << "Rejected invalid images data [" << incomingData[0] << "]!" << endl;
                        sendACK();
					}
				}
			}
			Sleep(300);
			sendLastACK();
			cout << endl;
			for (int i = 0; i < 3; i++)
			{
				cout << "Average color level (" << (i == 0 ? "-45" : i == 1 ? "0" : "45") << ") is " << color[i] << endl;
			}
			isWaitForStartData = false;
		}
		else if (isWaitForControlData)
		{
		    sendACK();
			//myFrameNo = 0;
			cout << endl;
			tmp = "";
			for (int i = 0; i < ccAmount; i++)
			{
				waitingAmount = 3;
				while (waitingAmount > 0)
				{
					readResult = 0;

					readResult = SPR->ReadData(incomingData,dataLength);
					Sleep(100);
					if (readResult != 0)
					{
						incomingData[readResult] = 0;
						if (isDebug)
							cout << "[" << i << "] Received data. [" << incomingData << "]" << endl;
						// cout << "[" << makeCRC(incomingData) << "]";
						if (isValidData(incomingData) && isImgData(incomingData))								// check CRC is right
						{
							if (isValidMyFrameNo(incomingData[0]))
							{
								if (isDebug)
									cout << "[" << i << "] Valid images data [" << incomingData[0] << "] received!" << endl;
								if (waitingAmount == 3)
								{
									x = binToInt8(extractedData(incomingData));
								}
								else if (waitingAmount == 2)
								{
									y = binToInt8(extractedData(incomingData));
								}
								else
								{
									colorLevel = binToInt8(extractedData(incomingData));
								}
								myFrameNo = (myFrameNo == 0 ? 1 : 0);						// toggle myFrameNo
								waitingAmount--;
                                Sleep(100);
							}
							else
							{
								if (isDebug)
									cout << "[" << i << "] Invalid Frame No. [" << incomingData[0] << "]. Sending ACK" << myFrameNo << " again.." << endl;
							}
							sendACK();
						}
						else
						{
							if (isDebug)
								cout << "[" << i << "] Rejected invalid images data [" << incomingData[0] << "]!" << endl;
                            sendACK();
						}
					}
				}
				tmp.append("[" + to_string(i) + "] [X: " + to_string(x) + "][Y: " + to_string(y) + "][colorLevel: " + to_string(colorLevel) + "]\n");
			}
			Sleep(300);
			sendLastACK();
			cout << endl << ccAmount << " RANDOM SAMPLES:" << endl << tmp;
			isWaitForControlData = false;
		}
		else
		{
			cout << endl << (isStarted ? "+" : "-") << " PC1: ";
			cin >> cmd;

			if (!cmd.compare("start") && !isStarted)
			{
				frameType = "00";												// command
				frameData = "00000000";											// start send frame
			}
			else if (!cmd.compare("stop") && isStarted)
			{
				frameType = "00";												// command
				frameData = "00111111";											// stop send frame
			}
			else if (!cmd.compare("control"))
			{
				cin >> colorInput;
				cin >> ccAmount;

				if (isStarted)
				{
					if (ccAmount > 63)
					{
						cout << "Data Error!" << endl;
						continue;
					}

					// TODO : use sprintf and loop instead of brutforce
					if (colorInput == color[0])
					{
						frameAngle = "01";											// -45
					}
					else if (colorInput == color[1])
					{
						frameAngle = "10";											// 0
					}
					else if (colorInput == color[2])
					{
						frameAngle = "11";											// 45
					}
					else
					{
						cout << "Data Error!" << endl;
						continue;
					}

					frameType = "10";												// control cam frame
					frameData = frameAngle + intToBin6(ccAmount);
				}
				else
				{
					cout << "Cannot control camera when service is stopped!" << endl;
					continue;
				}
			}
			else
			{
				cout << "Data Error!" << endl;
				continue;
			}

			outgoingData = encodeCRC(to_string(frameNo) + frameType + frameData);
            sendData(outgoingData);

            myFrameNo = (myFrameNo == 0 ? 1 : 0);

			if (!cmd.compare("start"))
			{
				isStarted = true;
				isWaitForStartData = true;
			}
			else if (!cmd.compare("control"))
			{
				isWaitForControlData = true;
			}
			else if (!cmd.compare("stop"))
			{
				isStarted = false;
				sendACK();
				sendLastACK();
				cout << "Stopped all service." << endl;
			}

			Sleep(300);
		}
	}
	return 0;
}

string intToBin6(int num)
{
	string result = "000000";
	int n = 0;
    for (int i = 32; i >= 1; i /= 2)
	{
		result[n++] = (num / i == 1 ? '1' : '0');
		num %= i;
	}
	result[n] = 0;
	return result;
}

int binToInt8(string data)
{
	int result = 0;
	for (int i = 0; i < 8; i++)
	{
		result += (data[i] - '0') * pow(2, 7 - i);
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
				cout << "Sended command [" << outgoingData << "]! Wait for ACK.." << endl;
			// Sleep(TIME_OUT);
			readResult = 0;

			for (int i = 0; i < TIME_OUT; i++)
			{
				readResult = SPR->ReadData(incomingData,dataLength);
				Sleep(300);
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
				if (isValidData(incomingData) && isACK(incomingData) && isValidFrameNo(incomingData[0] == '0' ? '1' : '0'))	// isACK and CRC is right
				{
					cout << "Received ACK" << incomingData[0] << "!" << endl;

					sendAgain = false;
					frameNo = (frameNo == 0 ? 1 : 0);							// toggle frameNo
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
		Sleep(100);
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
	return (decodeCRC(data) && data.length() == 19);					// [ frameNo ][ xx ][ xxxxxxxx ][ valid CRC ]
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

bool isImgData(string data)
{
	return (!data.substr(1, 2).compare("01"));									// [ x ][ 01 ][ xxxxxxxx ][ xxxxxxxx ]
}

void sendACK()
{
	string outgoingData = encodeCRC(to_string(myFrameNo) + "1101000001");			// [ frameNo ][ 11 ][ 01000001 ][ xxxxxxxx ]
	SPS->WriteData(outgoingData.c_str(), outgoingData.length());
	if (isDebug)
        cout << "Sended ACK" << myFrameNo << "!" << endl;
}

void sendLastACK()
{
    while (true)
	{
	    Sleep(TIME_OUT * 600);
		readResult = SPR->ReadData(incomingData,dataLength);
		if (readResult == 0)
		{
			break;
		}
		sendACK();
		if (isDebug)
			cout << "Sending additional ACK" << myFrameNo << ".." << endl;
	}
}

string extractedData(string data)
{
	return data.substr(3, 8);
}
