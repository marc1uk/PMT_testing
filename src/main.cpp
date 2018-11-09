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
#include <ctime>
//#include <thread>
#include <chrono>
#include <fstream>
#include <cstring>
#include <bitset>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"

#include "TApplication.h"
#include "TSystem.h"

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
	
	int i_channel;
	std::string FileName;
	int numacquisitions=10000;
	//std::cout << "Please enter number of writes to the action register: ";
	//std::cin >> numacquisitions;
	
	// ***************************************
	// GAIN MEASUREMENT ARGS
	// ***************************************
	if(argc<3||argc>4){
		std::cout<<"Usage: ./main ADC_channel outfilename acquisitions=10000"<<std::endl;
		return 0;
	}
	for (int i_arg=1;i_arg<argc;i_arg++){
		if (i_arg==1) i_channel = (int)*argv[i_arg]-'0';
		else if (i_arg==2) FileName = argv[i_arg];
		else if (i_arg==3) numacquisitions=atoi(argv[i_arg]);
		else std::cout <<"too many arguments, aborting."<<std::endl;
	}
	std::cout<<"Arguments are: "<<std::endl;
	std::cout<<"Channel "<<i_channel<<", "<<"filename "<<FileName<<std::endl;
	std::cout << std::endl << "There will be " << numacquisitions << " writes\n";
	
	int myargc = 1;
	char arg1[] = "potato";
	char* myargv[]={arg1};
	TApplication *PMTTestStandApp = new TApplication("PMTTestStandApp",&myargc,myargv);
	
//	// ***************************************
//	// COOLDOWN MEASUREMENT ARGS
//	// ***************************************
//	if(argc<3){
//		std::cout<<"usage: ./main [test_duration_in_hours] [time_between_samples_in_minutes] [count_time_in_seconds=60]"<<std::endl;
//		return 0;
//	}
//	// extract test stats
//	double testhours = atoi(argv[1]);
//	double testmins = testhours*60.;
//	double timeBet = atof(argv[2]);
//	double countmins = (argc>3) ? (double(atoi(argv[3]))/60.) : 1.;
//	double onesampletime = timeBet+countmins; // time between readings is wait time + count time
//	int numacquisitions = (testmins/onesampletime) + 1;
//	std::cout << "The scalers will be read " << numacquisitions << " times over " << testhours << " hours";
//	std::cout << " with " << timeBet << " minutes between readouts, and ";
//	std::cout << countmins << " minutes used to determine scalar rate.\n";
//	// ***************************************
	
//	//Ask User for PMT ID's
//	std::string pmtID1;
//	std::string pmtID2;
//	std::cout << "Please enter first PMT ID: ";
//	std::cin >> pmtID1;
//	std::cout << std::endl << "Enter second PMT ID: ";
//	std::cin >> pmtID2;
//	std::cout << std::endl;
	
	std::stringstream ss_channel;
	ss_channel<<i_channel;
	std::string str_channel = ss_channel.str();
	
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
	int scaler_pos = -1;                                        // position of scaler in list of cards
	int adc_pos = -2;                                           // position of scaler in list of cards
	int caen_pos = -2;                                          // position of CAEN controller in list of cards
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
			scaler_pos = List.CC["SCA"].size();
			//std::cout << "scaler found, list pos = " << scaler_pos << std::endl;
			List.CC["SCA"].push_back(Create(Lcard.at(i), Ccard.at(i), Ncard.at(i)));               //They use CC at 0
			//std::cout << "constructed Jorway85A module" << std::endl;
		}
		else if (Lcard.at(i) == "DISC")
		{
			List.CC["DISC"].push_back(Create(Lcard.at(i), Ccard.at(i), Ncard.at(i)));               //They use CC at 0
			std::cout << "constructed Lecroy4413 module" << std::endl;
		}
		else if (Lcard.at(i) == "CAEN")
		{
			caen_pos = List.CC["CAEN"].size();
			std::cout << "CAENET controller found, list pos = " <<caen_pos <<std::endl;
			List.CC["CAEN"].push_back(Create("CAEN",Ccard.at(i), Ncard.at(i)));
			std::cout << "constructed CAENC117B controller module" <<std::endl;
			std::cout << " "<<std::endl;
		}
		else std::cout << "\n\nUnkown card\n" << std::endl;
	}
	
//	// CAEN C117B tests
//	// ----------------
//	std::cout <<"CAENET Controller is in slot ";
//	std::cout << List.CC["CAEN"].at(caen_pos)->GetSlot() << std::endl;
//	Random Trials:
//	int ret_caen = List.CC["CAEN"].at(caen_pos)->ReadCrateOccupation();
//	List.CC["CAEN"].at(caen_pos)->TestOperation();
//	int ret_vmax = List.CC["CAEN"].at(caen_pos)->SetVmax(6,7,1000);
//	List.CC["CAEN"].at(caen_pos)->SetV1(6, 0, 100);
//	int readslot;
//	for (int i=0;i<1;i++){
//		readslot= List.CC["CAEN"].at(caen_pos)->ReadSlotN(i);
//	}
//	int ret_test;
//	for (int i_test=0; i_test<15; i_test++){
//		ret_test=List.CC["CAEN"].at(caen_pos)->TestOperation(i_test);
//	}
//	int ret_lam = List.CC["CAEN"].at(caen_pos)->EnLAM();
//	std::cout << List.CC["CAEN"].at(caen_pos)->TestLAM()<<std::endl;
	
	//std::cout << "Primary scaler is in slot ";
	//std::cout << List.CC["SCA"].at(scaler_pos)->GetSlot() << std::endl;
	
	//////////////////////////////////////
	// MRD branch: LeCroy.cpp - Initialise();
	
	// ***************************************
	// GAIN MEASUREMENT SETUP
	// ***************************************
	
	//Open File for ADC readouts
	std::string ADCFileName = FileName+".txt";
	std::ofstream ADCRead;
	ADCRead.open(ADCFileName.c_str());
	//ADCRead.open ("QDC_Vals_shorter_gate_channel3.txt");
	
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;  // Modify bit 1 of the register to "1" (CCUSB Trigger)
	long RegDeactivated = RegStore & ~0x02;  // Modify bit 1 to 0 (Deassert CCUSB trigger. Not strictly needed)
	
	CC->C();
	//CC->Z(); 
	// XXX CC->Z() CALLS Z() (initialize) ON ALL CARDS AS WELL. XXX
	// THIS WILL RESET REGISTER SETTINGS, REQUIRING RECONFIGURATION!
	usleep(100);
	
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
	// Fire the LEDs and read the Lecroys to measure the pulse integral
	
	// Print register settings to check the QDCs are configured properly
	//List.CC["ADC"].at(0)->GetRegister();             // read from the card into Control member
	//List.CC["ADC"].at(0)->DecRegister();             // decode 'Control' member into ECL, CLE etc. members
	//List.CC["ADC"].at(0)->PrintRegister();           // print ECL, CLE etc members
	//List.CC["ADC"].at(0)->PrintPedestal();           // print out pedestals
	//List.CC["ADC"].at(0)->SetPedestal();
	//List.CC["ADC"].at(0)->GetPedestal();             // retrieve pedastals into Ped member
	//List.CC["ADC"].at(0)->PrintPedestal();           // print out pedestals
	
	int command_ok;
	int ARegDat;
	long ARegRead;
	int printfreq=500;
	int plotfreq=100;
	// Execute: MAIN LOOP
	TCanvas* canv = new TCanvas("canv","canv",900,600);
	canv->cd();
	TH2D *hist = new TH2D("hist","ADC values : entry",100,0,numacquisitions,100,10,100);
	
//	// ***************************************
//	// COOLDOWN MEASUREMENT SETUP
//	// ***************************************
//	//Open up a new file called data.txt
//	std::ofstream data;
//	data.open ("data.txt");
//	data << "PMTID1 " << pmtID1 << ", PMTID2 " << pmtID2 << ", " << std::endl;
//	int thethreshold;
//	List.CC["DISC"].at(0)->ReadThreshold(thethreshold);
//	data << "Discriminator threshold "<<(thethreshold)<<" mV"<<std::endl;
//	data << "timestamp, ch1 reading, ch1 rate, ch2 reading, ch2 rate, "
//					"ch3 reading, ch3 rate, ch4 reading, ch4 rate"<<std::endl;
//	
//	// just for trials
//	//std::cout << "Test Channel: " << List.CC["SCA"].at(scaler_pos)->TestChannel(3) << std::endl;
//	//std::vector<int> thresholds{30,40,50,60,70,80,90,100,200,300,400};
//	//numacquisitions=thresholds.size();
//	// ***************************************
	
	
	//////////////////////////////////////
	// Execute: MAIN LOOP
	for (int i = 0; i < numacquisitions; i++)    // loop over readings
	{
		//break;
		
//		// ***************************************
//		// COOLDOWN MEASUREMENT
//		// ***************************************
//		// threshold trials
//		// ----------------
//		//std::cout<<"setting discriminator threshold to 30"<<std::endl;
//		//thethreshold=thresholds.at(i);
//		//List.CC["DISC"].at(0)->WriteThresholdValue(thethreshold);
//		//List.CC["DISC"].at(0)->ReadThreshold(thethreshold);
//		//std::cout<<"Threshold is now: "<<thethreshold<<std::endl;
//		
//		std::cout << "Clearing All Scalars" << std::endl;
//		List.CC["SCA"].at(scaler_pos)->ClearAll();
//		///////////////////// debug:
//		//std::cout << "zero'd values are: " << std::endl;
//		//for(int chan=0; chan<4; chan++){
//		//	std::cout << scalervals[chan];
//		//	if(chan<3) std::cout << ", ";
//		//}
//		//////////////////// end debug
//		
//		//waiting for one minute for counts to accumulate
//		std::cout << "Measuring rates" << std::endl;
//		//std::this_thread::sleep_for(std::chrono::seconds(60));
//		unsigned int counttimeinmicroseconds = countmins*60.*1000000.; //1000000
//		usleep(counttimeinmicroseconds);
//		
//		//Read the scalars
//		std::cout << "Reading Scalers and writing to file" << std::endl;
//		int ret = List.CC["SCA"].at(scaler_pos)->ReadAll(scalervals);
//		if(not ret < 0){        // FIXME need better error checking
//				std::cerr << "error reading scalers: response was " << ret << std::endl;
//			} else {
//				// get and write timestamp to file
//				std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
//				time_t tt;
//				tt = std::chrono::system_clock::to_time_t ( time );
//				std::string timeStamp = ctime(&tt);
//				timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);
//				data << timeStamp << ", ";
//				// write scalar values and rates to file
//				for(int chan=0; chan<4; chan++){
//					data << scalervals[chan] << ", ";
//					double darkRate = double (scalervals[chan]) / countmins;
//					data << darkRate;
//					if(chan<3) data << ", ";
//				}
//			}
//		data << std::endl;
//		std::cout << "Done writing to file" << std::endl;
//		
		
		// ***************************************
		// GAIN MEASUREMENT
		// ***************************************
/*
		// step 0: initialize the card
		// ---------------------------
		// XXX THIS IS NECESSARY AFTER Z() IS CALLED XXX
		for (int i = 0; i < List.CC["ADC"].size(); i++){
			std::cout<<"Initializing 4300B "<<i<<std::endl;
			command_ok = List.CC["ADC"].at(0)->Z();
			if(command_ok<0) std::cerr<<"4300B::Z() ret<0. Whatever that means"<<std::endl;
			usleep(1000); // sleep 1ms, and rest
			std::cout<<"Printing intialized register - expect top 7 bits to be '1'"<<std::endl;
			// check the initialize worked: bits 9-15 should be set to 1 by a Z() command 
			List.CC["ADC"].at(i)->GetRegister();  // so check it's set as expected;
			List.CC["ADC"].at(i)->DecRegister();
			List.CC["ADC"].at(i)->PrintRegister();
		
			// step 1: setup register
			std::cout<<"configuring 4300B from file "<<Ccard.at(1)<<std::endl;
			List.CC["ADC"].at(i)->SetConfig(Ccard.at(1));    // load ECL, CLE etc. members with desired configuration
			List.CC["ADC"].at(i)->EncRegister();             // encode ECL, CLE etc into Control member
			std::cout<<"Writing following register contents"<<std::endl;
			List.CC["ADC"].at(i)->PrintRegister();           // print ECL, CLE etc members
			List.CC["ADC"].at(i)->SetRegister();             // write Control member to the card
			usleep(1000);
			std::cout<<"Read back the following register contents"<<std::endl;
			List.CC["ADC"].at(i)->GetRegister();             // read from the card into Control member
			List.CC["ADC"].at(i)->DecRegister();             // decode 'Control' member into ECL, CLE etc. members
			List.CC["ADC"].at(i)->PrintRegister();           // print ECL, CLE etc members
			usleep(1000); // sleep 1ms, and rest 
		}
*/
		
		// step 2: clear the module to prep for test
		//std::cout<<"clearing ADCs"<<std::endl;
		for (int cardi = 0; cardi < List.CC["ADC"].size(); cardi++){
			command_ok = List.CC["ADC"].at(cardi)->ClearAll(); // Lecroy:4300b::C() also works
			//std::cout<<"Clear module " << cardi <<": " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		}
		usleep(100);
		
		// Fire LED! (and gate ADCs)
		command_ok = CC->ActionRegWrite(RegActivated);
		//std::cout<<"Firing LED " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		
		// alternatively, run the test, which connects internal charge generator and pulses the gate
		//for (int cardi = 0; cardi < List.CC["ADC"].size(); cardi++){
		//	command_ok = List.CC["ADC"].at(cardi)->InitTest();
		//	std::cout<<"test start "<<i<<" " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		//}
		
		//for(int sleeps=0; sleeps<5000; sleeps++)
		usleep(1000);
		
		for (int cardi = 0; cardi < List.CC["ADC"].size(); cardi++){
			int numchecks=2;
			int statusreglast=0;
			for(int checknum=0; checknum<numchecks; checknum++){
				// now check for the 'busy' signal?
				int statusreg=0, busy=0, funcok=0;
				command_ok = List.CC["ADC"].at(cardi)->READ(0, 0, statusreg, busy, funcok);  // F=0, A=0, N=slot (implicit)
				if(not funcok) std::cerr<<"X=0, Function 0 (read register) not recognised??"<<std::endl;
				// register contents are 0 when BUSY
				//if(not statusreg) std::cerr<<"Status register = "<<statusreg
				//													 <<", Camac LAM should at least be valid"<<std::endl;
				if(command_ok<0) std::cerr<<"ret<0. Whatever that means"<<std::endl;
				if(not busy){ /*std::cout<<"Q=0; BUSY is ON!"<<std::endl;*/ break; }
				else if(checknum==numchecks-1){
					std::cerr<<"ADC "<<i<< " BUSY was not raised following Gate / InitTest?" << std::endl;
				}
				else {
					// if(statusreg!=statusreglast)
					// std::cout<<"status reg: Q="<<busy<<", register = " << statusreg 
					//					<< "("<< std::bitset<16>(statusreg) << ")" << std::endl;
					//statusreglast=statusreg;
					usleep(100);
				} // sleep 10us
			}
		}
		
		// Put timestamp in file
		std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
		time_t tt;
		tt = std::chrono::system_clock::to_time_t ( time );
		std::string timeStamp = ctime(&tt);
		timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);
		ADCRead << timeStamp;
		
		//std::cout<<"Reading ADCvals at " << timeStamp<<std::endl;
		
		// Read ADC values
		for (int cardi = 0; cardi < List.CC["ADC"].size(); cardi++){
			//List.CC["ADC"].at(cardi)->PrintRegister();
			//List.CC["ADC"].at(cardi)->GetRegister();
			//List.CC["ADC"].at(cardi)->DecodeRegister();
			//List.CC["ADC"].at(cardi)->PrintRegister();
			//List.CC["ADC"].at(cardi)->PrintPedestal();
			// ReadOut behaviour depends on config file. If CSR (camac sequential readout) is set to 1, 
			// channel input is ignored, all 16 channels are returned by subsequent 'ReadOut' calls.
			// Otherwise if CSR = 0, channel input is used.
			// Or: dump all channels into a map - simply loops over ReadAll calls. Should work with CSR=0 or 1
			//std::cout<<"reading ADCvals"<<std::endl;
			std::map<int, int> ADCvals;
/*
			// this code works!
			for(int channeli=0; channeli<16; channeli++){
				int statusreg=0, busy=0, funcok=0;
				//:READ(int A, int F, int &Data, int &Q, int &X)
				std::cout<<"Reading channel "<<channeli<<std::endl;
				command_ok = List.CC["ADC"].at(cardi)->READ(channeli, 2, statusreg, busy, funcok);  // F=2, A=channeli
				if(not funcok) std::cerr<<"X=0, Function 0 (read register) not recognised??"<<std::endl;
				if(not statusreg) std::cerr<<"Data = 0. :("<<std::endl; else std::cout<<"SUCCESS: DATA["<<channeli<<"] = "<<statusreg<<std::endl;
				if(command_ok<0) std::cerr<<"ret<0. Whatever that means"<<std::endl;
				if(not busy){ std::cout<<"Q=0; No Data?"<<std::endl; usleep(1000); }
			}
*/
			// so does this
			// GetData calls DumpAll(ADCvals); or DumpCompressed based on register settings. Includes waiting for LAM
			List.CC["ADC"].at(cardi)->GetData(ADCvals);
			if(ADCvals.size()==0){ std::cerr<<"ADC "<<cardi<< " GetData returned no measurements!"<<std::endl; }
			for( std::map<int,int>::iterator aval = ADCvals.begin(); aval!=ADCvals.end(); aval++){
				ADCRead << ", ";
				ADCRead << aval->second;
				if (i%printfreq==0) {
					std::cout << /*aval->first << "=" <<*/ aval->second;
					if(aval!=ADCvals.begin()) std::cout<<", ";
				}
			}
			hist->Fill(i,ADCvals.at(i_channel));
			if (i%plotfreq==0) {hist->Draw("colz");
			canv->Modified();
			canv->Update();
			gSystem->ProcessEvents();
			}
			//if ((i%500)==0){TCanvas *c1 = new TCanvas(); hist->Draw("colz");}
			ADCRead << std::endl;
			if (i%printfreq==0) { std::cout<<std::endl; }
		}
		//ADCRead << std::endl;
		//for(int delayi=0; delayi<100; delayi++)
		//usleep(1000);
		
//		// ***************************************
//		// COOLDOWN MEASUREMENT DELAY
//		// ***************************************
//		//data << "End of Loop " << i << std::endl;
//		//For loop will now wait for user-specified time
//		std::cout<<"Waiting "<<timeBet<<" mins to next reading"<<std::endl;
//		if(timeBet>1){
//			// for monitoring the program, printout every minute
//			for(int count2=0; count2 < timeBet; count2++){
//				std::cout << (timeBet-count2) << " minutes to next reading..." << std::endl;
//				//std::this_thread::sleep_for (std::chrono::seconds(1));
//				double minuteinmicroseconds = 60.*1000000.;
//				usleep(minuteinmicroseconds);
//			}
//		} else {
//			double sleeptimeinmicroseconds = timeBet*60.*1000000.;
//			usleep(sleeptimeinmicroseconds);
//		}
		
		
	}
//	// ***************************************
//	// COOLDOWN MEASUREMENT CLEANUP
//	// ***************************************
//	data.close();//close data file
//	
//	// ***************************************
//	// GAIN MEASUREMENT CLEANUP
//	// ***************************************
	std::cout<<"closing file"<<std::endl;
	ADCRead.close();
	std::string adc_pdf = FileName+"_ch"+str_channel+"_stability.pdf";
	canv->SaveAs(adc_pdf.c_str());
	
	
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
	  std::cout<<"DISC"<<std::endl;
		ccp = new LeCroy4413(cardslot, config);
	}
	else if (cardname == "CAEN")
	{
		std::cout<<"CAEN"<<std::endl;
		ccp = new CAENC117B(cardslot, config);
	}
	else
	{
		std::cerr <<"Card type not specified " << cardname << std::endl;
	}
	return ccp;
}

