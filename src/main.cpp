/*
This Main File Is Mostly For Experimentation, It probably shouldn't be in any final program. If it is, 
something is probably wrong with the program - Max C
*/
/* vim:set noexpandtab tabstop=4 wrap */
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

unsigned int masks[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 
			0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000, 0x1000000, 0x2000000, 0x4000000, 0x8000000,
			0x10000000, 0x20000000, 0x40000000, 0x80000000};
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
			//std::cout<<"setting bit "<<firstbit+biti<<" to on via or with mask [" << std::bitset<32>(masks[firstbit+biti]) << "]"<<std::endl;
		} else {
			placeholder &= ~masks[firstbit+biti];
			//std::cout<<"setting bit "<<firstbit+biti<<" to off via and with mask [" << std::bitset<32>(~masks[firstbit+biti]) << "]"<<std::endl;
		}
		//placeholder & masks[bitnumber]
	}
	//std::cout<<"setting register "<<regnum<<" to ["<<std::bitset<32>(placeholder)<<"]"<<std::endl;
	myret = CC->WRITE(0,regnum,16,placeholder,myQ,myX);
	//std::cout<<"register set returned "<<myret<<std::endl;
	if(myret<0) std::cout<<"OH NO, RETURNED "<<myret<<" WHEN TRYING TO SET REGISTER "<<regnum<<" bit(s) "<<firstbit<<"-"<<(firstbit+numbits-1)<<" to "<<on_or_off<<std::endl;
	return myret;
};
void PrintReg(CamacCrate* CC,int regnum){
	long placeholder=0;
	int myQ, myX;
	short myret = CC->READ(0,regnum,0,placeholder,myQ, myX);
	std::cout<<"Register "<<regnum<<" is ["<<std::bitset<32>(placeholder)<<"]"<<std::endl;
}

CamacCrate* Create(std::string cardname, std::string config, int cardslot);
// see CCDAQ.cpp in ANNIEDAQ:MRD branch for example on using the CAMAC internal stack
// see CCUSB.cpp in ANNIEDAQ:MRD branch for example on doing Read/Write of LeCroy cards in ToolAnalysis chain

////////////////////////////////////
// master branch: MRDData.h
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
	///////////////////////////////////////
	// master branch: CCTrigger.h
	


	////////////Scrambled Egg Part 1 Begins////////////
	//Open up a new file called data.txt
	/*std::ofstream data;
	data.open ("data.txt");
	//Retrieve the desired number of times for the scaler to be read out
	int reps;
	std::cout << "Enter number of scaler readouts\n";
	std::cin >> reps;
	std::cout << "\nThe scalers will be read " << reps << " times\n";
	//Retrieve time (in minutes) between readouts (executed in a for loop)
	
	int timeBet;
	std::cout << "Enter the time (in minutes) between readouts: ";
	std::cin >> timeBet;
	std::cout << "There will be " << timeBet << " minutes between readouts.\n";
	
	//Initialise a counter for a loop
	int count = 0;*/
	
	////////////End of Scrambled Egg part 1////////////	


	std::string configcc;           // Module slots
	
	std::vector<std::string> Lcard, Ccard;
	std::vector<int> Ncard;
	
	std::map<std::string, std::vector<CamacCrate*> >::iterator iL;  //Iterates over Module.CC
	std::vector<CamacCrate*>::iterator iC;                          //Iterates over Module.CC<vector>
	
	////////////////////////////////////
	// master branch: MRDData.h
	Module List;                                                   //Data Model for Lecroys
	bool TRG;                                                      //Trigger?      Activate other tools
	
	//////////////////////////////////////
	// MRD branch: Lecroy.h
	Channel Data;
	std::string DC;                                                //Module slots DC is TDC or ADC
	
	//////////////////////////////////////
	// Added by me, for using.
	int scalervals[4];                                             //Read scaler values into this
	
	///////////////////////////////////////
	// master branch: CCTrigger.cpp - Initialise();
	// read module configurations
	configcc = "configfiles/ModuleConfig"; // card list file
	
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
	
	std::cout << "Number of cards: " << Lcard.size() << std::endl;
	CamacCrate* CC = new CamacCrate;                         // CamacCrate at 0
	
	//////////////////////////////////////
	// Create card object based on configuration read from file
	
	int trg_pos = 0;                                            // position of trigger card in list of cards
	int scaler_pos = -1;
	int adc_pos = -2;                                           // position of scaler in list of cards
	std::cout << "begin scan over " <<Lcard.size()<< " cards " << std::endl;
	for (int i = 0; i < Lcard.size(); i++)
	{
		if (Lcard.at(i) == "TDC" || Lcard.at(i) == "ADC")
		{
			std::cout << "Creating CamacCrate Object" << std::endl;
			CamacCrate* cardPointer = Create(Lcard.at(i), Ccard.at(i), Ncard.at(i));
			std::cout << "Successfully Created CamacCrate Object." << std::endl;
			if (cardPointer == NULL)
			{
				std::cerr << "unknown card type " << Lcard.at(i) << std::endl;
				return 0;
			} 
			else
			{
				//It falls over somewhere around here
				adc_pos = List.CC["TDC"].size();
				std::cout << "ADC found, list pos = " << adc_pos << std::endl;
				List.CC[Lcard.at(i)].push_back(cardPointer);  //They use CC at 0
				std::cout << "constructed Lecroy 4300b module" << std::endl;
			}
			
		}
		else if (Lcard.at(i) == "TRG")
		{
			trg_pos = List.CC["TDC"].size();
			List.CC["TDC"].push_back(Create("TDC", Ccard.at(i), Ncard.at(i)));              //They use CC at 0
		}
		else if (Lcard.at(i) == "SCA")
		{
			scaler_pos = List.CC["SCA"].size();    // we only have one scaler anyway
			std::cout << "scaler found, list pos = " << scaler_pos << std::endl;
			List.CC["SCA"].push_back(Create("SCA", Ccard.at(i), Ncard.at(i)));               //They use CC at 0
			std::cout << "constructed Jorway85A module" << std::endl;
		}
		else std::cout << "\n\nUnkown card\n" << std::endl;
	}
	
	//std::cout << "Primary scaler is in slot ";
	//std::cout << List.CC["SCA"].at(scaler_pos)->GetSlot() << std::endl;
	
	//////////////////////////////////////
	// MRD branch: LeCroy.cpp - Initialise();
	//std::cout << "Clearing modules and printing the registers" << std::endl;
	
	//Open File for ADC readouts
	std::ofstream ADCRead;
	ADCRead.open ("ADCRead.txt");
	
	int repeats=1;
	std::cout << "Please enter number of writes to the action register: ";
	std::cin >> repeats;
	std::cout << std::endl << "There will be " << repeats << " writes\n";
	
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;  // Modify bit 1 of the register to "1" (CCUSB Trigger)
	long RegDeactivated = RegStore & ~0x02;  // Modify bit 1 to 0 (disabled CCUSB trigger. (De-assert of the USB trigger is actually not needed)
	
	CC->C();
	CC->Z();
	usleep(1000);
	

	
	// Configure CC-USB DGG_A to fire on USB trigger, and output to NIM O1
	//SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off)
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
	usleep(1000);
	
	//10. FIRE TORPEDOS: write action register: just to test
	//for(int fires=0; fires<3; fires++){
	//	CC->ActionRegWrite(RegActivated);
	//	usleep(1000000);
	//}
	List.CC["ADC"].at(0)->GetPedestal();
	// Fire the LEDs and read the Lecroys to measure the pulse integral
	int ARegDat;
	long ARegRead;
	for (int i = 0; i < repeats; i++)
	{
		//break;
		// Clear all ADCs
		std::cout<<"clearing ADCs"<<std::endl;
		for (int i = 0; i < List.CC["ADC"].size(); i++)	{ List.CC["ADC"].at(i)->ClearAll(); }
		
		// Fire LED! (and gate ADCs)
		std::cout<<"Firing LEDs"<<std::endl;
		CC->ActionRegWrite(RegActivated);
		
		// Put timestamp in file
		std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
		time_t tt;
		tt = std::chrono::system_clock::to_time_t ( time );
		std::string timeStamp = ctime(&tt);
		timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);
		ADCRead << timeStamp;
		
		std::cout<<"Appending to file: " << timeStamp;
			
		// Read ADC value
		for (int i = 0; i < List.CC["ADC"].size(); i++){
		
			// ReadOut behaviour depends on config file. If CSR (camac sequential readout) is set to 1, 
			// channel input is ignored, all 16 channels are returned by subsequent 'ReadOut' calls.
			// If CSR = 0, channel input is used. 
			//int ADCData;
			//ADCRead << List.CC["ADC"].at(i)->ReadOut(ADCData, 1);
			//std::cout << "Your Data: " << ADCData << std::endl;
			
			// alt method: dump all channels into a map - simply loops over ReadAll calls. Should work with CSR=0 or 1
			std::map<int, int> ADCvals;
			List.CC["ADC"].at(i)->DumpAll(ADCvals);
			for( std::map<int,int>::iterator aval = ADCvals.begin(); aval!=ADCvals.end(); aval++){
				ADCRead << ", ";
				ADCRead << aval->second;
				std::cout<<", " << aval->first << "=" <<aval->second;
				int ADCData;
				std::cout<<"(" << List.CC["ADC"].at(i)->ReadOut(ADCData, 1) << ")";
			}
			ADCRead << std::endl;
			std::cout<<std::endl;
		}
		ADCRead << std::endl;
		
		usleep(1000000);
	}
	std::cout<<"closing file"<<std::endl;
	ADCRead.close();	
	
	/*
	// Execute: MAIN LOOP
	////////////scrambled egg code part 2////////////
	for(i=0; i < reps; i++)
	{
		std::cout << "Clearing All Scalars" << std::endl;

		for (int i = 0; i < List.CC["SCA"].size(); i++)
		{
			List.CC["SCA"].at(i)->ClearAll();
		}
		std::cout << "Done" << std::endl;
		
		//waiting for one minute
		std::cout << "Counting" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(60));
		//Reading the scalars
		int count2 = timeBet;
		std::cout << "Reading Scalers and writing to file" << std::endl;
		int ret = List.CC["SCA"].at(scaler_pos)->ReadAll(scalervals);

		if(not ret < 0){   // need better error checking
			std::cout << "error reading scalers: response was " << ret << std::endl;
		} 
		else {
			//Need to insert some sort of timestamp here
			//data << "scaler values were: ";
			//Get timestamp
			
			std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
			time_t tt;
			tt = std::chrono::system_clock::to_time_t ( time );
			std::string timeStamp = ctime(&tt);
			timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);
			data << timeStamp << ", ";
			
			for(int chan=0; chan<4; chan++){
				double darkRate = double (scalervals[chan]) / 60.;
				data << darkRate;
				if(chan<3) data << ", ";
			}
			data << std::endl;
		}
		std::cout << "Done" << std::endl;
		//data << "End of Loop " << count << std::endl;
		//For loop will now wait for user-specified time
		for(count2; count2 > 0; count2--)
		{
			std::cout << "Waiting\n";
			std::this_thread::sleep_for (std::chrono::seconds(1));
		}
	}
	//Test
	//std::cout << "Test Channel: " << List.CC["SCA"].at(scaler_pos)->TestChannel(3) << std::endl;
	////////////End of Scrambled Egg Code part 2////////////
	data.close();//close data file*/

	std::cout<<"cleanup"<<std::endl;
	Lcard.clear();
	Ncard.clear();
	Data.ch.clear();
}

////////////////////////////////////
// DataModel.h
	
//	std::vector<CardData*> carddata; 
//	TriggerData *trigdata;
	

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
		//std::cout<<"ADC"<<std::endl;
		ccp = new Lecroy4300b(cardslot, config);//FIXME 
	}
	else if (cardname == "SCA")
	{
		//std::cout<<"SCA"<<std::endl;
		ccp = new Jorway85A(cardslot, config);
	}
	else
	{
		std::cerr <<"Card type not specified " << cardname << std::endl;
	}
	return ccp;
}

