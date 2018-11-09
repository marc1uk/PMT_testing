/*
CAENC1178 CAMAC module class, single word only!
adapted from existing Lecroy3377 class
Author: Marcus O'Flaherty
*/

#ifndef CAENC117B_H
#define CAENC117B_H

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <bitset>
#include <vector>

#include <ctime>
#include <thread>
#include <chrono>
#include <unistd.h>

#include "libxxusb.h"
#include "CamacCrate.h"

class CAENC117B : public CamacCrate
{
	public:

		CAENC117B(int NSlot, std::string config, int i = 0);// : CamacCrate(i);	//Constructor !

		short ReadBuffer(int &Data, int &Q);
		short DoRead(std::vector<uint16_t> &readvals, int timeout_ms=500);
		int FillTxBuffer(uint16_t datain);
		short SendTxBuffer();
		void PrintError(int Data);
		int AssembleBuffer(std::vector<uint16_t> opcodes_and_values);
		short AssembleAndTransmit(std::vector<uint16_t> opcodes_and_values);
		uint16_t card_chan_to_word(int card, int channel);
		uint16_t setting_to_word(std::string setting, bool value);
		short ReadFwVer();
		int ClearAll();					//F(9)·A(0)
		int TestLAM();
		short DisableLAM();
		short EnableLAM();

		int READ(int F, int A, int &Data, int &Q, int &X);	//Generic READ
		int WRITE(int F, int A, int &Data, int &Q, int &X);	//Generic WRITE
		int GetID();						//Return ID of module
		int GetSlot();						//Return n of Slot of module
		void SetConfig(std::string config);			//Set register from configfile

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
		
		std::map<std::string,uint16_t> function_to_opcode;

		std::map<std::string, uint16_t> setting_to_mask;

		std::map<std::string, uint16_t> setting_to_flag;

};

#endif
