/* vim:set noexpandtab tabstop=4 wrap */
#include "CamacCrate.h"
#include "my_Caen_C117B.h"

CAENC117B::CAENC117B(int NSlot, std::string config, int i) : CamacCrate(i)	//Subclass constructor, n of Slot given
{
	Slot.push_back(NSlot);
	ID = Slot.size()-1;
	C();
	//ClearAll();  // same as above
	usleep(500000);
	SetConfig(config); // template: loads no configuration variables for now
	
	std::map<std::string,uint16_t> function_to_opcode_new{
		                                // Parameters
		{"read_fw_ver",0x00},           // None
		{"read_card_type",0x03},        // Slot number
		{"read_crate_map",0x04},        // None
		{"read_channel_status",0x01},   // slot_channel
		{"read_channel_params",0x02},   //   "    "    
		{"set_channel_v0set",0x10},     // slot_channel, V0set value
		{"set_channel_v1set",0x11},     // slot_channel, V1set value
		{"set_channel_i0set",0x12},     // slot_channel, I0 set value
		{"set_channel_i1set",0x13},     // slot_channel, I1 set value
		{"set_channel_vswmax",0x14},    // slot_channel, Vswmax value
		{"set_channel_rup",0x15},       // slot_channel, Rup value
		{"set_channel_rdwn",0x16},      // slot_channel, Rdown value
		{"set_channel_trip",0x17},      // slot_channel, Trip time
		{"set_channel_flags",0x18}      // slot_channel, Mask & Flag
	};
	function_to_opcode = function_to_opcode_new;
	
	std::map<std::string, uint16_t> setting_to_mask_new{
		{"external_trip_enable",9},
		{"power",11},
		{"password_enable",12},
		{"powerdown_status",13},
		{"enable",14},
		{"powerup_status",15}
	};
	setting_to_mask = setting_to_mask_new;
	
	std::map<std::string, uint16_t> setting_to_flag_new{
		{"external_trip_enable",1},
		{"power",3},
		{"password_enable",4},
		{"powerdown_status",5},
		{"enable",6},
		{"powerup_status",7}
	};
	setting_to_flag = setting_to_flag_new;
	
	//ReadFwVer();
}

short CAENC117B::ReadBuffer(int &Data, int &Q){
	int X;
	short numbytes = READ(0, 0, Data, Q, X);
	//std::cout<<"READ returned ret="<<numbytes<<", Q="<<Q<<", X="<<X<<std::endl;
	if(X==0){ std::cerr << "unrecognised function" << std::endl; return -3; }
	else if(numbytes<0){ std::cerr << "bad CAMC read" << std::endl; return -1; }
	else if(Q==0){ /*std::cerr << "no data in buffer" << std::endl;*/ return numbytes; }
	// numbytes typically 4 for successful read, even with Q=0, no data in read buffer
	else return numbytes;
	// n.b. if LAM is enabled, it will be raised once a response is obtained, 
	// and held high for subsequent READ calls, until the last datum has been read.
	// can therefore test LAM after each READ to check if there is more data
	
	// if LAM is not enabled, you must read while Q=1 (data received)
	// until Q=0 (no more data)
}

short CAENC117B::DoRead(std::vector<uint16_t> &readvals, int timeout_ms){
	unsigned long start_ms = clock()/(CLOCKS_PER_SEC/1000);
	unsigned long curr_ms = start_ms;
	int Data;
	int Q;
	readvals.clear();
	short ret;
	bool timedout=true;
	// first poll until we have Q.
	//std::cout<<"DoRead first loop"<<std::endl;
	while ((curr_ms-start_ms)<timeout_ms){
		ret = ReadBuffer(Data,Q);
		if(ret<0){ timedout=false; break; } // read error
		else if(ret>0 && Q==1){ readvals.push_back(static_cast<uint16_t>(Data)); timedout=false; break; }
		else if(TestLAM()){ std::cout<<"LAM break"<<std::endl; break; }        // LAM should be enabled when data is available, but ret = 0 says no bytes read?
		else curr_ms = clock()/(CLOCKS_PER_SEC/1000);
		usleep(1000); // don't poll *too* often
	}
	//std::cout<<"end of first DoRead loop - starting second loop"<<std::endl;
	if(timedout){ std::cerr<<"read timeout waiting for Q=1"<<std::endl; return -2; }
	if(ret<0){ return ret; }                    // read error
	else if((Data | 0x00FF)==0xFFFF){           // CAENC117B error code in buffer
		uint16_t error_code = (Data & 0x00FF);
		std::cerr << "Tx ack error: " << Data << " (code " << error_code << ")" << std::endl; 
		PrintError(Data);
		return -5;
	}
	
	// reset timers and read buffer contents
	// poll until we do not have Q.
	start_ms = clock()/(CLOCKS_PER_SEC/1000);
	curr_ms = start_ms;
	while ((curr_ms-start_ms)<timeout_ms){
		ret = ReadBuffer(Data,Q);
		if(ret<0){ break; }                 // read error 
		else if(ret>0 && Q==1){ readvals.push_back(static_cast<uint16_t>(Data)); }
		else if(ret>0 && Q==0){ break; }    // end of data
		else curr_ms = clock()/(CLOCKS_PER_SEC/1000);
		usleep(1000); // don't poll *too* often
	}
	//std::cout<<"end of DoRead second loop"<<std::endl;
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
	//std::cout<<"FillTxBuffer returned ret="<<ret<<", Q="<<Q<<", X="<<X<<std::endl;
	if(X==0){ std::cerr<< "unrecognised function" << std::endl; return -3; }
	else if(ret<0){ std::cerr << "bad CAMC write" << std::endl; return -1; }
	else if(Q==0){ std::cout << "buffer full or node is busy" << std::endl; return -2;}
	else return Q;
}

short CAENC117B::SendTxBuffer(){
	int Data=1;
	int Q, X;
	short ret = WRITE(17, 0, Data, Q, X);
	//std::cout<<"SendTxBuffer returned ret="<<ret<<", Q="<<Q<<", X="<<X<<std::endl;
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
	int ret = FillTxBuffer(0x0001); // controller identifier code - seems to always be this
	ret = FillTxBuffer(0x0001);     // crate number: 0x00**
	for(int vali=0; vali<opcodes_and_values.size(); vali++){
		uint16_t aval = opcodes_and_values.at(vali);
		FillTxBuffer(aval);     // SY527 operation code and vals if applicable
	}
	return ret;
}

short CAENC117B::AssembleAndTransmit(std::vector<uint16_t> opcodes_and_values){
	int ret = AssembleBuffer(opcodes_and_values);
	if(ret<0){ return ret; }
	ret = SendTxBuffer();
	if(ret<0){ return ret; }
	//std::cout<<"waiting 1s for Read buffer reply"<<std::endl;
	usleep(1000000); // wait at least 500ms for a reply
	
	// XXX this will mean we lose our response, unless we handle it here!? XXX
	// also should use DoRead not ReadBuffer, as former handles 
	// looping until end of data and timeout
//	// check for an error in the Rx buffer:
//	int Q;
//	int Data;
//	//ret = ReadBuffer(Data, Q);
//	//std::cout<<"ReadBuffer returned ret="<<ret<<", Q="<<Q<<std::endl;
//	if(ret<0){ std::cerr << "bad CAMAC read checking read buffer for Tx ack" << std::endl; return -1; }
//	else if((Data | 0x00FF)==0xFFFF){
//		uint16_t error_code = (Data & 0x00FF);
//		std::cerr << "Tx ack error: " << Data << " (code " << error_code << ")" << std::endl;
//		PrintError(Data);
//		return -5;
//	}
	return ret;
}

uint16_t CAENC117B::card_chan_to_word(int card, int channel){
	uint16_t word = static_cast<uint8_t>(card) << 8;
	word |= static_cast<uint8_t>(channel);
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
	std::vector<uint16_t> opcodes;
	opcodes.push_back(function_to_opcode.at("read_fw_ver"));
	//std::cout<<"assemble&tx"<<std::endl;
	short ret = AssembleAndTransmit(opcodes);
	//std::cout<<"retd"<<std::endl;
	if(ret<0) return ret;
	std::vector<uint16_t> response;
	//std::cout<<"reading ack"<<std::endl;
	ret = DoRead(response);
	//std::cout<<"read ack"<<std::endl;
	if(ret<0) return ret;
	std::string response_string;
	//std::cout<<"reading response"<<std::endl;
	for(int vali=0; vali<response.size(); vali++){
		uint16_t aval = response.at(vali);
		char achar = char(aval);
		response_string += achar;
	}
	std::cout<<"Firmware version is: "<<response_string<<std::endl;
	return ret;
}

//Set Channel Vset value
int CAENC117B::SetVoltage(int slot, int channel, double Vset, int buffer){
	std::vector<uint16_t> opcodes;
	uint16_t theopcode = (buffer) ? function_to_opcode.at("set_channel_v1set") : function_to_opcode.at("set_channel_v0set");
	opcodes.push_back(theopcode);
	opcodes.push_back(card_chan_to_word(slot,channel));
	opcodes.push_back(Vset*10.);  // UNITS ARE 0.1V
	
	short ret = AssembleAndTransmit(opcodes);
	if(ret<0) return ret;
	std::vector<uint16_t> response;
	ret = DoRead(response);
	if(ret<0) return ret;
	std::string response_string;
	for(int vali=0; vali<response.size(); vali++){
		uint16_t aval = response.at(vali);
		char achar = char(aval);
		response_string += achar;
	}
	//std::cout<<"Vset response (expected: empty string) is: "<<response_string<<" "<<std::endl; // has no response
	return ret;
}

//read Channel Status
int CAENC117B::ReadChannelStatus(int slot, int channel){
	
	std::vector<uint16_t> opcodes;
	opcodes.push_back(function_to_opcode.at("read_channel_status"));
	opcodes.push_back(card_chan_to_word(slot,channel));
	
	short ret = AssembleAndTransmit(opcodes);
	if(ret<0) return ret;
	std::vector<uint16_t> response;
	ret = DoRead(response);
	if(ret<0) return ret;
	
	if(response.size()!=6){
		std::cerr<<"Unexpected size of response? size is "<<response.size()<<", expected 6 bytes"<<std::endl;
	}
	
	double vmon = static_cast<double>(CombineBytes(response.at(2), response.at(1)))/10.;   // ordering??????
	std::cout<<"Monitored voltage: "<<vmon<<std::endl;
	int hvmax = response.at(3);
	std::cout<<"Vhwmax: "<<hvmax<<std::endl;
	int imon = response.at(4);
	std::cout<<"Monitored current: "<<imon<<std::endl;
	
	int channel_status = response.at(5);
	int bit0=((channel_status >> 0)  & 0x01);
	int bit3=((channel_status >> 3)  & 0x01);
	int bit4=((channel_status >> 4)  & 0x01);
	int bit5=((channel_status >> 5)  & 0x01);
	int bit6=((channel_status >> 6)  & 0x01);
	int bit8=((channel_status >> 8)  & 0x01);
	int bit9=((channel_status >> 9)  & 0x01);
	int bit10=((channel_status >> 10)  & 0x01);
	int bit11=((channel_status >> 11)  & 0x01);
	int bit12=((channel_status >> 12)  & 0x01);
	int bit13=((channel_status >> 13)  & 0x01);
	int bit14=((channel_status >> 14)  & 0x01);
	int bit15=((channel_status >> 15)  & 0x01);
	std::cout<<"Decode of Channel Status Bits: "<<std::bitset<16>(channel_status)<<std::endl;
	if (bit0==1) std::cout<<"Channel present."<<std::endl; else std::cout<<"Channel not present."<<std::endl;
	if (bit3==1) std::cout<<"Channel is current sink"<<std::endl; else std::cout<<"Channel is current source"<<std::endl; // Note: FW > 3.27
	if (bit4==1) std::cout<<"External disable active"<<std::endl; else std::cout<<"External disable inactive"<<std::endl; // Note: FW > 3.27
	if (bit5==1) std::cout<<"Internal trip."<<std::endl; else std::cout<<"No internal trip"<<std::endl;  // NOTE: FW < 3.04: not cleared when channel is re-enabled!
	if (bit6==1) std::cout<<"Killed"<<std::endl; else std::cout<<"Not Killed"<<std::endl;
	if (bit8==1) std::cout<<"Limited by hardware Vmax"<<std::endl; else std::cout<<"Not limited by hardware Vmax"<<std::endl;
	if (bit9==1) std::cout<<"External trip."<<std::endl; else std::cout<<"No external trip"<<std::endl;
	if (bit10==1) std::cout<<"Overvoltage."<<std::endl; else std::cout<<"Not in Overvoltage"<<std::endl;
	if (bit11==1) std::cout<<"Undervoltage."<<std::endl; else std::cout<<"Not in Undervoltage"<<std::endl;
	if (bit12==1) std::cout<<"Overcurrent."<<std::endl; else std::cout<<"Not in Overcurrent"<<std::endl;
	if (bit13==1) std::cout<<"Down."<<std::endl; else std::cout<<"Not ramping down"<<std::endl;
	if (bit14==1) std::cout<<"Up."<<std::endl; else std::cout<<"Not ramping up"<<std::endl;
	if (bit15==1) std::cout<<"Channel on."<<std::endl; else std::cout<<"Channel off"<<std::endl;
	std::cout<<std::endl;
	
	return ret;
}

//read Channel Parameter Values
int CAENC117B::ReadChannelParameterValues(int slot, int channel){
		
	std::vector<uint16_t> opcodes;
	opcodes.push_back(function_to_opcode.at("read_channel_params"));
	opcodes.push_back(card_chan_to_word(slot,channel));
	
	short ret = AssembleAndTransmit(opcodes);
	if(ret<0) return ret;
	std::vector<uint16_t> response;
	ret = DoRead(response);
	if(ret<0) return ret;
	
	std::cout << "The parameter values of channel (slot "<<slot<<", channel "<<channel<<") are: "<<std::endl;
	//std::cout<<"readouti=0 was "<<response.at(0)<<std::endl;  // expected: 0
	std::string channel_name;
	for(int readouti=1; readouti<6; readouti++){
		channel_name += ConvertInttoAsci(GetLowByte(response.at(readouti)));
		channel_name += ConvertInttoAsci(GetHighByte(response.at(readouti)));
	}
	std::cout<<"Channel name: "<<channel_name<<std::endl;
	int v0set = static_cast<double>(CombineBytes(response.at(8),response.at(7)))/10.;    // units are 0.1V
	std::cout<<"Set V0: "<<v0set<<std::endl;
	int v1set = static_cast<double>(CombineBytes(response.at(10),response.at(9)))/10.;  // units are 0.1V
	std::cout<<"Set V1: "<<v1set<<std::endl;
	int I0set = static_cast<double>(response.at(11))/100.;
	std::cout<<"Set I0: "<<I0set<<std::endl;
	int I1set = static_cast<double>(response.at(12))/100.;
	std::cout<<"Set I1: "<<I1set<<std::endl;
	int Vswmax = response.at(13); // precision? just one byte?
	std::cout<<"Vswmax: "<<Vswmax<<std::endl;
	int Rampuptime = response.at(14);
	std::cout<<"Rampuptime: "<<Rampuptime<<std::endl;
	int Rampdowntime = response.at(15);
	std::cout<<"Rampdowntime: "<<Rampdowntime<<std::endl;
	double Triptime = static_cast<double>(response.at(16))/10.;
	std::cout<<"Triptime: "<<Triptime<<std::endl;
	// NOTE: Manual says Flag is stored in word 17, but in fact seems to be in word 18???
	std::cout<<"Decode of Channel Status Bits: "<<std::bitset<16>(response.at(18))<<std::endl;
	int channel_status=response.at(18);
	// bits 0-7 indicate number of decimal digits in Vswmax
	int bit9 =((channel_status >>  9)  & 0x01);
	int bit11=((channel_status >> 11)  & 0x01);
	int bit12=((channel_status >> 12)  & 0x01);
	int bit13=((channel_status >> 13)  & 0x01);
	int bit14=((channel_status >> 14)  & 0x01);
	int bit15=((channel_status >> 15)  & 0x01);
	if (bit9) std::cout<<"External Trip active."<<std::endl; else std::cout<<"External Trip inactive"<<std::endl;
	if (bit11) std::cout<<"Powered on."<<std::endl; else std::cout<<"Powered off."<<std::endl;
	if (bit12) std::cout<<"Password Required"<<std::endl; else std::cout<<"Password Not Required"<<std::endl;
	if (bit13) std::cout<<"Power down: Ramp Down"<<std::endl; else std::cout<<"Power down: Kill"<<std::endl;
	if (bit14) std::cout<<"On/Off: Password Required"<<std::endl; else std::cout<<"On/Off: Password Not Required"<<std::endl;
	if (bit15) std::cout<<"Status On Crate Powerup: Enabled"<<std::endl; else std::cout<<"Status On Crate Powerup: Disabled"<<std::endl;
	std::cout<<std::endl;
	
	return ret;
}

std::string CAENC117B::ConvertInttoAsci(int asci_code){
	std::string ret;
	char c = static_cast<char>(asci_code);
	ret.push_back(c);
	return ret;
}

int CAENC117B::GetHighByte(int large_number){
	int high_byte= large_number & 0xFF;
	return high_byte;
}

int CAENC117B::GetLowByte(int large_number){
	int low_byte= large_number>>8;
	return low_byte;
}

int CAENC117B::CombineBytes(int low, int high){
	int combined= low | (high<<8);
	return combined;
}


//----------------------------------------
//start of generic functions definitions
//----------------------------------------


int CAENC117B::READ(int F, int A, int &Data, int &Q, int &X)	//Generic READ
{
	long lData;
	int ret = CamacCrate::READ(GetID(), A, F, lData, Q, X);
	Data = lData;
	return ret;
}

int CAENC117B::WRITE(int F, int A, int &Data, int &Q, int &X)	//Gneric WRITE
{
	long lData = long(Data);
	return CamacCrate::WRITE(GetID(), A, F, lData, Q, X);
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
