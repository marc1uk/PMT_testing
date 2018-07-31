#include "CamacCrate.h"
#include <cstdint>
bool CamacCrate::IsOpen = false;
int CamacCrate::ndev = 0;
xxusb_device_type CamacCrate::devices[100] = {};
struct usb_device *CamacCrate::Mdev = 0;
usb_dev_handle *CamacCrate::Mudev = 0;
std::vector<int> CamacCrate::Slot;

CamacCrate::CamacCrate(int i)	//Class constructor
{
  	//std::cout<<"c1 open = " << CamacCrate::IsOpen <<std::endl;
	if (!CamacCrate::IsOpen) {
	    Init(i);
	}
	//else std::cout << "\nWarning: USB device has already been opened\n\n";   // FIXME how are you supposed to do it?
}

CamacCrate::~CamacCrate()		//Class destructor
{
	if (CamacCrate::IsOpen) USBClose();
	CamacCrate::IsOpen = false;
}

void CamacCrate::Init(int i)		//Initialize device
{
	Slot.push_back(25);		// Find XX_USB devices and open the first one found.
	USBFind();
	USBOpen(i);
	Z();
	C();
}

void CamacCrate::USBFind()		//Find usb devices
{
	ndev = xxusb_devices_find(devices);
	for (int i = 0; i < ndev; i++)
		std::cout << i << " device is " << devices[i].SerialString << std::endl;
}

void CamacCrate::USBOpen(int i)		//Open i device and create handler
{
	Mdev = devices[i].usbdev;
	Mudev = xxusb_device_open(Mdev); 
	if(!Mudev) std::cerr << "\n\nFailed to open CC-USB. \n" << std::endl;
	else 
	{
		CamacCrate::IsOpen = true;
		std::cout << "\n\nCC-USB opened. \n" << std::endl;
	}
}

void CamacCrate::USBClose()		//Close USB devices
{
	int ret = xxusb_device_close(Mudev);
	if (ret < 0) std::cerr << "\n\nFailed to close CC-USB. \n" << std::endl;
	else std::cout << "\n\nCC-USB closed. \n" << std::endl;
}

int CamacCrate::SetLAMmask(std::string &Mask)
{
	int Q = 0, X = 0;
	std::cout << "L0\n";
	std::bitset<24> lam(Mask);
	std::cout << "L1\n";
	long llam = lam.to_ulong();
	std::cout << "L2\n";
	return WRITE(0, 9, 16, llam, Q, X); //LAM
}

int CamacCrate::SetLAMmask(long &Mask)
{
	int Q = 0, X = 0;
	return WRITE(0, 9, 16, Mask, Q, X); //LAM
}

int CamacCrate::GlobalRegister(std::string &Reg)
{
	int Q = 0, X = 0;
	std::bitset<16> glob(Reg);
	long lglob = glob.to_ulong();
	return WRITE(0, 1, 16, lglob, Q, X);
}

int CamacCrate::GlobalRegister(long &Reg)
{
	int Q = 0, X = 0;
	return WRITE(0, 1, 16, Reg, Q, X);
}

void CamacCrate::LoadStack(std::string fname)
{
	std::ifstream fin (fname.c_str());
	std::string Line;
	std::stringstream ssL;
	long val;

	vStack.clear();

	while (getline(fin, Line))
	{
		if (Line[0] == '#') continue;
		else
		{
			ssL.str("");
			ssL.clear();
			ssL << Line;
			ssL >> std::hex >> val;
			vStack.push_back(val);
		}
	}
	if (vStack.size() > 768)
		std::cout << "Stack too long! Exceeding max value (768 words)\n\n";
}

void CamacCrate::EncStack(std::string fname)
{
	std::ifstream fin (fname.c_str());
	std::string Line;
	std::stringstream ssL;
	int N, A, F, CB, max, cmd;

	vStack.clear();

	while (getline(fin, Line))
	{
		if (Line[0] == '#') continue;
		else
		{
			ssL.str("");
			ssL.clear();
			ssL << Line;
			ssL >> N;
			ssL >> A;
			ssL >> F;
			ssL >> CB;
			ssL >> max;
			cmd = (CB << 15) + (N << 9) + (A << 5) + F;
			vStack.push_back(cmd);
			if (CB)
			{
				cmd = 1 << 4;
				vStack.push_back(cmd);
				vStack.push_back(max);
			}
		}
	}
	fin.close();

	vStack.push_back(0x0010);
	vStack.push_back(0xffff);

	if (vStack.size() > 768)
		std::cout << "Stack too long! Exceeding max value (768 words)\n\n";
}

int CamacCrate::PushStack()
{
	long stack[768];
	stack[0] = vStack.size();
	for (int i = 1; i < vStack.size()+1; i++)
		stack[i] = vStack.at(i);
	return xxusb_stack_write(Mudev, 2, stack);
}

int CamacCrate::PullStack()
{
	long stack[768];
	int ret = xxusb_stack_read(Mudev, 2, stack);
	vStack.clear();
	if (ret >= 0)
	{
		for (int i = 0; i < ret/2; i++)
			vStack.push_back(stack[i]);
	}
	return ret;
}

void CamacCrate::PrintStack()
{
	std::cout << "Stack\n";

	for (int i = 0; i < vStack.size(); i++)
	{
		std::cout << std::dec << i << "\t";
		std::cout << std::hex << vStack.at(i) << "\t";
		std::cout << std::dec << vStack.at(i) << "\n";
	}
}
			
int CamacCrate::StartStack()
{
        return xxusb_register_write(Mudev, 1, 0x1); // start acquisition
}

int CamacCrate::StopStack()
{
        int ret = xxusb_register_write(Mudev, 1, 0x0); // stop acquisition
	ClearFIFO();
	return ret;
}

int CamacCrate::ReadFIFO(std::vector<int> &vData)
{
	short IntArray[1000];
	std::string dt;
	std::stringstream ssdt;
	int ret = xxusb_bulk_read(Mudev, IntArray, 8192, 500);
	if (ret >= 0)
	{
		for (int i = 0; i < ret/2; i++)
			vData.push_back(IntArray[i]);
	}
	return ret;
}

int CamacCrate::ClearFIFO()
{
	short IntArray[1000];
	int ret;
	do
		ret = xxusb_bulk_read(Mudev, IntArray, 8192, 1000);
	while (ret > 0);
	return ret;
}

int CamacCrate::READ(int ID, int F, int A, long &Data, int &Q, int &X)	//Generic READ func, suitable for F 0-15, 24-31
{
	return CAMAC_read(Mudev, Slot.at(ID), F, A, &Data, &Q, &X);
}

int CamacCrate::WRITE(int ID, int F, int A, long &Data, int &Q, int &X)	//Generic WRITE func, suitable for F 16-23
{
	return CAMAC_write(Mudev, Slot.at(ID), F, A, Data, &Q, &X);
}

void CamacCrate::ListSlot()		//List all modules in CAMAC
{
	for (int i = 0; i < Slot.size(); i++)
		std::cout << "[" << i << "] :\tmodule slot " << Slot.at(i);
}

int CamacCrate::GetSlot(int ID)		//Return n of Slot, ID given
{
	std::cout << "CC slot\n";
	return Slot.at(ID);
}

int CamacCrate::Z()			//Global ZERO
{
	return CAMAC_Z(Mudev);
}

int CamacCrate::C()			//Global CLEAR
{
	return CAMAC_C(Mudev);
}

int CamacCrate::I(bool inh)		//Global INHIBIT
{
	return CAMAC_I(Mudev, inh);
}

int CamacCrate::BittoInt(std::bitset<16> &bitref, int a, int b)
{
	int tmp, sum = 0;
	if (a > b)
	{
		tmp = a;
		a = b;
		b = tmp;
	}
	for (int i = b; i >= a; i--)
	{
		sum *= 2;
		sum += bitref.test(i);
	}
	return sum;
}
/*Useful PMT Test Stand Functions, see CC_USB manual section 7 for description.
*
*Added by M Calle
*/
short CamacCrate::DelayGateGen(short chan, short trig, short out, int delay, int gate, short invert, short latch)//Produces A NIM gate signal outputted from A front panel channel
{
	return CAMAC_DGG(Mudev, chan, trig, out, delay, gate, invert, latch);	
}
short CamacCrate::RegWrite(short A, long Data)
{
	return CAMAC_register_write(Mudev, A, Data);
}
short CamacCrate::RegRead(short A, long &Data)
{
	return CAMAC_register_read(Mudev, A, &Data);
}
short CamacCrate::CC_NIM_Out(short A, short F, short inv, short latch)//Controls NIM output on the CC-USB -> doesn't produce a gate
{
	return CAMAC_Output_settings(Mudev, A, F, inv, latch);
}
short CamacCrate::ActionRegWrite(long value)
{
	//Writes data to the action register. Should return number of bytes written to register if successful
	//If not successful, returns a -ve number.
	//value |= (1u << 1); //hopefully changes the value of the second bit to 1
	return xxusb_register_write(Mudev, 1, value);//Action register is only option, address of AR is 1
}
short CamacCrate::ActionRegRead(long &Data){
	short actionregaddr = 0;
	long databuffer=0;
	long* databufferp = &databuffer;
	short numbytes =  xxusb_register_read(Mudev, actionregaddr, databufferp);
	Data=databuffer;
	return numbytes;
	//return xxusb_register_read(Mudev, 1, &Data);
}
