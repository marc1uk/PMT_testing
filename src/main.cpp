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
	CamacCrate* CC2 = new CamacCrate;
	//////////////////////////////////////
	// Create card object based on configuration read from file
	
	int trg_pos = 0;                                            // position of trigger card in list of cards
	int scaler_pos = -1;                                         // position of scaler in list of cards
	std::cout << "begin scan over " <<Lcard.size()<< " cards " << std::endl;
	//Guessing this for loop scans over all cards
	for (int i = 0; i < Lcard.size(); i++)                      // CHECK i
	{
		if (Lcard.at(i) == "TDC" || Lcard.at(i) == "ADC")
		{
			CamacCrate* cardPointer = Create(Lcard.at(i), Ccard.at(i), Ncard.at(i));
			if (cardPointer == NULL)
			{
				std::cerr << "unknown card type " << Lcard.at(i) << std::endl;
				return 0;
			} 
			else
			{
				List.CC[Lcard.at(i)].push_back(cardPointer);  //They use CC at 0
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

	//Spaghetti Code
	long data1;
	CC->RegRead(7,data1); 
	std::cout << "DGG A Register Reads " << data1<< std::endl;
	std::cout << "WriteReg DGG A Result: " << CC->RegWrite(7,1) << std::endl;
	std::cout << "WriteReg DGG B Result: " << CC->RegWrite(8,0) << std::endl;
	CC->RegRead(7,data1);
	std::cout << "WriteReg Result DGG_EXT: " << CC->RegWrite(13,0) << std::endl;
	std::cout << "DGG A Register Reads " << data1<< std::endl;
	
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
	
	int repeats;
	std::cout << "Please enter number of writes to the action register: ";
	std::cin >> repeats;
	std::cout << std::endl << "There will be " << repeats << " writes\n";
	
	long RegStore;
	CC->ActionRegRead(RegStore);
	std::cout<<"Taking reference for action register: " << RegStore << std::endl;
	long RegDeactivated = RegStore;          // FIXME replace with a version with action reg asserted to 0
	long RegActivated = RegStore | 1u << 1;  // Modify bit 1 of the register to "1" (CCUSB Trigger)
	int ARegDat;
	long ARegRead;
	for (int i = 0; i < repeats; i++)
	{
		//Clearing Settings on channels
		std::cout << "01: disabling USB trigger on both channels"<<std::endl;
		// DelayGateGen(DGG, trig_src, NIM_out, delay_ns, gate_ns, NIM_invert, NIM_latch);	
		ARegDat = CC->DelayGateGen(0,0,1,1,25,0,0); // set DGG 0 on output 1 to no src (disabled)
		if(ARegDat<0) std::cerr<<"failed to disable USB trigger on output 0!" << std::endl;
		ARegDat = CC->DelayGateGen(1,0,2,1,5,0,0); // "   "   1 "   "     2  "   "
		if(ARegDat<0) std::cerr<<"failed to disable USB trigger on output 0!" << std::endl;
		//usleep(500);
		
		//Change the trigger mode of O1 to USB
		std::cout << "02: Enabling USB trigger on Channel O1" << std::endl;
		ARegDat = CC->DelayGateGen(0,6,1,1,25,0,0); // set DGG 0 on output 1 to USB trigger
		if(ARegDat<0) std::cerr<<"failed to enable USB trigger on output 0!" << std::endl;
		//usleep(500);
		
		//Change the second bit to 1 to trigger the response from O1
		std::cout << "03: " << RegActivated << " will be written to the register" <<std::endl;
		ARegDat = CC->ActionRegWrite(RegActivated);
		std::cout<<"	ActionRegWrite wrote "<<ARegDat<<" bytes to register, ";
		if(ARegDat<0) std::cerr<<"failed to fire USB trigger on output 1!" << std::endl;
		//usleep(500);	
		ARegDat = CC->ActionRegRead(ARegRead);
		if(ARegDat<0) std::cerr<<"failed (" << ARegDat << ") to read register to confirm write success!" << std::endl;  // XXX
		else std::cout << "ARegRead has become " << ARegRead << std::endl;
		
		//Change the value of the second bit back to its original value
		std::cout << "04: Restoring Register to original value" << std::endl;
		ARegDat = CC->ActionRegWrite(RegDeactivated);
		if(ARegDat<0) std::cerr<<"failed to (" << ARegDat << ") deactivate USB trigger on output 1!" << std::endl;    // XXX 
		else std::cout << "Wrote " << ARegDat << " bytes to register, ";
		//usleep(500);
		ARegDat = CC->ActionRegRead(ARegRead);
		if(ARegDat<0) std::cerr<<"failed (" << ARegDat << ") to read register to confirm write success!" << std::endl; // XXX
		else std::cout << "ARegRead has become " << ARegRead << std::endl;
		//usleep(500);
		//Change the trigger mode of O1 to "End Trigger" and set O2 to USB trigger
		std::cout<<std::endl;
		
		
		// CHANNEL 2
		std::cout << "05: Disabling USB trigger on Channel 01 and enabling on Channel O2" << std::endl;
		ARegDat = CC->DelayGateGen(0,0,1,1,25,0,0);    // disable USB trigger on DGG 0
		if(ARegDat<0) std::cerr<<"failed to disable USB trigger on output 0!" << std::endl;
		ARegDat = CC->DelayGateGen(1,6,2,1,5,0,0);    // enable  USB trigger on DGG 1
		if(ARegDat<0) std::cerr<<"failed to enable USB trigger on output 1!" << std::endl;
		
		//Trigger on channel 2
		//FALLS OVER HERE (SOMETIMES): WRITING TO REG RETURNS -VE VALUE
		std::cout << "06: " << RegActivated << " Will be written to the register" <<std::endl;
		ARegDat = CC->ActionRegWrite(RegActivated);
		if(ARegDat<0) std::cerr<<"failed to fire USB trigger on output 2!" << std::endl;
		else std::cout << "ActionRegWrite Wrote " << ARegDat << " bytes to register." <<std::endl;
		usleep(500);
		//It doesn't seem to want to actually spit out a signal
		
		//Finishing triggering on channel 2
		std::cout << "07: Ending Trigger on Channel 2 and resetting Register" << std::endl;
		ARegDat = CC->ActionRegWrite(RegDeactivated);
		if(ARegDat<0) std::cerr<<"failed to deactivate USB trigger on output 2!" << std::endl;
		else std::cout << "Wrote " << ARegDat << " bytes to file. Register restored." <<std::endl;
		usleep(500);
		ARegDat = CC->DelayGateGen(1,0,2,1,5,0,0);   // disable DGG 1
		if(ARegDat<0) std::cerr<<"failed to diable USB trigger on output 2!" << std::endl;
		std::cout<<std::endl;
		std::cout<<"============================"<<std::endl;
		
		for (int j = 0; j < 1; j++)
		{ 
			for (int i = 0; i < List.CC["ADC"].size(); i++)
				{
					int ADCData;
					//List.CC["ADC"].at(i)->ClearAll();
					//List.CC["ADC"].at(i)->GetRegister();
					//List.CC["ADC"].at(i)->PrintRegRaw();   // not implemented for Scaler yet
					//ADCRead << List.CC["ADC"].at(i)->ReadOut(ADCData, 1);
					int initTest = List.CC["ADC"].at(i)->InitTest();
					std::cout << "Your InitTest returned " << initTest << std::endl;
					/*
					std::cout << "Your Data: " << ADCData << std::endl;
					*/
					List.CC["ADC"].at(i)->EncRegister();
					List.CC["ADC"].at(i)->PrintRegister();
					//ADCRead << List.CC["ADC"].at(i)->DumpCompressed(std::map<int, int> &mData);
					//ADCRead << List.CC["ADC"].at(i)->ReadOut(ADCData, 0) << " ";
					
	
				}
			ADCRead << std::endl;
		}

		
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
	std::cout<<"cardname is "<<cardname<<", comp with 'ADC' is " << (cardname=="ADC") <<std::endl;
	std::cout<<"strcmp is = "<<strcmp(cardname.c_str(),"ADC") << std::endl;
	if (cardname == "ADC")
	{
		std::cout<<"ADC"<<std::endl;
		ccp = new Lecroy4300b(cardslot, config);
	}
	else if (cardname == "SCA")
	{
	  std::cout<<"SCA"<<std::endl;
		ccp = new Jorway85A(cardslot, config);
	}
	else
	{
		std::cerr <<"Card type not specified " << cardname << std::endl;
	}
	return ccp;
}


