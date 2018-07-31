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
//#include <chrono>
#include <fstream>
#include <cstring>
#include <bitset>

unsigned int masks[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 
			0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000, 0x1000000, 0x2000000, 0x4000000, 0x8000000,
			0x10000000, 0x20000000, 0x40000000, 0x80000000};
short SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off){
	std::cout<<"setting register "<<regnum<<" bit(s) "<<firstbit;
	if(numbits>1) std::cout<<"-"<<(firstbit+numbits-1);
	std::cout<<" to "<<on_or_off<<std::endl;
	
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
	std::cout<<"setting register "<<regnum<<" to ["<<std::bitset<32>(placeholder)<<"]"<<std::endl;
	myret = CC->WRITE(0,regnum,16,placeholder,myQ,myX);
	std::cout<<"register set returned "<<myret<<std::endl;
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
	
	/*
	//Get trigger setting from user for each channel;
	int trig1;
	int trig2;
	
	std::cout << "Enter Trigger for Channel 1 Gate Generator (No. 0-7): ";
	std::cin >> trig1;
	std::cout << std::endl << "Enter Trigger for Channel 2 Gate Generator (No. 0-7): ";
	std::cin >> trig2;
	std::cout << std::endl << "Cheers Mate" << std::endl;
	
	trig1 = 5;
	trig2 = 5;
	*/
	
	//Set the gates from each generator channel, outputting on outputs O1 and O2
	//std::cout << CC->DelayGateGen(0, trig1, 1, 1, 25, 0, 0) << std::endl;
	//std::cout << CC->DelayGateGen(1, trig2, 2, 3, 23, 0, 0) << std::endl;
	//Yet More Spaghetti Code
	//Open File for ADC readouts
	std::ofstream ADCRead;
	ADCRead.open ("ADCRead.txt");
	
	int repeats=1;
	//std::cout << "Please enter number of writes to the action register: ";
	//std::cin >> repeats;
	std::cout << std::endl << "There will be " << repeats << " writes\n";
	
	long RegStore;
	CC->ActionRegRead(RegStore);
	std::cout<<"Taking reference for action register: " << std::bitset<32>(RegStore) << std::endl;
	long RegDeactivated = RegStore;          // FIXME replace with a version with action reg asserted to 0
	long RegActivated = RegStore | 1u << 1;  // Modify bit 1 of the register to "1" (CCUSB Trigger)
	std::cout<<"RegActivated = " << std::bitset<32>(RegActivated) << ", RegDeactivated = " << std::bitset<32>(RegDeactivated) << std::endl;
	
	long FWver;
	int myQ, myX, readbytes;
	readbytes = CC->READ(0, 0, 0, FWver, myQ, myX);
	std::cout<<"Reading firmware ver read "<<readbytes<<" bytes, Q="<<myQ<<", X="<<myX<<", firmware ver is "<<FWver<<std::endl;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	std::cout<<"sizeof(long) = "<<sizeof(long) << std::endl;
	
	fw>5.01
	set nim output sources by code in register 5
	nim output 1: bits 0-2
	nim output 2: bits 8-10
	nim output 3: bits 16-18
	in these bits place the corresponding 2-bit code:
	DGG_A = 2, DGG_B = 3
	usb trigger is not available as on usb trigger for any nim outputs
	set register 4 to set source of leds: set bits 8-10=3, 12=0 (invert off), 13=0 (latch off) to flicker green LED on usb trigger 
	as per v<1.01, DGG sources are set by register 6, the User Devices Source Selector register, in bits 16-18 (DGG_A) and 24-26 (DGG_B)
	use code 6 for both to set to usb trigger
	register 7 and 8 set up the delay and gate times for DGG_A and DGG_B respectively, register 13 sets extended delay bits
	fine times are 16-bit numbers in units of 10ns, coarse times add further 16 bits.
	register 7 and 8 bits 0-15 are delay fine, bits 16-31 are gate fine
	register 13 bits 0-15 are DGG_A delay coarse and bits 16-31 are DGG_B delay coarse
	
	fw<5.01
	Nim output 1: usb trigger = mode 2,      DGG_A = 3, DGG_B = 4
	Nim output 2: usb trigger not available, DGG_A = 4, DGG_B = 5
	Nim output 3: usb trigger = mode 7,      DGG_A = 5, DGG_B = 6
	
	1.01<fw<4.02
	delay and gates in range 10ns - 650us
	set the appropriate codes in the User Devices Register and delays in the DGG register, sec 3.2.7
	User Devices Source Selector register (register 6) has: bits 16-18 for DGG_A, bits 24-26 for DGG_B
	The values entered into those bits selects the source; use 6 for USB trigger.
	
	fw<1.01
	nim output as per v<5.01, 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	*/
	CC->C();
	CC->Z();
	usleep(1000);
	/*
	std::cout<<"before executing anything, registers read: "<<std::endl;
	PrintReg(CC,5);
	PrintReg(CC,6);
	PrintReg(CC,7);
	PrintReg(CC,8);
	PrintReg(CC,13);
	usleep(1000);
	readbytes = CC->DelayGateGen(0,6,1,1,1,0,0); // set DGG 0 on output 1 to USB trigger with delay 10ns, gate 10ns, invert off, latch off
	usleep(1000);
	std::cout<<"after executing DGG setup, registers read: "<<std::endl;
	PrintReg(CC,5);
	PrintReg(CC,6);
	PrintReg(CC,7);
	PrintReg(CC,8);
	PrintReg(CC,13);
	*/

	//SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off)
	//1. set up Nim out 1 to DGG_A: write register 5 bits 0-2 to 2 (3 for firmware<5 gives no output)
	SetRegBits(CC,5,0,3,false);    // set bits 0-2 on
	SetRegBits(CC,5,1,1,true);     // set bit 1 on
	PrintReg(CC,5);
	//2. set up Nim output 2 to DGG_B: write register 5 bits 8-10 to 3 (5 for newer firmware gives no output)
	//SetRegBits(CC,5,8,2,true);    // set bits 8 to on
	//SetRegBits(CC,5,9,1,false);   // set bits 9 to off
	//PrintReg(CC,5);
	//3. set DGG_A source to usb: write register 6 bits 16-18 to 6
	SetRegBits(CC,6,16,1,false);  // set bit 16 to off
	SetRegBits(CC,6,17,2,true);   // set bits 17,18 to on
	PrintReg(CC,6);
	//4. set DGG_B source to usb: write register 6 bits 24-26 to 6
	//SetRegBits(CC,6,24,1,false);  // set bit 24 to off
	//SetRegBits(CC,6,25,2,true);   // set bits 25,26 to on
	//PrintReg(CC,6);
	//5. set DGG_A delay to 0ns: write register 7 bits 0-15 to 0
	SetRegBits(CC,7,0,16,false);
	//6. set DGG_A gate (duration) to maximum (655,350 ns) : write register 7 bits 16-31 to 1        // actually gives 2.5us = 2500ns = 250 of 10ns
	SetRegBits(CC,7,16,16,false); // set bits 16-31 off
	SetRegBits(CC,7,20,1,true);   // set bit 16 on
	//SetRegBits(CC,7,16,16,true);
	PrintReg(CC,7);
	//7. set DGG_B delay to 40ns: write register 8 bits 0-15 to 4
	//SetRegBits(CC,8,0,16,false);  // set bits 0-16 to off
	//SetRegBits(CC,8,3,1,true);    // set bit 3 to on
	//PrintReg(CC,8);
	//8. set DGG_B gate to 160ns: write register 8 bits 16-31 to 16
	//SetRegBits(CC,8,16,16,true);
	//SetRegBits(CC,8,0,16,false);  // set bits 0-16 off
	//SetRegBits(CC,8,16,1,true);   // set bit 16 on
	//PrintReg(CC,8);
	//9. set extended delays on DGG_A and DGG_B to to 0: write register 13 bits 0-15 and 16-31 to 0
	SetRegBits(CC,13,0,32,false);
	PrintReg(CC,13);
	
	//9.5 optional: set register 4 bits 8-10=3, 12=0 (invert off), 13=0 (latch off) to flicker green LED on usb trigger
	SetRegBits(CC,4,8,2,true);   // set bits 8-9 on
	SetRegBits(CC,4,10,1,false); // set bit 10 off
	SetRegBits(CC,4,12,2,false); // set bits 12 and 13 off
	
	usleep(1000);
	
	//10. FIRE TORPEDOS: write action register
	for(int fires=0; fires<3; fires++){
		CC->ActionRegWrite(RegActivated);   // <<< this works,
		usleep(1000000);
	}
	// NEED TO CALL Z (initialize) OR C (clear) TO UNFREEZE CCUSB...
	//CC->C();
	
	int ARegDat;
	long ARegRead;
	repeats=1;
	short pulselength=500;
	for (int i = 0; i < repeats; i++)
	{
		break;
		//Clearing Settings on channels
		std::cout << "01: disabling USB trigger on both channels"<<std::endl;
		// DelayGateGen(DGG, trig_src, NIM_out, delay_ns, gate_ns, NIM_invert, NIM_latch);
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->DelayGateGen(0,0,1,1,25,0,0); // set DGG 0 on output 1 to no src (disabled)
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to disable USB trigger on output 0!" << std::endl;
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->DelayGateGen(1,0,2,1,25,0,0); // "   "   1 "   "     2  "   "
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to disable USB trigger on output 0!" << std::endl;
		usleep(500);
		
		//Change the trigger mode of O1 to USB
		std::cout << "02: Enabling USB trigger on Channel O1" << std::endl;
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->DelayGateGen(0,6,1,1,pulselength,0,0); // set DGG 0 on output 1 to USB trigger
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to enable USB trigger on output 0!" << std::endl;
		usleep(500);
		
		//Change the second bit to 1 to trigger the response from O1
		std::cout << "03: " << RegActivated << " will be written to the register" <<std::endl;
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->ActionRegWrite(RegActivated);
			if(ARegDat>0) break;
			usleep(500);
		}
		std::cout<<"	ActionRegWrite wrote "<<ARegDat<<" bytes to register, ";
		if(ARegDat<0) std::cerr<<"failed to fire USB trigger on output 1!" << std::endl;
		usleep(500);	
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->ActionRegRead(ARegRead);
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed (" << ARegDat << ") to read register to confirm write success!" << std::endl;  // XXX
		else std::cout << "ARegRead has become " << ARegRead << std::endl;
		
		//usleep(2.*pulselength*100.); // hold action reg (usb trigger) high for 1s
		usleep(5000000);
		std::cout<<"moving on"<<std::endl;
		
		//Change the value of the second bit back to its original value
		std::cout << "04: Restoring Register to original value" << std::endl;
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->ActionRegWrite(RegDeactivated);
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to (" << ARegDat << ") deactivate USB trigger on output 1!" << std::endl;    // XXX 
		else std::cout << "Wrote " << ARegDat << " bytes to register, ";
		usleep(500);
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->ActionRegRead(ARegRead);
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed (" << ARegDat << ") to read register to confirm write success!" << std::endl; // XXX
		else std::cout << "ARegRead has become " << ARegRead << std::endl;
		usleep(500);
		//Change the trigger mode of O1 to "End Trigger" and set O2 to USB trigger
		std::cout<<std::endl;
		
		
		// CHANNEL 2
		std::cout << "05: Disabling USB trigger on Channel O1 and enabling on Channel O2" << std::endl;
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->DelayGateGen(0,0,1,1,25,0,0);    // disable USB trigger on DGG 0
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to disable USB trigger on output 0!" << std::endl;
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->DelayGateGen(1,6,2,1,pulselength,0,0);    // enable USB trigger on DGG 1
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to enable USB trigger on output 1!" << std::endl;
		
		//Trigger on channel 2
		//FALLS OVER HERE (SOMETIMES): WRITING TO REG RETURNS -VE VALUE
		std::cout << "06: " << RegActivated << " Will be written to the register" <<std::endl;
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->ActionRegWrite(RegActivated);
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to fire USB trigger on output 2!" << std::endl;
		else std::cout << "ActionRegWrite Wrote " << ARegDat << " bytes to register." <<std::endl;
		usleep(500);
		//It doesn't seem to want to actually spit out a signal
		
		usleep(2.*pulselength*100.); // hold action register (usb trigger) high for 1s
		std::cout<<"moving on"<<std::endl;
		
		//Finishing triggering on channel 2
		std::cout << "07: Ending Trigger on Channel 2 and resetting Register" << std::endl;
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->ActionRegWrite(RegDeactivated);
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to deactivate USB trigger on output 2!" << std::endl;
		else std::cout << "Wrote " << ARegDat << " bytes to file. Register restored." <<std::endl;
		usleep(500);
		for(int tryi=0; tryi<5; tryi++){ 
			ARegDat = CC->DelayGateGen(1,0,2,1,25,0,0);   // disable DGG 1
			if(ARegDat>0) break;
			usleep(500);
		}
		if(ARegDat<0) std::cerr<<"failed to diable USB trigger on output 2!" << std::endl;
		std::cout<<std::endl;
		std::cout<<"============================"<<std::endl;
		
		for (int i = 0; i < List.CC["ADC"].size(); i++)
				{
					int ADCData;
					//List.CC["ADC"].at(i)->ClearAll();
					//List.CC["ADC"].at(i)->GetRegister();
					//List.CC["ADC"].at(i)->PrintRegRaw();   // not implemented for Scaler yet
					//ADCRead << List.CC["ADC"].at(i)->ReadOut(ADCData, 1);
					int initTest = List.CC["ADC"].at(i)->InitTest();
					std::cout << "Your InitTest returned " << initTest << std::endl;
					std::cout << "Your Data: " << ADCData << std::endl;
					
					List.CC["ADC"].at(i)->EncRegister();
					List.CC["ADC"].at(i)->PrintRegister();
					//ADCRead << List.CC["ADC"].at(i)->DumpCompressed(std::map<int, int> &mData);
					//ADCRead << List.CC["ADC"].at(i)->ReadOut(ADCData, 0) << " ";
					
	
				}
		ADCRead << std::endl;
		

		
		usleep(1000000);
	}
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

/// for c files, cannot have <bitset>, use:

/* --- PRINTF_BYTE_TO_BINARY macro's --- */
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)    \
    (((i) & 0x80ll) ? '1' : '0'), \
    (((i) & 0x40ll) ? '1' : '0'), \
    (((i) & 0x20ll) ? '1' : '0'), \
    (((i) & 0x10ll) ? '1' : '0'), \
    (((i) & 0x08ll) ? '1' : '0'), \
    (((i) & 0x04ll) ? '1' : '0'), \
    (((i) & 0x02ll) ? '1' : '0'), \
    (((i) & 0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 \
    PRINTF_BINARY_PATTERN_INT8              PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
    PRINTF_BYTE_TO_BINARY_INT8((i) >> 8),   PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
    PRINTF_BINARY_PATTERN_INT16             PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
    PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64    \
    PRINTF_BINARY_PATTERN_INT32             PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) \
    PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)
/* --- end macros --- */
// use like:
// printf("register 0 is: " PRINTF_BINARY_PATTERN_INT32 "\n", PRINTF_BYTE_TO_BINARY_INT32(Data));

