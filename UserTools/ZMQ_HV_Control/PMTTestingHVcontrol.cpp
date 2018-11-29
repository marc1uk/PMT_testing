#include "PMTTestingHVcontrol.h"

//adapted from HVComs.cpp ToolAnalysis file

PMTTestingHVcontrol::PMTTestingHVcontrol(std::string HVIPin="192.168.163.61", int HVPortin=5555) : HVIP(HVIPin), HVPort(HVPortin){

}

// template specializations must come before any implicit uses: so these need to be at the top of the file
template <typename T>
std::string PMTTestingHVcontrol::EncodePair(std::string key, T value){
  std::string output = "\"" + key + "\":\"" + std::to_string(value) + "\"";
  return output;
}

template <>
std::string PMTTestingHVcontrol::EncodePair<std::string>(std::string key, std::string value){
  std::string output = "\"" + key + "\":\"" + value + "\"";
  return output;
}

template <>
std::string PMTTestingHVcontrol::EncodePair<const char*>(std::string key, const char* value){
  std::string output = "\"" + key + "\":\"" + std::string(value) + "\"";
  return output;
}

template <>
std::string PMTTestingHVcontrol::EncodePair<char*>(std::string key, char* value){
  std::string output = "\"" + key + "\":\"" + std::string(value) + "\"";
  return output;
}

//std::string PMTTestingHVcontrol::EncodePair(std::string key, std::string value){
//  std::string output = "\"" + key + "\":\"" + value + "\"";
//  return output;
//}

bool PMTTestingHVcontrol::SetRun(int run_number, int subrun_number){

  zmq::context_t* Context = new zmq::context_t(1); //???, apparently it should be ok...
  zmq::socket_t HV(*Context, ZMQ_DEALER);

  //zmq::socket_t HV(*(m_data->context), ZMQ_DEALER);
  std::stringstream address;
  address<<"tcp://"<<HVIP<<":"<<HVPort;
  HV.connect(address.str().c_str());

  zmq::pollitem_t HVin [] = {
    { HV, 0, ZMQ_POLLIN, 0 }
  };

  zmq::pollitem_t HVout [] = {
    { HV, 0, ZMQ_POLLOUT, 0 }
  };
  
  // cleaning up output
  bool clean=true;
  while (clean){
    zmq::poll (&HVin[0], 1, 6000);
    if (HVin[0].revents & ZMQ_POLLIN) {
      zmq::message_t message;
      HV.recv(&message);
    }
    else clean=false;
  }
  
  zmq::poll (&HVout[0], 1, 10000);
  if (HVout[0].revents & ZMQ_POLLOUT) {

    int message_id = 1;
    
    std::string tmp="";
    tmp += "{" + EncodePair("msg_id",message_id);
    tmp += "," + EncodePair("msg_type","run");
    tmp += "," + EncodePair("run",run_number);
    tmp += "," + EncodePair("subrun",subrun_number) + "}";
    
   // bb>>tmp;
    zmq::message_t send(tmp.length()+1);
    snprintf ((char *) send.data(), tmp.length()+1 , "%s" ,tmp.c_str()) ;
    HV.send(send);
    
    
    zmq::poll (&HVin[0], 1, 10000);
    if (HVin[0].revents & ZMQ_POLLIN) {  

      zmq::message_t message;
      HV.recv(&message);

      std::istringstream iss(static_cast<char*>(message.data()));

      zmq::poll (&HVin[0], 1, 20000);
      if (HVin[0].revents & ZMQ_POLLIN) {
        
        HV.recv(&message);

        std::istringstream iss2(static_cast<char*>(message.data()));
        
      }
      else {
                     std::cout<<"reply from HV time out"<<std::endl;
                     //return false;
      }
    }
    else {
              std::cout<<"akn reply from HV time out"<<std::endl;
              //return false;
    }
  }
  else {
              std::cout<<"HV time out for sending message"<<std::endl;
              // return false;
  }
  return true;
}
  
//-----------------------------------------------------------
//--------set voltage, also as a json string----------------
//-----------------------------------------------------------
  // json string structure: 
  // msg_type: "write"
  // msg_property: "vset"
  // active: voltage_value (in V)

bool PMTTestingHVcontrol::SetVoltage(int voltage){

  zmq::context_t* Context = new zmq::context_t(1); //???, apparently it should be ok...
  zmq::socket_t HV(*Context, ZMQ_DEALER);

  //zmq::socket_t HV(*(m_data->context), ZMQ_DEALER);
  std::stringstream address;
  address<<"tcp://"<<HVIP<<":"<<HVPort;
  HV.connect(address.str().c_str());

  zmq::pollitem_t HVin [] = {
    { HV, 0, ZMQ_POLLIN, 0 }
  };

  zmq::pollitem_t HVout [] = {
    { HV, 0, ZMQ_POLLOUT, 0 }
  };
  
  // cleaning up output
  bool clean=true;
  while (clean){
    zmq::poll (&HVin[0], 1, 6000);
    if (HVin[0].revents & ZMQ_POLLIN) {
      zmq::message_t message;
      HV.recv(&message);
    }
    else clean=false;
  }

  zmq::poll (&HVout[0], 1, 10000);
  if (HVout[0].revents & ZMQ_POLLOUT) {
    
    int message_id = 2;

    std::string tmp="";
    tmp += "{" + EncodePair("msg_id",message_id);
    tmp += "," + EncodePair("msg_type","write");
    tmp += "," + EncodePair("msg_property","vset");
    tmp += "," + EncodePair("active",voltage) + "}";

    zmq::message_t send(tmp.length()+1);
    snprintf ((char *) send.data(), tmp.length()+1 , "%s" ,tmp.c_str()) ;
    HV.send(send);
    
    
    zmq::poll (&HVin[0], 1, 10000);
    if (HVin[0].revents & ZMQ_POLLIN) {  

      zmq::message_t message;
      HV.recv(&message);

      std::istringstream iss(static_cast<char*>(message.data()));

      zmq::poll (&HVin[0], 1, 20000);
      if (HVin[0].revents & ZMQ_POLLIN) {
  
     HV.recv(&message);

     std::istringstream iss2(static_cast<char*>(message.data()));
    
    }
    else {
          std::cout<<"reply from HV time out"<<std::endl;
         }
    }
    else {
          std::cout<<"akn reply from HV time out"<<std::endl;
         }
    }
    else {
          std::cout<<"HV time out for sending message"<<std::endl;
    }
  return true;
}

//-------------------------------------------------------------------------
//-------------------------GetEvent----------------------------------------
//-------------------------------------------------------------------------

bool PMTTestingHVcontrol::GetEvent(){
  //original finalizing part
  

  zmq::context_t* Context = new zmq::context_t(1); //???, apparently it should be ok...
  zmq::socket_t HV(*Context, ZMQ_DEALER);

  std::stringstream address;
  address<<"tcp://"<<HVIP<<":"<<HVPort;
  HV.connect(address.str().c_str());
  
  zmq::pollitem_t HVin [] = {
    { HV, 0, ZMQ_POLLIN, 0 }
  };
  
  zmq::pollitem_t HVout [] = {
    { HV, 0, ZMQ_POLLOUT, 0 }
  };
  
  bool clean=true;
  while (clean){
    zmq::poll (&HVin[0], 1, 6000);
    if (HVin[0].revents & ZMQ_POLLIN) {
      zmq::message_t message;
      HV.recv(&message);
    }
    else clean=false;
  }
  
  zmq::poll (&HVout[0], 1, 10000);
  if (HVout[0].revents & ZMQ_POLLOUT) {

    int message_id=3;
    
    std::string tmp="";
    tmp += "{" + EncodePair("msg_id",message_id);
    tmp += "," + EncodePair("msg_type","get");
    tmp += "," + EncodePair("msg_property","event");
    tmp += "," + EncodePair("XXX","XXX") + "}";
    
    zmq::message_t send(tmp.length()+1);
    snprintf ((char *) send.data(), tmp.length()+1 , "%s" ,tmp.c_str()) ;
    HV.send(send);
    
    
    zmq::poll (&HVin[0], 1, 10000);
    if (HVin[0].revents & ZMQ_POLLIN) {
      
      zmq::message_t message;
      HV.recv(&message);

      std::istringstream iss(static_cast<char*>(message.data()));
    
      zmq::poll (&HVin[0], 1, 10000);
      if (HVin[0].revents & ZMQ_POLLIN) {
        
                 HV.recv(&message);

                 std::istringstream issb(static_cast<char*>(message.data()));

                 zmq::poll (&HVout[0], 1, 10000);
                 if (HVout[0].revents & ZMQ_POLLOUT) {
          
          
                  message_id=4;
                  tmp="";
                  tmp += "{" + EncodePair("msg_id",message_id);
                  tmp += "," + EncodePair("msg_type","get");
                  tmp += "," + EncodePair("msg_property","error");
                  tmp += "," + EncodePair("XXX","XXX") + "}";


                  zmq::message_t send2(tmp.length()+1);
                  snprintf ((char *) send2.data(), tmp.length()+1 , "%s" ,tmp.c_str()) ;
                  HV.send(send2);
                  //std::cout<<"sent "<<tmp<<std::endl;
    
    
                  zmq::poll (&HVin[0], 1, 10000);
                  if (HVin[0].revents & ZMQ_POLLIN) {
      
                          zmq::message_t message2;
                          HV.recv(&message2);
     
                          std::istringstream iss2(static_cast<char*>(message2.data()));
      
      
                           zmq::poll (&HVin[0], 1, 10000);
                           if (HVin[0].revents & ZMQ_POLLIN) {
        
                                  HV.recv(&message2);
                                  std::istringstream iss2b(static_cast<char*>(message2.data()));
                           }
                           else std::cout<<"HV reply timeout"<<std::endl;
                   } 
                  else std::cout<<"HV akn reply timeout"<<std::endl;
             }
             else std::cout<<"HV send timeout"<<std::endl;
      }
      else std::cout<<"HV reply timeout"<<std::endl;
    }
    else std::cout<<"HV akn reply timeout"<<std::endl;
  }
  else std::cout<<"HV send timeout"<<std::endl;
  
  return true;
}
