#include "CamacCrate.h"
#include "CAENC117B.h"

CAENC117B::CAENC117B(int NSlot, std::string config, int i) : CamacCrate(i)	//Subclass constructor, n of Slot given
{
	Slot.push_back(NSlot);
	ID = Slot.size()-1;
	std::cout<<"Clear All: "<<ClearAll()<<std::endl;
	SetConfig(config);
}

//print out module identifier information
int CAENC117B::ModuleIdentifier(){
		
	int ret;
	std::vector<int> values(0), response;
	int controller=1;
	int CrateNumber=1;
	int operation=0;	//word 3 is %0

	//initiate Communication Sequence to read the Slot N characteristics
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	if (response[0]==0){
		std::cout<<"The Module Identifier is ";
		for (int i=1;i<=11;i++){
			std::cout<<ConvertInttoAsci(response[i]);
		}
		std::cout <<" ."<<std::endl;
	}

	return ret;
}


//read slot N Board characteristics
int CAENC117B::ReadSlotN(int n){
		
	int ret;
	std::vector<int> values(1), response;
	values[0]=n;
	int controller=1;
	int CrateNumber=1;
	int operation=3;	//word 3 is %3

	//initiate Communication Sequence to read the Slot N characteristics
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	if (response[0]==0){
		std::cout<<"The characteristics of board "<<n<<" are:"<<std::endl;
		
		int voltage[4];
		int current[2];
		int rampmin[2];
		int rampmax[2];
		int vres[2];
		int ires[2];
		int vdec[2];
		int idec[2];

		for (int i=1;i<28;i++){
			int low_byte=GetLowByte(response[i]);
			int high_byte=GetHighByte(response[i]);
			if (i<=4){
				if (i==1) std::cout<<"Board name: ";
				if (high_byte !=0) std::cout<<ConvertInttoAsci(high_byte);
				if (low_byte !=0) std::cout<<ConvertInttoAsci(low_byte);
				if (i==4) {
					std::cout<<"Current units: ";
					switch (low_byte){
						case 0:	std::cout<<"Ampere"<<std::endl;
							break;
						case 1: std::cout<<"mA"<<std::endl;
							break;
						case 2:	std::cout<<"###A"<<std::endl;
							break;
						case 3: std::cout<<"nA"<<std::endl;
							break;
					}
					}
				}
			else if (i==5){
				std::cout<<"Serial number: "<< response[i]<<std::endl;
			}
			else if (i==6){
				std::cout<<"Version of Software Libraries: ";
				std::cout<<ConvertInttoAsci(high_byte)<<".";
				std::cout<<ConvertInttoAsci(low_byte)<<std::endl;
			}
			else if (i==16){
				std::cout<<"Number of Channels on board: "<<response[i]<<std::endl;
			}
			else if (i==18){
				voltage[3]=low_byte;
			}
			else if (i==19){
				voltage[2]=high_byte;
				voltage[1]=low_byte;
			}
			else if (i==20){
				voltage[0]=high_byte;
				current[1]=low_byte;
			}
			else if (i==21){
				current[0]=high_byte;
				rampmin[1]=low_byte;
			}
			else if (i==22){
				rampmin[0]=high_byte;
				rampmax[1]=low_byte;
			}			
			else if (i==23){
				rampmax[0]=high_byte;
				vres[1]=low_byte;
			}			
			else if (i==24){
				vres[0]=high_byte;
				ires[1]=low_byte;
			}			
			else if (i==25){
				ires[0]=high_byte;
				vdec[1]=low_byte;
			}			
			else if (i==26){
				vdec[0]=high_byte;
				idec[1]=low_byte;
			}			
			else if (i==27){
				idec[0]=high_byte;
				int voltage_max=Combine4Bytes(voltage[0],voltage[1],voltage[2],voltage[3]);
				int current_max=CombineBytes(current[0],current[1]);
				int ramp_min=CombineBytes(rampmin[0],rampmin[1]);
				int ramp_max=CombineBytes(rampmax[0],rampmax[1]);
				int voltage_res=CombineBytes(vres[0],vres[1]);
				int current_res=CombineBytes(ires[0],ires[1]);
				int voltage_dec=CombineBytes(vdec[0],vdec[1]);
				int current_dec=CombineBytes(idec[0],idec[1]);
				std::cout<<"Maximum voltage: "<<voltage_max<<std::endl;
				std::cout<<"Maximum current: "<<current_max<<std::endl;
				std::cout<<"Minimum Ramp Up/Down value: "<<ramp_min<< "V/s"<<std::endl;
				std::cout<<"Maximum Ramp Up/Down value: "<<ramp_max<<" V/s"<<std::endl;
				std::cout<<"Voltage resolution: "<<voltage_res<<std::endl;
				std::cout<<"Current resolution: "<<current_res<<std::endl;
				std::cout<<"Number of significant figures (Voltage): "<<voltage_dec<<std::endl;
				std::cout<<"Number of significant figures (Current): "<<current_dec<<std::endl;
			}

		}
	}

	return ret;
}

int CAENC117B::TestOperation(int crateN){
		
	int ret;
	std::vector<int> values(0), response;
	
	int controller=1;
	int CrateNumber2=3;
	int operation=crateN;	//word 3 is %3
	

	//initiate Communication Sequence to read the Slot N characteristics
	ret = CommunicationSequence(controller,CrateNumber2,operation,values,response);
	std::cout << "Test operation finished "<<std::endl;

	return ret;
}

//read Crate Occupation
int CAENC117B::ReadCrateOccupation(){
		
	int ret;
	std::vector<int> values(0), response;
	int controller=1;
	int CrateNumber=1;
	int operation=4;		//word 3 is %4

	//initiate Communication Sequence to read crate occupation
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);

	if (response.size()==1) std::cout <<"No data received from crate. Maybe turned off?"<<std::endl;
	else {
		if (response[1]==1) std::cout << "The crate is occupied. "<<std::endl;
		if (response[1]==0) std::cout << "The crate is not occupied. "<<std::endl;
	}

	return ret;
}

//read Channel Status
int CAENC117B::ReadChannelStatus(int slot, int channel){
		
	int ret;
	std::vector<int> values(1), response;
	int controller=1;
	int CrateNumber=1;
	int operation=1;			//word 3 is %01
	values[0]=ChanneltoHex(slot,channel);

	//initiate Communication Sequence to read Channel Status
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	if (response[0]==0){
		std::cout<<"The characteristics of Channel (slot "<<slot<<" , channel "<<channel<<") are:"<<std::endl;
		
		int vmon[2];

		for (int i=1;i<6;i++){
			if (i==1){
				vmon[1]=response[i];
				}
			else if (i==2){
				vmon[0]=response[i];
				int voltage_mon=CombineBytes(vmon[0],vmon[1]);
				std::cout<<"Monitored voltage: "<<voltage_mon<<std::endl;
			}
			else if (i==3){
				int hvmax=response[i];
				std::cout<<"Maximum HV: "<<hvmax<<std::endl;
			}
			else if (i==4){
				int current_mon=response[i];
				std::cout<<"Monitored current: "<<current_mon<<std::endl;
			}
			else if (i==5){
				int channel_status=response[i];
				std::cout<<"Channel Status: "<<std::endl;
				int bit0=((channel_status >> 0)  & 0x01);
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
				if (bit0==1) std::cout<<"Channel present."<<std::endl;
				else if (bit0==0) std::cout<<"Channel not present."<<std::endl;
				if (bit5==1) std::cout<<"Internal trip."<<std::endl;
				if (bit6==1) std::cout<<"Kill."<<std::endl;
				if (bit8==1) std::cout<<"Vmax."<<std::endl;
				if (bit9==1) std::cout<<"External trip."<<std::endl;
				if (bit10==1) std::cout<<"Overvoltage."<<std::endl;
				if (bit11==1) std::cout<<"Undervoltage."<<std::endl;
				if (bit12==1) std::cout<<"Overcurrent."<<std::endl;
				if (bit13==1) std::cout<<"Down."<<std::endl;
				if (bit14==1) std::cout<<"Up."<<std::endl;
				if (bit15==1) std::cout<<"Channel on."<<std::endl;
				else if (bit15==0) std::cout<<"Channel off."<<std::endl;

			}
		}
	}

	return ret;
}

//read Channel Parameter Values
int CAENC117B::ReadChannelParameterValues(int slot, int channel){
		
	int ret;
	std::vector<int> values(1), response;
	int controller=1;
	int CrateNumber=1;
	int operation=2;			//word 3 is %02
	values[0]=ChanneltoHex(slot,channel);

	//initiate Communication Sequence to read Channel Parameter Values
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	if (response[0]==0){

		std::cout << "The parameter values of channel (slot "<<slot<<", channel "<<channel<<") are: "<<std::endl;
		int vmon[2];
		int v0set[2];
		int v1set[2];
		std::string channel_name[12];
		


		for (int i=1;i<18;i++){
			if (i==1){
				channel_name[0]=ConvertInttoAsci(GetHighByte(response[i]));
				channel_name[1]=ConvertInttoAsci(GetLowByte(response[i]));
			}
			else if (i==2){
				channel_name[2]=ConvertInttoAsci(GetHighByte(response[i]));
				channel_name[3]=ConvertInttoAsci(GetLowByte(response[i]));
			}
			else if (i==3){
				channel_name[4]=ConvertInttoAsci(GetHighByte(response[i]));
				channel_name[5]=ConvertInttoAsci(GetLowByte(response[i]));
			}
			else if (i==4){
				channel_name[6]=ConvertInttoAsci(GetHighByte(response[i]));
				channel_name[7]=ConvertInttoAsci(GetLowByte(response[i]));
			}
			else if (i==5){
				channel_name[8]=ConvertInttoAsci(GetHighByte(response[i]));
				channel_name[9]=ConvertInttoAsci(GetLowByte(response[i]));
			}
			else if (i==6){
				channel_name[10]=ConvertInttoAsci(GetHighByte(response[i]));
				channel_name[11]=ConvertInttoAsci(GetLowByte(response[i]));
				int i_channel=0;
				std::cout<<"Channel Name: ";
				while (channel_name[i_channel]!="0"){
						std::cout<<channel_name[i_channel];
						i_channel++;
				}
				std::cout<<std::endl;
			}
			else if (i==7){
				v0set[1]=response[i];
			}
			else if (i==8){
				v0set[0]=response[i];
				std::cout<<"Set V0 value: "<<CombineBytes(v0set[0],v0set[1])<<std::endl;
			}
			else if (i==9){
				v1set[1]=response[i];
			}
			else if (i==10){
				v1set[0]=response[i];
				std::cout<<"Set V1 value: "<<CombineBytes(v1set[0],v1set[1])<<std::endl;
			}
			else if (i==11){
				std::cout<<"Set IO value: "<<response[i]<<std::endl;
			}
			else if (i==12){
				std::cout<<"Set I1 value: "<<response[i]<<std::endl;
			}
			else if (i==13){
				std::cout<<"Vmax software value: "<<response[i]<<std::endl;
			}
			else if (i==14){
				std::cout<<"Ramp-Up value: "<<response[i]<<std::endl;
			}
			else if (i==15){
				std::cout<<"Ramp-Down value: "<<response[i]<<std::endl;
			}
			else if (i==16){
				std::cout<<"Trip value: "<<response[i]<<std::endl;
			}
			else if (i==17){
				std::cout<<"Flag value: "<<response[i]<<std::endl;
				int channel_status=response[i];
				int bit9=((channel_status >> 9)  & 0x01);
				int bit11=((channel_status >> 11)  & 0x01);
				int bit12=((channel_status >> 12)  & 0x01);
				int bit13=((channel_status >> 13)  & 0x01);
				int bit14=((channel_status >> 14)  & 0x01);
				int bit15=((channel_status >> 15)  & 0x01);
				if (bit9==1) std::cout<<"External Trip enabled."<<std::endl;
				if (bit11==1) std::cout<<"Power on."<<std::endl;
				else if (bit11==0) std::cout<<"Power off."<<std::endl;
				if (bit12==1) std::cout<<"Password: Required"<<std::endl;
				else if (bit12==0) std::cout<<"Password: "<<std::endl;
				if (bit13==1) std::cout<<"Power down: Ramp Down"<<std::endl;
				else if (bit13==0) std::cout<<"Power down: Kill"<<std::endl;
				if (bit14==1) std::cout<<"On/Off: Enabled"<<std::endl;
				else if (bit14==0) std::cout<<"On/Off: "<<std::endl;
				if (bit15==1) std::cout<<"Pwon: On"<<std::endl;
				else if (bit15==0) std::cout<<"Pwon: Off"<<std::endl;
			}
		}

	}
	return ret;
}

//Set Channel V0set value
int CAENC117B::SetV0(int slot, int channel, int V0){
	
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=16;				//word 3 is %10
	values[0]=ChanneltoHex(slot,channel);
	values[1]=V0;

	//initiate Communication Sequence to Set the Voltage V1
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The voltage V0 of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<V0<<std::endl;

	return ret;
}

//Set Channel V1set value
int CAENC117B::SetV1(int slot, int channel, int V1){
		
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=17;				//word 3 is %11
	values[0]=ChanneltoHex(slot,channel);
	values[1]=V1;

	//initiate Communication Sequence to Set the Voltage V1
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The voltage V1 of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<V1<<std::endl;

	return ret;
}

//Set Channel I0set value
int CAENC117B::SetI0(int slot, int channel, int I0){
	
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=18;				//word 3 is %12
	values[0]=ChanneltoHex(slot,channel);
	values[1]=I0;

	//initiate Communication Sequence to Set the current I1
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The current I0 of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<I0<<std::endl;

	return ret;
}

//Set Channel I1set value
int CAENC117B::SetI1(int slot, int channel, int I1){
	
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=19;				//word 3 is %13
	values[0]=ChanneltoHex(slot,channel);
	values[1]=I1;

	//initiate Communication Sequence to Set the current I1
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The current V1 of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<I1<<std::endl;

	return ret;
}

//Set Vmax software value
int CAENC117B::SetVmax(int slot, int channel, int Vmax){
	
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=20;				//word 3 is %14
	values[0]=ChanneltoHex(slot,channel);
	values[1]=Vmax;

	//initiate Communication Sequence to Set the Vmax software value
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The Vmax software value of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<Vmax<<std::endl;

	return ret;
}

//Set Ramp Up value
int CAENC117B::SetRampUp(int slot, int channel, int Rup){
	
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=21;				//word 3 is %15
	values[0]=ChanneltoHex(slot,channel);
	values[1]=Rup;

	//initiate Communication Sequence to Set the Ramp up value
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The Ramp Up value of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<Rup<<std::endl;

	return ret;
}

//Set Ramp Down value
int CAENC117B::SetRampDown(int slot, int channel, int Rdown){
		
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=22;				//word 3 is %16
	values[0]=ChanneltoHex(slot,channel);
	values[1]=Rdown;

	//initiate Communication Sequence to Set the Ramp Down value
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The Ramp Down value of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<Rdown<<std::endl;

	return ret;
}


//Set Trip value
int CAENC117B::SetTrip(int slot, int channel, int Trip){
	
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=23;				//word 3 is %17
	values[0]=ChanneltoHex(slot,channel);
	values[1]=Trip;

	//initiate Communication Sequence to Set the Trip value
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The Trip value of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<Trip<<std::endl;

	return ret;
}

//Set Flags values (Pon, On/Off, Password, Power)
int CAENC117B::SetFlag(int slot, int channel, int Flag){
	
	int ret;
	std::vector<int> values(2), response;
	int controller=1;
	int CrateNumber=1;
	int operation=24;				//word 3 is %18
	values[0]=ChanneltoHex(slot,channel);
	values[1]=Flag;

	//initiate Communication Sequence to Set the Vmax software value
	ret = CommunicationSequence(controller,CrateNumber,operation,values,response);
	std::cout << "The flag value of channel (slot "<<slot<<", channel "<<channel<<") was set to "<<Flag<<std::endl;

	return ret;
}




int CAENC117B::CommunicationSequence(int controller, int CrateNumber, int operation, std::vector<int> &values, std::vector<int> &response){

	ClearAll();
	usleep(5000);
	EnLAM();
	//usleep(5000);
	
	std::cout <<"ID: "<<ID<<std::endl;
	int ret=0;
	int slave_response;
	std::cout <<"CAENC117B Communication Sequence started."<<std::endl;

	//std::cout <<"Testing LAM"<<TestLAM()<<std::endl;
	//std::cout <<"Enabling LAM"<<std::endl;
	//EnLAM();
	//std::cout <<"Testing LAM again"<<TestLAM()<<std::endl;

	std::cout <<"Inititializing data values ..."<<std::endl;
	std::vector<int> data;
	data.push_back(controller);
	data.push_back(CrateNumber);
	data.push_back(operation);

	for (int i=0;i<values.size();i++){
		//std::cout<<"iteration "<<i<<" , value "<<values[i]<<std::endl;
		data.push_back(values[i]);
	}
	for (int i=0;i<data.size();i++){
		//std::cout<<"iteration "<<i<<" , data value "<<data[i]<<std::endl;
	}
	

	//write data packet in Transmit Data Buffer
	std::cout <<" Writing the packet in the Transmit Data Buffer..."<<std::endl;
	WriteFIFOall(data);
	
	//transmit the data packet
	std::cout  <<"Transmitting the data packet...."<<std::endl;
	Transmit();

	//wait for Slave Response
	std::cout <<"Waiting for Slave Response...."<<std::endl;
	slave_response=WaitforSlave();	
	response.push_back(slave_response);

	//read response

	if (slave_response == 0){
		std::cout<<"Reading rest of response ...."<<std::endl;
		ReadFIFOall(response);
	} 

	std::cout <<"Evaluating response ...."<<std::endl;
	for (int i=0;i<response.size();i++){
		std::cout <<"response at "<<i<<" : "<<response[i]<<std::endl;
	}


	if (response[0]==0){
		std::cout <<"Reading Buffer successful."<<std::endl;
	}
	else if (response[0]==65280){
		std::cout <<"ERROR (READING BUFFER): Module Busy; it has tried to effect an operation while the module is performing a previous 		operation."<<std::endl; 
	}
	else if (response[0]==65281){
		std::cout <<"ERROR (READING BUFFER): Code not recognized or message incorrect."<<std::endl; 
	}
	else if (response[0]==65282){
		std::cout <<"ERROR (READING BUFFER): Value out of range."<<std::endl; 
	}
	else if (response[0]==65283){
		std::cout <<"ERROR (READING BUFFER): Channel or Board not present."<<std::endl; 
	}
	else if (response[0]==65533){
		std::cout <<"ERROR (READING BUFFER): No data to be transmitted. Tried transmission with the Transmit data Buffer empty!"<<std::endl;
	}
	else if (response[0]==65534){
		std::cout <<"ERROR (READING BUFFER): H.S. CAENET Controller identifier is incorrect."<<std::endl;
	}
	else if (response[0]==65535){
		std::cout <<"ERROR (READING BUFFER): The addressed slave does not exist."<<std::endl;
	}
	else {
		std::cout <<"ERROR (READING BUFFER): Unknown error, please look it up in the manual for more information. The error code is: "<<response[0]<<std::endl;
	}

	return ret;
}


int CAENC117B::WaitforSlave(){
	
	int Data = 0, Q = 0, X = 0, ret=0, first=0;
	int counter=0;

	while (ret==0 /*&& counter<2000*/){
	//std::cout <<"Waiting for slave... Iteration no "<<counter<<std::endl;
	//check if LAM is enabled
	if (TestLAM()==1) {
		//std::cout <<"TestLAM = 1. LAM is true!"<<std::endl;
		first = READ(0,0,Data,Q,X);
		//std::cout <<"return: "<<first<<std::endl;
		ret=1;
	}
	/*else {		//if LAM is not enabled
	//	std::cout <<"LAM is not true"<<std::endl;
		first = READ(0,0,Data,Q,X);
	//	std::cout <<"Q = "<<Q<<std::endl;
	//	std::cout <<"return: "<<first<<std::endl;
		ret=Q;
		}*/
	//std::cout<<"sleep"<<std::endl;
	//usleep (1000);
	//std::cout<<"increment counter"<<std::endl;
	counter++;
	}
	//std::cout<<"waiting time was "<<counter<<" ms."<<std::endl;
	std::cout<<"Data value is "<<Data<<std::endl;
	return Data;

}

int CAENC117B::ReadFIFOall(std::vector<int> &mData) //Read Receive Data Buffer data until end of event, Q = 1 for valid data, Q = 0 at end.
{
	int Data = 0;
	int Q = 1, X = 0, ret = 0;
	int counter=1;

	while (Q == 1)
	{
		//std::cout <<"counter: "<<counter<<std::endl;
		ret = READ(0, 0, Data, Q, X);
		//std::cout <<"after reading"<<std::endl;
		//std::cout <<"Data = "<<Data<<std::endl;

		if (Q == 1) 
		{
			std::cout <<"Accepting data read.... Buffer filled."<<counter<<std::endl;
			mData.push_back(Data);
			//mData[counter]=Data;
		}
		else{
			std::cout <<"Discarding data read... Buffer empty."<<std::endl;
		}
		counter++;
	}
	return ret;
}

int CAENC117B::TestLAM() //Test LAM.
{
	int Data = 0;
	int Q = 0, X = 0;
	int ret = READ(8, 0, Data, Q, X);
	return Q;
}

int CAENC117B::ClearAll() //F(9)·A(0)resets the CAENC117B, i.e. buffers are cleared, LAM is cleared, LAM is disabled, every data transfer aborted, and the C117B does not accept commands anymore
{	
	int Data = 0;
	int Q = 0, X = 0;
	int ret = READ(9, 0, Data, Q, X);
	return ret;
}

int CAENC117B::WriteFIFOall(std::vector<int> &data) //F(16)·A(0): Write 16-bit data to Transmit Data Buffer, data asserted on WRITE lines W<1...16>.
{
	int controller=data[0];
	int CrateNumber=data[1];
	int operation=data[2];
	
	std::cout <<"controller number "<<controller<<", Crate number "<<CrateNumber<<", Operation number "<<operation<<std::endl;

	int Q = 1, X = 0, ret = 0, counter=1, random=0;

	while (Q == 1)
	{
		if (counter==1){
			//std::cout <<"counter is 1"<<std::endl;
			ret = WRITE(16, 0, controller, Q, X);
			//std::cout <<"data: "<<controller<<", return value: "<<ret<<std::endl;
		}
		else if (counter==2){
			//std::cout<<"counter is 2"<<std::endl;
			ret = WRITE(16, 0, CrateNumber, Q, X);
			//std::cout <<"return value: "<<ret<<", data:  "<<CrateNumber<<std::endl;
		}
		else if (counter==3){
			//std::cout<<"counter is 3"<<std::endl;
			ret = WRITE(16, 0, operation, Q, X);
			//std::cout <<"return value: "<<ret<<", data: "<<operation<<std::endl;
		}
		//else {				//
		else if (counter<=data.size()){
			//int counter2=counter*10000;
			int value=data[counter-1];
			//std::cout<<"counter is "<<counter<<std::endl;
			ret = WRITE(16, 0, value, Q, X);
			//std::cout <<"data: "<<data[counter-1]<<", return value: "<<ret<<std::endl;

		}else break;
		counter++;
		//std::cout <<"Q: "<<Q<<std::endl;
		usleep(1000000);
	}
	//std::cout <<"return value: "<<ret<<std::endl;
	std::cout <<"writing into transmit data buffer has finished."<<std::endl;
	return ret;
}

int CAENC117B::Transmit() //transmit the data packet stored in the Transmit data buffer on the cable.
{
	int set=1, Q = 0, X = 0, ret=0;
	std::cout <<"set: "<<set<<std::endl;
	ret = WRITE(17, 0, set, Q, X);

	/*for (int i=0; i<Data.size(); i++){
		set=Data[i];
		ret = WRITE(17, 0, set, Q, X);
	*/
		if (Q==1) std::cout << "data package is ready to be transmitted."<<std::endl;
		if (Q==0) std::cout << "data package is not ready to be transmitted."<<std::endl;
	//}
	std::cout <<"return value: "<<ret<<std::endl;
	std::cout <<"X value: "<<X<<std::endl;	
	std::cout <<"set: "<<set<<std::endl;


	return ret;
}

int CAENC117B::DisLAM() //Disable LAM.
{
	int Data = 0;
	int Q = 0, X = 0;
	//int ret = READ(0, 24, Data, Q, X);
	int ret = READ(24, 0, Data, Q, X);
	std::cout <<"Disabling LAM was successful: "<<Q<<std::endl;
	return ret;
}

int CAENC117B::EnLAM() //Enable LAM.
{
	int Data = 0;
	int Q = 0, X = 0;
	//int ret = READ(0, 26, Data, Q, X);
	int ret = READ(26, 0, Data, Q, X);
	std::cout<<"Enable LAM was successful: "<<Q<<std::endl;	
	return ret;
}

int CAENC117B::ChanneltoHex(int slot, int channel){

	int ret;
	ret=slot*16*16+channel;
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

unsigned int CAENC117B::Combine4Bytes(int byte1, int byte2, int byte3, int byte4){

	unsigned int combined= byte1 | (byte2<<8) | (byte3<<16) | (byte4<<24);
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

int CAENC117B::WRITE(int F, int A, int &Data, int &Q, int &X)	//Generic WRITE
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
