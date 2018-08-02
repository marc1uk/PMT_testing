#include "CamacCrate.h"
#include "my_Caen_C117B.h"

CAENC117B::CAENC117B(int NSlot, std::string config, int i) : CamacCrate(i)	//Subclass constructor, n of Slot given
{
	Slot.push_back(NSlot);
	ID = Slot.size()-1;
	ClearAll();
	SetConfig(config);

	std::map<std::string,uint16_t> function_to_opcode_new{
		{"read_fw_ver",0x00},
		{"read_card_type",0x03},
		{"read_crate_map",0x04},
		{"read_channel_status",0x01},
		{"read_channel_params",0x02},
		{"set_channel_v0set",0x10},
		{"set_channel_v1set",0x11},
		{"set_channel_i0set",0x12},
		{"set_channel_i1set",0x13},
		{"set_channel_vswmax",0x14},
		{"set_channel_rup",0x15},
		{"set_channel_rdwn",0x16},
		{"set_channel_trip",0x17},
		{"set_channel_flags",0x18}
	};
	function_to_opcode = function_to_opcode_new;

	std::map<std::string, uint16_t> setting_to_mask_new{
		{"power",11},
		{"enable",14},
		{"powerup_status",15},
		{"password_enable",12}
	};
	setting_to_mask = setting_to_mask_new;

	std::map<std::string, uint16_t> setting_to_flag_new{
		{"enable",6},
		{"power",3},
		{"powerup_status",7},
		{"password_enable",4}
	};
	setting_to_flag = setting_to_flag_new;
	
	ReadFwVer();
}

short CAENC117B::ReadBuffer(int &Data, int &Q){
	int X;
	short numbytes = READ(0, 0, Data, Q, X);
	if(X==0){ std::cerr << "unrecognised function" << std::endl; return -3; }
	else if(numbytes<0){ std::cerr << "bad CAMC read" << std::endl; return -1; }
	else if(Q==0){ std::cerr << "no data in buffer" << std::endl; return -2; }
	else return numbytes;
	// n.b. if LAM is enabled, it will be raised once a response is obtained, 
	// and held high for subsequent READ calls, until the last datum has been read.
	// can therefore test LAM after each READ to check if there is more data
	
	// if LAM is not enabled, you must read until Q=1 (data received)
	// then read until Q=0 (no more data)
}

short CAENC117B::DoRead(std::vector<uint16_t> &readvals, int timeout_ms){
	unsigned long start_ms = clock();
	unsigned long curr_ms = start_ms;
	int Data;
	int Q;
	readvals.clear();
	short ret;
	while ((curr_ms-start_ms)<timeout_ms){
		ret = ReadBuffer(Data,Q);
		if(ret<0){ break; } // read error
		else if(ret>0 && Q==1){ readvals.push_back(static_cast<uint16_t>(Data)); break; }
		else curr_ms = clock();
		usleep(100); // don't poll *too* often
	}
	if(ret<0) return ret;
	else if(readvals.size()==0){ std::cerr<<"read timeout waiting for Q=1"<<std::endl; return -2; }
	
	// reset timers and read buffer contents
	start_ms = clock();
	curr_ms = start_ms;
	while ((curr_ms-start_ms)<timeout_ms){
		ret = ReadBuffer(Data,Q);
		if(ret<0){ break; } // read error 
		else if(ret>0 && Q==1){ readvals.push_back(static_cast<uint16_t>(Data)); }
		else if(ret>0 && Q==0){ break; } // end of data
		else curr_ms = clock();
		usleep(100); // don't poll *too* often
	}
	if(ret<0) return ret;
	else return 0;
}

int CAENC117B::TestLAM(){
	int Data;
	int Q, X;
	short ret = READ(8, 0, Data, Q, X);
	if(X==0){ std::cerr<< "unrecognised function" << std::endl; return -3; }
	else if(ret<0){ std::cerr << "bad CAMC read" << std::endl; return -1; }
	else return Q;
}

int CAENC117B::ClearAll(){
	int Data;
	int Q, X;
	short ret = READ(9, 0, Data, Q, X);
	if(X==0){ std::cerr<< "unrecognised function" << std::endl; return -3; }
	else if(Q==0){ std::cerr << "uhh, no Q response?" << std::endl; return -2; }
	else if(ret<0){ std::cerr << "bad CAMC read" << std::endl; return -1; }
	else {
		usleep(3000); // node will sleep for 3ms after a reset
		return ret;
	}
}

int CAENC117B::FillTxBuffer(uint16_t datain){
	int Data = datain;
	int Q, X;
	short ret = WRITE(16, 0, Data, Q, X);
	if(X==0){ std::cerr<< "unrecognised function" << std::endl; return -3; }
	else if(ret<0){ std::cerr << "bad CAMC write" << std::endl; return -1; }
	else if(Q==0){ std::cout << "buffer full or node is busy" << std::endl; return -2;}
	else return Q;
}

short CAENC117B::SendTxBuffer(){
	int Data=1;
	int Q, X;
	short ret = WRITE(17, 0, Data, Q, X);
	if(X==0){ std::cerr<< "unrecognised function" << std::endl; return -3; }
	else if(Q==0){ std::cerr << "unable to transmit - node may be busy" << std::endl; return -2; }
	else if(ret<0){ std::cerr << "bad CAMC read" << std::endl; return -1; }
	else return ret;
}

void CAENC117B::PrintError(int Data){
	if(Data==0) return;
	if(Data==0xFF00){ std::cerr<<"module busy"<<std::endl;}
	if(Data==0xFF01){ std::cerr<<"unrecognised code or incorrect message"<<std::endl;}
	if(Data==0xFF02){ std::cerr<<"value out of range"<<std::endl;}
	if(Data==0xFF03){ std::cerr<<"channel or board not present"<<std::endl;}
	if(Data==0xFFFD){ std::cerr<<"transmit buffer called with empty buffer"<<std::endl;}
	if(Data==0xFFFE){ std::cerr<<"controller identifier is incorrect"<<std::endl;}
	if(Data==0xFFFF){ std::cerr<<"timeout: addressed module does not exist"<<std::endl;}
}
short CAENC117B::DisableLAM(){
	int Data;
	int Q, X;
	short ret = WRITE(24, 0, Data, Q, X);
	if(X==0){ std::cerr<< "unrecognised function" << std::endl; return -3; }
	else if(Q==0){ std::cerr << "uhh, no Q response?" << std::endl; return -2; }
	else if(ret<0){ std::cerr << "bad CAMC read" << std::endl; return -1; }
	else return ret;
}

short CAENC117B::EnableLAM(){
	int Data;
	int Q, X;
	short ret = WRITE(26, 0, Data, Q, X);
	if(X==0){ std::cerr<< "unrecognised function" << std::endl; return -3; }
	else if(Q==0){ std::cerr << "uhh, no Q response?" << std::endl; return -2; }
	else if(ret<0){ std::cerr << "bad CAMC read" << std::endl; return -1; }
	else return ret;
}

int CAENC117B::AssembleBuffer(std::vector<uint16_t> opcodes_and_values){
	int ret = FillTxBuffer(0x0001); // controller identifier code
	ret = FillTxBuffer(0x0001); // crate number
	for(int vali=0; vali<opcodes_and_values.size(); vali++){
		uint16_t aval = opcodes_and_values.at(vali);
		FillTxBuffer(aval); // SY527 operation code and vals if applicable
	}
	return ret;
}

short CAENC117B::AssembleAndTransmit(std::vector<uint16_t> opcodes_and_values){
	int ret = AssembleBuffer(opcodes_and_values);
	if(ret<0){ return ret; }
	ret = SendTxBuffer();
	if(ret<0){ return ret; }
	usleep(2000000); // wait at least 500ms for a reply
	
	// check for an error in the Rx buffer:
	int Q;
	int Data;
	ret = ReadBuffer(Data, Q);
	if(ret<0){ std::cerr << "bad CAMAC read checking read buffer for Tx ack" << std::endl; return -1; }
	else if(Data==0xFFFF){ std::cerr << "Transmit timed out waiting for reply" << std::endl; return -4; }
	else if((Data | 0x00FF)==0xFFFF){
		uint16_t error_code = (Data & 0x00FF);
		std::cerr << "Tx ack error: " << Data << " (code " << error_code << ")" << std::endl; 
		PrintError(Data);
		return -5;
	}
	return ret;
}

uint16_t CAENC117B::card_chan_to_word(int card, int channel){
	uint16_t word = card << 8;
	word |= channel;
	// std::cout << std::hex << std::uppercase << word << std::endl;
	return word;
}

uint16_t CAENC117B::setting_to_word(std::string setting, bool value){
	uint16_t mask = 1;
	mask = mask << (setting_to_mask.at(setting));
	uint16_t flag = value;
	flag = flag << (setting_to_flag.at(setting));
	uint16_t word = mask & flag;
	return word;
}

short CAENC117B::ReadFwVer(){
	std::vector<uint16_t> opcode;
	opcode.push_back(function_to_opcode.at("read_fw_ver"));
	std::cout<<"assemble&tx"<<std::endl;
	short ret = AssembleAndTransmit(opcode);
	std::cout<<"retd"<<std::endl;
	if(ret<0) return ret;
	std::vector<uint16_t> response;
	std::cout<<"reading ack"<<std::endl;
	ret = DoRead(response);
	std::cout<<"read ack"<<std::endl;
	if(ret<0) return ret;
	std::string response_string;
	std::cout<<"reading response"<<std::endl;
	for(int vali=0; vali<response.size(); vali++){
		uint16_t aval = response.at(vali);
		char achar = char(aval);
		response_string += achar;
	}
	std::cout<<"Response is: "<<response_string<<std::endl;
	return ret;
}

//----------------------------------------
//start of generic functions definitions
//----------------------------------------


int CAENC117B::READ(int F, int A, int &Data, int &Q, int &X)	//Generic READ
{
	long lData;
	int ret = CamacCrate::READ(GetID(), F, A, lData, Q, X);
	Data = lData;
	return ret;
}

int CAENC117B::WRITE(int F, int A, int &Data, int &Q, int &X)	//Gneric WRITE
{
	long lData = long(Data);
	return CamacCrate::WRITE(GetID(), F, A, lData, Q, X);
}

int CAENC117B::GetID()		//Return ID of module
{
	return ID;
}

int CAENC117B::GetSlot()	//Return n of Slot of module
{
	return Slot.at(GetID());
}

void CAENC117B::SetConfig(std::string config)
{
	std::ifstream fin (config.c_str());
	std::string Line;
	std::stringstream ssL;

	std::string sEmp;
	int iEmp;
	//std::cout<<"conf 1"<<std::endl;
	while (getline(fin, Line))
	{
	  //std::cout<<"conf 2"<<std::endl;
		if (Line.empty()) continue;
		if (Line[0] == '#') continue;
		else
		{
		  //	  std::cout<<"conf 3"<<std::endl;
			Line.erase(Line.begin()+Line.find('#'), Line.end());
			//	std::cout<<"conf 3.5"<<std::endl;
			ssL.str("");
			ssL.clear();
			ssL << Line;
			if (ssL.str() != "")
			{
			  //std::cout<<"conf 4"<<std::endl;
				ssL >> sEmp >> iEmp;

				if (sEmp == "Common") Common = iEmp;   //modify this line to match the real variables which will be read by the config file

			}
		}
	}
	//std::cout<<"conf 8"<<std::endl;
	fin.close();
	//std::cout<<"conf 9"<<std::endl;
}
