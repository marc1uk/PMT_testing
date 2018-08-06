/* vim:set noexpandtab tabstop=4 wrap */
// This Main File Is Mostly For Experimentation, It probably shouldn't be in any final program. If it is, something is probably wrong with the program - Max C
#include <unistd.h>
#include "CamacCrate.h"
//#include "Lecroy3377.h"
#include "Lecroy4300b.h"
#include "Jorway85A.h"
#include "libxxusb.h"
#include "usb.h"
#include <ctime>
//#include <thread>
#include <chrono>
#include <fstream>
#include <cstring>
#include <bitset>

unsigned int masks[] = {0x01,       0x02,       0x04,       0x08,
						0x10,       0x20,       0x40,       0x80,
						0x100,      0x200,      0x400,      0x800,
						0x1000,     0x2000,     0x4000,     0x8000,
						0x10000,    0x20000,    0x40000,    0x80000,
						0x100000,   0x200000,   0x400000,   0x800000,
						0x1000000,  0x2000000,  0x4000000,  0x8000000,
						0x10000000, 0x20000000, 0x40000000, 0x80000000};

short SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off);
void PrintReg(CamacCrate* CC,int regnum);
CamacCrate* Create(std::string cardname, std::string config, int cardslot);
// see CCDAQ.cpp in ANNIEDAQ:MRD branch for example on using the CAMAC internal stack
// see CCUSB.cpp in ANNIEDAQ:MRD branch for example on doing Read/Write of LeCroy cards in ToolAnalysis chain

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

int main(int argc, char* argv[]){
	
	std::string configcc = "configfiles/ModuleConfig";         // card list file;
	
	// Declare variables to store the card objects
	//////////////////////////////////////////////
	std::vector<std::string> Lcard, Ccard;
	std::vector<int> Ncard;
	std::map<std::string, std::vector<CamacCrate*> >::iterator iL;  //Iterates over Module.CC
	std::vector<CamacCrate*>::iterator iC;                          //Iterates over Module.CC<vector>
	Module List;                                                    //Data Model for Lecroys
	
	// read the config file specifying the list of cards
	////////////////////////////////////////////////////
	std::ifstream fin (configcc.c_str());
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
	
	std::cout << "Number of cards specified in config file: " << Lcard.size() << std::endl;
	CamacCrate* CC = new CamacCrate;                         // CamacCrate at 0
	
	// Create card objects based on configuration read from file
	////////////////////////////////////////////////////////////
	std::cout << "begin scan over " <<Lcard.size()<< " cards " << std::endl;
	for (int i = 0; i < Lcard.size(); i++){
		if (Lcard.at(i) == "TDC" || Lcard.at(i) == "ADC" || Lcard.at(i) == "SCA")
		{
			std::cout << "Creating " << Lcard.at(i) <<" Object" << std::endl;
			CamacCrate* cardPointer = Create(Lcard.at(i), Ccard.at(i), Ncard.at(i));
			if (cardPointer == NULL){
				std::cerr << "failed to create " << Lcard.at(i) << " object" << std::endl;
				return 0;
			};
			List.CC[Lcard.at(i)].push_back(cardPointer); 
		} else if (Lcard.at(i) == "TRG") {
			trg_pos = List.CC["TDC"].size();
			List.CC["TDC"].push_back(Create("TDC", Ccard.at(i), Ncard.at(i)));
		} else {
			std::cerr << "\n\nUnkown card\n" << std::endl;
			return 0;
		}
	}
	
	// Set up the Wiener CCUSB NIM output 1 to fire on USB trigger
	// used to trigger NIM DGGs for supplying the gate the QDC and fire the LED
	////////////////////////////////////////////////////////////////////////////
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;   // Modify bit 1 of the register to "1" (CCUSB Trigger)
	long RegDeactivated = RegStore & ~0x02;  // Modify bit 1 to 0 (disabled CCUSB trigger.
	
	CC->C();       // clear the crate controller
	CC->Z();       // initilize the crate controller
	usleep(1000);
	
	// Configure CC-USB DGG_A to fire on USB trigger, and output to NIM O1
	// SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off)
	//1. set up Nim out 1 to DGG_A: write register 5 bits 0-2 to 2 (3 for firmware<5 gives no output)
	SetRegBits(CC,5,0,3,false);    // set bits 0-2 on
	SetRegBits(CC,5,1,1,true);     // set bit 1 on
	//3. set DGG_A source to usb: write register 6 bits 16-18 to 6
	SetRegBits(CC,6,16,1,false);  // set bit 16 to off
	SetRegBits(CC,6,17,2,true);   // set bits 17,18 to on
	//5. set DGG_A delay to 0ns: write register 7 bits 0-15 to 0
	SetRegBits(CC,7,0,16,false);
	//6. set DGG_A gate (duration) to maximum (655,350 ns) : write register 7 bits 16-31 to 1        // actually gives 2.5us = 2500ns = 250 of 10ns
	SetRegBits(CC,7,16,16,false); // set bits 16-31 off
	SetRegBits(CC,7,20,1,true);   // set bit 16 on
	//9. set extended delays on DGG_A and DGG_B to to 0: write register 13 bits 0-15 and 16-31 to 0
	SetRegBits(CC,13,0,32,false);
	//9.5 optional: set register 4 bits 8-10=3, 12=0 (invert off), 13=0 (latch off) to flicker green LED on usb trigger
	SetRegBits(CC,4,8,2,true);   // set bits 8-9 on
	SetRegBits(CC,4,10,1,false); // set bit 10 off
	SetRegBits(CC,4,12,2,false); // set bits 12 and 13 off
	usleep(100000);
	
	
	//Open file for saving readouts
	///////////////////////////////
	std::ofstream ADCRead;
	ADCRead.open ("ADCRead.txt");
	
	// Fire the LEDs (or run Test function) and read the Lecroys to measure the pulse integral
	//////////////////////////////////////////////////////////////////////////////////////////
	int numreads=1;
	for (int i = 0; i < numreads; i++){
		// Clear all ADCs
		for (int i = 0; i < List.CC["ADC"].size(); i++)	{ List.CC["ADC"].at(i)->ClearAll(); }
		usleep(300000);
		
		// Fire LED and gate ADCs
		//CC->ActionRegWrite(RegActivated); // enable this to use NIM output 1 to fire LED and gate QDC
		
		// alt: use the Test function to connect internal gate & supply to charge QDCs
		int test_start_success = List.CC["ADC"].at(0)->InitTest();
		std::cout<<"test start success = "<<test_start_success<<std::endl;
		
		// Put timestamp in file
		std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
		time_t tt;
		tt = std::chrono::system_clock::to_time_t ( time );
		std::string timeStamp = ctime(&tt);
		timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);
		ADCRead << timeStamp;
			
		// Read ADC values
		for (int i = 0; i < List.CC["ADC"].size(); i++){
			// Lecroy4300b::ReadOut behaviour depends on config file.
			// If CSR (camac sequential readout) is set to 1, channel input is ignored, 
			// and instead all 16 channels are returned by subsequent 'ReadOut' calls.
			// If CSR = 0, channel input argument is used.
			
			//int ADCData;
			//for(int chani=0; chani<16; chani++){
				//List.CC["ADC"].at(i)->ReadOut(ADCData, chani);
				//ADCRead << ADCData;
				//std::cout << "ADC "<<i<<" channel " << chani << " = " << ADCData << std::endl;
			//}
			
			// alt method: dump all channels into a map
			// Should work with CSR=0 or 1
			std::map<int, int> ADCvals;
			List.CC["ADC"].at(i)->GetData(ADCvals);   //calls DumpAll or DumpCompressed based on CCE
			for(std::map<int,int>::iterator aval = ADCvals.begin(); aval!=ADCvals.end(); aval++){
				ADCRead << ", " << aval->second;
				std::cout<<", " << aval->first << "=" <<aval->second;
				int ADCData;
				std::cout<<"(" << List.CC["ADC"].at(i)->ReadOut(ADCData, 1) << ")";
			}
			ADCRead << std::endl;
			std::cout<<std::endl;
		} // end loop over ADC cards
		ADCRead << std::endl;
		
		usleep(1000000);
	} // end loop over read repeats
	
	// Close file and cleanup
	/////////////////////////
	std::cout<<"closing file"<<std::endl;
	ADCRead.close();
	
	std::cout<<"cleanup"<<std::endl;
	Lcard.clear();
	Ncard.clear();
}

CamacCrate* Create(std::string cardname, std::string config, int cardslot)
{
	CamacCrate* ccp= NULL;
//	if (cardname == "TDC")
//	{
//	  std::cout<<"TDC"<<std::endl;
//		ccp = new Lecroy3377(cardslot, config);
//	}
	//std::cout<<"cardname is "<<cardname<<", comp with 'ADC' is " << (cardname=="ADC") <<std::endl;
	//std::cout<<"strcmp is = "<<strcmp(cardname.c_str(),"ADC") << std::endl;
	if (cardname == "ADC")
	{
		ccp = new Lecroy4300b(cardslot, config);//FIXME 
	}
	else if (cardname == "SCA")
	{
		ccp = new Jorway85A(cardslot, config);
	}
	else
	{
		std::cerr <<"Card type " << cardname <<" not recognized " << cardname << std::endl;
	}
	return ccp;
}

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
			//         <<std::bitset<32>(masks[firstbit+biti]) << "]"<<std::endl;
		} else {
			placeholder &= ~masks[firstbit+biti];
			//std::cout<<"setting bit "<<firstbit+biti<<" to off via and with mask ["
			//         << std::bitset<32>(~masks[firstbit+biti]) << "]"<<std::endl;
		}
		//placeholder & masks[bitnumber]
	}
	//std::cout<<"setting register "<<regnum<<" to ["<<std::bitset<32>(placeholder)<<"]"<<std::endl;
	myret = CC->WRITE(0,regnum,16,placeholder,myQ,myX);
	//std::cout<<"register set returned "<<myret<<std::endl;
	if(myret<0) std::cout<<"OH NO, RETURNED "<<myret<<" WHEN TRYING TO SET REGISTER "<<regnum
	                     <<" bit(s) "<<firstbit<<"-"<<(firstbit+numbits-1)<<" to "<<on_or_off<<std::endl;
	return myret;
};

void PrintReg(CamacCrate* CC,int regnum){
	long placeholder=0;
	int myQ, myX;
	short myret = CC->READ(0,regnum,0,placeholder,myQ, myX);
	std::cout<<"Register "<<regnum<<" is ["<<std::bitset<32>(placeholder)<<"]"<<std::endl;
}
