/*
 * CC-USB CAMAC controller module and Crate initialiser class 
 *
 * Author: Tommaso Boschi
 */


#ifndef CamacCrate_H
#define CamacCrate_H

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <bitset>

#include "libxxusb.h"

class CamacCrate
{
	public:

		//CAMACCRATE
		CamacCrate(int i = 0);
		virtual ~CamacCrate();
		int READ(int ID, int F, int A, long &Data, int &Q, int &X);
		int WRITE(int ID, int F, int A, long &Data, int &Q, int &X);
		void LoadStack(std::string fname);		//Load stack from file
		void EncStack(std::string fname);		//Encode stack from file
		int PushStack();				//Push stack to CC USB
		int PullStack();				//Pull stack from CC USB
		void PrintStack();				//Print stack
		int StartStack();				//Start Acquisition Mode
		int StopStack();				//Stop Acquisition Mode
		int SetLAMmask(std::string &Mask);		//Set LAM mask as a binary string
		int SetLAMmask(long &Mask);			//Set LAM mask as an integer
		int GlobalRegister(std::string &Reg);		//Write the CC global register as a string
		int GlobalRegister(long &Reg);			//Write the CC global register as an int
		int ReadFIFO(std::vector<int> &vData);		//Read USB fifo
		int ClearFIFO();
		int GetSlot(int ID);
		void ListSlot();

		int Z();		//Z
		int C();		//C
		int I(bool inh);	//I

		//LECROY3377
		virtual int ReadFIFOall(std::map<int, int> &vData) { return 0; }
		virtual int ReadFIFO() { return 0; }
		virtual int ExFIFOOut() { return 0; }
		virtual int ReadReg(int R) { return 0; }
		virtual int ReadTestReg() { return 0; }
		virtual int WriteFIFOData() { return 0; }
		virtual int WriteFIFOtag() { return 0; }
		virtual int WriteReg(int R, int *Data) { return 0; }
		virtual int DisLAM() { return 0; }
		virtual int DisAcq() { return 0; }
		virtual int EnLAM() { return 0; }
		virtual int EnAcq() { return 0; }
		virtual int TestBuff() { return 0; }
		virtual int TestBusy() { return 0; }
		virtual int TestEvent() { return 0; }
		virtual int TestFIFO() { return 0; }
		virtual int CommonStop() { return 0; }
		virtual int CommonStart() { return 0; }
//		virtual void StartTestReg() { ; }
//		virtual void StopTestReg() { ; }

		//LECROY4300B
		virtual int ReadReg(int &Data)	{ return 0; }
		virtual int ReadPed(int Ch, int &Data) { return 0; }
		virtual int ReadOut(int &Data, int) { return 0; }
		virtual int WriteReg(int &Data) { return 0; }
		virtual int WritePed(int Ch, int &Data) { return 0; }
		virtual int DumpCompressed(std::map<int, int> &mData) { return 0; }
		virtual int DumpAll(std::map<int, int> &mData) { return 0; }
		virtual int GetPedestal() { return 0; }					//Get Pedestal from card
		virtual int SetPedestal() { return 0; }					//Set Pedestal to card
		virtual void LoadPedestal(std::string fname) { ; }
		virtual void PrintPedestal() { ; }

		//JORWAY85A
		virtual int ReadScaler(int scalernum) { return 0; }
		virtual int ClearScaler(int scalernum) { return 0; }
		virtual int TestChannel(int scalernum) { return 0; }
		virtual int ReadAndClear(int scalernum) { return 0; }
		virtual int ReadAll(int *Data) { return 0; }

		//LECROY4413
		virtual int ReadThreshold(int& threshold) { return 0; }
		virtual int ReadThreshold() { return 0; }
		virtual int WriteThresholdValue(int thresholdin) { return 0; }
		virtual int EnableFPthreshold() { return 0; }
		virtual int EnableProgrammedThreshold() { return 0; }
		virtual int ReadChannelMask(int& channelmaskin) { return 0; }
		virtual int ReadChannelMask() { return 0; }
		virtual int WriteChannelMask(int& channelmaskin) { return 0; }
		//virtual int TestInit() { return 0; }
		virtual int SetRemoteMode(int modein) { return 0; }
		virtual int GetRemoteMode() { return 0; }

		//CAENETC117B
		virtual int CommunicationSequence(int controller, int CrateNumber, int operation, std::vector<int> &values, std::vector<int> &response) {return 0;}
		virtual int ReadSlotN(int n) {return 0;}
		virtual int TestOperation(int crateN) {return 0;}
		virtual int ReadCrateOccupation() {return 0;}
		virtual int ReadChannelStatus(int slot, int channel) {return 0;}
		virtual int ReadChannelParameterValues(int slot, int channel) {return 0;}
		virtual int SetV0(int slot, int channel, int V0) {return 0;}
		virtual int SetV1(int slot, int channel, int V1) {return 0;}
		virtual int SetI0(int slot, int channel, int I0) {return 0;}
		virtual int SetI1(int slot, int channel, int I1) {return 0;}
		virtual int SetVmax(int slot, int channel, int Vmax) {return 0;}
		virtual int SetRampUp(int slot, int channel, int Rup) {return 0;}
		virtual int SetRampDown(int slot, int channel, int Rdown) {return 0;}
		virtual int SetTrip(int slot, int channel, int Trip) {return 0;}
		virtual int SetFlag(int slot, int channel, int Flag) {return 0;}
		virtual int GetHighByte(int large_number) {return 0;}
		virtual int GetLowByte(int large_number) {return 0;}
		virtual std::string ConvertInttoAsci(int asci_code) {return 0;}
		virtual int CombineBytes(int low, int high) {return 0;}
		virtual unsigned int Combine4Bytes(int byte1, int byte2, int byte3, int byte4) {return 0;}

		//COMMON
		virtual int GetData(std::map<int, int> &mData) { return 0; }
		virtual int ClearAll() { return 0; }
		virtual int ClearLAM() { return 0; }
		virtual int TestLAM() { return 0; }
		virtual int InitTest() { return 0; }
		virtual void ParseCompData(int Word, int &Stat, int &Num, bool &B0) { ; }
		virtual void DecRegister() { ; }
		virtual void EncRegister() { ; }
		virtual void GetRegister() { ; }
		virtual void SetRegister() { ; }
		virtual void PrintRegister() { ; }
		virtual void PrintRegRaw() { ; }
		virtual void SetConfig(std::string config) { ; }
		virtual int READ(int F, int A, int &Data, int &Q, int &X) { return 0; }//F(x)·A(y) 
		virtual int WRITE(int F, int A, int &Data, int &Q, int &X) { return 0; }//F(x)·A(y)
		virtual int GetSlot() { return 0; }
		virtual int GetID() { return 0; }

	protected:

		int BittoInt(std::bitset<16> &bitref, int a, int b);
		static std::vector<int> Slot;

	private:

		void USBFind();
		void USBOpen(int i);
		void USBClose();
		void Init(int i);

		std::vector<int> vStack;
		static bool IsOpen;
		static int ndev;
		static xxusb_device_type devices[100];
		static struct usb_device *Mdev;
		static usb_dev_handle *Mudev;

};

#endif

