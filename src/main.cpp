/* vim:set noexpandtab tabstop=4 wrap */
#include "CamacCrate.h"
#include "CAENC117B.h"
//#include "Lecroy3377.h"
//#include "Lecroy4300b.h"
#include "Jorway85A.h"
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
	///////////////////////////////////////
	// master branch: CCTrigger.h
	
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
			scaler_pos = List.CC["SCA"].size();    // we only have one scaler anyway
			std::cout << "scaler found, list pos = " << scaler_pos << std::endl;
			List.CC["SCA"].push_back(Create("SCA", Ccard.at(i), Ncard.at(i)));               //They use CC at 0
			std::cout << "constructed Jorway85A module" << std::endl;
			std::cout << "---------------------------------------------------"<<std::endl;
			std::cout << " "<<std::endl;
		}
		else if (Lcard.at(i) == "CAEN")
		{
			caen_pos = List.CC["CAEN"].size();
			std::cout << "CAENET controller found, list pos = " <<caen_pos <<std::endl;
			List.CC["CAEN"].push_back(Create("CAEN",Ccard.at(i), Ncard.at(i)));
			std::cout << "constructed CAENC117B controller module" <<std::endl;
			std::cout << "---------------------------------------------------"<<std::endl;
			std::cout << " "<<std::endl;
		}
		else std::cout << "\n\nUnkown card\n" << std::endl;
	}
	
	std::cout <<"CAENET Controller is in slot ";
	std::cout << List.CC["CAEN"].at(caen_pos)->GetSlot() << std::endl;

	//std::cout << "Primary scaler is in slot ";
	//std::cout << List.CC["SCA"].at(scaler_pos)->GetSlot() << std::endl;

	std::cout << "---------------------------------------------------"<<std::endl;
	std::cout << " "<<std::endl;
	
	//////////////////////////////////////
	// MRD branch: LeCroy.cpp - Initialise();
	//std::cout << "Clearing modules and printing the registers" << std::endl;

	for (int i = 0; i < List.CC[DC].size(); i++)
	{
		List.CC[DC].at(i)->ClearAll();
		//List.CC[DC].at(i)->GetRegister();   // not implemented for Scaler yet
		//List.CC[DC].at(i)->PrintRegRaw();   // not implemented for Scaler yet
	}
	
	// Execute: MAIN LOOP
	//for (int loopi=0; loopi<10; loopi++){
	
		//////////////////////////////////////
		// Master branch: CCTrigger.cpp - Execute();
		
		/* these were disabled - unused?
		//Clearing all the cards, using iterators
			iL = List.CC.begin();		//iL is an iterator over a map<string, vector<CamacCrate*> >
			for ( ; iL != List.CC.end(); ++iL)
			{
				iC = iL->second.begin();	//iC is an iterator over a vector<CamacCrate*>
				for ( ; iC != iL->second.end(); ++iC)
					(*iC)->ClearAll();
			}
		
		//Clearing all the cards, using indices
			for (int i = 0; i < List.CC["TDC"].size(); i++)
				List.CC["TDC"].at(i)->ClearAll();
			for (int i = 0; i < List.CC["ADC"].size(); i++)
				List.CC["ADC"].at(i)->ClearAll();
		*/
		
		// unused for PMT testing. for now, at least
//		TRG = false;
//		if(List.CC["TDC"].at(trg_pos)->TestEvent() == 1) TRG = true;  // for real (not soft/random) trigger
		
		////////////////////////////////////
		// MRD branch: Lecroy.cpp - Execute();
		
		// the ToolChain seems to have two LeCroy tools back-to-back, to read TDCs and ADCs.
		// effectively run the code once with DC="ADC", then again with DC="TDC".
//		DC = "TDC";
//		if (TRG){
//			//std::cout << "TRG on!\n" << std::endl;
//			for (int i = 0; i < List.CC[DC].size(); i++)
//			{
//				Data.ch.clear();
//				if (trg_mode == 2) List.CC[DC].at(i)->InitTest();
//				List.CC[DC].at(i)->GetData(Data.ch);
//				if (Data.ch.size() != 0)
//				{
//					List.Data[DC].Slot.push_back(List.CC[DC].at(i)->GetSlot());
//					List.Data[DC].Num.push_back(Data);
//					List.CC[DC].at(i)->ClearAll();
//				}
//			}
//		}
		
	/*	int ret = List.CC["SCA"].at(scaler_pos)->ReadAll(scalervals);
		if(not ret < 0){   // need better error checking
			std::cout << "error reading scalers: response was " << ret << std::endl;
		} else {
			std::cout << "scaler values were: ";
			for(int chan=0; chan<4; chan++){
				std::cout << scalervals[chan];
				if(chan<3) std::cout << ", ";
			}
			std::cout<<std::endl;
		}
*/
		//int ret_caen = List.CC["CAEN"].at(caen_pos)->ReadCrateOccupation();
	//	List.CC["CAEN"].at(caen_pos)->TestOperation();		
		//int ret_vmax = List.CC["CAEN"].at(caen_pos)->SetVmax(6,7,1000);
	//	List.CC["CAEN"].at(caen_pos)->SetV1(6, 0, 100);
	/*(	int readslot;
		for (int i=0;i<1;i++){
			readslot= List.CC["CAEN"].at(caen_pos)->ReadSlotN(i);
		}
*/
		int ret_test;
		for (int i_test=0;i_test<15	;i_test++){
			ret_test=List.CC["CAEN"].at(caen_pos)->TestOperation(i_test);
		}
	//	int ret_lam = List.CC["CAEN"].at(caen_pos)->EnLAM();
	//	ret_lam = List.CC["CAEN"].at(caen_pos)->EnLAM();
	//	std::cout << List.CC["CAEN"].at(caen_pos)->TestLAM()<<std::endl;

		
	//}  // end ToolAnalysis Execute / for loop
	
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
	if (cardname == "CAEN")
	{
		std::cout<<"CAEN"<<std::endl;
		ccp = new CAENC117B(cardslot, config);
	}
	return ccp;
}
