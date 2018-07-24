/* vim:set noexpandtab tabstop=4 wrap */
/*
 * Jorway 85A Scaler CAMAC module class
 *
 * Author: Marcus O'Flaherty, based on code by Tommaso Boschi
 */

#ifndef Jorway85A_H
#define Jorway85A_H

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <bitset>

#include "libxxusb.h"
#include "CamacCrate.h"

class Jorway85A : public CamacCrate
{
public:
/*		Jorway85A(int NSlot, int i = 0) : CamacCrate(i)	//Subclass constructor, n of Slot given
	{
		Slot.push_back(NSlot);
		ID = Slot.size()-1;
	}
*/

// Dataway Controls
// ================
// Initialize: Z-S2 Clear all scalers & overflows
// Clear: C-S2 Clear all scalers & overflows
// Inhibit: I controls gating of all channels when front panel gating switch is in appropriate position.
// A Q response is generated for each Read, Clear and Test command.
// ================
//#   N-FO-AO                   Read Scaler 1
//#   N-FO-Al                   Read Scaler 2
//#   N-FO-A2                   Read Scaler 3
//#   N-FO-A3                   Read Scaler 4
//#   N-F9-AO-S2                Clear Scaler 1
//#   N-F9-A1-S2                Clear Scaler 2
//#   N-F9-A2-S2                Clear Scaler 3
//#   N-F9-A3-S2                Clear Scaler 4
//#   N-F25-(AO+Al+A2+A3)-S2    Test
// Read and Clear F(2) with appropriate subaddress is included. F(2) reads channels in an identical manner to F(0) and clears subaddressed channel at S2.
// Status
// ======
// Q=1 for: N-FO-(AO+A1+A2+A3) and
// N-F9-(AO+A1+A2+A3) and
// N-F25-(AO+A1+A2+A3)
// NOTE:  The Q response for Clear (F9) and Test (F25) commands may be disabled.
// Output
// ======
// Data: 24 bits read out on the read lines on command. Least significant digit is R1.
// Carry: A 1 us. (min. ) carry pulse is generated for each channel whenever a scaler exceeds its capacity.

///////////////////////////

	Jorway85A(int NSlot, std::string config, int i = 0);// : CamacCrate(i);	//Constructor !

	int ReadScaler(int scalernum);							//F(0)·A(scalernum), scalernum 0-3
	int ClearScaler(int scalernum);							//F(9)·A(scalernum)·S2
	int TestChannel(int scalernum);							//F(25)·A(0+1+2+3?)·S2
	int ReadAndClear(int scalernum);				//F(2)·A(scalernum)·S??
	
	int READ(int F, int A, int &Data, int &Q, int &X);		//F(x)·A(y) 
	int WRITE(int F, int A, int &Data, int &Q, int &X);		//F(x)·A(y)

	int ReadAll(int *Data);					//Read all scalers
	int ClearAll();							//Clear all scalers
	int GetID();							//Return ID
	int GetSlot();							//Return Slot
	
	void SetConfig(std::string config);		//Set register from configfile

private:

	uint64_t counts[4];
	int ID;
	int Common; // just for demonstration purposes of reading from config file

};

#endif

//////////////////////////////////////////////
// from other example files
//			#define JORWAY85A_CHANNELS 4
//			#define JORWAY_F_CLEAR 9
//			#define JORWAY_F_DATA 0
//			
//			long int scalData[CAMAC_N_SLOTS][JORWAY85A_CHANNELS];
//			void readJorway(usb_dev_handle *crate, int n, int nChannels, long int * data)
//			{
//			  int retcod = 0;
//			  int qResponse=0;
//			  int xResponse=0;
//			  int i;

//			  if ( n > 0 )
//			  {
//			    for ( i=0; i<nChannels; i++)
//			    {
//			      retcod = CAMAC_read(crate,
//						  n,
//						  i,
//						  JORWAY_F_DATA,
//						  &data[i],
//						  &qResponse,
//						  &xResponse);
//			      checkResponse(n,JORWAY_F_DATA,retcod, qResponse,xResponse);
//			    }
//			  }
//			}
//			
//			void configureJorway(usb_dev_handle *crate, int n)
//			{
//			  int i,q,x,retcod;
//			  long int data;

//			  /* Clear scaler counts */

//			  for (i=0; i<JORWAY85A_CHANNELS; i++)
//			  {
//			    retcod = CAMAC_read(crate,
//						n,
//						i,
//						JORWAY_F_CLEAR,
//						&data,
//						&q,
//						&x);
//			    checkResponse(n,JORWAY_F_CLEAR,retcod, q,x);
//			  }
//			}
