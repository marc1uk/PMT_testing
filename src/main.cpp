/* vim:set noexpandtab tabstop=4 wrap */
#include "CamacCrate.h"
//#include "CAENC117B.h"
#include "my_Caen_C117B.h"
//#include "Lecroy3377.h"
//#include "Lecroy4300b.h"
#include "Jorway85A.h"
#include "Lecroy4413.h"
#include "libxxusb.h"
#include "usb.h"
#include <ctime>
#include <thread>
#include <chrono>
#include <fstream>
// c++98 does not define sleep_for, need to use posix usleep
#include <unistd.h>

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

	if(argc<3){
		std::cout<<"usage: ./main [test_duration_in_hours] [time_between_samples_in_minutes] [count_time_in_seconds=60]"<<std::endl;
		return 0;
	}
	
	
	///////////////////////////////////////
	// master branch: CCTrigger.h
	


	////////////Scrambled Egg Part 1 Begins////////////
	//Open up a new file called data.txt
	std::ofstream data;
	data.open ("data.txt");
	
	// extract test stats
	double testhours = atoi(argv[1]);
	double testmins = testhours*60.;
	double timeBet = atof(argv[2]);
	double countmins = (argc>3) ? (double(atoi(argv[3]))/60.) : 1.;
	double onesampletime = timeBet+countmins; // time between readings is wait time + count time
	int reps = (testmins/onesampletime) + 1;
	std::cout << "The scalers will be read " << reps << " times over " << testhours << " hours";
	std::cout << " with " << timeBet << " minutes between readouts, and " << countmins << " minutes used to determine scalar rate.\n";
	
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
	// CamacCrate* CC = new CamacCrate;                         // CamacCrate at 0
	
	//////////////////////////////////////
	// Create card object based on configuration read from file
	
	int trg_pos = 0;                                            // position of trigger card in list of cards
	int scaler_pos = -1;                                        // position of scaler in list of cards
	int caen_pos = -2;                                          // position of CAEN controller in list of cards
	std::cout << "begin scan over " <<Lcard.size()<< " cards " << std::endl;
	for (int i = 0; i < Lcard.size(); i++)                      // CHECK i
	{
		if (Lcard.at(i) == "TDC" || Lcard.at(i) == "ADC")
		{
			List.CC[Lcard.at(i)].push_back(Create(Lcard.at(i), Ccard.at(i), Ncard.at(i)));  //They use CC at 0
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
	
	std::cout << "Primary scaler is in slot ";
	std::cout << List.CC["SCA"].at(scaler_pos)->GetSlot() << std::endl;
	
	//////////////////////////////////////
	// MRD branch: LeCroy.cpp - Initialise();
	// clear all modules
	for (int i = 0; i < List.CC[DC].size(); i++)
	{
		List.CC[DC].at(i)->ClearAll();
	}
	//Ask User for PMT ID's
	std::string pmtID1;
	std::string pmtID2;
	std::cout << "Please enter first PMT ID: ";
	std::cin >> pmtID1;
	std::cout << std::endl << "Enter second PMT ID: ";
	std::cin >> pmtID2;
	std::cout << std::endl;
	
	
	data << "PMTID1 " << pmtID1 << ", PMTID2 " << pmtID2 << ", " << std::endl;
	int thethreshold;
	List.CC["DISC"].at(0)->ReadThreshold(thethreshold);
	data << "Discriminator threshold "<<(thethreshold)<<" mV"<<std::endl;
	data << "timestamp, ch1 reading, ch1 rate, ch2 reading, ch2 rate, ch3 reading, ch3 rate, ch4 reading, ch4 rate"<<std::endl;
	// Execute: MAIN LOOP
	////////////scrambled egg code part 2////////////
	//std::vector<int> thresholds{30,40,50,60,70,80,90,100,200,300,400};
	//reps=thresholds.size();
	for( int count=0; count < reps; count++)  // loop over readings
	{
		std::cout << "Clearing All Scalars" << std::endl;
		List.CC["SCA"].at(scaler_pos)->ClearAll();
		
		//std::cout<<"setting discriminator threshold to 30"<<std::endl;
		//thethreshold=thresholds.at(count);
		//List.CC["DISC"].at(0)->WriteThresholdValue(thethreshold);
		//List.CC["DISC"].at(0)->ReadThreshold(thethreshold);
		//std::cout<<"Threshold is now: "<<thethreshold<<std::endl;
		
		//waiting for one minute for counts to accumulate
		std::cout << "Measuring rates" << std::endl;
		//std::this_thread::sleep_for(std::chrono::seconds(60));
		unsigned int counttimeinmicroseconds = countmins*60.*1000000.; //1000000
		usleep(counttimeinmicroseconds);
		
		//Reading the scalars
		std::cout << "Reading Scalers and writing to file" << std::endl;
		int ret = List.CC["SCA"].at(scaler_pos)->ReadAll(scalervals);

		if(not ret < 0){   // FIXME need better error checking
			std::cout << "error reading scalers: response was " << ret << std::endl;
		}
		else {
			// get and write timestamp to file
			std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
			time_t tt;
			tt = std::chrono::system_clock::to_time_t ( time );
			std::string timeStamp = ctime(&tt);
			timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);
			data << timeStamp << ", ";
			// write scalar values and rates to file
			for(int chan=0; chan<4; chan++){
				data << scalervals[chan] << ", ";
				double darkRate = double (scalervals[chan]) / countmins;
				data << darkRate;
				if(chan<3) data << ", ";
			}
			data << std::endl;
		}
		std::cout << "Done writing to file" << std::endl;

///////////////////// debug:
//		std::cout << "Clearing All Scalars" << std::endl;
//		List.CC["SCA"].at(scaler_pos)->ClearAll();
//		std::cout << "zero'd values are: " << std::endl;
//		for(int chan=0; chan<4; chan++){
//			std::cout << scalervals[chan];
//			if(chan<3) std::cout << ", ";
//	}
//		data << std::endl;
//////////////////// end debug
		
		//data << "End of Loop " << count << std::endl;
		//For loop will now wait for user-specified time
		std::cout<<"Waiting "<<timeBet<<" mins to next reading"<<std::endl;
		if(timeBet>1){
			// for monitoring the program, printout every minute
			for(int count2=0; count2 < timeBet; count2++)
			{
				std::cout << (timeBet-count2) << " minutes to next reading..." << std::endl;
				//std::this_thread::sleep_for (std::chrono::seconds(1));
				double minuteinmicroseconds = 60.*1000000.;
				usleep(minuteinmicroseconds);
			}
		} else {
			double sleeptimeinmicroseconds = timeBet*60.*1000000.;
			usleep(sleeptimeinmicroseconds);
		}

	} // end of loop over readings = end of test
	
	//Test
	//std::cout << "Test Channel: " << List.CC["SCA"].at(scaler_pos)->TestChannel(3) << std::endl;
	////////////End of Scrambled Egg Code part 2////////////
	data.close();//close data file

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
	CamacCrate* ccp;
//	if (cardname == "TDC")
//	{
//		std::cout<<"TDC"<<std::endl;
//		ccp = new Lecroy3377(cardslot, config);
//	}
//	if (cardname == "ADC")
//	{
//		std::cout<<"ADC"<<std::endl;
//		ccp = new Lecroy4300b(cardslot, config);
//	}
	if (cardname == "SCA")
	{
		std::cout<<"SCA"<<std::endl;
		ccp = new Jorway85A(cardslot, config);
	}
	if (cardname == "DISC")
	{
	  std::cout<<"DISC"<<std::endl;
		ccp = new LeCroy4413(cardslot, config);
	}
	if (cardname == "CAEN")
	{
		std::cout<<"CAEN"<<std::endl;
		ccp = new CAENC117B(cardslot, config);
	}
	return ccp;
}
