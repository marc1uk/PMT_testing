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

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h> // /usr/include/X11/extensions/XTest.h

#include "PMTTestingHVcontrol.h"
#include "zmq.hpp"

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"

#include "TApplication.h"
#include "TSystem.h"

// see CCDAQ.cpp in ANNIEDAQ:MRD branch for example on using the CAMAC internal stack
// see CCUSB.cpp in ANNIEDAQ:MRD branch for example on doing Read/Write of LeCroy cards in ToolAnalysis chain

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
struct TestVars{        // For loading test settings from config file
	public:
	std::vector<std::string> pmtNames{"Test"};    // assumed connected to ADC / discriminator inputs 0, 1, 2...
	
	std::string cooldownfilenamebase="NA";  // skip test if "NA"
	double cooldowntotalmins=20;
	double cooldowncountsecs=60;
	double cooldownloopsecs=60;
	double cooldownvoltage=1500;
	int threshold=15;
	int thresholdmode=0;
	
	std::string gainfilenamebase="gain";
	std::vector<double> gainVoltages={1500};
	int gainacquisitions=10000;
	bool gainliveplot=false;
	int gainliveplotchannel=0;
	int gainliveplotfreq=500;
	int gainprintfreq=500;
};

// convenience struct for passing related parameters
struct KeyPressVars{
	public:
	Display* display=nullptr;
	Window winRoot;
	Window winFocus;
	int    revert;
};

// SUPPORT FUNCTIONS
short SetRegBits(CamacCrate* CC, int regnum, int firstbit, int numbits, bool on_or_off);
void PrintReg(CamacCrate* CC,int regnum);
CamacCrate* Create(std::string cardname, std::string config, int cardslot);
int LoadConfigFile(std::string configfile, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, std::vector<int> &Ncard);
int ConstructCards(Module &List, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, std::vector<int> &Ncard);
int DoCaenTests(Module &List);
int SetupWeinerSoftTrigger(CamacCrate* CC, Module &List);
int ReadRates(Module &List, double countsecs, std::ofstream &data);
int IntializeADCs(Module &List, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, bool defaultinit);
int WaitForAdcData(Module &List);
int ReadAdcVals(Module &List, std::map<int, int> &ADCvals);
int LoadTestSetup(std::string configfile, TestVars thesettings);
XKeyEvent createKeyEvent(Display *display, Window &win, Window &winRoot, bool press, int keycode, int modifiers);
void KeyPressInitialise(KeyPressVars thevars);
void DoKeyPress(int KEYCODE, KeyPressVars thevars);
void KeyPressFinalise(KeyPressVars thevars);
void RunWavedump(std::promise<int> finishedin);
int LoadWavedumpFile(std::string filepath, std::vector<std::vector<std::vector<double>>> &data);
int MeasureIntegralsFromWavedump(std::string waveformfile, std::vector<TBranch*> branches);

// MEASUREMENT FUNCTIONS
int MeasureScalerRates(Module &List, TestVars testsettings, bool append);
int MeasurePulseHeightDistribution(Module &List, CamacCrate* CC, TestVars testsettings, std::string outputfilename);
int MeausrePulseHeightDistributionDigitizer(CamacCrate* CC, KeyPressVars thekeypressvars, TestVars testsettings);

// CONSTANTS
unsigned int masks[] = {0x01, 0x02, 0x04, 0x08, 
												0x10, 0x20, 0x40, 0x80, 
												0x100, 0x200, 0x400, 0x800, 
												0x1000, 0x2000, 0x4000, 0x8000, 
												0x10000, 0x20000, 0x40000, 0x80000, 
												0x100000, 0x200000, 0x400000, 0x800000, 
												0x1000000, 0x2000000, 0x4000000, 0x8000000,
												0x10000000, 0x20000000, 0x40000000, 0x80000000};

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

// ***************************************************************************
//                             START MAIN PROGRAM
// ***************************************************************************

int main(int argc, char* argv[]){
	
	// ***************************************
	// TEST SETUP ARGS
	// ***************************************
	std::string testconfigfile;
	if(argc!=2){
		std::cout<<"Usage: ./main configfile"<<std::endl;
		return 0;
	}
	for (int i_arg=1;i_arg<argc;i_arg++){
		     if (i_arg==1) testconfigfile = argv[i_arg];
//		else if (i_arg==2) i_channel = (int)*argv[i_arg]-'0';
//		else if (i_arg==3) numacquisitions=atoi(argv[i_arg]);
	}
	TestVars testsettings;
	int setupok = LoadTestSetup(testconfigfile, testsettings);
	
	// Open a ROOT TApplication for live plotting
	// ==========================================
	int myargc = 1;
	char arg1[] = "potato";
	char* myargv[]={arg1};
	TApplication *PMTTestStandApp = new TApplication("PMTTestStandApp",&myargc,myargv);
	
	// Get the X11 display for sending keypresses to other programs
	// ============================================================
	KeyPressVars thekeypressvars;
	KeyPressInitialise(thekeypressvars);
	
	// Variables for holding the CamacCrate and derived class objects
	// ==============================================================
	std::vector<std::string> Lcard, Ccard;
	std::vector<int> Ncard;
	Module List;                                                   //Data Model for Lecroys
	
	// read module configuration file
	// ==============================
	std::string configcc = "configfiles/ModuleConfig"; // card list file
	int configok = LoadConfigFile(configcc, Lcard, Ccard, Ncard);
	if(not configok){
		std::cerr << "Failed to load card configuration from file " << configcc << std::endl;
		return 0;
	}
	std::cout << "Number of cards: " << Lcard.size() << std::endl;
	CamacCrate* CC = new CamacCrate;                         // CamacCrate at 0
	
	// Create card objects based on configuration read from file
	// =========================================================
	int constructok = ConstructCards(List, Lcard, Ccard, Ncard);
	if(not constructok){
		std::cerr << "Error during card object construction" << std::endl;
		return 0;
	}
	
	// CAEN C117B tests
	// ================
	//int caen_test_result = DoCaenTests(List);
	
	// Create class for doing ZMQ control of HV
	// ========================================
	PMTTestingHVcontrol* hvcontrol = new PMTTestingHVcontrol();
	
	// Set up CCUSB NIM output for triggering
	// ======================================
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;   // Modify bit 1 of the register to "1" (CCUSB Trigger)
	int weiner_softrigger_setup_ok = SetupWeinerSoftTrigger(CC, List);
	
	// Set Discriminator threshold
	// ===========================
	if(testsettings.thresholdmode!=0){
		List.CC["DISC"].at(0)->EnableProgrammedThreshold();
		List.CC["DISC"].at(0)->WriteThresholdValue(testsettings.threshold);
	}
	else List.CC["DISC"].at(0)->EnableFPthreshold();
	
	// Run cooldown test
	// =================
	if(testsettings.cooldownfilenamebase!="NA"){
		bool appendtofile=false;  // append/overwrite any existing file. TODO ALERT USER IF FILE EXISTS
		int cooldownok = MeasureScalerRates(List, testsettings, appendtofile);
	}
	
	// =========
	// MAIN LOOP
	// =========
	int command_ok;
	for (int i = 0; i < testsettings.gainVoltages.size(); i++)    // loop over readings
	{
		//break;
		
		// Set voltages
		// ============
		double testvoltage = testsettings.gainVoltages.at(i);
		hvcontrol->SetVoltage(testvoltage);  // XXX XXX XXX
		
		// Do Gain Measurement
		// ===================
		char voltageasstring[10];
		sprintf(voltageasstring, "%f", testvoltage);
		std::string gainfilename = testsettings.gainfilenamebase + std::string(voltageasstring)+"V";
		
		// version that uses the QDC to measure pulse integral
		// this fires the LED a given number of times, reads the pulse integrals and writes them to file
		//int gainmeasureok = MeasurePulseHeightDistribution(List, CC, testsettings, gainfilename);
		// saves data to a txt file, would need integration from plotQDChisto.cpp
		
		// alternative version using the UCDavis digitizer
		// this also fires the LED a given number of times, reads the waveforms and writes them to file
		int gainmeasureok = MeausrePulseHeightDistributionDigitizer(CC, thekeypressvars, testsettings);
		
		// Create a ROOT file to save the pulse height distribution data
		gainfilename += ".root";
		TFile* rootfout = new TFile(gainfilename.c_str(),"RECREATE");
		TTree* roottout = new TTree("pmt","PMT Test Results");
		std::vector<TBranch*> branchpointers;
		int pulseintegral;
		TBranch* theaddress=nullptr;
		branchpointers.push_back(roottout->Branch("Channel0",&pulseintegral));
		branchpointers.push_back(roottout->Branch("Channel1",&pulseintegral));
		branchpointers.push_back(roottout->Branch("Channel2",&pulseintegral));
		branchpointers.push_back(roottout->Branch("Channel3",&pulseintegral));
		
		// read the waveforms, subtract pedestal and calculate the integral. Fill into the tree.
		int calculateintegralsok = MeasureIntegralsFromWavedump("wave0.txt", branchpointers);
		
		// we call Fill on branches, so need to manually set the number of TTree entries
		roottout->SetEntries(branchpointers.at(0)->GetEntries());
		roottout->Write();
		rootfout->Close();
		delete rootfout;       // also cleans up roottout
		
		
	}
	
	std::cout<<"End of data taking"<<std::endl;
	
	// Cleaup X11 stuff for sending keypresses
	// =======================================
	std::cout<<"Cleaning up X11"<<std::endl;
	KeyPressFinalise(thekeypressvars);
	
	Lcard.clear();
	Ncard.clear();
}

// ***************************************************************************
//                             END MAIN PROGRAM
// ***************************************************************************

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

// ***************************************************************************
//                             PULSE RATE TEST
// ***************************************************************************
int MeasureScalerRates(Module &List, TestVars testsettings, bool append)
{
	// ***************************************
	// COOLDOWN MEASUREMENT ARGS
	// ***************************************
	// extract arguments from test config struct
	std::string outputfilename = testsettings.cooldownfilenamebase;
	int totalmins = testsettings.cooldowntotalmins;
	double countsecs = testsettings.cooldowncountsecs;
	double loopsecs = testsettings.cooldownloopsecs;
	std::vector<std::string> PMTNames = testsettings.pmtNames;
	int nummeasurements = totalmins/(loopsecs/60.);
	double waitmins = (loopsecs-countsecs)/60.;
	
	std::cout << "The scalers will be read " << nummeasurements;
	std::cout << " times with " << countsecs << " seconds used to determine scalar rate and ";
	std::cout << waitmins << " minutes of wait time between readouts\n";
	// ***************************************
	
	// ***************************************
	// COOLDOWN MEASUREMENT SETUP
	// ***************************************
	//Open up a new file
	std::string CooldownFileName = outputfilename+".csv";
	std::ofstream data;
	std::ios_base::openmode writemode = (append) ? std::ofstream::app : std::ofstream::out;
	data.open (CooldownFileName.c_str(), writemode);
	data << "PMT_IDs"<<std::endl;
	for(int pmti=0; pmti<PMTNames.size(); pmti++){
		data << PMTNames.at(pmti) << ", ";
	}
	data << std::endl;
	int thethreshold;
	List.CC["DISC"].at(0)->ReadThreshold(thethreshold);
	data << "Discriminator threshold "<<(thethreshold)<<" mV"<<std::endl
			 << "timestamp, ch1 reading, ch1 rate, ch2 reading, ch2 rate, "
			 << "ch3 reading, ch3 rate, ch4 reading, ch4 rate"<<std::endl;
	
	// just for trials
	//std::cout << "Test Channel: " << List.CC["SCA"].at(0)->TestChannel(3) << std::endl;
	//std::vector<int> thresholds{30,40,50,60,70,80,90,100,200,300,400};
	//nummeasurements=thresholds.size();
	// ***************************************

	for(int i=0; i<nummeasurements; i++){
		// ***************************************
		// COOLDOWN MEASUREMENT
		// ***************************************
		// threshold trials
		// ----------------
		//std::cout<<"setting discriminator threshold to 30"<<std::endl;
		//thethreshold=thresholds.at(i);
		//List.CC["DISC"].at(0)->WriteThresholdValue(thethreshold);
		//List.CC["DISC"].at(0)->ReadThreshold(thethreshold);
		//std::cout<<"Threshold is now: "<<thethreshold<<std::endl;
		
		// Read rates and write to file
		// ----------------------------
		int readscalerok = ReadRates(List, countsecs, data);  // TODO read >1 scaler?
		
		// ***************************************
		// COOLDOWN MEASUREMENT DELAY
		// ***************************************
		//For loop will now wait for user-specified time
		if(waitmins==0) continue;
		std::cout<<"Waiting "<<waitmins<<" mins to next reading"<<std::endl;
		if(waitmins>1){
			// for monitoring the program, printout every minute
			for(int count2=0; count2 < waitmins; count2++){
				std::cout << (waitmins-count2) << " minutes to next reading..." << std::endl;
				//std::this_thread::sleep_for (std::chrono::seconds(1));
				double minuteinmicroseconds = 60.*1000000.;
				usleep(minuteinmicroseconds);
			}
		} else {
			double sleeptimeinmicroseconds = waitmins*60.*1000000.;
			usleep(sleeptimeinmicroseconds);
		}
	}
	
	// ***************************************
	// COOLDOWN CLEANUP
	// ***************************************
	data.close();    //close data file
	return 1;
}

// ***************************************************************************
//               PULSE HEIGHT DISTRIBUTION TEST - Digitizer Ver
// ***************************************************************************

int MeausrePulseHeightDistributionDigitizer(CamacCrate* CC, KeyPressVars thekeypressvars, TestVars testsettings){
	
	// Run wavedump in an external thread so it can execute while we continue and send keys to it
	// first create a promise we can use to hold until the external thread is done
	std::promise<int> barrier;
	std::future<int> barrier_future = barrier.get_future();
	
	// start the thread
	std::cout<<"starting wavedump"<<std::endl;
	std::thread athread(RunWavedump, std::move(barrier));
	
	// wait for just a little bit for wavedump to initialize
	std::this_thread::sleep_for(std::chrono::seconds(3));
	
	// configure it
	std::cout<<"enabling continuous saving"<<std::endl;
	int KEYCODE = XK_W; // XK_s sends lower case s etc.
	DoKeyPress(KEYCODE, thekeypressvars);
	
	// start it
	std::cout<<"starting acquisition"<<std::endl;
	KEYCODE = XK_s; // XK_s sends lower case s etc.
	DoKeyPress(KEYCODE, thekeypressvars);
	
	// we need to fire the LED (which will also trigger acquisition)
	// Get register settings with soft-trigger bit (un)set
	// ===================================================
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;   // Modify bit 1 of the register to "1" (CCUSB Trigger)
	
	for(int triggeri=0; triggeri<testsettings.gainacquisitions; triggeri++){
		// Fire LED! (and gate ADCs)
		// =========================
		int command_ok = CC->ActionRegWrite(RegActivated);
		//std::cout<<"Firing LED " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		usleep(1000); // wait for 1ms  XXX may need tuning
	}
	
	// stop it
	std::cout<<"stopping acquisition"<<std::endl;
	KEYCODE = XK_s; // XK_s sends lower case s etc.
	DoKeyPress(KEYCODE, thekeypressvars);
	
	// close it
	std::cout<<"quitting wavedump"<<std::endl;
	KEYCODE = XK_q; // XK_s sends lower case s etc.
	DoKeyPress(KEYCODE, thekeypressvars);
	
	// wait for the external thread to indicate completion
	std::cout<<"waiting for external thread to complete"<<std::endl;
	std::chrono::milliseconds span(100);
	std::future_status thestatus;
	do {
		thestatus = barrier_future.wait_for(span);
	} while (thestatus==std::future_status::timeout);
	// get the return to know if the external thead succeeded
	std::cout << "thread returned " << barrier_future.get() << std::endl;
	
	// call join to cleanup the external thread
	athread.join();
	
	return 1;
}

// ***************************************************************************
//                  PULSE HEIGHT DISTRIBUTION TEST - QDC Ver
// ***************************************************************************

int MeasurePulseHeightDistribution(Module &List, CamacCrate* CC, TestVars testsettings, std::string outputfilename){
	
	// ***************************************
	// GAIN MEASUREMENT ARGS
	// ***************************************
	// Extract arguments from test settings
	int numacquisitions = testsettings.gainacquisitions;
	std::vector<std::string> PMTNames = testsettings.pmtNames;
	bool liveplot = testsettings.gainliveplot;
	int plot_channel = testsettings.gainliveplotchannel;
	int printfreq = testsettings.gainliveplotfreq;
	int plotfreq = testsettings.gainprintfreq;
	
	std::cout << "The QDCs will be read " << numacquisitions<<" times"<<std::endl;
	
	// Open File for ADC readouts
	// ==========================  // TODO check if file exists!!!
	std::string ADCFileName = outputfilename+".csv";
	std::ofstream ADCRead;
	ADCRead.open(ADCFileName.c_str());
	ADCRead << "PMT_IDs"<<std::endl;
	for(int pmti=0; pmti<PMTNames.size(); pmti++){
		ADCRead << PMTNames.at(pmti) << ", ";
	}
	ADCRead << std::endl;
	
	// Open the ROOT canvas for live plotting
	// ======================================
	TCanvas* canv;
	TH2D* hist;
	if(liveplot){
		canv = new TCanvas("canv","canv",900,600);
		canv->cd();
		hist = new TH2D("hist","ADC values : entry",100,0,numacquisitions,100,10,100);
	}
	
	// Get register settings with soft-trigger bit (un)set
	// ===================================================
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;   // Modify bit 1 of the register to "1" (CCUSB Trigger)
	long RegDeactivated = RegStore & ~0x02;  // Modify bit 1 to 0 (Deassert CCUSB trigger. Not strictly needed)
	
	// =========
	// MAIN LOOP
	// =========
	int command_ok;
	for (int i = 0; i < numacquisitions; i++)    // loop over readings
	{
		//break;
		
		// ***************************************
		// GAIN MEASUREMENT
		// ***************************************
		
		//int adcinitializeok = IntializeADCs(List, Lcard, Ccard);     // Register trials. Debug only.
		
		// Clear ADCs
		// ==========
		//std::cout<<"clearing ADCs"<<std::endl;
		for (int cardi = 0; cardi < List.CC["ADC"].size(); cardi++){
			command_ok = List.CC["ADC"].at(cardi)->ClearAll();
			//std::cout<<"Clear module " << cardi <<": " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		}
		usleep(100);
		
		// Fire LED! (and gate ADCs)
		// =========================
		command_ok = CC->ActionRegWrite(RegActivated);
		//std::cout<<"Firing LED " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		
		// Or run internal test
		// ====================
		// This connects internal charge generator and pulses the gate. Values are ready to read.
		//for (int cardi = 0; cardi < List.CC["ADC"].size(); cardi++){
		//	command_ok = List.CC["ADC"].at(cardi)->InitTest();
		//	std::cout<<"test start "<<i<<" " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		//}
		
		usleep(1000);
		
		// Poll ADCs for data availability
		// ===============================
		int adcsgotdata = WaitForAdcData(List);
		
		// Put timestamp in file
		// =====================
		std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
		time_t tt;
		tt = std::chrono::system_clock::to_time_t ( time );
		std::string timeStamp = ctime(&tt);
		timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);
		ADCRead << timeStamp;
		
		// Read ADC values
		// ===============
		//std::cout<<"Reading ADCvals at " << timeStamp<<std::endl;
		 std::map<int, int> ADCvals;
		 int gotadcdata = ReadAdcVals(List, ADCvals);
		
		// Put ADC values into file
		// ========================
		for( std::map<int,int>::iterator aval = ADCvals.begin(); aval!=ADCvals.end(); aval++){
			ADCRead << ", ";
			ADCRead << aval->second;
			if (i%printfreq==0) {
				std::cout << /*aval->first << "=" <<*/ aval->second;
				if(aval!=ADCvals.begin()) std::cout<<", ";
			}
		}
		ADCRead << std::endl;
		if (i%printfreq==0) { std::cout<<std::endl; }
		
		// Plot the data for live view
		// ===========================
		if(liveplot){
			hist->Fill(i,ADCvals.at(plot_channel));
			if (i%plotfreq==0){
				hist->Draw("colz");
				canv->Modified();
				canv->Update();
				gSystem->ProcessEvents();
			}
		}
	}
	
	std::cout<<"End of ADC measurements, closing files"<<std::endl;
	
	// Save PDF version of histogram
	// =============================
	if(liveplot){
		std::stringstream ss_channel;
		ss_channel<<plot_channel;
		std::string str_channel = ss_channel.str();
		std::string adc_pdf = ADCFileName+"_ch"+str_channel+"_stability.pdf";
		canv->SaveAs(adc_pdf.c_str());
	}
	
	// ***************************************
	// GAIN CLEANUP
	// ***************************************
	ADCRead.close();
	
	if(liveplot){
		canv->Clear();
		if(hist){ delete hist; hist=nullptr; }
		if(canv){ delete canv; canv=nullptr; }
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

// ***************************************************************************
//                             SUPPORT FUNCTIONS
// ***************************************************************************

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
				List.CC[Lcard.at(i)].push_back(cardPointer);  //They use CC at 0
				std::cout << "constructed Lecroy 4300b module" << std::endl;
			}
		}
		else if (Lcard.at(i) == "SCA")
		{
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
			List.CC["CAEN"].push_back(Create("CAEN",Ccard.at(i), Ncard.at(i)));
			std::cout << "constructed CAENC117B controller module" <<std::endl;
			std::cout << " "<<std::endl;
		}
		else std::cout << "\n\nUnkown card\n" << std::endl;
	}
	
	//std::cout << "Primary scaler is in slot ";
	//std::cout << List.CC["SCA"].at(0)->GetSlot() << std::endl;
	
	return 1;
}

// ***************************************
// CAEN C117B TESTS
// ***************************************
int DoCaenTests(Module &List){
	std::cout <<"CAENET Controller is in slot ";
	std::cout << List.CC["CAEN"].at(0)->GetSlot() << std::endl;
	
	// Random functionality trials
	int ret_caen = List.CC["CAEN"].at(0)->ReadCrateOccupation();
	int crateN = 0; // ???? 
	List.CC["CAEN"].at(0)->TestOperation(crateN);
	int ret_vmax = List.CC["CAEN"].at(0)->SetVmax(6,7,1000);
	List.CC["CAEN"].at(0)->SetV1(6, 0, 100);
	int readslot;
	for (int i=0;i<1;i++){
		readslot= List.CC["CAEN"].at(0)->ReadSlotN(i);
	}
	int ret_test;
	for (int i_test=0; i_test<15; i_test++){
		ret_test=List.CC["CAEN"].at(0)->TestOperation(i_test);
	}
	int ret_lam = List.CC["CAEN"].at(0)->EnLAM();
	std::cout << List.CC["CAEN"].at(0)->TestLAM()<<std::endl;
	
	return 1;
}

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
	
	return 1;// todo error checking
}

// ***************************************
// MEASURE PULSE RATE WITH SCALER
// ***************************************
int ReadRates(Module &List, double countsecs, std::ofstream &data){
	std::cout << "Reading Scaler Rates" << std::endl;
	int scalervals[4];
	
	// clear counters
	List.CC["SCA"].at(0)->ClearAll();
	///////////////////// debug:
	//List.CC["SCA"].at(0)->ReadAll(scalervals);
	//std::cout << "zero'd values are: " << std::endl;
	//for(int chan=0; chan<4; chan++){
	//	std::cout << scalervals[chan];
	//	if(chan<3) std::cout << ", ";
	//}
	//////////////////// end debug
	
	//waiting for requested duration for counts to accumulate
	//std::this_thread::sleep_for(std::chrono::seconds(60));       // sleep_for supported on annielx01 g++ ver
	unsigned int counttimeinmicroseconds = countsecs*60.*1000000.; //1000000
	usleep(counttimeinmicroseconds);
	
	//Read the scalars
	int ret = List.CC["SCA"].at(0)->ReadAll(scalervals);
	if(not (ret<0)){
			std::cerr << "error reading scalers: response was " << ret << std::endl;
			return 0;
	} else {
		// get and write timestamp to file
		std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
		time_t tt;
		tt = std::chrono::system_clock::to_time_t ( time );
		std::string timeStamp = ctime(&tt);
		timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);  // trim any trailing whitespace/EOL
		data << timeStamp << ", ";
		// write scalar values and rates to file
		for(int chan=0; chan<4; chan++){
			data << scalervals[chan] << ", ";
			double darkRate = double (scalervals[chan]) / countsecs;
			data << darkRate;
			if(chan<3) data << ", ";
		}
	}
	data << std::endl;
	//std::cout << "Done writing to file" << std::endl;
	
	return 1;// todo error checking
}

// ***************************************
// RE-SET ADC REGISTERS
// ***************************************
int IntializeADCs(Module &List, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, bool defaultinit=false){
		
		// CC->Z() initializes (resetting all settings to default) ALL CARDS IN THE CRATE!
		// This probably also applies when calling on cards... (confirm)
		// After calling Z() ALL CARDS (not just ADCs) in the crate must be reconfigured!
		if(defaultinit){
			std::cout<<"Initializing ADCs."<<std::endl;
			std::cout<<"WARNING! This operation may default-configure all cards in the crate!"<<std::endl;
			for (int i = 0; i < List.CC["ADC"].size(); i++){  // loop is probably redundant
				int command_ok = List.CC["ADC"].at(i)->Z();
				if(command_ok<0) std::cerr<<"4300B::Z() for card "<<i<<" returned ret<0"<<std::endl;
				
//			// Debug checks: check the registers have been reinitialized
//			std::cout<<"Printing intialized register - expect top 7 bits to be '1'"<<std::endl;
//			// check the initialize worked: bits 9-15 should be set to 1 by a Z() command 
//			List.CC["ADC"].at(i)->GetRegister();  // so check it's set as expected;
//			List.CC["ADC"].at(i)->DecRegister();
//			List.CC["ADC"].at(i)->PrintRegister();
			}
		}
		
		// find the configuration file for ADC cards
		int configi=-1;
		for(; configi < Lcard.size(); configi++){
			if(Lcard.at(configi) == "ADC"){
				break;
			}
		}
		if(configi<0){
			std::cerr<<"could not find ADC config file in list of config files?"<<std::endl;
			return 0;
		}
		
		// Configure register with settings from config file
		for (int i = 0; i < List.CC["ADC"].size(); i++){
			//std::cout<<"configuring 4300B from file "<<Ccard.at(configi)<<std::endl;
			List.CC["ADC"].at(i)->SetConfig(Ccard.at(configi)); // load ECL, CLE etc. members with desired vals
			List.CC["ADC"].at(i)->EncRegister();                // encode ECL, CLE etc into Control member
			//std::cout<<"Writing following register contents"<<std::endl;
			//List.CC["ADC"].at(i)->PrintRegister();              // print ECL, CLE etc members
			List.CC["ADC"].at(i)->SetRegister();                // write Control member to the card
			usleep(1000);
			//std::cout<<"Read back the following register contents"<<std::endl;
			//List.CC["ADC"].at(i)->GetRegister();                // read from the card into Control member
			//List.CC["ADC"].at(i)->DecRegister();                // decode 'Control' member into ECL, CLE etc. members
			//List.CC["ADC"].at(i)->PrintRegister();              // print ECL, CLE etc members
			//usleep(1000); // sleep 1ms, and rest 
		}
		
		return 1; // todo error checking
}

// ***************************************
// POLL ADCs FOR BUSY SIGNAL
// ***************************************
int WaitForAdcData(Module &List){
	
	// if something failed in the triggering, the ADCs may not have receieved a gate signal
	// or potentially if 'GetData' is called too soon after gating the ADCs, the data may not be ready
	// poll the card to confirm data availability 
	
	for (int cardi = 0; cardi < List.CC["ADC"].size(); cardi++){
		int numchecks=2;
		int statusreglast=0;
		for(int checknum=0; checknum<numchecks; checknum++){
			// now check for the 'busy' signal?
			int statusreg=0, busy=0, funcok=0;
			int command_ok = List.CC["ADC"].at(cardi)->READ(0, 0, statusreg, busy, funcok);
			if(not funcok) std::cerr<<"X=0, Function 0 (read register) not recognised??"<<std::endl;
			// register contents are 0 when BUSY
			//if(not statusreg) std::cerr<<"Status register = "<<statusreg
			//													 <<", Camac LAM should at least be valid"<<std::endl;
			if(command_ok<0) std::cerr<<"ret<0. Whatever that means"<<std::endl;
			if(not busy){ /*std::cout<<"Q=0; BUSY is ON!"<<std::endl;*/ break; }
			else if(checknum==numchecks-1){
				std::cerr<<"ADC "<<cardi<< " BUSY was not raised following Gate / InitTest?" << std::endl;
				return 0;
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
	
	return 1;
}

// ***************************************
// READ ADC DATA INTO MAP
// ***************************************
int ReadAdcVals(Module &List, std::map<int, int> &ADCvals){
	
	//std::cout<<"Reading ADCvals at " << timeStamp<<std::endl;
	for (int cardi = 0; cardi < List.CC["ADC"].size(); cardi++){
		
		// Debug: check register settings
		// ------------------------------
		//List.CC["ADC"].at(cardi)->PrintRegister();
		//List.CC["ADC"].at(cardi)->GetRegister();
		//List.CC["ADC"].at(cardi)->DecodeRegister();
		//List.CC["ADC"].at(cardi)->PrintRegister();
		//List.CC["ADC"].at(cardi)->PrintPedestal();
		// ReadOut behaviour depends on config file. If CSR (camac sequential readout) is set to 1, 
		// channel input is ignored, all 16 channels are returned by subsequent 'ReadOut' calls.
		// Otherwise if CSR = 0, channel input is used.
		// Or: dump all channels into a map - simply loops over ReadAll calls. Should work with CSR=0 or 1
		
		// Get the data from ADC
		// ----------------------
		//// manual loop ver
		//for(int channeli=0; channeli<16; channeli++){
		//	int statusreg=0, busy=0, funcok=0;
		//	//:READ(int A, int F, int &Data, int &Q, int &X)
		//	std::cout<<"Reading channel "<<channeli<<std::endl;
		//	command_ok = List.CC["ADC"].at(cardi)->READ(channeli, 2, statusreg, busy, funcok);  // F=2, A=channeli
		//	if(not funcok) std::cerr<<"X=0, Function 0 (read register) not recognised??"<<std::endl;
		//	if(not statusreg) std::cerr<<"Data = 0. :("<<std::endl;
		//	else std::cout<<"SUCCESS: DATA["<<channeli<<"] = "<<statusreg<<std::endl;
		//	if(command_ok<0) std::cerr<<"ret<0. Whatever that means"<<std::endl;
		//	if(not busy){ std::cout<<"Q=0; No Data?"<<std::endl; usleep(1000); }
		//}
		
		// GetData calls DumpAll(ADCvals); or DumpCompressed based on register settings
		List.CC["ADC"].at(cardi)->GetData(ADCvals);
		if(ADCvals.size()==0){
			std::cerr<<"ADC "<<cardi<< " GetData returned no measurements!"<<std::endl;
			return 0;
		}
	}
	
	return 1;
}

int LoadTestSetup(std::string configfile, TestVars thesettings){
	std::ifstream fin (configfile.c_str());
	std::string Line;
	std::stringstream ssL;
	
	std::string sEmp;
	std::string iEmp;
	bool settingPmtNames=false;
	bool settingGainVoltages=false;
	thesettings.pmtNames.clear();
	while (getline(fin, Line))
	{
	  //std::cout<<"conf 2"<<std::endl;
		if (Line.empty()) continue;
		if (Line.find("StartPmtNames") != std::string::npos) settingPmtNames = true;
		if (Line.find("EndPmtNames") != std::string::npos) settingPmtNames = false;
		if (Line.find("StartGainVoltages") != std::string::npos) settingGainVoltages = true;
		if (Line.find("EndGainVoltages") != std::string::npos) settingGainVoltages = false;
		
		if (Line[0] == '#') continue;
		else
		{
			Line.erase(Line.begin()+Line.find('#'), Line.end());
			ssL.str("");
			ssL.clear();
			ssL << Line;
			if (ssL.str() != "")
			{
				ssL >> sEmp >> iEmp;
				// Read internal parameters from file here...
				     if (sEmp == "CooldownFileNameBase") thesettings.cooldownfilenamebase = iEmp;
				else if (sEmp == "CooldownTotalMins") thesettings.cooldowntotalmins = stof(iEmp);
				else if (sEmp == "CooldownCountSecs") thesettings.cooldowncountsecs = stof(iEmp);
				else if (sEmp == "CooldownLoopSecs") thesettings.cooldownloopsecs = stof(iEmp);
				else if (sEmp == "CooldownVoltage") thesettings.cooldownvoltage = stof(iEmp);
				else if (sEmp == "Threshold") thesettings.threshold = stoi(iEmp);
				else if (sEmp == "ThresholdMode") thesettings.thresholdmode = stoi(iEmp); // potentiometer or programmed
				
				else if (sEmp == "GainFileNameBase") thesettings.gainfilenamebase = iEmp;
				else if (sEmp == "GainNumAcquisitions") thesettings.gainacquisitions = stoi(iEmp);
				else if (sEmp == "GainLivePlot") thesettings.gainliveplot = (iEmp=="1");
				else if (sEmp == "GainLivePlotChannel") thesettings.gainliveplotchannel = stoi(iEmp);
				else if (sEmp == "GainLivePlotFreq") thesettings.gainliveplotfreq = stoi(iEmp);
				else if (sEmp == "GainPrintFreq") thesettings.gainprintfreq = stoi(iEmp);
				
				else if(settingPmtNames){
					if(thesettings.pmtNames.size()>16){
						std::cerr<<"WARNING: Too many PMT names specified!! Ignoring ID for PMT "
										<<thesettings.pmtNames.size()<<"!"<<std::endl;
					} else {
						thesettings.pmtNames.push_back(iEmp);
					}
				}
				
				else if(settingGainVoltages){
					thesettings.gainVoltages.push_back(stof(iEmp));
				}
				
				else std::cerr<<"WARNING: Test Setup ignoring unknown configuration option \""<<sEmp<<"\""<<std::endl;
			}
		}
	}
	fin.close();
	return 1;
}

// used to send keys to wavedump
// Function to create a keyboard event
XKeyEvent createKeyEvent(Display *display, Window &win,
                           Window &winRoot, bool press,
                           int keycode, int modifiers)
{
   XKeyEvent event;
   
   event.display     = display;
   event.window      = win;
   event.root        = winRoot;
   event.subwindow   = None;
   event.time        = CurrentTime;
   event.x           = 1;
   event.y           = 1;
   event.x_root      = 1;
   event.y_root      = 1;
   event.same_screen = True;
   event.keycode     = XKeysymToKeycode(display, keycode);
   event.state       = modifiers;
   
   if(press)
      event.type = KeyPress;
   else
      event.type = KeyRelease;
   
   return event;
}

void KeyPressInitialise(KeyPressVars thevars){
		// Obtain the X11 display.
	thevars.display = XOpenDisplay(0);
	if(thevars.display == NULL){
		std::cerr<<"Could not open X Display!"<<std::endl;
		return;
	}
	// Note multiple calls to XOpenDisplay() with the same parameter return different handles;
	// sending the press event with one handle and the release event with another will not work.
	
	// Get the root window for the current display.
	thevars.winRoot = XDefaultRootWindow(thevars.display);
	
	// Prevent keydown-keyup generating multiple keypresses due to autorepeat
	XAutoRepeatOff(thevars.display);
	// XXX THIS AFFECTS THE GLOBAL X SETTINGS AFTER THE PROGRAM EXITS!!! XXX
	// RE-ENABLE BEFORE CLOSING THE PROGRAM TO REVERT TO NORMAL BEHAVIOUR! XXX
	
	// Find the window which has the current keyboard focus.
	XGetInputFocus(thevars.display, &thevars.winFocus, &thevars.revert);
}

void DoKeyPress(int KEYCODE, KeyPressVars thevars){
	XKeyEvent event = createKeyEvent(thevars.display, thevars.winFocus, thevars.winRoot, true, KEYCODE, 0);
	XTestFakeKeyEvent(event.display, event.keycode, True, CurrentTime);
	XGetInputFocus(thevars.display, &thevars.winFocus, &thevars.revert);
	XTestFakeKeyEvent(event.display, event.keycode, False, CurrentTime);
}

void KeyPressFinalise(KeyPressVars thevars){
	// Re-enable auto-repeat
	XAutoRepeatOn(thevars.display);
	
	// Cleanup X11 display
	XCloseDisplay(thevars.display);
}


void RunWavedump(std::promise<int> finishedin){
	
	// if we need to pass between member functions in classes, must use std::move
	// otherwise we can just call set_value on finishedin
	std::promise<int> finished;
	finished = std::promise<int>(std::move(finishedin));
	
	// start the external program, which will wait for a keypress then return
	const char* command = "wavedump";
	std::cout<<"calling command "<<command<<std::endl;
	system(command);
	std::cout<<"returned from command"<<std::endl;
	
	// release the barrier, allowing the caller to continue
	finished.set_value(1);
}

int LoadWavedumpFile(std::string filepath, std::vector<std::vector<std::vector<double>>> &data){
	
	// data is a 3-deep vector of: readout, channel, datavalue
	std::ifstream fin (filepath.c_str());
	std::string Line;
	std::stringstream ssL;
	
	int value;
	int readout=0;
	int channel=0;
	int recordlength;
	std::string dummy1, dummy2;
	bool dataline=false;
	
	std::cout<<"Loading data"<<std::endl;
	while (getline(fin, Line))
	{
		if (Line.empty()) continue;
		//if (Line[0] == '#') continue;
		if (Line.find("Record") != std::string::npos){
			// start of new readout
			ssL.str("");
			ssL.clear();
			ssL << Line;  // line should be of the form "Record Length: 1030"
			if (ssL.str() != "")
			{
				ssL >> dummy1 >> dummy2 >> recordlength;
			}
			dataline=false;  // mark that we're reading the header
		}
		else if (Line.find("Channel") != std::string::npos){
			ssL.str("");
			ssL.clear();
			ssL << Line;  // line should be of the form "Channel: 0"
			if (ssL.str() != "")
			{
				ssL >> dummy1 >> channel;
			}
		}
		else if (Line.find("Event") != std::string::npos){
			ssL.str("");
			ssL.clear();
			ssL << Line;  // line should be of the form "Event Number: 1"
			if (ssL.str() != "")
			{
				ssL >> dummy1 >> dummy2 >> readout;
			}
		}
		else if (Line.find("offset") != std::string::npos){
			// last header line, mark the next line as start of values
			// not necessary to actually read the line
			dataline=true;
		}
		else if( (Line.find("BoardID") != std::string::npos) ||
				 (Line.find("Pattern") != std::string::npos) ||
				 (Line.find("Trigger") != std::string::npos) ){
			// other header lines, ignore. redundant, but hey.
		}
		else if(dataline)
		{
			ssL.str("");
			ssL.clear();
			ssL << Line;
			if (ssL.str() != "")
			{
				ssL >> value;
				if(data.at(channel).size()<(readout+1)){ data.at(channel).push_back(std::vector<double>{}); }
				data.at(channel).at(readout).push_back(value);
			}
		}
	}
	fin.close();
	std::cout<<"finished loading data"<<std::endl;
	return 1;
}

int MeasureIntegralsFromWavedump(std::string waveformfile, std::vector<TBranch*> branches){
	
	int maxreadouts=100; // process only the first n readouts
	bool verbose = false;
	
	std::vector<std::vector<std::vector<double>>> alldata(4);  // channel, readout, datavalue
	int loadok = LoadWavedumpFile(waveformfile, alldata);
	
	// histogram, used for fitting for pedestal
	TH1D* hwaveform=nullptr;
	// range of histogram values. should be sufficient for all data
	double maxvalue = 000;  // XXX XXX XXX THESE MAY NEED TUNING
	double minvalue = 10000;
	
	std::vector<double> integralvals(4);
	for(int channeli=0; channeli<4; channeli++){
		branches.at(channeli)->SetAddress(&integralvals.at(channeli));
	}
	
	//std::vector<std::vector<double>> integrals(4);
	for(int channel=0; channel<4; channel++){
		
		std::vector<std::vector<double>> &allchanneldata = alldata.at(channel);
		int numreadouts = allchanneldata.size();
		if(verbose) std::cout<<"we have "<<numreadouts<<" readouts for channel "<<channel<<std::endl;
		
		for(int readout=0; readout<std::min(maxreadouts,numreadouts); readout++){
			
			std::vector<double> data = allchanneldata.at(readout);
			
			//double maxvalue = (*(std::max_element(data.begin(),data.end())));
			//double minvalue = (*(std::min_element(data.begin(),data.end())));
			if(hwaveform==nullptr){ hwaveform = new TH1D("hwaveform","Sample Distribution",200,minvalue,maxvalue); }
			else { hwaveform->Reset(); } // clear it's data
			for(double sampleval : data) hwaveform->Fill(sampleval);
			
			hwaveform->Draw("goff");
			hwaveform->Fit("gaus", "Q");
			TF1* gausfit = hwaveform->GetFunction("gaus");
			double gauscentre = gausfit->GetParameter(1); // 0 is amplitude, 1 is centre, 2 is width
			if(verbose) std::cout<<"pedestal is "<<gauscentre<<std::endl;
			
			uint16_t peaksample = std::distance(data.begin(), std::min_element(data.begin(),data.end()));
			double peakamplitude = data.at(peaksample);
			if(verbose) std::cout<<"peak sample is at "<<peaksample<<std::endl;
			
			// from the peak sample, scan backwards for the start sample
			int startmode = 2;
			double thresholdfraction = 0.1;  // 20% of max
			// 3 ways to do this:
			// 0. scan away from peak until the first sample that changes direction
			// 1. scan away from peak until the first sample that crosses pedestal
			// 2. scan away from peak until we drop below a given fraction of the amplitude
			uint16_t startsample = peaksample-1;
			while(startsample>0){
				if( ( (startmode==0) && (data.at(startsample) < data.at(startsample-1)) ) ||
					( (startmode==1) && (data.at(startsample) < gauscentre) ) ||
					( (startmode==2) && ((data.at(startsample)-gauscentre) < ((peakamplitude-gauscentre)*thresholdfraction)) ) ){
					startsample--;
				} else {
					break;
				}
			}
			if(verbose) std::cout<<"pulse starts at startsample "<<startsample<<std::endl;
			
			// from peak sample, scan forwards for the last sample
			int endmode = 2;
			uint16_t endsample = peaksample+1;
			while(uint16_t(endsample+1)<data.size()){
				if(((endmode==0) && (data.at(endsample+1) < data.at(endsample)) ) ||
					 ((endmode==1) && (data.at(endsample)   < gauscentre) ) ||
					 ((endmode==2) && ((data.at(endsample)-gauscentre) < ((peakamplitude-gauscentre)*thresholdfraction)))){
					endsample++;
				} else {
					break;
				}
			}
			if(verbose) std::cout<<"pulse ends at endsample "<<endsample<<std::endl;
			
			// calculate the integral
			// we'll integrate over a little wider region than the strict cut, but not much
			uint16_t paddingsamples=4;
			int theintegral=0;
			for(int samplei=std::max(0,startsample-paddingsamples);
					samplei<std::min(int(data.size()),endsample+paddingsamples);
					samplei++){
				theintegral += data.at(samplei) - gauscentre;
			}
			if(verbose) std::cout<<"pulse integral = "<<theintegral<<std::endl;
			
			//integrals.at(channel).push_back(theintegral);
			integralvals.at(channel) = theintegral;
			branches.at(channel)->Fill();
			
		}// end loop over readouts for a given channel
		
	} // end loop over channels
	
	// cleanup
	if(hwaveform) delete hwaveform;
	return 1;
}
