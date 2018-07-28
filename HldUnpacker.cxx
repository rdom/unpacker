#include "HldUnpacker.h"
#include "../prttools/prttools.C"

Int_t gImgid;
TCanvas *gCanvas = new TCanvas("gCanvas","gCanvas",0,0,800,400);
TH1F* hTimeDiff = new TH1F("hTimeDiff","hTimeDiff;time [ns];entries [#]",1000,-300,300);
TH1F* hRefCh = new TH1F("RefCh","RefCh;tdc [#];entries [#]",prt_ntdc,0,prt_ntdc);
TH1F* hCh = new TH1F("hCh","cannels;channel [#];entries [#]",prt_maxch,0,prt_maxch);
TH1F* hTdcId = new TH1F("TdcId","TdcId;tdc [#];entries [#]",10000,0,10000);

HldUnpacker::HldUnpacker(string inHld, string outRoot ,string tdcFName, UInt_t subEventId, UInt_t ctsAddress,
			 UInt_t mode, UInt_t freq, UInt_t verbose, UInt_t uniqid) : fRootName(outRoot), fMode(mode), fFreq(freq), fVerbose(verbose), fUniqId(uniqid),fTotalHits(0),fMcpHits(0){
  fTriggerChannel = 816;
  fTrailingTime.resize(prt_maxch);
  fRefTime.resize(prt_ntdc);
  prt_createMap();
  fFlippedEntry=0;
  fDataType=0;
  
  prt_setRootPalette(1);
  gImgid=0;
  
  if(fVerbose>0) std::cout<<"File  "<< inHld <<std::endl;
  
  if(fMode<3){
    fHldFile.open(inHld.c_str(), ifstream::in | ifstream::binary);
    if(fHldFile.fail()) exit(-1);
  }
  
  if(fMode<3)  IndexEvents();
  if(fMode!=0) prt_initDigi(1);
}

void HldUnpacker::Reset(){
  fTotalHits = 0;
  fMcpHits = 0;
  
  hTimeDiff->Reset();
  hRefCh->Reset();
  hCh->Reset();
  hTdcId->Reset();
  if(fMode==3 || fFreq!=0) prt_resetDigi();
  fFlippedEntry=0;
}

void HldUnpacker::Decode(Int_t startEvent, Int_t endEvent) {

  TFile *file;
  TTree *tree;
  PrtEvent event;

  fHldFile.clear();
  fHldFile.seekg(0,ios::beg);
  Reset();

  if(fMode==0){
    file = new TFile(fRootName.c_str(),"RECREATE");
    tree = new TTree("data","dirc@gsi hld unpacker",2);  
    tree->Branch("PrtEvent","PrtEvent",&event,128000,2);
    fHldFile.seekg(fEvtIndex.at(startEvent),ios::beg);
    if(endEvent==0) endEvent = fEvtIndex.size();
    std::cout<<"# of events  "<< endEvent<<std::endl;
  }else{
    if(endEvent==0) endEvent = 1000000;
  }
  for(Int_t e = startEvent; e<endEvent; e++){
    if(e%1==0) std::cout<<"event # "<< e <<std::endl;
    
    if(fMode==0) event = PrtEvent();
    if(ReadEvent(&event, kTRUE)){
      if(fMode==0 && event.GetHitSize()>0) tree->Fill();      
    }else break;

    if(fFreq !=0 && e%fFreq==0 &&  fMode!=0) Report(1);
  }

  //return;
  
  if(fMode==0) {
    tree->Write();
    file->Write();
    file->Close();
  }
  else Report(1);
  
  
}

void savePic(TCanvas *c, TString dir, TString path, TString link){
  c->Modified();
  c->Update();
  c->Print(dir+path);
  
  gSystem->Exec("cd "+ dir +"&& rm -f last_"+link+" &&  ln -s "+path+" last_"+link);
}

void HldUnpacker::Report(Int_t flag){
  std::cout<<"fFlippedEntry "<<fFlippedEntry<<std::endl;
  
  if(flag==1){
    TString rand = prt_randstr(10);
    gImgid+=fUniqId;

    hTimeDiff->Draw();
    gCanvas->Modified();
    gCanvas->Update();
    gCanvas->Print(Form("hle_%d.png",gImgid));

    hCh->Draw();
    gCanvas->Modified();
    gCanvas->Update();
    gCanvas->Print(Form("hch_%d.png",gImgid));

    hRefCh->Draw();
    gCanvas->Modified();
    gCanvas->Update();
    gCanvas->Print(Form("hrc_%d.png",gImgid));    
    
    hTdcId->Draw();
    gCanvas->Modified();
    gCanvas->Update();
    gCanvas->Print(Form("hti_%d.png",gImgid));

    
    prt_drawDigi("m,p,v\n",2018,-2,0);
    prt_cdigi->Print(Form("hhp_%d.png",gImgid));

    gImgid++;

    // for(Int_t i=0; i<10000; i++){
    //   if(hTdcId->GetBinContent(i+1)!=0){
    //     std::cout<<"tdc "<<i << "  0x" << hex << i << dec <<" has "<< hTdcId->GetBinContent(i+1) << " entries"<<std::endl;	
    //   }
    // }
    Reset();
  }

  if(flag==2){
    TString rand = prt_randstr(5);
    std::stringstream strm;
    strm << time(NULL);
    TString id, unixtime = strm.str();
    if(fMode==3) id = unixtime;
    else id = Form("%d",gImgid++);

    TString dir = "../prtonline/data/";
    
    ofstream  file;
    if (!std::ifstream(dir+"timeline.csv")){
      file.open(dir+"timeline.csv");
      file<< "time,total,mcp,flipped,type\n";
      file.close();
    }

    if(fTotalHits>0){
      file.open(dir+"timeline.csv", std::ios::app);
      file<< unixtime+Form(",%d,%d,%d,%d\n",fTotalHits,fMcpHits,fFlippedEntry,fDataType);
      file.close();
    }

    file.open(dir+"last_timeline");
    file<< "time,total,mcp,flipped,type\n" + unixtime+Form(",%d,%d,%d,%d\n",fTotalHits,fMcpHits,fFlippedEntry,fDataType);
    file.close();
  

    
    gCanvas =  new TCanvas("gCanvas","gCanvas",0,0,800,400);
    hTimeDiff->Draw();
    savePic(gCanvas,dir,"pics/hle_"+id+".png","hle");
    
    gCanvas =  new TCanvas("gCanvas","gCanvas",0,0,800,400);
    hCh->Draw();
    savePic(gCanvas,dir,"pics/hch_"+id+".png","hch");

    gCanvas =  new TCanvas("gCanvas","gCanvas",0,0,800,400);
    hRefCh->Draw();
    savePic(gCanvas,dir,"pics/hrc_"+id+".png","hrc");

    prt_drawDigi("m,p,v\n",2018,-2,0);  
    savePic(prt_cdigi,dir,"pics/hhp_"+id+".png", "hhp");

    Reset();
  }
  
}

void HldUnpacker::DecodePos(Int_t startPos, Int_t endPos) {

  fHldFile.clear();
  fHldFile.seekg(startPos,ios::beg);
  if(fFreq==0) Reset();

  PrtEvent event;
  Int_t e = 0;
  while(fHldFile.tellg() < endPos){
    if(++e%100000==0) std::cout<<"event # "<< e <<std::endl;
    if(!ReadEvent(&event, kTRUE))  break;    
    if(fEvents++>=fFreq && fFreq!=0){
      Report(2);
      fEvents=0;
    }
  }
  if(fFreq==0) Report(2);
}

vector<string>  gOldFiles0;
void HldUnpacker::DecodeOnline(string inHld){
  
  if(inHld.find("trb") != std::string::npos) fDataType=0;
  if(inHld.find("pilas") != std::string::npos) fDataType=1;
  if(inHld.find("beam") != std::string::npos) fDataType=2;
  
  gOldFiles0.push_back(inHld);
  
  Int_t startPos(0), endPos(0);
  fHldFile.close();
  fHldFile.clear();
  fHldFile.open(inHld.c_str(), ifstream::in | ifstream::binary);
  
  if(fHldFile.is_open()){
    while (true){
      fHldFile.clear();
      fHldFile.seekg(0, std::ios::end);
      endPos = fHldFile.tellg();
      
      std::cout<<"File "<<inHld<<"  startPos  "<< startPos << "   endPos  "<<endPos<<std::endl;
      if(startPos<endPos){
	DecodePos(startPos,endPos);
      }
      if(startPos==endPos){
	system(fDataRegex.c_str());
	ifstream t("newfile.tmp");
	string inFile = string((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
	if (std::find(gOldFiles0.begin(), gOldFiles0.end(), inFile) == gOldFiles0.end()){
	  cout<<"There is a new file  "<<endl;
	  break;
	}
      }
     
      fHldFile.clear();
      startPos = fHldFile.tellg();
      sleep(2);
    }
  }
}

Int_t HldUnpacker::IndexEvents(){
  fHldFile.seekg(0,ios::beg);
  fEvtIndex.clear();
  fEvtIndex.reserve(10000000);
  while(fHldFile.good()){
    Int_t tEvtIndex = fHldFile.tellg();
    if(ReadEvent(NULL, kFALSE))  fEvtIndex.push_back(tEvtIndex);
  }
  return fEvtIndex.size();
}

Bool_t HldUnpacker::ReadEvent(PrtEvent *event, Bool_t all){
  UInt_t ehHSize = sizeof(HLD_HEADER);
  Int_t index = fHldFile.tellg();
  // read header of the event
  fHldFile.read((char*)&fEventHeader,ehHSize);
  if(fHldFile.gcount() != ehHSize) return kFALSE;

  //check header
  while(!GoodHeader(fEventHeader)){
    index += 4;
    fHldFile.seekg(index,ios::beg);
    fHldFile.read((char*)&fEventHeader,ehHSize);
  }
 
  fDataBytes = fEventHeader.nSize - ehHSize;
  UInt_t baseEventSize = 1 << ((fEventHeader.nDecoding >> 16) & 0xFF);
  
  if(fEventHeader.nSize == ehHSize){
    UInt_t skipbytes = baseEventSize * (size_t)((fEventHeader.nSize-1)/baseEventSize + 1) - fEventHeader.nSize;
    if(skipbytes>0) fHldFile.ignore(skipbytes);
    return kTRUE;
  }
  
  if(!all) fHldFile.ignore(fDataBytes);  
  else{
    if(fVerbose){
      cout << "  Size: \t\t" << fEventHeader.nSize
    	   << "  Decoding: \t" << fEventHeader.nDecoding << dec
    	   << "  ID: \t\t" << hex << fEventHeader.nId << dec
    	   << "  Seq Nr: \t" << fEventHeader.nSeqNr 
    	   << "  Date: \t\t" << hex << fEventHeader.nDate << dec 
    	   << "  Time: \t\t" << hex << fEventHeader.nTime << dec 
    	   << "  Run: \t\t" << hex << fEventHeader.nRun << dec
    	   << "  Pad: \t\t" << hex << fEventHeader.nPad << dec <<endl;
    }
    
    fHitArray.clear();
    //    std::fill(fTrailingTime.begin(), fTrailingTime.end(), 0);
    std::fill(fRefTime.begin(), fRefTime.end(), 0);
    ReadSubEvent(fDataBytes);
    
    fTotalHits += fHitArray.size();
    for(Int_t i=0; i<fHitArray.size(); i++){
      PrtHit hit;
      hit = fHitArray[i];
      Int_t ch = hit.GetChannel();
      Double_t le = (hit.GetLeadTime() - fRefTime[map_tdc[hit.GetTrb()]]) - (fTriggerTime - fTriggerRefTime);
      //      hit.SetLeadTime(hit.GetLeadTime() -fTriggerTime);
      hit.SetLeadTime(le);
      hit.SetTotTime(hit.GetLeadTime()-fTrailingTime[ch]);
      if(fMode==0) event->AddHit(hit);

      if(fMode!=0){
	if(ch<prt_maxdircch)  hTimeDiff->Fill(le);

	//if(le >-150 && le<-50){
	  if(hit.GetMcpId()<15){
	    prt_hdigi[hit.GetMcpId()]->Fill(map_col[ch],map_row[ch]);
	    fMcpHits++;
	  }
	  //}
      }
    }    
    if(fMode==0) event->SetTime(10);
  }
  
  //skip empty bytes at the end of event
  UInt_t skipbytes = baseEventSize * (size_t)((fEventHeader.nSize-1)/baseEventSize + 1) - fEventHeader.nSize;
  if(skipbytes>0) fHldFile.ignore(skipbytes);
  return kTRUE;
}

Bool_t HldUnpacker::GoodHeader(HLD_HEADER header){
  if(header.nDecoding == 196609 ) return true;
  else return false;
}

Bool_t HldUnpacker::ReadSubEvent(UInt_t data){
  UInt_t trbWords(0), trbAddress(0), tdcChannel, subEvtId(0), tdcErrCode,
    edge(0), epochCounter(0), coarseTime(0), fineTime(0), startdata = data;
  Int_t tdcLastChannelNo(0);
  UInt_t shHSize = sizeof(SUB_HEADER);
  UInt_t stHSize = sizeof(SUB_TRAILER);
    
  //read subevent header
  fHldFile.read((char*)&fSubEventHeader,shHSize);
  data -= fHldFile.gcount();

  fSubEventHeader.nSize	= __builtin_bswap32(fSubEventHeader.nSize);
  fSubEventHeader.nDecoding	= __builtin_bswap32(fSubEventHeader.nDecoding);
  fSubEventHeader.nEventId	= __builtin_bswap32(fSubEventHeader.nEventId);
  fSubEventHeader.nTrigger	= __builtin_bswap32(fSubEventHeader.nTrigger);

  if(fVerbose){
    cout << "\t  Size:  \t" << fSubEventHeader.nSize
	 << "\t  Decoding: \t" << hex << fSubEventHeader.nDecoding << dec
	 << "\t  ID: \t" << hex << fSubEventHeader.nEventId << dec
	 << "\t Trigger: \t" << fSubEventHeader.nTrigger<< endl;
    int ii; cin>>ii;
  }
      
  // read subevent data
  std::vector<UInt_t> fTrbData;
  size_t trbDataBytes = fSubEventHeader.nSize-shHSize-stHSize;
  fTrbData.resize(trbDataBytes/4);
    
  fHldFile.read((char*)&fTrbData[0],trbDataBytes);
  data -= fHldFile.gcount();

  // for(UInt_t i=0;i<fTrbData.size();i=i+4){
  //   std::cout<<std::bitset<32>(fTrbData[i]) <<"  "<<std::bitset<32>(fTrbData[i+1]) <<"  "<<std::bitset<32>(fTrbData[i+2]) <<"  "<<std::bitset<32>(fTrbData[i+3]) <<std::endl;
  // }
  // std::cout<<"============================ " <<  fTrbData.size() <<std::endl;
    
  if(false)
    for(UInt_t i=0;i<fTrbData.size();i=i+1){
      UInt_t word = __builtin_bswap32(fTrbData[i]);
      trbAddress = word & 0xFFFF;
      trbWords   = word>>16;

      // if(trbAddress< 10000 && map_tdc[trbAddress]!=0) {
      if(trbAddress< 10000){
	std::cout<<"map_tdc[trbAddress] "<<map_tdc[trbAddress] <<std::endl;
	std::cout<<RED<<hex<<fTrbData[i]  <<CYAN <<"  a "<<trbAddress << dec<<YELLOW<<"  w "<< trbWords<<std::endl;
      }
    }

  UInt_t tdcWords(0), tdcAddress(0);
  for(UInt_t i=0;i<fTrbData.size();i++){
    UInt_t word = __builtin_bswap32(fTrbData[i]);
    //std::cout<<"word "<< hex<< word  << dec<<std::endl;
      
    trbAddress = word & 0xFFFF;
    trbWords   = word>>16;
    //std::cout<<"trbAddress "<< dec << trbAddress <<"  "<<hex<<trbAddress << dec<<std::endl;

    if(trbAddress< 10000 && map_tdc[trbAddress]>=0) tdcWords  = trbWords;

    if(tdcWords>0){

      Int_t mult[prt_maxch]={0};
      memset(mult, 0, sizeof(mult));
      for(UInt_t t=0; t<tdcWords; t++){
	word = __builtin_bswap32(fTrbData[i+1+t]);
	//std::cout<<" "<< hex<< word  << dec<<std::endl;
	if(t==0){ // read TDC header
	  TDC_HEADER tdcHeader;

	  if(((word>>29) & 0x7) != TDC_HEADER_MARKER){
	    std::cout<<"not tdc header " <<std::endl; 
	    continue;
	  }

	  tdcHeader.nRandomBits	= (word>>16) & 0xFF;
	  tdcHeader.nErrorBits	= word & 0xFFFF;
	  tdcLastChannelNo = -1;
	  continue;
	}

	// read TDC data	
	UInt_t idBit = (word>>29) & 0x7; // EPOCH or DEBUG
	if(idBit == TDC_EPOCH_MARKER) {
	  epochCounter = word & 0xFFFFFFF;
	  tdcLastChannelNo = -2; // epoch
	  continue;
	}	
	if(idBit == TDC_DEBUG_MARKER) continue;
	if((word>>31) != 1) continue; // not time data
	  
	tdcChannel = (word>>22) & 0x7F; // TDC channel number is represented by 7 bits

	if(tdcLastChannelNo>=0 && (Int_t)tdcChannel != tdcLastChannelNo) {
	  if(fVerbose) cout << "Epoch Counter reset since channel has changed" << endl;
	  epochCounter = 0; 
	}

	fineTime      = (word>>12) & 0x3FF; // TDC fine time is represented by 10 bits
	edge	        = (word>>11) & 0x1;   // TDC edge indicator: 1->rising edge, 0->falling edge 
	coarseTime    = word & 0x7FF;       // TDC coarse time is represented by 11 bits
	Double_t time = 5*(epochCounter*pow(2.0,11) + coarseTime)-(fineTime-31)*0.0102;
	tdcLastChannelNo = (Int_t)tdcChannel;

	{
	  Int_t ch, timeLe,timeTe, timeTot;

	  if(fMode==1) hTdcId->Fill(trbAddress);
	  if(tdcChannel>48) continue;
	  if(tdcChannel==0){
	    Int_t tdc = map_tdc[trbAddress];
	    if(tdc<0) continue;
	    fRefTime[tdc]=time; 
	    if(fMode!=0) hRefCh->Fill(tdc);
	    if(ch==fTriggerChannel) fTriggerRefTime = time;
	    continue;
	  }

	  ch = prt_getChannelNumber(map_tdc[trbAddress],tdcChannel-1);
	    //48*map_tdc[trbAddress]+tdcChannel-1;
	  hCh->Fill(ch);
	  
	  //std::cout<<"trbAddress "<< dec << trbAddress <<"  "<<hex<<trbAddress << dec<<std::endl;
	  mult[ch]++;
	  
	    if(edge==0){
	      if(mult[ch]==1) fFlippedEntry++;
	      fTrailingTime[ch]=time;
	      continue;
	    }
	  if(ch==fTriggerChannel) {
	    fTriggerTime = time;
	  }
	  PrtHit hit;
	  hit.SetTdc(tdcChannel);
	  hit.SetTrb(trbAddress);
	  hit.SetLeadTime(time);
	  hit.SetChannel(ch);

	  if(ch<prt_maxdircch){
	    hit.SetMcpId(map_mcp[ch]);
	    hit.SetPixelId(map_pix[ch]);
	  }else {
	    hit.SetMcpId(20);
	    hit.SetPixelId(0);
	  }
	    
	  fHitArray.push_back(hit);
	}
      }
    } 

    i += trbWords;
  }

  // read subevent trailer information
  fHldFile.read((char*)&fSubEventTrailer,stHSize);
  data -= fHldFile.gcount();
  if(fHldFile.gcount() != stHSize) {
    cerr << "Error reading subevent trailer from HLD file!" << endl;
    return kFALSE;
  }
  fSubEventTrailer.nSebHeader	= __builtin_bswap32(fSubEventTrailer.nSebHeader);
  fSubEventTrailer.nSebError	= __builtin_bswap32(fSubEventTrailer.nSebError);
 
  if(fSubEventTrailer.nSebError != SEB_ERROR_CODE){
    if(fVerbose) cout << "Error in Subevent Builder detected!" << endl;
  }

  if((startdata-data)%8){
    fHldFile.ignore(4);
    if(data>=4) data -= 4;
    if(data==4) data -= 4; 
  }

  //std::cout<<"DATA  "<<data <<" "<<fDataBytes<<std::endl;

  if(!data) return kTRUE;
  ReadSubEvent(data);

  return kTRUE;
}
