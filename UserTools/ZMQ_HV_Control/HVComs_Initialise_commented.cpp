bool HVComs::Initialise(std::string configfile, DataModel &data){

	/// standard reading config variables
  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  m_variables.Get("HVIP",HVIP); //get the laptops ip form config file
  m_variables.Get("HVPort",HVPort); // get the port for communication form the config file

	
	////// creating the socet for comunication
  zmq::socket_t HV(*(m_data->context), ZMQ_DEALER);
  std::stringstream address;
  address<<"tcp://"<<HVIP<<":"<<HVPort;
  HV.connect(address.str().c_str());
/////////////////////////////////////////////
	
	
	//// setting up two poll objects so we can test if messages can be sent or received
	
  zmq::pollitem_t HVin [] = {
    { HV, 0, ZMQ_POLLIN, 0 }
  };

  zmq::pollitem_t HVout [] = {
    { HV, 0, ZMQ_POLLOUT, 0 }
  };
  /////////////////////////////
	
  // cleaning up output this is to make sure there are no old messgaes in the buffer before we start
  bool clean=true;
  while (clean){
    zmq::poll (&HVin[0], 1, 6000);
    if (HVin[0].revents & ZMQ_POLLIN) {
      zmq::message_t message;
      HV.recv(&message);
    }
    else clean=false;
  }
  ////////////////////////////////////
	
	
	///////////// check the poll to see if the laptop is ready to receive a message
  zmq::poll (&HVout[0], 1, 10000);
  if (HVout[0].revents & ZMQ_POLLOUT) {
    
	Store bb; // create a store to make the json command string
    
    bb.Set("msg_id",1);
    bb.Set("msg_type","run");
    bb.Set("run",m_data->RunNumber);
    bb.Set("subrun",m_data->SubRunNumber);
    
    std::string tmp="";
    bb>>tmp; //// use store to output a json command string
    zmq::message_t send(tmp.length()+1); /// create a message the same siaze of string
    snprintf ((char *) send.data(), tmp.length()+1 , "%s" ,tmp.c_str()) ; /// fill message
    HV.send(send); // send message
    bb>>m_data->InfoMessage; /// ignore this
    
    zmq::poll (&HVin[0], 1, 10000); 
    if (HVin[0].revents & ZMQ_POLLIN) {  
      //std::cout<<"sent "<<tmp<<std::endl;
      zmq::message_t message;
      HV.recv(&message);
      
      //char *test;
      // test = (static_cast<char*>(message.data()));
      std::istringstream iss(static_cast<char*>(message.data()));
      //test=iss.str();                                                                                                                                                                                                  
      //std::cout<<"message received: "<<iss.str()<<std::endl;//<<" size = "<<message.size()<<std::endl;
      //std::cout<<"message received2: "<<test<<std::endl;

      m_data->InfoMessage+=iss.str();

		/////// check poll to see if received a reply form laptop
      zmq::poll (&HVin[0], 1, 20000);
      if (HVin[0].revents & ZMQ_POLLIN) {
	
	HV.recv(&message); // receive message and put it into message object
	
	//char *test;
	// test = (static_cast<char*>(message.data()));
	std::istringstream iss2(static_cast<char*>(message.data())); // convert message data to string steram
	//test=iss.str();			\
	
	//std::cout<<"message received: "<<iss2.str()<<std::endl;
	
		  
		  /// filling a ttree with message data
	m_data->InfoTitle="HVComs";
	
	m_data->InfoMessage+=iss2.str();
	m_data->GetTTree("RunInformation")->Fill();
		  /////////////////////////////
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
    //    return false;
  }
  
  return true;
}