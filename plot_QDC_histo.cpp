/* vim:set noexpandtab tabstop=4 wrap */
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>


#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1.h"

using std::cout;
using std::endl;

void LoadFile(const char* filepath, TTree* treetofill, std::string &timestamp, std::vector<int> &QDCvals);

int plot_QDC_histo(const char* QDCfile, int ch){  // Read QDC data into ROOT file
	
	
	// create the ROOT output file
	std::stringstream ss_ch;
	ss_ch<<ch;
	std::string string_ch = ss_ch.str();
	std::string QDCfilestring_1 = QDCfile;
	std::string QDCfilestring=QDCfilestring_1+"_ch"+string_ch;

//	std::size_t pos = QDCfilestring.find(".");      // strip extension
//	QDCfilestring = QDCfilestring.substr(0,pos);
	cout<<"making file of name "<<QDCfilestring<<".root"<<endl;
	TFile* fileout = new TFile(TString::Format("%s.root",QDCfilestring.c_str()),"RECREATE");
	fileout->cd();
	
	// create the tree and branches
	TTree* treeout = new TTree("QDCvals","QDC pulse integrals");
	std::string timestamp;
	std::vector<int> QDCvals;
	treeout->Branch("TimeStamp",&timestamp);
	treeout->Branch("QDCvs",&QDCvals);
	
	// fill the tree from the file
	std::string QDCtextfile = std::string(QDCfile)+".txt";
	//const char* QDCfile_char = (const char*) QDCtextfile;
	std::cout <<"QDC txt string file name is: "<<QDCtextfile<<std::endl;
	//std::cout <<"QDC txt file name is: "<<QDCfile_char<<std::endl;
	cout<<"filling tree"<<endl;
	LoadFile(QDCtextfile.c_str(), treeout, timestamp, QDCvals);
	
	// write to file and cleanup
	fileout->Write();
	fileout->Close();
	return 0;
}

void LoadFile(const char* filepath, TTree* treetofill, std::string &timestamp, std::vector<int> &QDCvals){
	// Loads timestamps from a file into a valarray
	// ============================================
	std::ifstream fin (filepath);           // filestream
	std::string Line;                       // file line
	std::stringstream ssL;                  // file line as stringstream
	std::string field;                      // temp variable to hold next value in csv row
	
	int linenum=0;
	unsigned int startindex=0; // first reading to fill into file
	unsigned int maxvalues=-1; // maximum number of values to read
	//cout<<"looping over file lines"<<endl;
	int count=0;
	while (getline(fin, Line))
	{
//		if (count<50000) count++;
//		else {
		//cout<<"line "<<linenum<<" = \""<<Line<<"\""<<endl;
		if (Line.empty()) continue;
		//cout<<"line is not empty"<<endl;
		if (Line[0] == '#') continue;
		//cout<<"line is not a comment"<<endl;
		linenum++;
		if ((linenum-1)<startindex) continue;
		//cout<<"line is not before we wish to start reading"<<endl;
		if (linenum>maxvalues && maxvalues>0) break;
		//cout<<"line is not after the number of lines we wish to read"<<endl;
		{
			//Remove comments at end of line
			//cout<<"stripping comments from end of line"<<endl;
			size_t commentstart = Line.find('#');
			//cout<<"Line.find('#')="<<commentstart<<endl;
			if(commentstart!=string::npos) Line.erase(Line.begin()+Line.find('#'), Line.end());
			//cout<<"putting line into stringstream object"<<endl;
			ssL.str("");                                            //clears stringstream
			ssL.clear();                                            //clears errorstate
			ssL << Line;                                            // read next line
			if (ssL.str() != ""){
				// pass into variables
				//ssL >> iReadOut >> delim >> iUTCval;          // doesn't seem to separate fields
				int fieldnum=0;
				QDCvals.clear();
				while(getline(ssL, field, ',')){                 // note SINGLE quotes!
					//cout<<"field "<<fieldnum<<" = "<<field<<endl;
					if(fieldnum==0) timestamp = field;
					/*else {
						for (int i=0;i<field.size();i++){
							QDCvals.push_back(field.at(i));
						}
	
					}*/else QDCvals.push_back(atoi(field.c_str()));
					fieldnum++;
				}
				treetofill->Fill();
			}
		}
//		count++;}
	}
	fin.close();
}

