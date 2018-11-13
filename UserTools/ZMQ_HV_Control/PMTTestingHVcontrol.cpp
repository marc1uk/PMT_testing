#include "PMTTestingHVcontrol.h"

//adapted from HVComs.cpp ToolAnalysis file

PMTTestingHVcontrol::PMTTestingHVcontrol(){

}

bool PMTTestingHVcontrol::SetRun(int run_number, int subrun_number){

  HVIP="192.168.163.61";    //to be changed??
  HVPort=5555;              //to be changed??

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

    std::string message_id="msg_id";
    std::string message_type="msg_type";
    std::string run_num="run";
    std::string subrun_num="subrun";
    std::string curly_left="{";
    std::string curly_right="}";
    std::string colon=":";
    std::string comma=",";
    std::string quotation="\"";

    

    std::string message_id_value="1";
    std::string message_type_value="run";
    std::stringstream ss;
    ss<<run_number;
    std::string run_num_value=ss.str();
    std::stringstream ss2;
    ss2<<subrun_number;
    std::string subrun_num_value=ss2.str();

    std::string tmp="";

    tmp.append(curly_left);
    tmp.append(quotation);
    tmp.append(message_id);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(message_id_value);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(message_type);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(quotation);
    tmp.append(message_type_value);
    tmp.append(quotation);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(run_num);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(run_num_value);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(subrun_num);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(subrun_num_value);
    tmp.append(curly_right);

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

  HVIP="192.168.163.61";    //to be changed??
  HVPort=5555;                      //to be changed??

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
    
    std::string message_id="msg_id";
    std::string message_type="msg_type";
    std::string message_property="msg_property";
    std::string last_key="active";
    std::string curly_left="{";
    std::string curly_right="}";
    std::string colon=":";
    std::string comma=",";
    std::string quotation="\"";

    

    std::string message_id_value="2";
    std::string message_type_value="write";
    std::string message_property_value="vset";
    std::stringstream ss;
    ss<<voltage;
    std::string voltage_value=ss.str();

    std::string tmp="";

    tmp.append(curly_left);
    tmp.append(quotation);
    tmp.append(message_id);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(message_id_value);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(message_type);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(quotation);
    tmp.append(message_type_value);
    tmp.append(quotation);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(message_property);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(quotation);
    tmp.append(message_property_value);
    tmp.append(quotation);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(last_key);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(voltage_value);
    tmp.append(curly_right);
    
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
  

  HVIP="192.168.163.61";    //to be changed??
  HVPort=5555;                      //to be changed??

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

    std::string message_id="msg_id";
    std::string message_type="msg_type";
    std::string message_property="msg_property";
    std::string last_key="XXX";
    std::string curly_left="{";
    std::string curly_right="}";
    std::string colon=":";
    std::string comma=",";
    std::string quotation="\"";

    std::string message_id_value="3";
    std::string message_type_value="get";
    std::string message_property_value="event";
    std::string last_value="XXX";

    std::string tmp="";

    tmp.append(curly_left);
    tmp.append(quotation);
    tmp.append(message_id);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(message_id_value);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(message_type);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(quotation);
    tmp.append(message_type_value);
    tmp.append(quotation);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(message_property);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(quotation);
    tmp.append(message_property_value);
    tmp.append(quotation);
    tmp.append(comma);
    tmp.append(quotation);
    tmp.append(last_key);
    tmp.append(quotation);
    tmp.append(colon);
    tmp.append(quotation);
    tmp.append(last_value);
    tmp.append(quotation);
    tmp.append(curly_right);
    
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
	  
	  
                  message_id_value="4";
                  message_property_value="error";
	                tmp="";
                  tmp.append(curly_left);
                  tmp.append(quotation);
                  tmp.append(message_id);
                  tmp.append(quotation);
                  tmp.append(colon);
                  tmp.append(message_id_value);
                  tmp.append(curly_right);
                  tmp.append(comma);
                  tmp.append(curly_left);
                  tmp.append(quotation);
                  tmp.append(message_property);
                  tmp.append(quotation);
                  tmp.append(colon);
                  tmp.append(quotation);
                  tmp.append(message_property_value);
                  tmp.append(quotation);
                  tmp.append(curly_right);


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
