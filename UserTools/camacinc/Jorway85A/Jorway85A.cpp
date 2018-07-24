#include "CamacCrate.h"
#include "Jorway85A.h"

Jorway85A::Jorway85A(int NSlot, std::string config, int i) : CamacCrate(i)	//Subclass constructor, n of Slot given
{
	Slot.push_back(NSlot);
	ID = Slot.size()-1;
	//ClearAll();
	SetConfig(config);
//	EncRegister();
	
	//std::cout << "l slot size " << Slot.size() << std::endl;
	//	for (int i = 0; i < Slot.size(); ++i)
	  //	std::cout << "ll " << Slot.at(i) << std::endl;
	  //std::cout<<"l7"<<std::endl; 
}

int Jorway85A::ReadScaler(int scalernum) //Read Scaler scalernum.
{
	std::cout<<"reading scalar "<<scalernum;
	if(scalernum>3||scalernum<0) return -1;
	int Data = 0;
	int Q = 0, X = 0;

	int ret = READ(scalernum, 0, Data, Q, X);
	std::cout<< ", return val = "<< ret<<", Data = "<<Data<<std::endl;
	if (ret < 0){
		return ret;
	} else {
		counts[scalernum] = Data;
		return Data;
	}
}

int Jorway85A::ClearScaler(int scalernum) // Clear Scaler scalernum.
{
	if(scalernum>3||scalernum<0) return -1;
	int Data = 0;
	int Q = 0, X = 0;
	int ret = WRITE(9, scalernum, Data, Q, X);
	counts[scalernum] = 0;
	return Q;
}

int Jorway85A::TestChannel(int scalernum)	//Test Scaler scalernum
{
	if(scalernum>3||scalernum<0) return -1;
	int Data = 0;
	int Q = 0, X = 0;
	int ret = READ(25, scalernum, Data, Q, X);
	if (ret < 0) return ret;
	else return Q;
}

int Jorway85A::ReadAndClear(int scalernum) // combined read and clear - is this how it works?
{
	if(scalernum>3||scalernum<0) return -1;
	int Data = 0;
	int Q = 0, X = 0;
	int ret = READ(2, scalernum, Data, Q, X);
	if (ret < 0){
		return ret;
	} else {
		counts[scalernum] = 0;
		return Q;
	}
}

int Jorway85A::ReadAll(int *Data) //Read all scalers
{
	int returnval=0;
	for(int scalernum=0; scalernum<4; scalernum++){
		std::cout << "Scalernum = " << scalernum << " ";
		returnval = ReadScaler(scalernum);
		Data[scalernum]=counts[scalernum];
	}
	return returnval;
}

int Jorway85A::ClearAll() //Clear all scalers
{
	int returnval=0;
	for(int scalernum=0; scalernum<4; scalernum++){
		returnval += ClearScaler(scalernum);
	}
	return returnval;
}

//////////////////////
// common CAMAC commands

int Jorway85A::READ(int F, int A, int &Data, int &Q, int &X)	//Generic READ
{
	long lData;
	int ret = CamacCrate::READ(GetID(), F, A, lData, Q, X);
	Data = lData;
	//std::cout << "GetID (): " << GetID() << "\n lData = " << lData << std::endl;
	return ret;
}

int Jorway85A::WRITE(int F, int A, int &Data, int &Q, int &X)	//Gneric WRITE
{
	long lData = long(Data);
	return CamacCrate::WRITE(GetID(), F, A, lData, Q, X);
}

int Jorway85A::GetID()		//Return ID of module
{
	return ID;
}

int Jorway85A::GetSlot()	//Return n of Slot of module
{
	return Slot.at(GetID());
}

//int Jorway85A::EnLAM() //Enable LAM.
//{
//	int Data = 0;
//	int Q = 0, X = 0;
//	int ret = READ(0, 26, Data, Q, X);
//	return ret;
//}

//int Jorway85A::TestLAM() //Test LAM.
//{
//	int Data = 0;
//	int Q = 0, X = 0;
//	int ret = READ(0, 8, Data, Q, X);
//	return Q;
//}

//int Jorway85A::ClearLAM() //Clear LAM.
//{
//	int Data = 0;
//	int Q = 0, X = 0;
//	int ret = READ(0, 10, Data, Q, X);
//	return ret;
//}

//int Jorway85A::DisLAM() //Disable LAM.
//{
//	int Data = 0;
//	int Q = 0, X = 0;
//	int ret = READ(0, 24, Data, Q, X);
//	return ret;
//}

// not really applicable, but set any internal configurations with settings from file
void Jorway85A::SetConfig(std::string config)
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
				
				// Read internal parameters from file here... 
				if (sEmp == "Common") Common = iEmp;
			}
		}
	}
	//std::cout<<"conf 8"<<std::endl;
	fin.close();
	//std::cout<<"conf 9"<<std::endl;
}

