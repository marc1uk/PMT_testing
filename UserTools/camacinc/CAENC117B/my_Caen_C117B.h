/* vim:set noexpandtab tabstop=4 wrap 
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

		short ReadBuffer(int &Data, int &Q);                                    // wrapper of READ with minimal error checking
		short DoRead(std::vector<uint16_t> &readvals, int timeout_ms=800);      // wrapper for ReadBuffer with looping until end of data or read timeout
		int FillTxBuffer(uint16_t datain);                                      // write data into the transmit buffer
		short SendTxBuffer();                                                   // trigger sending of the transmit buffer
		void PrintError(int Data);                                              // interpret error codes filled into the read buffer by the C117B
		int AssembleBuffer(std::vector<uint16_t> opcodes_and_values);           // construct a transmit buffer based on function and values
		short AssembleAndTransmit(std::vector<uint16_t> opcodes_and_values);    // combination of assemble and transmit
		uint16_t card_chan_to_word(int card, int channel);                      // combine card and channel into one word & add to the write buffer
		uint16_t setting_to_word(std::string setting, bool value);              // convert a user-friendly setting string to the corresponding opcode & add to buffer
		short ReadFwVer();                                                      // Read Crate firmware version
		int ClearAll();                                                         // clears buffers, LAM enable, halts operations. 3ms delay after sending. Same as C().
		int TestLAM();                                                          // return if LAM raised
		short DisableLAM();                                                     // disable LAM usage (default - gets re-set on Z(), C(), ClearAll(), Powerup...)
		short EnableLAM();                                                      // enable LAM usage

		std::string ConvertInttoAsci(int asci_code);
		int GetLowByte(int number);
		int GetHighByte(int number);
		int CombineBytes(int low, int high);

		int SetVoltage(int slot, int channel, double Vset, int buffer=0);          // set voltage: buffer 0 = set V0, 1 = set V1
		int ReadChannelStatus(int slot, int channel);
		int ReadChannelParameterValues(int slot, int channel);

		int READ(int F, int A, int &Data, int &Q, int &X);	//Generic READ
		int WRITE(int F, int A, int &Data, int &Q, int &X);	//Generic WRITE
		int GetID();						//Return ID of module
		int GetSlot();						//Return n of Slot of module
		void SetConfig(std::string config);			//Set register from configfile

	private:

		int ID;
		int Common;	//just for demonstration purposes for reading from config file

//		int n;
//		int V0;
//		int V1;
//		int I0;
//		int I1;
//		double Vmax;
//		int Rup;
//		int Rdown;
//		int Trip;
//		int Flag;
		
		std::map<std::string,uint16_t> function_to_opcode;

		std::map<std::string, uint16_t> setting_to_mask;

		std::map<std::string, uint16_t> setting_to_flag;

};

#endif
