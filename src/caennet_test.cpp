/* vim:set noexpandtab tabstop=2 wrap */
#include <unistd.h>
#include "CamacCrate.h"
//#include "CAENC117B.h"
#include "my_Caen_C117B.h"
//#include "Lecroy3377.h"
#include "Lecroy4300b.h"
#include "Jorway85A.h"
#include "Lecroy4413.h"
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
short SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off);
void PrintReg(CamacCrate* CC,int regnum);
CamacCrate* Create(std::string cardname, std::string config, int cardslot);
int LoadConfigFile(std::string configfile, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, std::vector<int> &Ncard);
int ConstructCards(Module &List, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, std::vector<int> &Ncard);
int DoCaenTests(Module &List);

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
	Module List;                                             //Data Model for Lecroys
	
	// read module configuration file
	// ==============================
	if(verbosity>message) std::cout<<"Loading CAMAC configuration"<<std::endl;
	std::string configcc = "configfiles/CAENTestConfig"; // card list file
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
	
	// CAEN C117B tests
	// ================
	// if(verbosity>message) std::cout<<"Running CAEN_C117B tests"<<std::endl;
	int caen_test_result = DoCaenTests(List);
	
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

// ***************************************************************************
// CAEN C117B HV CONTROL CARD TESTS
// ***************************************************************************
int DoCaenTests(Module &List){
	std::cout <<"CAENET Controller is in slot ";
	std::cout << List.CC["CAEN"].at(0)->GetSlot() << std::endl;
	
	// Random functionality trials
//	// Michael's Version
//	int ret_caen = List.CC["CAEN"].at(0)->ReadCrateOccupation();
//	
//	int crateN = 1; // crate address? starts from 1
//	List.CC["CAEN"].at(0)->TestOperation(crateN);
//	
//	int ret_vmax = List.CC["CAEN"].at(0)->SetVmax(6,7,1000);
//	List.CC["CAEN"].at(0)->SetV1(6, 0, 100);
//	
//	int readslot;
//	for (int i=0;i<1;i++){
//		readslot= List.CC["CAEN"].at(0)->ReadSlotN(i);
//	}
//	
//	int ret_test;
//	for (int i_test=0; i_test<15; i_test++){
//		ret_test=List.CC["CAEN"].at(0)->TestOperation(i_test);
//	}
	
	// Marcus' Version
	// constructor tries to read FW version
	// XXX crate number is currently hard-coded as 1 in AssembleBuffer XXX
	
	// messages are constructed by making a vector where the first entry is the function, which may be followed by 0, 1 or 2 subsequent value entries.
	// use "function_to_opcode" to convert a user friendly function string to the opcode to add
	// use "setting_to_word"    to convert a user friendly setting to a value word to add
	// use "card_chan_to_word"  to convert a user friendly card and channel pair to a channel code to add
	// see SY527 manual p83 Tab. 21 for data packet format descriptions
	
	// units should be:
	//V0set value Volt / 10Vdec
	//V1set value Volt / 10Vdec
	//I0set value Current Units / 10Idec
	//I1set value Current Units / 10Idec
	//Vmax soft. value Volt
	//Rup value Volt/sec
	//Rdwn value Volt/sec
	//Trip value tenth of second

	
//	std::vector<uint16_t> opcode;
//	opcode.push_back(function_to_opcode.at("read_fw_ver"));
//	short ret = AssembleAndTransmit(opcode);
	std::cout<<"Reading FW ver"<<std::endl;
	CAENC117B* caencard = (CAENC117B*)List.CC["CAEN"].at(0);
	caencard->ReadFwVer();
	std::cout<<std::endl;
	
	std::cout<<"Setting slot 0 channel 10 V0 to 1234.5V"<<std::endl;
	caencard->SetVoltage(0, 10, 1234.5);
	
	std::cout<<"Reading slot 0 channel 10 status"<<std::endl;
	caencard->ReadChannelStatus(0, 10);
	std::cout<<"Reading slot 0 channel 10 parameters"<<std::endl;
	caencard->ReadChannelParameterValues(0, 10);
	
	//int ret_lam = caencard->EnLAM();
	//std::cout << caencard->TestLAM()<<std::endl;
	
	return 1;
}

// ***************************************************************************
// LOAD CRATE CONFIGURATION
// ***************************************************************************
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

// ***************************************************************************
// CONSTRUCT CARDS FROM CONFIGURATION
// ***************************************************************************
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

// ***************************************************************************
// CONSTRUCT A CARD
// ***************************************************************************
CamacCrate* Create(std::string cardname, std::string config, int cardslot){
	CamacCrate* ccp= NULL;
//	if (cardname == "TDC")
//	{
//		std::cout<<"TDC"<<std::endl;
//		ccp = new Lecroy3377(cardslot, config);
//	}
	//std::cout<<"cardname is "<<cardname<<", comp with 'ADC' is " << (cardname=="ADC") <<std::endl;
	//std::cout<<"strcmp is = "<<strcmp(cardname.c_str(),"ADC") << std::endl;
	if (cardname == "ADC")
	{
		//std::cout<<"ADC"<<std::endl;
		ccp = new Lecroy4300b(cardslot, config);
	}
	else if (cardname == "SCA")
	{
		//std::cout<<"SCA"<<std::endl;
		ccp = new Jorway85A(cardslot, config);
	}
	else if (cardname == "DISC")
	{
		//std::cout<<"DISC"<<std::endl;
		ccp = new LeCroy4413(cardslot, config);
	}
	else if (cardname == "CAEN")
	{
		//std::cout<<"CAEN"<<std::endl;
		ccp = new CAENC117B(cardslot, config);
	}
	else
	{
		std::cerr <<"Card type not specified " << cardname << std::endl;
	}
	return ccp;
}

// ***************************************************************************
// CONFIGURE WEINER REGISTER
// ***************************************************************************
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

// ***************************************************************************
// PRINT WEINER REGISTER
// ***************************************************************************
void PrintReg(CamacCrate* CC,int regnum){
	long placeholder=0;
	int myQ, myX;
	short myret = CC->READ(0,regnum,0,placeholder,myQ, myX);
	std::cout<<"Register "<<regnum<<" is ["<<std::bitset<32>(placeholder)<<"]"<<std::endl;
}

