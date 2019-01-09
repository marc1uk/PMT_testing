double MAX_PULSE_INTEGRAL = 150e6;
TH1D* ahistp = (TH1D*)_file0->Get("phd_S3");
ahistp->Draw();
if(ahistp==nullptr){ std::cerr<<"Failed to find PulseHeightDistribution drawn from file!"<<std::endl; return 0; }
int maxpos1=-1, maxpos2=-1, interpos=-1;
int max1=-1, max2=-1;
int peaktovalleymin=5; // SPE peak must be at least > 5 counts higher than the minimum, to prevent noise being identified as 'SPE peak'
int intermin=99999999;
for(int bini=1; bini<ahistp->GetNbinsX(); bini++){
  int bincont = ahistp->GetBinContent(bini);
  cout<<"bin "<<bini<<" center "<<ahistp->GetBinCenter(bini)<<" has content "<<bincont<<endl;
  cout<<"maxpos1="<<maxpos1<<", at "<<ahistp->GetBinCenter(maxpos1)<<", maxpos2="<<maxpos2<<" at "<<ahistp->GetBinCenter(maxpos2)<<", interpos="<<interpos<<" at "<<ahistp->GetBinCenter(interpos)<<", max1="<<max1<<", max2="<<max2<<", intermin="<<intermin<<endl;
  if(bincont>max1){ max1=bincont; maxpos1=bini; intermin=99999999;}
  else if((bincont<intermin)&&(max1>0)&&(max2==-1)){ intermin=bincont; interpos=bini; }
  else if((interpos>0)&&(bincont>(intermin+peaktovalleymin))&&(bincont>max2)){ max2=bincont; maxpos2=bini; }
}
cout<<"maxpos1="<<maxpos1<<", at "<<ahistp->GetBinCenter(maxpos1)<<", maxpos2="<<maxpos2<<" at "<<ahistp->GetBinCenter(maxpos2)<<", interpos="<<interpos<<" at "<<ahistp->GetBinCenter(interpos)<<endl;
cout<<"ahistp->GetMean()="<<ahistp->GetMean()<<", *1.5 = "<<ahistp->GetMean()*1.5<<endl;
// to do a double-gaus fit we need to define the fit function first
double fitrangelo = ahistp->GetXaxis()->GetBinLowEdge(0);  // XXX FIT RANGE MAY NEED TUNING
double fitrangeup = ahistp->GetXaxis()->GetBinUpEdge(ahistp->GetNbinsX());
TF1 fit_func("my_fit_func","gaus(0)+gaus(3)+gaus(6)",fitrangelo,fitrangeup);
//TF1 fit_func("fit_func","gaus(0)+gaus(3)",fitrangelo,fitrangeup);
//TF1 fit_func("fit_func","gaus(0)+gaus(3)+gaus(6)+(x>[7])*exp(8)",fitrangelo,fitrangeup);
// unfortunately we also need to give it initial values for it to work
double gaus1amp = ahistp->GetEntries()/10;
double gaus1centre = 0; //ahistp->GetMean(); pedestal ~0.
double gaus1width = ahistp->GetEntries()*50;
double gaus2amp = gaus1amp/5;
double gaus2centre = (/*(ahistp->GetBinCenter(maxpos2)<ahistp->GetMean())*/(max2>10)&&(maxpos2>0)) ? ahistp->GetBinCenter(maxpos2) : ahistp->GetMean()*1.5;
double gaus2width = gaus1width*4;
double gaus3amp = gaus2amp/2.;
double gaus3centre = gaus2centre*2.;
double gaus3width = gaus2width;
// attempt to fit the in-between region between pedestal and spe - does not work
double expstart = 12e3;
double expoffset = 6.;
double expdecayconst = -1e-6;
std::vector<double> theparams{gaus1amp, gaus1centre, gaus1width, gaus2amp, gaus2centre, gaus2width, gaus3amp, gaus3centre, gaus3width, expstart, expoffset, expdecayconst};

fit_func.SetParameters(gaus1amp, gaus1centre, gaus1width, gaus2amp, gaus2centre, gaus2width, gaus3amp, gaus3centre, gaus3width);
fit_func.SetNpx(1000);
if((maxpos2>0)&&(max2>10)){
fit_func.SetParLimits(1,-1e3,ahistp->GetBinCenter(interpos));
fit_func.SetParLimits(4,ahistp->GetBinCenter(interpos),ahistp->GetBinCenter(maxpos2)*1.2);
fit_func.SetParLimits(7,ahistp->GetBinCenter(maxpos2)*1.2,MAX_PULSE_INTEGRAL);
}
fit_func.Draw("same");
