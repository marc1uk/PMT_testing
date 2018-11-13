#ifndef PMTTestingHVcontrol_H
#define PMTTestingHVcontrol_H

#include <string>
#include <iostream>
#include <sstream>
//#include </home/annie_local/Downloads/zeromq-4.0.7-master/include/zmq.hpp>
#include "zmq.hpp"

class PMTTestingHVcontrol{

 public:

  PMTTestingHVcontrol();

  bool SetRun(int run_number, int subrun_number);
  bool SetVoltage(int voltage);
  bool GetEvent();

 private:

  std::string HVIP;
  int HVPort;
};


#endif
