/* vim:set noexpandtab tabstop=2 wrap */
#include <string>
#include <ctime>
#include <thread>
#include <future>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <bitset>
#include <algorithm>
#include <numeric>

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

#define FIT_OFFSET
#define DRAW_OFFSETFIT

// digitizer values
const double PREAMPLIFIER = 1.;
const double DT5730_INPUT_IMPEDANCE = 50.; // Ohms
const double DT5730_SAMPLE_PERIOD = 2E-9;  // seconds
const double DT5730_ADC_TO_VOLTS = 8192;   // depends on digitizer resolution
const double ELECTRON_CHARGE = 1.6E-19;    // turn charge from coulombs to electrons
const double MAX_PULSE_INTEGRAL = 20e6;    // exclude especially large pulses from PHD fit

int LoadWavedumpFile(std::string filepath, std::vector<std::vector<std::vector<double>>> &data);

int main(int argc, char* argv[]){
	std::string waveformfile = "wave0.txt";
	int channel = 0;
	bool subtractoffset=true;
	if((argc>1) && (strcmp(argv[1],"--help")==0)){
		std::cout<<"usage: plot_waveform [file=\"wave0.txt\"] [channel=0] [subtract_DC_offset=true]"<<std::endl;
		return 1;
	}
	if(argc>1) waveformfile = std::string(argv[1]);
	if(argc>2) channel=atoi(argv[2]);
	if(argc>3) subtractoffset= ((strcmp(argv[3],"true")==0)||(atoi(argv[3])==1));
	
	std::vector<std::vector<std::vector<double>>> alldata;  // readout, channel, datavalue
	int loadok = LoadWavedumpFile(waveformfile, alldata);
	std::cout<<"alldata.size()="<<alldata.size()
					 <<", alldata.at(0).size()="<<alldata.at(0).size()
					 <<", alldata.at(0).at(0).size()="<<alldata.at(0).at(0).size()<<std::endl;
	
	int myargc = 1;
	char arg1[] = "potato";
	char* myargv[]={arg1};
	TApplication *PMTTestStandApp = new TApplication("WaveformPlotApp",&myargc,myargv);
	
	TCanvas* c1;
	TCanvas* c2;
	TGraph* waveform = nullptr;
	TH1D* hwaveform=nullptr;      // histogram, used for fitting for offset
	
	std::vector<double> integralvals(8);
	std::vector<double> offsetvals(8);
	std::vector<double> startvals(8);
	std::vector<double> endvals(8);
	std::vector<double> peaksamples(8);
	std::vector<double> peakvals(8);
	
	int numreadouts = alldata.size();
	bool verbose=false;
	
	int maxreadouts=std::numeric_limits<int>::max(); // process only the first n readouts
	
	if(verbose) std::cout<<"we have "<<numreadouts<<" readouts"<<std::endl;
	for(int readout=0; readout<std::min(maxreadouts,numreadouts); readout++){
		
		std::vector<std::vector<double>> &allreadoutdata = alldata.at(readout);
		//for(int channel=0; channel<8; channel++){
			
			std::vector<double> data = allreadoutdata.at(channel);
			if(data.size()==0){
				continue;
			}
			
			double maxvalue = (*(std::max_element(data.begin(),data.end())));
			double minvalue = (*(std::min_element(data.begin(),data.end())));
			if(hwaveform==nullptr){ hwaveform = new TH1D("hwaveform","Sample Distribution",200,minvalue,maxvalue); }
			else { hwaveform->Reset(); hwaveform->SetAxisRange(minvalue, maxvalue, "X"); }
			for(double sampleval : data){ hwaveform->Fill(sampleval); }
			
#ifdef FIT_OFFSET  // actually the fit really isn't needed, just take the bin with the highest count.
#ifdef DRAW_OFFSETFIT
			if(gROOT->FindObject("c1")==nullptr) c1 = new TCanvas("c1");
			c1->cd();
			hwaveform->Draw();
			c1->Modified();
			c1->Update();
			gSystem->ProcessEvents();
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
			if(gROOT->FindObject("c1")==nullptr) c1 = new TCanvas("c1");
			c1->cd();
			gausfit->Draw("same");
			c1->Modified();
			c1->Update();
			gSystem->ProcessEvents();
#endif
#else // !FIT_OFFSET
			double gauscentre = hwaveform->GetXaxis()->GetBinCenter(hwaveform->GetMaximumBin());
#endif
			
			uint16_t peaksample = std::distance(data.begin(), std::min_element(data.begin(),data.end()));
			double peakamplitude = data.at(peaksample);
			if(verbose) std::cout<<"peak sample is at "<<peaksample<<" with peak value "<<peakamplitude<<std::endl;
			
			// from the peak sample, scan backwards for the start sample
			int startmode = 2;
			double thresholdfraction = 0.1;  // 20% of max
			if(verbose) std::cout<<"threshold value set to "
													 <<((peakamplitude-gauscentre)*thresholdfraction)
													 <<" ADC counts"<<std::endl;
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
			if(verbose) std::cout<<"converted integral = " <<thecharge<<std::endl;
			
			// draw the waveform
			if(waveform) delete waveform;
			std::vector<double> numberline(data.size());
			std::iota(numberline.begin(),numberline.end(),0);
			std::vector<double> offsetsubtractedata;
			if(subtractoffset){
				for(auto aval : data) offsetsubtractedata.push_back(aval-gauscentre);
				waveform = new TGraph(data.size(), numberline.data(), offsetsubtractedata.data());
			} else {
				waveform = new TGraph(data.size(), numberline.data(), data.data());
			}
			if(gROOT->FindObject("c2")==nullptr) c2 = new TCanvas("c2");
			c2->cd();
			waveform->Draw("alp");
			c2->Modified();
			c2->Update();
			gSystem->ProcessEvents();
			
			// std::this_thread::sleep_for(std::chrono::seconds(15));
			do{
				gSystem->ProcessEvents();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			} while (gROOT->FindObject("c2")!=nullptr);
			//while(c2->GetCanvasImp()!=nullptr);
			
			delete waveform; waveform=nullptr;
			
	//	}// end loop over channels for a given readout
		
	} // end loop over readouts
	
	// cleanup
	// reset the branch addresses before any subsequent Draw() calls, they will soon to invalid vectors
	if(hwaveform){ delete hwaveform; hwaveform=nullptr; }
	if(waveform){ delete waveform; waveform=nullptr; }
	if(gROOT->FindObject("c2")!=nullptr){ delete c2; c2=nullptr; }
	return 1;
}

//****************************************************************************
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
