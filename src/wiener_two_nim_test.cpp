/* vim:set noexpandtab tabstop=2 wrap */
#include <unistd.h>
#include "CamacCrate.h"
//#include "CAENC117B.h"
//#include "my_Caen_C117B.h"
//#include "Lecroy3377.h"
//#include "Lecroy4300b.h"
//#include "Jorway85A.h"
//#include "Lecroy4413.h"
#include "libxxusb.h"
#include "usb.h"
#include <string>
#include <ctime>
#include <thread>
#include <future>
#include <chrono>
#include <fstream>
#include <iostream>
#include <cstring>
#include <bitset>
#include <algorithm>
#include <numeric>

// CUSTOM STRUCTS
struct Channel          //Bunch of Channel makes Card
{
	std::map<int, int> ch;
};
struct Card             //Array of Card make Module
{
	std::vector<Channel> Num;
	std::vector<int> Slot;
};
struct Module           // One Struct to rule them all, One Struct to find them, One Struct to
                        // bring them all and in the darkness bind them
{
	std::map<std::string, Card> Data;                          //Data
	std::map<std::string, std::vector<CamacCrate*> > CC;       //Camac module class
};

// SUPPORT FUNCTIONS
int LoadConfigFile(std::string configfile, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, std::vector<int> &Ncard);
short SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off);
void PrintReg(CamacCrate* CC,int regnum);
CamacCrate* Create(std::string cardname, std::string config, int cardslot);
int ConstructCards(Module &List, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, std::vector<int> &Ncard);
int SetupWeinerSoftTrigger(CamacCrate* CC, Module &List);

// CONSTANTS
const unsigned int masks[] = {0x01, 0x02, 0x04, 0x08, 
												0x10, 0x20, 0x40, 0x80, 
												0x100, 0x200, 0x400, 0x800, 
												0x1000, 0x2000, 0x4000, 0x8000, 
												0x10000, 0x20000, 0x40000, 0x80000, 
												0x100000, 0x200000, 0x400000, 0x800000, 
												0x1000000, 0x2000000, 0x4000000, 0x8000000,
												0x10000000, 0x20000000, 0x40000000, 0x80000000};

// verbosity levels
const int verbosity=2; // must be > a level to print that level
const int message=1;
const int debug=2;

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

// ***************************************************************************
//                             START MAIN PROGRAM
// ***************************************************************************

int main(int argc, char* argv[]){
	
	// Variables for holding the CamacCrate and derived class objects
	// ==============================================================
	std::vector<std::string> Lcard, Ccard;
	std::vector<int> Ncard;
	Module List;                                                   //Data Model for Lecroys
	
	// read module configuration file
	// ==============================
	if(verbosity>message) std::cout<<"Loading CAMAC configuration"<<std::endl;
	std::string configcc = "configfiles/WienerTestConfig"; // card list file
	int configok = LoadConfigFile(configcc, Lcard, Ccard, Ncard);
	if(not configok){
		std::cerr << "Failed to load card configuration from file " << configcc << std::endl;
		return 0;
	}
	CamacCrate* CC = new CamacCrate;                         // CamacCrate at 0
	
	// Create card objects based on configuration read from file
	// =========================================================
	if(verbosity>message) std::cout<<"Constructing Cards"<<std::endl;
	int constructok = ConstructCards(List, Lcard, Ccard, Ncard);
	if(not constructok){
		std::cerr << "Error during card object construction" << std::endl;
		return 0;
	}
	
	// Set up CCUSB NIM output for triggering
	// ======================================
	if(verbosity>message) std::cout<<"Setting up Weiner USB trigger"<<std::endl;
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;   // Modify bit 1 of the register to "1" (CCUSB Trigger)
	//int weiner_softrigger_setup_ok = SetupWeinerSoftTrigger(CC, List);  // this sets up NIM O1 to fire on ActionRegWrite
	
	// =========
	// MAIN LOOP
	// =========
	if(verbosity>message) std::cout<<"Beginning Main loop"<<std::endl;
	int command_ok;
	int numrepeats=1;
	int ARegDat;
	long ARegRead;
	for (int i = 0; i < numrepeats; i++)
	{
		// NOTE: THIS HAS BEEN TESTED AND CONSISTENTLY WORKS.
		std::cout<<"Enabling Nim out 1 on DGG_0, and Disabling NIM out 2 on DGG_1"<<std::endl;
		//            DelayGateGen(DGG, trig_src, NIM_out, delay_ns, gate_ns, NIM_invert, NIM_latch);
		ARegDat = CC->DelayGateGen(0,       6,       1,       1,       25,        0,          0); // set DGG 0 on output 1 to USB trigger
		if(ARegDat<0) std::cerr<<"failed to enable USB trigger on DGG_0 output 0!" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1)); // after setting this NIM O1 is ENABLED
		ARegDat = CC->DelayGateGen(1,       0,       2,       1,       25,        0,          0); // set DGG 1 on output 2 to disabled
		if(ARegDat<0) std::cerr<<"failed to disable USB trigger on DGG_1 output 1!" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1)); // after setting this, NIM O2 is DISABLED
		
		std::cout<<"firing NIM out 1 three times"<<std::endl;
		for(int firei=0; firei<3; firei++){
			std::cout<<"FIRE!"<<std::endl;
			int command_ok = CC->ActionRegWrite(RegActivated);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		std::cout<<"done, sleeping for 5 seconds"<<std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(5));
		
		std::cout<<"Disabling Nim out 1 on DGG_0, and Enabling NIM out 2 on DGG_1"<<std::endl;
		ARegDat = CC->DelayGateGen(0,       0,       1,       1,       25,        0,          0); // set DGG 0 on output 1 to disabled
		if(ARegDat<0) std::cerr<<"failed to disable USB trigger on DGG_0 output 0!" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1)); // after setting this NIM O1 is DISABLED
		ARegDat = CC->DelayGateGen(1,       6,       2,       1,       25,        0,          0); // set DGG 1 on output 2 to USB trigger
		if(ARegDat<0) std::cerr<<"failed to enable USB trigger on DGG_1 output 1!" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1)); // after setting this NIM O2 is ENABLED
		
		std::cout<<"firing NIM out 1 three times"<<std::endl;
		for(int firei=0; firei<3; firei++){
			std::cout<<"FIRE!"<<std::endl;
			int command_ok = CC->ActionRegWrite(RegActivated);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		
	} // loop over repeats
	
	std::cout<<"End of data taking"<<std::endl;
	
	// Clean up CAMAC stuff
	// ====================
	if(verbosity>message) std::cout<<"Cleaning up CAMAC"<<std::endl;
	Lcard.clear();
	Ncard.clear();
	
	std::cout<<"Tests complete. Goodbye."<<std::endl;
}

// ***************************************************************************
//                             END MAIN PROGRAM
// ***************************************************************************

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

// ***************************************************************************
//               PULSE HEIGHT DISTRIBUTION TEST - Digitizer Ver
// ***************************************************************************

int MeausrePulseHeightDistributionDigitizer(CamacCrate* CC){
	
	// we need to fire the LED (which will also trigger acquisition)
	// Get register settings with soft-trigger bit (un)set
	// ===================================================
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;   // Modify bit 1 of the register to "1" (CCUSB Trigger)
	
	int numacquisitions=5;
	std::cout<<"taking "<<numacquisitions<<" acquisitions"<<std::endl;
	for(int triggeri=0; triggeri<numacquisitions; triggeri++){
		// Fire LED! (and gate ADCs)
		// =========================
		int command_ok = CC->ActionRegWrite(RegActivated);
		//std::cout<<"Firing LED " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		usleep(1000);
	}
	
	return 1;
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

// ***************************************************************************
//                             SUPPORT FUNCTIONS
// ***************************************************************************

// ***************************************
// SETUP WEINER NIM OUT
// ***************************************
int SetupWeinerSoftTrigger(CamacCrate* CC, Module &List){
	
	// Intialize the card
	//CC->Z(); 
	// XXX CC->Z() CALLS Z() (initialize) ON ALL CARDS AS WELL. XXX
	// THIS WILL RESET REGISTER SETTINGS, REQUIRING RECONFIGURATION!
	
	// Clear the card
	CC->C();
	usleep(100);
	
	// Configure CC-USB DGG_A to fire on USB trigger, and output to NIM O1
	//SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off)
	//1. set up Nim out 1 to DGG_A: write register 5 bits 0-2 to 2 (3 for firmware<5 gives no output)
	SetRegBits(CC,5,0,3,false);    // set bits 0-2 on
	SetRegBits(CC,5,1,1,true);     // set bit 1 on
	//2. set DGG_A source to usb: write register 6 bits 16-18 to 6
	SetRegBits(CC,6,16,1,false);  // set bit 16 to off
	SetRegBits(CC,6,17,2,true);   // set bits 17,18 to on
	//3. set DGG_A delay to 0ns: write register 7 bits 0-15 to 0
	SetRegBits(CC,7,0,16,false);
	//4. set DGG_A gate (duration) to maximum (655,350 ns) : write register 7 bits 16-31 to 1        // actually gives 2.5us = 2500ns = 250 of 10ns
	SetRegBits(CC,7,16,16,false); // set bits 16-31 off
	SetRegBits(CC,7,20,1,true);   // set bit 16 on
	//5. set extended delays on DGG_A and DGG_B to to 0: write register 13 bits 0-15 and 16-31 to 0
	SetRegBits(CC,13,0,32,false);
	//5.5 optional: set register 4 bits 8-10=3, 12=0 (invert off), 13=0 (latch off) to flicker green LED on usb trigger
	SetRegBits(CC,4,8,2,true);   // set bits 8-9 on
	SetRegBits(CC,4,10,1,false); // set bit 10 off
	SetRegBits(CC,4,12,2,false); // set bits 12 and 13 off
	usleep(1000);
	
	//10. FIRE TORPEDOS: write action register: just to test
	//for(int fires=0; fires<3; fires++){
	//	CC->ActionRegWrite(RegActivated);
	//	usleep(1000000);
	//}
	// Fire the LEDs and read the Lecroys to measure the pulse integral
	
	return 1;// todo error checking
}

// ***************************************
// CONSTRUCT A CARD
// ***************************************
CamacCrate* Create(std::string cardname, std::string config, int cardslot){
	CamacCrate* ccp= NULL;
//	if (cardname == "TDC")
//	{
//		std::cout<<"TDC"<<std::endl;
//		ccp = new Lecroy3377(cardslot, config);
//	}
	//std::cout<<"cardname is "<<cardname<<", comp with 'ADC' is " << (cardname=="ADC") <<std::endl;
	//std::cout<<"strcmp is = "<<strcmp(cardname.c_str(),"ADC") << std::endl;
//	if (cardname == "ADC")
//	{
//		//std::cout<<"ADC"<<std::endl;
//		ccp = new Lecroy4300b(cardslot, config);
//	}
//	else if (cardname == "SCA")
//	{
//		//std::cout<<"SCA"<<std::endl;
//		ccp = new Jorway85A(cardslot, config);
//	}
//	else if (cardname == "DISC")
//	{
//		//std::cout<<"DISC"<<std::endl;
//		ccp = new LeCroy4413(cardslot, config);
//	}
//	else if (cardname == "CAEN")
//	{
//		//std::cout<<"CAEN"<<std::endl;
//		ccp = new CAENC117B(cardslot, config);
//	}
//	else
	{
		std::cerr <<"Card type not specified " << cardname << std::endl;
	}
	return ccp;
}

// ***************************************
// CONFIGURE WEINER REGISTER
// ***************************************
short SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off){
	//std::cout<<"setting register "<<regnum<<" bit(s) "<<firstbit;
	//if(numbits>1) std::cout<<"-"<<(firstbit+numbits-1);
	//std::cout<<" to "<<on_or_off<<std::endl;
	
	long placeholder=0;
	int myQ, myX;
	short myret = CC->READ(0,regnum,0,placeholder,myQ, myX);
	for(int biti=0; biti<numbits; biti++){
		if(on_or_off){
			placeholder |= masks[firstbit+biti];
			//std::cout<<"setting bit "<<firstbit+biti<<" to on via or with mask [" 
			//				 << std::bitset<32>(masks[firstbit+biti]) << "]"<<std::endl;
		} else {
			placeholder &= ~masks[firstbit+biti];
			//std::cout<<"setting bit "<<firstbit+biti<<" to off via and with mask [" 
			//				 << std::bitset<32>(~masks[firstbit+biti]) << "]"<<std::endl;
		}
		//placeholder & masks[bitnumber]
	}
	//std::cout<<"setting register "<<regnum<<" to ["<<std::bitset<32>(placeholder)<<"]"<<std::endl;
	myret = CC->WRITE(0,regnum,16,placeholder,myQ,myX);
	//std::cout<<"register set returned "<<myret<<std::endl;
	if(myret<0) 
		std::cout<<"OH NO, RETURNED "<<myret<<" WHEN TRYING TO SET REGISTER "
						 <<regnum<<" bit(s) "<<firstbit<<"-"<<(firstbit+numbits-1)<<" to "<<on_or_off<<std::endl;
	return myret;
};

// ***************************************
// PRINT WEINER REGISTER
// ***************************************
void PrintReg(CamacCrate* CC,int regnum){
	long placeholder=0;
	int myQ, myX;
	short myret = CC->READ(0,regnum,0,placeholder,myQ, myX);
	std::cout<<"Register "<<regnum<<" is ["<<std::bitset<32>(placeholder)<<"]"<<std::endl;
}

// ***************************************
// LOAD CRATE CONFIGURATION
// ***************************************
int LoadConfigFile(std::string configfile, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, std::vector<int> &Ncard){
	
	// read module configurations
	std::ifstream fin (configfile.c_str());  // TODO return error if not found
	std::string Line;
	std::stringstream ssL;
	std::string sEmp;
	int iEmp;
	
	int c = 0;
	while (getline(fin, Line))
	{
		if (Line[0] == '#') continue;
		else
		{
			ssL.str("");
			ssL.clear();
			ssL << Line;
			if (ssL.str() != "")
			{
				ssL >> sEmp;
				Lcard.push_back(sEmp);		//Modele L
				ssL >> iEmp;
				Ncard.push_back(iEmp);		//Slot N
				ssL >> sEmp;
				Ccard.push_back(sEmp);		//Slot register file
			}
		}
	}
	fin.close();
	
	return 1;
}

// ***************************************
// CONSTRUCT CARDS FROM CONFIGURATION
// ***************************************
int ConstructCards(Module &List, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, std::vector<int> &Ncard){
	// construct the card objects
	std::cout << "begin scan over " <<Lcard.size()<< " cards " << std::endl;
	for (int i = 0; i < Lcard.size(); i++)
	{
		if (Lcard.at(i) == "TDC" || Lcard.at(i) == "ADC")
		{
			std::cout << "Creating CamacCrate"<< std::endl;
			CamacCrate* cardPointer = Create(Lcard.at(i), Ccard.at(i), Ncard.at(i));
			//std::cout << "Successfully Created CamacCrate Object." << std::endl;
			if (cardPointer == NULL)
			{
				std::cerr << "unknown card type " << Lcard.at(i) << std::endl;
				return 0;
			} 
			else
			{
				//It falls over somewhere around here
				List.CC[Lcard.at(i)].push_back(cardPointer);  //They use CC at 0
				std::cout << "constructed Lecroy 4300b module" << std::endl;
			}
		}
		else if (Lcard.at(i) == "SCA")
		{
			List.CC["SCA"].push_back(Create(Lcard.at(i), Ccard.at(i), Ncard.at(i)));               //They use CC at 0
			std::cout << "constructed Jorway85A module" << std::endl;
		}
		else if (Lcard.at(i) == "DISC")
		{
			List.CC["DISC"].push_back(Create(Lcard.at(i), Ccard.at(i), Ncard.at(i)));               //They use CC at 0
			std::cout << "constructed Lecroy4413 module" << std::endl;
		}
		else if (Lcard.at(i) == "CAEN")
		{
			List.CC["CAEN"].push_back(Create("CAEN",Ccard.at(i), Ncard.at(i)));
			std::cout << "constructed CAENC117B controller module" <<std::endl;
		}
		else std::cout << "\n\nUnkown card\n" << std::endl;
	}
	
	//std::cout << "Primary scaler is in slot ";
	//std::cout << List.CC["SCA"].at(0)->GetSlot() << std::endl;
	
	return 1;
}

