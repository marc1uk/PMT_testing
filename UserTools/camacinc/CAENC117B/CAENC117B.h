/*
 * CAENC1178 CAMAC module class, single word only!
 * adapted from existing Lecroy3377 class
 *
 * Author: Michael Nieslony
 */

#ifndef CAENC117B_H
#define CAENC117B_H

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <bitset>
#include <vector>

#include "libxxusb.h"
#include "CamacCrate.h"

class CAENC117B : public CamacCrate
{
	public:

		CAENC117B(int NSlot, std::string config, int i = 0);// : CamacCrate(i);	//Constructor !

		int GetData(std::vector<int> &mData);		//Store data into map

		int ReadFIFOall(std::vector<int> &data);	//F(0)·A(0) 
		int TestLAM();					//F(8)·A(0)
		int WriteFIFOall(std::vector<int> &data); 	//F(16)·A(0)
		int Transmit();					//F(17)·A(0)
		int WaitforSlave(); 				//no special F(x)·A(y)
		int ClearAll();					//F(9)·A(0)
		int DisLAM();					//F(24)·A(0)
		int EnLAM();					//F(26)·A(0)
		int READ(int F, int A, int &Data, int &Q, int &X);		//F(x)·A(y) F:0..15,24-31
		int WRITE(int F, int A, int &Data, int &Q, int &X);		//F(x)·A(y) F:16-23

		int GetID();				//Return ID
		int GetSlot();				//Return Slot

		void SetConfig(std::string config);	//Set register from configfile


		int CommunicationSequence(int controller, int CrateNumber, int operation, std::vector<int> &values, std::vector<int> &response);
		int ModuleIdentifier();
		int ReadSlotN(int n);
		int ReadCrateOccupation();
		int ReadChannelStatus(int slot, int channel);
		int ReadChannelParameterValues(int slot, int channel);
		int SetV0(int slot, int channel, int V0);
		int SetV1(int slot, int channel, int V1);
		int SetI0(int slot, int channel, int I0);
		int SetI1(int slot, int channel, int I1);
		int SetVmax(int slot, int channel, int Vmax);
		int SetRampUp(int slot, int channel, int Rup);
		int SetRampDown(int slot, int channel, int Rdown);
		int SetTrip(int slot, int channel, int Trip);
		int SetFlag(int slot, int channel, int Flag);

		int ChanneltoHex(int slot, int channel);
		std::string ConvertInttoAsci(int asci_code);
		int GetLowByte(int number);
		int GetHighByte(int number);
		int CombineBytes(int low, int high);
		unsigned int Combine4Bytes(int byte1, int byte2, int byte3, int byte4);
		int TestOperation(int crateN);






	private:

		int ID;
		int Common;	//just for demonstration purposes for reading from config file

		int n;
		int V0;
		int V1;
		int I0;
		int I1;
		int Vmax;
		int Rup;
		int Rdown;
		int Trip;
		int Flag;


};

#endif
