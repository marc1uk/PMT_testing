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

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h> // /usr/include/X11/extensions/XTest.h

#include "PMTTestingHVcontrol.h"
#include "zmq.hpp"

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TROOT.h"
#include "TApplication.h"
#include "TSystem.h"

//#define DRAWPHD 1
//#define DRAW_BAD_WAVEFORMS 1
//#define DRAW_WAVEFORMS 1
//#define FIT_OFFSET // Otherwise just use bin with most entries.
//#define DRAW_OFFSETFIT 1
//#define SAVE_WAVEFORM_DATA   // saves waveform data for channel 0

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
	
	std::string outdir=""; // set in program, not read from file
};

// convenience struct for passing related parameters
struct KeyPressVars{
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
int SetupWeinerSoftTrigger(CamacCrate* CC);
int SetupWienerNIMout(CamacCrate* CC, bool EnableNimOut1, bool EnableNimOut2);
int ReadRates(Module &List, double countsecs, std::ofstream &data);
int IntializeADCs(Module &List, std::vector<std::string> &Lcard, std::vector<std::string> &Ccard, bool defaultinit);
int WaitForAdcData(Module &List);
int ReadAdcVals(Module &List, std::map<int, int> &ADCvals);
int LoadTestSetup(std::string configfile, TestVars &thesettings);
XKeyEvent createKeyEvent(Display *display, Window &win, Window &winRoot, bool press, int keycode, int modifiers);
void KeyPressInitialise(KeyPressVars &thevars);
void DoKeyPress(int KEYCODE, KeyPressVars thevars, bool caps=false, int state=0);
void KeyPressFinalise(KeyPressVars thevars);
void RunExternalProcess(std::string command, std::promise<int> finishedin);
int LoadWavedumpFile(std::string filepath, std::vector<std::vector<std::vector<double>>> &data);
int MeasureIntegralsFromWavedump(std::string waveformfile, std::vector<TBranch*> branches);
int MakePulseHeightDistribution(TTree* thetree, std::vector<std::vector<double>> &gainvector);
int RunQDC(Module &List, CamacCrate* CC, TestVars testsettings, std::string outputfilename);
int RunDigitizer(CamacCrate* CC, KeyPressVars thekeypressvars, int numacquisitions);

// MEASUREMENT FUNCTIONS
int DoCooldownTest(Module &List, TestVars testsettings, bool append);
//int RunAfterpulseTest(CamacCrate* CC, Module &List, TestVars testsettings);
int MeasureGain(Module &List, CamacCrate* CC, TestVars testsettings, KeyPressVars thekeypressvars, double testvoltage, std::vector<std::vector<double>> &gainvector);

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

// digitizer values
const double PREAMPLIFIER = 1.;
const double DT5730_INPUT_IMPEDANCE = 50.; // Ohms
const double DT5730_SAMPLE_PERIOD = 2E-9;  // seconds
const double DT5730_ADC_TO_VOLTS = 8192;   // depends on digitizer resolution
const double ELECTRON_CHARGE = 1.6E-19;    // turn charge from coulombs to electrons
const double MAX_PULSE_INTEGRAL = 20e6;    // exclude especially large pulses from PHD fit XXX may require tuning


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
	if(verbosity>message) std::cout<<"Loading test setup"<<std::endl;
	int setupok = LoadTestSetup(testconfigfile, testsettings);
	
	// Make the output directory for output files
	// ==========================================
	for(int pmti=0; pmti<testsettings.pmtNames.size(); pmti++){
		testsettings.outdir += testsettings.pmtNames.at(pmti);
		if((pmti+1)!=(testsettings.pmtNames.size())) testsettings.outdir += "_";
	}
	std::string command = "mkdir " + testsettings.outdir;
	system(command.c_str());
	
	// Open a ROOT TApplication for live plotting
	// ==========================================
	if(verbosity>message) std::cout<<"Creating TApplication"<<std::endl;
	int myargc = 1;
	char arg1[] = "potato";
	char* myargv[]={arg1};
	TApplication *PMTTestStandApp = new TApplication("PMTTestStandApp",&myargc,myargv);
	TCanvas* c1 = new TCanvas();
	
	// Get the X11 display for sending keypresses to other programs
	// ============================================================
	if(verbosity>message) std::cout<<"Initialising X11 for sending keystrokes"<<std::endl;
	KeyPressVars thekeypressvars;
	KeyPressInitialise(thekeypressvars);
	if(thekeypressvars.display==nullptr){ std::cerr<<"Keypress Initialization Failed!!"<<std::endl; return 0; }
	
	// Variables for holding the CamacCrate and derived class objects
	// ==============================================================
	std::vector<std::string> Lcard, Ccard;
	std::vector<int> Ncard;
	Module List;                                                   //Data Model for Lecroys
	
	// read module configuration file
	// ==============================
	if(verbosity>message) std::cout<<"Loading CAMAC configuration"<<std::endl;
	std::string configcc = "configfiles/ModuleConfig"; // card list file
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
	//int caen_test_result = DoCaenTests(List);
	
	// Create class for doing ZMQ control of HV
	// ========================================
	if(verbosity>message) std::cout<<"Creating HV Control class"<<std::endl;
	PMTTestingHVcontrol* hvcontrol = new PMTTestingHVcontrol("192.168.120.9",5555);
	
	// Set up CCUSB NIM output for triggering
	// ======================================
	if(verbosity>message) std::cout<<"Setting up Weiner USB trigger"<<std::endl;
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;   // Modify bit 1 of the register to "1" (CCUSB Trigger)
	//int weiner_softrigger_setup_ok = SetupWeinerSoftTrigger(CC); // this at least enables NIM O1 for USB, probably doesn't set NIM O2
	int weiner_softrigger_setup_ok = SetupWienerNIMout(CC, true, false); // enable NIM O1 (low LED), disable NIM O2 (bright LED)
	
	// Set Discriminator threshold
	// ===========================
	if(verbosity>message) std::cout<<"Setting up discriminator threshold"<<std::endl;
	if(testsettings.thresholdmode!=0){
		List.CC["DISC"].at(0)->EnableProgrammedThreshold();
		List.CC["DISC"].at(0)->WriteThresholdValue(testsettings.threshold);
	}
	else List.CC["DISC"].at(0)->EnableFPthreshold();
	
	// Run cooldown test
	// =================
	if(testsettings.cooldownfilenamebase!="NA"){
		if(verbosity>message) std::cout<<"Performing Cooldown test"<<std::endl;
		bool appendtofile=false;  // append/overwrite any existing file. TODO ALERT USER IF FILE EXISTS
		int cooldownok = DoCooldownTest(List, testsettings, appendtofile);
	} else {
		if(verbosity>message) std::cout<<"Skipping Cooldown test"<<std::endl;
	}
	
	// =========
	// GAIN LOOP
	// =========
	if(verbosity>message) std::cout<<"Beginning Gain loop"<<std::endl;
	int command_ok;
	std::vector<std::vector<double>> gainvector(8);               // for plotting gain vs HV
	for (int i = 0; i < testsettings.gainVoltages.size(); i++)    // loop over readings
	{
		//break;
		
		// Set voltages
		// ============
		double testvoltage = testsettings.gainVoltages.at(i);
		if(verbosity>message) std::cout<<"Setting next voltage: "<<testvoltage<<"V"<<std::endl;
		//hvcontrol->SetVoltage(testvoltage);  // sets the voltage of the 'active' group XXX disabled for debug FIXME, ack but group set fails
		
		// Do Gain Measurement
		// ===================
		// This does 3 steps:
		// 1. fires the LED a number of times and aquires traces
		// 2. loads the data and measures the integrals
		// 3. fits the integral distribution and measures the gain
		int measuregainok = MeasureGain(List, CC, testsettings, thekeypressvars, testvoltage, gainvector);
		
		if(verbosity>message) std::cout<<"Done testing at this voltage"<<std::endl;
	} // loop over voltages
	
	std::cout<<"End of gain data taking"<<std::endl;
	
	// make plot of gain vs HV for these PMTs
	// ======================================
	if(verbosity>message) std::cout<<"Making Gain vs HV plots"<<std::endl;
	for(int channeli=0; channeli<testsettings.pmtNames.size(); channeli++){
		TGraph gainvsHV(testsettings.gainVoltages.size(),
										testsettings.gainVoltages.data(),
										gainvector.at(channeli).data());
		gainvsHV.SaveAs(TString::Format("%s/Gain_Vs_HV_%s.C",testsettings.outdir.c_str(),testsettings.pmtNames.at(channeli).c_str()));
		c1->cd();
		gainvsHV.Draw();
		c1->SaveAs(TString::Format("%s/Gain_Vs_HV_%s.pdf",testsettings.outdir.c_str(),testsettings.pmtNames.at(channeli).c_str()));
	}
	
	// Do afterpulse test
	// ==================
	//int afterpulsetestok = RunAfterpulseTest(CC, List, testsettings, thekeypressvars);
	
	// Cleaup X11 stuff for sending keypresses
	// =======================================
	if(verbosity>message) std::cout<<"Cleaning up X11"<<std::endl;
	KeyPressFinalise(thekeypressvars);
	
	// Clean up ROOT stuff
	// ===================
	if(verbosity>message) std::cout<<"Cleaning up ROOT"<<std::endl;
	if(c1) delete c1;
	if(PMTTestStandApp){ PMTTestStandApp->Terminate(); delete PMTTestStandApp; }
	
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
//                             PULSE RATE TEST
// ***************************************************************************
int DoCooldownTest(Module &List, TestVars testsettings, bool append)
{
	// ***************************************
	// COOLDOWN MEASUREMENT ARGS
	// ***************************************
	// extract arguments from test config struct
	std::string outputfilename = testsettings.outdir + "/" + testsettings.cooldownfilenamebase;
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
	// fill the header lines
	data << "PMT_IDs"<<std::endl;
	for(int pmti=0; pmti<PMTNames.size(); pmti++){
		data << PMTNames.at(pmti) << ", ";
	}
	data << std::endl;
	int thethreshold;
	List.CC["DISC"].at(0)->ReadThreshold(thethreshold);
	data << "Discriminator threshold "<<(thethreshold)<<" mV"<<std::endl
			 << "timestamp";
	for(int scaleri=0; scaleri<List.CC.at("SCA").size(); scaleri++){
		for(int channeli=0; channeli<4; channeli++){
			data << ", ch_" << ((scaleri*4)+channeli) << "_counts";
		}
	}
	for(int scaleri=0; scaleri<List.CC.at("SCA").size(); scaleri++){
		for(int channeli=0; channeli<4; channeli++){
			data << ", ch_" << ((scaleri*4)+channeli) << "_Hz";
		}
	}
	data << std::endl;
	
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
		int readscalerok = ReadRates(List, countsecs, data);
		
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

int MeasureGain(Module &List, CamacCrate* CC, TestVars testsettings, KeyPressVars thekeypressvars, double testvoltage, std::vector<std::vector<double>> &gainvector){
	
	int verbosity = 1;
	std::string gainfilename = testsettings.outdir + "/" + testsettings.gainfilenamebase + std::to_string(static_cast<int>(testvoltage))+"V.root";
	if(verbosity) std::cout<<"Recording PHD Data"<<std::endl;
	
	// use the QDC to measure pulse integral
	// this fires the LED a given number of times, reads the pulse integrals and writes them to file
	//int runqdcok = RunQDC(List, CC, testsettings, "RawQDCdata");
	// saves data to a txt file, would need integration from plotQDChisto.cpp
	
	// alternative version using the UCDavis digitizer
	// this also fires the LED a given number of times, reads the waveforms and writes them to file
	int rundigitizerok = RunDigitizer(CC, thekeypressvars, testsettings.gainacquisitions); // XXX disable for debug
	
	// Create a ROOT file to save the pulse height distribution data
	if(verbosity) std::cout<<"Creating output ROOT file "<<gainfilename<<std::endl;
	TFile* rootfout = new TFile(gainfilename.c_str(),"RECREATE");
	TTree* roottout = new TTree("pmt","PMT Test Results");
	std::vector<TBranch*> branchpointers;
	double dummy[8];
	branchpointers.push_back(roottout->Branch("Integral",dummy,"Integral[8]/Double_t"));
	branchpointers.push_back(roottout->Branch("DC_Offset",dummy,"DC_Offset[8]/Double_t"));
	branchpointers.push_back(roottout->Branch("Start_Sample",dummy,"Start_Sample[8]/Double_t"));
	branchpointers.push_back(roottout->Branch("End_Sample",dummy,"End_Sample[8]/Double_t"));
	branchpointers.push_back(roottout->Branch("Peak_Sample",dummy,"Peak_Sample[8]/Double_t"));
	branchpointers.push_back(roottout->Branch("Peak_Amplitude",dummy,"Peak_Amplitude[8]/Double_t"));
#ifdef SAVE_WAVEFORM_DATA
	std::vector<double> waveformdummy;
	std::vector<double> waveformdummyp = &waveformdummy;
	branchpointers.push_back(roottout->Branch("Waveform",&waveformdummy));
#endif
	
	// read the waveforms, calculate the integral and fill the tree
	// ============================================================
	if(verbosity) std::cout<<"Calculating integrals from raw data and filling into ROOT file"<<std::endl;
	int calculateintegralsok = MeasureIntegralsFromWavedump("wave0.txt", branchpointers);
	
	// write tree to file
	if(verbosity) std::cout<<"writing "<<roottout->GetEntries()<<" entries to ROOT file"<<std::endl;
	rootfout->cd();
	roottout->Write();
	
	// we'll keep the raw data, but rename, move and zippit
	std::string command = "zip -3 wave0.zip wave0.txt";  // compress for space - 10k readouts for 1 channel are ~40MB in size! This gets down to ~9MB
	system(command.c_str());
	std::string newrawfilename = "wave0_" + std::to_string(static_cast<int>(testvoltage))+"V.zip";
	command = "mv wave0.zip " + testsettings.outdir + "/" + newrawfilename;
	system(command.c_str());
	
	// Plot the pulse height distributions and measure the gains
	// =========================================================
	if(verbosity) std::cout<<"Fitting PHDs and Calculating Gain"<<std::endl;
	int measuregainsok = MakePulseHeightDistribution(roottout, gainvector);
	
	// CLEANUP
	// =======
	if(verbosity) std::cout<<"Closing ROOT file"<<std::endl;
	roottout->ResetBranchAddresses();
	rootfout->Close();
	if(verbosity) std::cout<<"deleting the file"<<std::endl;
	delete rootfout;       // also cleans up roottout
	rootfout=nullptr;
	
	return 1;
}

// ***************************************************************************
//                       AFTERPULSE TEST - Digitizer Ver
// ***************************************************************************
//int RunAfterpulseTest(CamacCrate* CC, Module &List, TestVars testsettings){
//	
//	// Set up Wiener CCUSB to fire NIM O2 on ActionRegWrite rather than NIM O1
//	// (this is connected to a much brighter LED)
//	int wienersetupok = SetupWienerNIMout(CC, false, true);
//	
//	// fire the LEDs and acquire data from the digitizer to "wave0.txt"
//	int recorddataok = RunDigitizer(CC, thekeypressvars, testsettings.numafterpulseacquisitions);
//	
//	
//	
//}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

// ***************************************************************************
//                             SUPPORT FUNCTIONS
// ***************************************************************************

// ***************************************************************************
//                           RUN CAEN DT5730B DIGITIZER
// ***************************************************************************

int RunDigitizer(CamacCrate* CC, KeyPressVars thekeypressvars, int numacquisitions){
	
	// Run wavedump in an external thread so it can execute while we continue and send keys to it
	// first create a promise we can use to hold until the external thread is done
	std::promise<int> barrier;
	std::future<int> barrier_future = barrier.get_future();
	
	// start the thread
	std::cout<<"starting wavedump"<<std::endl;
	std::string wavedumpcommand = "wavedump WaveDumpConfig.txt";
	std::thread athread(RunExternalProcess, wavedumpcommand, std::move(barrier));
	
	// wait for just a little bit for wavedump to initialize
	std::this_thread::sleep_for(std::chrono::seconds(3));
	
	// configure it
	std::cout<<"enabling continuous output saving"<<std::endl;
	int KEYCODE = XK_w; // USE LOWER CASE, BUT WITH BOOL 'CAPS' MODIFIER PARAMETER!
	DoKeyPress(KEYCODE, thekeypressvars, true);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	// start it
	std::cout<<"beginning acquisition"<<std::endl;
	KEYCODE = XK_s; // XK_s sends lower case s etc.
	DoKeyPress(KEYCODE, thekeypressvars);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	
	// we need to fire the LED (which will also trigger acquisition)
	// Get register settings with soft-trigger bit (un)set
	// ===================================================
	long RegStore;
	CC->ActionRegRead(RegStore);
	long RegActivated   = RegStore | 0x02;   // Modify bit 1 of the register to "1" (CCUSB Trigger)
	
	std::cout<<"taking "<<numacquisitions<<" acquisitions"<<std::endl;
	for(int triggeri=0; triggeri<numacquisitions; triggeri++){
		// Fire LED! (and gate ADCs)
		// =========================
		int command_ok = CC->ActionRegWrite(RegActivated);
		//std::cout<<"Firing LED " << ((command_ok) ? "OK" : "FAILED") << std::endl;
		// DT5730B acquires 1024 samples with 500MHz rate = 2us per readout
		usleep(1000);
	}
	
	// stop it
	std::cout<<"ending acquisition"<<std::endl;
	KEYCODE = XK_s; // XK_s sends lower case s etc.
	DoKeyPress(KEYCODE, thekeypressvars);
	
	// wait for it to finalise itself
	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	// close it
	std::cout<<"quitting wavedump"<<std::endl;
	KEYCODE = XK_q; // XK_s sends lower case s etc.
	DoKeyPress(KEYCODE, thekeypressvars);
	
	// wait for the external thread to indicate completion
	std::chrono::milliseconds span(100);
	std::future_status thestatus;
	int i=0;
	do {
		thestatus = barrier_future.wait_for(span);
		if(i>20){ std::cout<<"repeating Q keypress"<<std::endl; DoKeyPress(KEYCODE, thekeypressvars); i=0; }   // WHY NEEDED?
		i++;
	} while (thestatus==std::future_status::timeout);
	// get the return to know if the external thead succeeded
	std::cout << "thread returned " << barrier_future.get() << std::endl;
	
	// call join to cleanup the external thread
	athread.join();
	
	return 1;
}

// ***************************************************************************
//                            RUN LECROY 4300B QDC
// ***************************************************************************

int RunQDC(Module &List, CamacCrate* CC, TestVars testsettings, std::string outputfilename){
	
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
// CAEN C117B HV CONTROL CARD TESTS
// ***************************************************************************
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

// ***************************************************************************
// SETUP WEINER NIM OUT
// ***************************************************************************
int SetupWeinerSoftTrigger(CamacCrate* CC){
	
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
	
	return 1;// todo error checking
}

int SetupWienerNIMout(CamacCrate* CC, bool EnableNimOut1, bool EnableNimOut2){
	
	int trig_src = (EnableNimOut1) ? 6 : 0;
	//            DelayGateGen(DGG, trig_src, NIM_out, delay_ns, gate_ns, NIM_invert, NIM_latch);
	int ARegDat1 = CC->DelayGateGen(0,   trig_src,       1,       1,       25,        0,          0);
	if(ARegDat1<0) std::cerr<<"failed to "<<((EnableNimOut1) ? "enable" : "disable")<<" USB trigger on DGG_0 output 0!" << std::endl;
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	trig_src = (EnableNimOut2) ? 6 : 0;
	int ARegDat2 = CC->DelayGateGen(1,   trig_src,       2,       1,       25,        0,          0);
	if(ARegDat2<0) std::cerr<<"failed to "<<((EnableNimOut2) ? "enable" : "disable")<<" USB trigger on DGG_1 output 1!" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	if((ARegDat1<0)||(ARegDat2<0)) return 0;
	return 1;
}

// ***************************************************************************
// MEASURE PULSE RATE WITH SCALER
// ***************************************************************************
int ReadRates(Module &List, double countsecs, std::ofstream &data){
	
	// clear counters
	//std::cout<<"clearing scalers"<<std::endl;
	for(int scaleri=0; scaleri<List.CC.at("SCA").size(); scaleri++){
		List.CC["SCA"].at(scaleri)->ClearAll();
	}
	///////////////////// debug:
	//List.CC["SCA"].at(0)->ReadAll(scalervals);
	//std::cout << "zero'd values are: " << std::endl;
	//for(int chan=0; chan<4; chan++){
	//	std::cout << scalervals[chan];
	//	if(chan<3) std::cout << ", ";
	//}
	//////////////////// end debug
	
	//waiting for requested duration for counts to accumulate
	std::this_thread::sleep_for(std::chrono::seconds(int(countsecs)));
	
	std::vector<std::vector<int>> allscalervals;
	//Read the scalars
	for(int scaleri=0; scaleri<List.CC.at("SCA").size(); scaleri++){
		std::cout<<"reading out scaler "<<scaleri<<std::endl;
		std::vector<int> scalervals(4);
		int ret = List.CC["SCA"].at(scaleri)->ReadAll(scalervals.data());
		allscalervals.push_back(scalervals);
		if(ret<0){
			std::cerr << "error reading scaler "<<scaleri<<": response was " << ret << std::endl;
		}
	}
	
	// get and write timestamp to file
	std::cout<<"writing to file"<<std::endl;
	std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
	time_t tt;
	tt = std::chrono::system_clock::to_time_t ( time );
	std::string timeStamp = ctime(&tt);
	timeStamp.erase(timeStamp.find_last_not_of(" \t\n\015\014\013")+1);  // trim any trailing whitespace/EOL
	data << timeStamp << ", ";
	
	// write scalar values to file
	for(int scalercard=0; scalercard<allscalervals.size(); scalercard++){
		for(int chan=0; chan<4; chan++){
			data << allscalervals.at(scalercard).at(chan) << ", ";
		}
	}
	
	// write scalar rates to file
	for(int scalercard=0; scalercard<allscalervals.size(); scalercard++){
		for(int chan=0; chan<4; chan++){
			double darkRate = double (allscalervals.at(scalercard).at(chan)) / countsecs;
			data << darkRate;
			if(((scalercard+1)!=(allscalervals.size())) || chan<3) data << ", ";
		}
	}
	
	data << std::endl;
	std::cout << "Done writing to file" << std::endl;
	
	return 1;// todo error checking
}

// ***************************************************************************
// RE-SET ADC REGISTERS
// ***************************************************************************
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
	
	// Print register settings to check the QDCs are configured properly
	//List.CC["ADC"].at(0)->GetRegister();             // read from the card into Control member
	//List.CC["ADC"].at(0)->DecRegister();             // decode 'Control' member into ECL, CLE etc. members
	//List.CC["ADC"].at(0)->PrintRegister();           // print ECL, CLE etc members
	//List.CC["ADC"].at(0)->PrintPedestal();           // print out pedestals
	//List.CC["ADC"].at(0)->SetPedestal();
	//List.CC["ADC"].at(0)->GetPedestal();             // retrieve pedastals into Ped member
	//List.CC["ADC"].at(0)->PrintPedestal();           // print out pedestals
	
	return 1; // todo error checking
}

// ***************************************************************************
// POLL ADCs FOR BUSY SIGNAL
// ***************************************************************************
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

// ***************************************************************************
// READ ADC DATA INTO MAP
// ***************************************************************************
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

// ***************************************************************************
// READ TEST SETTING FROM CONFIG FILE
// ***************************************************************************
int LoadTestSetup(std::string configfile, TestVars &thesettings){
	std::ifstream fin (configfile.c_str());
	std::string Line;
	std::stringstream ssL;
	
	std::string sEmp;
	std::string iEmp;
	bool settingPmtNames=false;
	bool settingGainVoltages=false;
	thesettings.pmtNames.clear();
	thesettings.gainVoltages.clear();
	
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
			if (Line.find("#") != std::string::npos) Line.erase(Line.begin()+Line.find('#'), Line.end());
			ssL.str("");
			ssL.clear();
			ssL << Line;
			if (ssL.str() != "")
			{
				if(not (settingPmtNames || settingGainVoltages)) ssL >> sEmp >> iEmp;
				else ssL >> sEmp; // only 1 argument
				
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
						thesettings.pmtNames.push_back(sEmp);
					}
				}
				
				else if(settingGainVoltages){
					thesettings.gainVoltages.push_back(stof(sEmp));
				}
				
				else std::cerr<<"WARNING: Test Setup ignoring unknown configuration option \""<<sEmp<<"\""<<std::endl;
			}
		}
	}
	fin.close();
	return 1;
}

// ***************************************************************************
// CREATE X KEYPRESS EVENTS
// ***************************************************************************
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

// ***************************************************************************
// SETUP SENDING KEYS TO OTHER WINDOWS WITH X
// ***************************************************************************
void KeyPressInitialise(KeyPressVars &thevars){
		// Obtain the X11 display.
	thevars.display = XOpenDisplay(0);
	if(thevars.display == nullptr){
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
	
	//tell X11 this client wants to receive key-press and key-release events associated with winFocus.
	XSelectInput(thevars.display, thevars.winFocus, KeyPressMask|KeyReleaseMask); 
}

// ***************************************************************************
// SEND A KEYPRESS EVENT
// ***************************************************************************
void DoKeyPress(int KEYCODE, KeyPressVars thevars, bool caps, int state){  // state=0: press+release, 1=press, 2=release
	
	if(caps){
	// KEYCODE for w and W are the same!!
	// we need to manually send the modifier keys!
		DoKeyPress(XK_Shift_L, thevars, false, 1);
	}
	
	int modifiers = 0; //(caps) ? ShiftMask : 0;  // did not seem to work for sending caps
	
	if(state!=2){
		// generate the key-down event
		XGetInputFocus(thevars.display, &thevars.winFocus, &thevars.revert);
		XKeyEvent event = createKeyEvent(thevars.display, thevars.winFocus, thevars.winRoot, true, KEYCODE, modifiers);
		int ret = XTestFakeKeyEvent(event.display, event.keycode, True, CurrentTime);
		if(ret==0) std::cerr<<"XTestFakeKeyEvent returned 0! Could not send keypress?!"<<std::endl;
	}
	
	// ensure the thing has gone through? Did not help with some keys not getting seen?
	//XSync(thevars.display,False);
	//usleep(100);
	
	if(state!=1){
		// repeat with a key-release event
		XGetInputFocus(thevars.display, &thevars.winFocus, &thevars.revert);
		XKeyEvent event = createKeyEvent(thevars.display, thevars.winFocus, thevars.winRoot, false, KEYCODE, modifiers);
		int ret = XTestFakeKeyEvent(event.display, event.keycode, False, CurrentTime);
		if(ret==0) std::cerr<<"XTestFakeKeyEvent returned 0! Could not send keypress?!"<<std::endl;
	}
	
	if(caps){
		DoKeyPress(XK_Shift_L, thevars, false, 2);
	}
}

// ***************************************************************************
// CLEANUP SENDING KEYS TO OTHER WINDOWS
// ***************************************************************************
void KeyPressFinalise(KeyPressVars thevars){
	// Re-enable auto-repeat
	XAutoRepeatOn(thevars.display);
	
	// Cleanup X11 display
	XCloseDisplay(thevars.display);
}

// ***************************************************************************
// RUN A COMMAND IN A SEPARATE THREAD AND WAIT FOR RETURN
// ***************************************************************************
void RunExternalProcess(std::string command, std::promise<int> finishedin){
	
	// if we need to pass between member functions in classes, must use std::move
	// otherwise we can just call set_value on finishedin
	std::promise<int> finished;
	finished = std::promise<int>(std::move(finishedin));
	
	// start the external program, which will wait for a keypress then return
	system(command.c_str());
	
	// release the barrier, allowing the caller to continue
	finished.set_value(1);
}

// ***************************************************************************
// LOAD A WAVEDUMP "wave0.txt" FILE INTO VECTORS
// ***************************************************************************
int LoadWavedumpFile(std::string filepath, std::vector<std::vector<std::vector<double>>> &data){
	
	// data is a 3-deep vector of: readout, channel, datavalue
	std::ifstream fin (filepath.c_str());
	std::string Line;
	std::stringstream ssL;
	
	double value;
	int readout=0;
	int channel=0;
	int recordlength=0;
	std::string dummy1, dummy2;
	bool dataline=false;
	int linenum=0;
	
	int datalinenum=0;
	int headerlinenum=0;
	int numheaderlines=7;  // there are 7 headerlines
	
	while (getline(fin, Line))
	{
		linenum++;
		if (Line.empty()) continue;
		if(not dataline){
			//if (Line[0] == '#') continue;
			//if (Line.find("Record") != std::string::npos){
			if(headerlinenum==0){
				if(recordlength==0){
					// start of new readout
					ssL.str("");
					ssL.clear();
					ssL << Line;  // line should be of the form "Record Length: 1030"
					if (ssL.str() != "")
					{
						ssL >> dummy1 >> dummy2 >> recordlength;
					}
				} // else don't even bother reading the line
				headerlinenum++;
			}
			//else if (Line.find("Channel") != std::string::npos){
			else if(headerlinenum==2){
				ssL.str("");
				ssL.clear();
				ssL << Line;  // line should be of the form "Channel: 0"
				if (ssL.str() != "")
				{
					ssL >> dummy1 >> channel;
					headerlinenum++;
				}
			}
			//else if (Line.find("Event") != std::string::npos){
			else if(headerlinenum==3){
				ssL.str("");
				ssL.clear();
				ssL << Line;  // line should be of the form "Event Number: 1"
				if (ssL.str() != "")
				{
					ssL >> dummy1 >> dummy2 >> readout;
					if((readout%1000)==0){
						if(readout==0) std::cout<<"loading readout ";
						std::cout<<readout<<"..."<<std::flush;
					}
					headerlinenum++;
				}
			}
//			else if (Line.find("offset") != std::string::npos){
//				// last header line, mark the next line as start of values
//				// not necessary to actually read this line
//				dataline=true;
//			}
//			else if( (Line.find("BoardID") != std::string::npos) ||
//					 (Line.find("Pattern") != std::string::npos) ||
//					 (Line.find("Trigger") != std::string::npos) ){
//				// other header lines, ignore. redundant, but hey.
//			}
			else {  // this is some header line we don't care about
				headerlinenum++;
				if(headerlinenum==numheaderlines) dataline=true;
			}
		} else {
			ssL.str("");
			ssL.clear();
			ssL << Line;
			if (ssL.str() != "")
			{
				ssL >> value;
				if(data.size()<(readout+1)) data.emplace_back(std::vector<std::vector<double>>(8));
				data.at(readout).at(channel).push_back(value);
				datalinenum++;
				if(datalinenum==recordlength){
					dataline=false; // this should be the last data line, next will be header line
					headerlinenum=0;
					datalinenum=0;
				}
			}
		}
	}
	fin.close();
	std::cout<<std::endl;
	return 1;
}

// ***************************************************************************
// CALCULATE INTEGRALS FROM DATA TRACES AND WRITE TO A TTREE
// ***************************************************************************
int MeasureIntegralsFromWavedump(std::string waveformfile, std::vector<TBranch*> branches){
	
	int maxreadouts=std::numeric_limits<int>::max(); // process only the first n readouts
	bool verbose = false;
	
	std::vector<std::vector<std::vector<double>>> alldata;  // readout, channel, datavalue
	int loadok = LoadWavedumpFile(waveformfile, alldata);
	//std::cout<<"alldata.size()="<<alldata.size()<<", alldata.at(0).size()="<<alldata.at(0).size()<<", alldata.at(0).at(0).size()="<<alldata.at(0).at(0).size()<<std::endl;
	
#if defined DRAW_WAVEFORMS || defined DRAW_BAD_WAVEFORMS
	TCanvas* c2 = new TCanvas();
	TGraph* waveform = nullptr;
#endif
#ifdef DRAW_OFFSETFIT
	TCanvas* c3 = new TCanvas();
#endif
	
	// histogram, used for fitting for offset
	TH1D* hwaveform=nullptr;
	
	std::vector<double> integralvals(8);
	std::vector<double> offsetvals(8);
	std::vector<double> startvals(8);
	std::vector<double> endvals(8);
	std::vector<double> peaksamples(8);
	std::vector<double> peakvals(8);
	
	branches.at(0)->SetAddress(integralvals.data());
	branches.at(1)->SetAddress(offsetvals.data());
	branches.at(2)->SetAddress(startvals.data());
	branches.at(3)->SetAddress(endvals.data());
	branches.at(4)->SetAddress(peaksamples.data());
	branches.at(5)->SetAddress(peakvals.data());
	
#ifdef SAVE_WAVEFORM_DATA
	std::vector<double> waveformdata;
	int recordlength = alldata.at(0).at(0).size();
	waveformdata.reserve(recordlength);
	std::vector<double>* waveformdatap = &waveformdata;
	branches.at(6)->SetAddress(&waveformdatap);
#endif
	
	int numreadouts = alldata.size();
	if(verbose) std::cout<<"we have "<<numreadouts<<" readouts"<<std::endl;
	for(int readout=0; readout<std::min(maxreadouts,numreadouts); readout++){
		
		std::vector<std::vector<double>> &allreadoutdata = alldata.at(readout);
		for(int channel=0; channel<8; channel++){
			
			std::vector<double> data = allreadoutdata.at(channel);
			if(data.size()==0){
				integralvals.at(channel) = 0;
				offsetvals.at(channel) = 0;
				startvals.at(channel) = 0;
				endvals.at(channel) = 0;
				peaksamples.at(channel) = 0;
				peakvals.at(channel) = 0;
				continue;
			}
			
			double maxvalue = (*(std::max_element(data.begin(),data.end())));
			double minvalue = (*(std::min_element(data.begin(),data.end())));
			if(hwaveform==nullptr){ hwaveform = new TH1D("hwaveform","Sample Distribution",200,minvalue,maxvalue); }
			else { hwaveform->Reset(); hwaveform->SetAxisRange(minvalue, maxvalue, "X"); } // clear it's data and reset axis range
			for(double sampleval : data){ hwaveform->Fill(sampleval); }
			
#ifdef FIT_OFFSET  // actually the fit really isn't needed, just take the bin with the highest count.
#ifdef DRAW_OFFSETFIT
			{
				TCanvas* c1=nullptr;
				if(gPad) c1 = gPad->GetCanvas();
				c3->cd();
				hwaveform->Draw();
				c3->Modified();
				c3->Update();
				gSystem->ProcessEvents();
				if(c1) c1->cd();
			}
#else
			hwaveform->Draw("goff");
#endif
			// fit the offset with a gaussian
			// first get some estimates for the gaussian fit, ROOT's not capable on it's own
			double amplitude=hwaveform->GetBinContent(hwaveform->GetMaximumBin());
			double mean=hwaveform->GetXaxis()->GetBinCenter(hwaveform->GetMaximumBin());
			double width=50; //hwaveform->GetStdDev();  -- gives about 600!
			if(verbose) std::cout<<"Max bin is "<<mean<<", amplitude is "<<amplitude<<", stdev is "<<width<<std::endl;
			TF1 agausfit("offsetfit","gaus",mean-10*width,mean+10*width);
			TF1* gausfit = &agausfit;
			gausfit->SetParameters(amplitude,mean,width);
			
			// fit without prior parameters set - does not work reliably
			//hwaveform->Fit("gaus", "Q");
			//TF1* gausfit = hwaveform->GetFunction("gaus");
			//gausfit->SetNpx(1000);  // better resolution for drawing (possibly not needed for actual fit?)
			hwaveform->Fit(gausfit, "RQ");  // use R to limit range! This is what makes it work!
			double gauscentre = gausfit->GetParameter(1); // 0 is amplitude, 1 is centre, 2 is width
			if(verbose) std::cout<<"offset is "<<gauscentre<<std::endl;
#ifdef DRAW_OFFSETFIT
			{
				TCanvas* c1=nullptr;
				if(gPad) c1 = gPad->GetCanvas();
				c3->cd();
				gausfit->Draw("same");
				c3->Modified();
				c3->Update();
				gSystem->ProcessEvents();
				if(c1) c1->cd();
			}
#endif
#else // !FIT_OFFSET
			double gauscentre = hwaveform->GetXaxis()->GetBinCenter(hwaveform->GetMaximumBin());
#endif
			
#ifdef DRAW_WAVEFORMS
			std::vector<double> numberline(data.size());
			std::iota(numberline.begin(),numberline.end(),0);
			std::vector<double> offsetsubtractedata;
			for(auto aval : data) offsetsubtractedata.push_back(aval-gauscentre);
			if(waveform) delete waveform;
			waveform = new TGraph(data.size(), numberline.data(), offsetsubtractedata.data());
			{
				TCanvas* c1=nullptr;
				if(gPad) c1 = gPad->GetCanvas();
				c2->cd();
				waveform->Draw("alp");
				c2->Modified();
				c2->Update();
				gSystem->ProcessEvents();
				if(c1) c1->cd();
			}
#endif
			
			uint16_t peaksample = std::distance(data.begin(), std::min_element(data.begin(),data.end()));
			double peakamplitude = data.at(peaksample);
			if(verbose) std::cout<<"peak sample is at "<<peaksample<<" with peak value "<<peakamplitude<<std::endl;
			
			// from the peak sample, scan backwards for the start sample
			int startmode = 2;
			double thresholdfraction = 0.1;  // 20% of max
			if(verbose) std::cout<<"threshold value set to "<<((peakamplitude-gauscentre)*thresholdfraction)<<" ADC counts"<<std::endl;
			// 3 ways to do this:
			// 0. scan away from peak until the first sample that changes direction
			// 1. scan away from peak until the first sample that crosses offset
			// 2. scan away from peak until we drop below a given fraction of the amplitude
			uint16_t startsample = std::max(int(0),static_cast<int>(peaksample-1));
			if(verbose) std::cout<<"searching for pulse start: will require offset-subtracted data value is LESS than "
													 <<((peakamplitude-gauscentre)*thresholdfraction)<<" to be within pulse."<<std::endl;
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
			uint16_t endsample = std::min(static_cast<int>(data.size()-1),static_cast<int>(peaksample+1));
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
			double theintegral=0;
			for(int samplei=std::max(0,int(startsample-paddingsamples));
					samplei<std::min(int(data.size()),int(endsample+paddingsamples));
					samplei++){
				theintegral += data.at(samplei) - gauscentre;
			}
			if(verbose) std::cout<<"pulse integral = "<<theintegral<<std::endl;
			
			// convert to absolute gain
			double integrated_volt_samples = -theintegral / (DT5730_ADC_TO_VOLTS*PREAMPLIFIER);
			double ELECTRONS_PER_SECOND = (DT5730_SAMPLE_PERIOD/ELECTRON_CHARGE);
			double thecharge = integrated_volt_samples*ELECTRONS_PER_SECOND/DT5730_INPUT_IMPEDANCE;
			
//			// Afterpulse measurement! while we have the waveform data, and now that we have the DC offset,
//			// scan the waveform and count the number of pulses. Any > 1 are considered afterpulses.
//			int APinsamples = testsettings.afterpulseminsamplesabovethreshold;
//			int APoutsamples = testsettings.afterpulseminholdoffsamples;
//			double APthreshold = testsettings.afterpulsethresholdfraction;
//			for(auto aval : data){
//				double offsetsubtractedval = aval - gauscentre;
//				if(inpulse){
//					if(offsetsubtractedval > (APthreshold*peakamplitude
//				}
//			}
			
#if defined DRAW_WAVEFORMS || defined DRAW_BAD_WAVEFORMS
			if(abs(thecharge)>40e6){   // if(abs(theintegral)>200e3)
#ifdef DRAW_BAD_WAVEFORMS
				std::vector<double> numberline(data.size());
				std::iota(numberline.begin(),numberline.end(),0);
				std::vector<double> offsetsubtractedata;
				for(auto aval : data) offsetsubtractedata.push_back(aval-gauscentre);
				if(waveform) delete waveform;
				waveform = new TGraph(data.size(), numberline.data(), offsetsubtractedata.data());
				{
					TCanvas* c1=nullptr;
					if(gPad) c1 = gPad->GetCanvas();
					c2->cd();
					waveform->Draw("alp");
					c2->Modified();
					c2->Update();
					gSystem->ProcessEvents();
					if(c1) c1->cd();
					
					std::cout<<"bad offset is "<<gauscentre<<std::endl;
					std::cout<<"bad peak sample is at "<<peaksample<<" with peak value "<<peakamplitude<<std::endl;
					std::cout<<"bad pulse starts at startsample "<<startsample<<std::endl;
					std::cout<<"bad pulse ends at endsample "<<endsample<<std::endl;
					std::cout<<"pulse start/ends found requiring offset-subtracted data value is LESS than "
													 <<((peakamplitude-gauscentre)*thresholdfraction)<<" to be within pulse."<<std::endl;
				}
#endif
				// std::this_thread::sleep_for(std::chrono::seconds(15));
				do{
					gSystem->ProcessEvents();
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				} while(1);
				delete waveform; waveform=nullptr;
			}
#endif
			
 			integralvals.at(channel) = thecharge;
			offsetvals.at(channel) = gauscentre;
			startvals.at(channel) = startsample;
			endvals.at(channel) = endsample;
			peaksamples.at(channel) = peaksample;
			peakvals.at(channel) = peakamplitude;
#ifdef SAVE_WAVEFORM_DATA
			if(channel==0){
				waveformdata.clear();
				for(auto aval : data) waveformdata.push_back(aval-gauscentre);
			}
#endif
			
		}// end loop over channels for a given readout
		
		branches.at(0)->GetTree()->Fill();
		
	} // end loop over readouts
	
	// cleanup
	// reset the branch addresses before any subsequent Draw() calls, they will soon to invalid vectors
	branches.at(0)->GetTree()->ResetBranchAddresses();
	if(hwaveform){ delete hwaveform; hwaveform=nullptr; }
#ifdef DRAW_WAVEFORMS
	if(c2){ delete c2; c2=nullptr; }
	if(waveform){ delete waveform; waveform=nullptr; }
#endif
#ifdef DRAW_OFFSETFIT
	if(c3){ delete c3; c3=nullptr; }
#endif
	return 1;
}

// ***************************************************************************
// FIT A PULSE HEIGHT DISTRIBUTION TO MEASURE GAIN
// ***************************************************************************
int MakePulseHeightDistribution(TTree* thetree, std::vector<std::vector<double>> &gainvector){
	
	// loop over the channels
	for(int channelnum=0; channelnum<8; channelnum++){
		
		std::cout<<"measuring PHD for channel "<<channelnum<<std::endl;
		std::string branchname = "Integral[" + std::to_string(channelnum)+"]";
		std::string histname = "ahist_" + std::to_string(channelnum);
		std::string drawname = branchname + ">>" + histname;
		std::string cutstring = branchname + "<" + std::to_string(MAX_PULSE_INTEGRAL);
#ifdef DRAWPHD
		thetree->Draw(drawname.c_str(), cutstring.c_str());
#else
		thetree->Draw(drawname.c_str(), cutstring.c_str(), "goff");
#endif
		TH1D* ahistp = (TH1D*)gROOT->FindObject(histname.c_str());
		if(ahistp==nullptr){ std::cerr<<"Failed to find PulseHeightDistribution drawn from file!"<<std::endl; return 0; }
		
		// to do a double-gaus fit we need to define the fit function first
		double fitrangelo = ahistp->GetXaxis()->GetBinLowEdge(0);  // XXX FIT RANGE MAY NEED TUNING
		double fitrangeup = ahistp->GetXaxis()->GetBinUpEdge(ahistp->GetNbinsX());
		TF1 fit_func("fit_func","gaus(0)+gaus(3)",fitrangelo,fitrangeup);
		// unfortunately we also need to give it initial values for it to work
		double gaus1amp = ahistp->GetEntries()/10;
		double gaus1centre = 0; //ahistp->GetMean(); pedestal ~0.
		double gaus1width = ahistp->GetEntries()*50;
		double gaus2amp = gaus1amp/5;
		double gaus2centre = ahistp->GetMean()*1.5;
		double gaus2width = gaus1width*4;
		fit_func.SetParameters(gaus1amp, gaus1centre, gaus1width, gaus2amp, gaus2centre, gaus2width);
		//for(int parami=0; parami<6; parami++){
		//	std::cout<<"default parameter "<<parami<<" = "<<fit_func.GetParameter(parami)<<std::endl;
		//}
		
		// Do the fit
		fit_func.SetNpx(1000);
		ahistp->Fit("fit_func","Q");
		
		// Extract the gain
		double mean_pedestal = fit_func.GetParameter(1);
		double mean_spe = fit_func.GetParameter(4);
		
		double gain = mean_spe-mean_pedestal;  // for LeCroy QDC see old fit_QDC_Histo.cpp
		std::cout <<"Pedestal mean is: "<<mean_pedestal<<std::endl;
		std::cout <<"SPE mean is: "<<mean_spe<<std::endl;
		std::cout <<"Gain is: "<<gain<<std::endl;
		
		thetree->GetCurrentFile()->cd();
		ahistp->Write(TString::Format("phd_%d",channelnum));
		gROOT->cd();
		
		if(gPad){
			std::string filenamebase = thetree->GetCurrentFile()->GetName();
			filenamebase = filenamebase.substr(0,filenamebase.length()-5); // strip '.root' extension
			std::string QDCfilestring = filenamebase + "_ch" + std::to_string(static_cast<int>(channelnum)) + ".png";
			//std::cout<<"Saving PHD to name "<<QDCfilestring<<std::endl;
			gPad->GetCanvas()->SaveAs(QDCfilestring.c_str());
			gPad->Clear();
		} // else, just retrieve it from the .root file
		
		gainvector.at(channelnum).push_back(gain);
		
#ifdef DRAWPHD
		std::this_thread::sleep_for(std::chrono::seconds(15));
//		do{
//			std::this_thread::sleep_for(std::chrono::milliseconds(100));
//			gSystem->ProcessEvents();
//		} while(1);
#endif
		
	} // end loop over channels
	
	std::cout<<"returning"<<std::endl;
	return 0;
}
