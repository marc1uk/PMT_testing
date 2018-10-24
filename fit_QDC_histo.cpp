/* vim:set noexpandtab tabstop=4 wrap */
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>


#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1.h"

#include "TApplication.h"
#include "TSystem.h"

using std::cout;
using std::endl;

int fit_QDC_histo(const char* QDCfile, int channel_id){  // Read QDC data  ROOT file
	
	gStyle->SetOptFit(1111);
	
	double pedestal[16] = {29, 24, 38, 33, 38, 30, 31, 29, 31, 31, 36, 28, 31, 31, 29, 31};
	
	int myargc = 0;
	char *myargv[] = {(const char*) "somestring"};
	//TApplication *FitSPE = new TApplication("FitSPE",&myargc,myargv);  // only if compiling and linking against ROOT! not if calling FROM ROOT
	
	
	//gROOT->ProcessLine("#include <vector>");
	
	// read the ROOT input file
	std::string QDCfilestring_1=QDCfile;
	std::stringstream ss_ch;
	ss_ch<<channel_id;
	std::string string_ch = ss_ch.str();
	std::string QDCfilestring=QDCfilestring_1+"_ch"+string_ch+".root";
	std::string QDCfilestring_pdf = QDCfilestring_1+"_ch"+string_ch+"_fit.pdf";
	cout<<"making file of name "<<QDCfilestring<<endl;
	TFile* filein = new TFile(QDCfilestring.c_str(),"READ");
	filein->cd();
	
	// create the tree and branches
	vector<int> * qdc_val =0;
	//int qdc_val[16];
	TTree* treein = (TTree*) filein->Get("QDCvals");
	TH1F *hist_qdc = new TH1F("hist_qdc","QDC values",100,0,100);
	treein->SetBranchAddress("QDCvs",&qdc_val);
	
	for (int i_entry=0;i_entry<treein->GetEntries();i_entry++){
		treein->GetEntry(i_entry);
		//std::cout <<qdc_val->size();
		std::cout <<qdc_val.at(channel_id)<<std::endl;
		hist_qdc->Fill(qdc_val.at(channel_id));
	}
	
	TCanvas *c1 = new TCanvas("c1","fit_canvas",900,600);
	hist_qdc->Draw();
	c1->Modified();
	c1->Update();
	
	TF1 *fit_func = new TF1("fit_func","gaus(0)+gaus(3)",0,80);
	fit_func->SetParameters(1500,pedestal[channel_id],2,200,pedestal[channel_id]+10,5);
	hist_qdc->Fit("fit_func","R");
	c1->Modified();
	c1->Update();
	
	double mean_pedestal;
	double mean_spe;
	
	mean_pedestal = fit_func->GetParameter(1);
	mean_spe = fit_func->GetParameter(4);
	std::cout <<"Pedestal mean is: "<<mean_pedestal<<std::endl;
	std::cout <<"SPE mean is: "<<mean_spe<<std::endl;
	double gain = (mean_spe-mean_pedestal)*0.25E-12/1.6E-19;	//0.25pC/count (see LeCroy4300B manual), 1.6E-19C for 1 electron
	double dummy;
	std::cout <<"Result: Gain is "<<gain<<std::endl;
	//std::cout <<"Press any key + enter to exit."<<std::endl;
	//std::cin >> dummy;
	c1->SaveAs(QDCfilestring_pdf.c_str());
	c1->SaveAs(TString::Format("%s_ch%d_fit.C",QDCfile,channel_id));
	c1->SetLogy();
	c1->SaveAs(TString::Format("%s_ch%d_fit_logy.pdf",QDCfile,channel_id));
	return 0;
}

