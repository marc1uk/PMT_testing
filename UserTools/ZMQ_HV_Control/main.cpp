#include <string>
#include <sstream>
#include <iostream>
#include "PMTTestingHVcontrol.h"
#include </home/annie_local/Downloads/zeromq-4.0.7-master/include/zmq.hpp>



int main(){

      PMTTestingHVcontrol *hv1 = new PMTTestingHVcontrol("192.168.120.9",5555);

      hv1->SetRun(3,1);

}

