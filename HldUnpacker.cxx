#include "HldUnpacker.h"
#include <TH1F.h>
#include <TCanvas.h>

#include "../prttools/prttools.C"

Int_t gImgid;
TCanvas *gCanvas = new TCanvas("gCanvas","gCanvas",0,0,800,400);
TH1F* hTimeDiff = new TH1F("hTimeDiff","hTimeDiff;time [ns];entries [#]",100,-200,200);
TH1F* hRefCh = new TH1F("RefCh","RefCh;tdc [#];entries [#]",tdcnum,0,tdcnum);

HldUnpacker::HldUnpacker(string inHld, string outRoot ,string tdcFName, UInt_t subEventId, UInt_t ctsAddress,
			 UInt_t mode,UInt_t verbose, UInt_t uniqid) : fRootName(outRoot), fMode(mode),fVerbose(verbose), fUniqId(uniqid),fTotalHits(0),fMcpHits(0){
  fTriggerChannel = 1776;
  fTrailingTime.resize(maxch);
  fRefTime.resize(tdcnum);
  CreateMap();  
  SetRootPalette(1);
  gImgid=0;

  if(fVerbose>0) std::cout<<"File  "<< inHld <<std::endl;
  
  
  if(fMode<3){
    fHldFile.open(inHld.c_str(), ifstream::in | ifstream::binary);
    if(fHldFile.fail()) exit(-1);
  }
  
  if(fMode<3)  IndexEvents();
  if(fMode!=0) initDigi(0);
}

void HldUnpacker::Reset(){
  fTotalHits = 0;
  fMcpHits = 0;
  
  hTimeDiff->Reset();
  hRefCh->Reset();
  if(fMode==3) resetDigi();
}

void HldUnpacker::Decode(Int_t startEvent, Int_t endEvent) {

  TFile *file;
  TTree *tree;
  PrtEvent event;

  fHldFile.clear();
  fHldFile.seekg(0,ios::beg);
  Reset();
  
  if(fMode<3){
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
    if(e%10000==0) std::cout<<"event # "<< e <<std::endl;

    if(fMode==0) event = PrtEvent();
    if(ReadEvent(&event, kTRUE)){
      if(event.GetHitSize()>0) tree->Fill();
    }else break;
  }
  
  if(fMode==0) tree->Write();
  else{
    TString rand = randstr(10);
    gImgid+=fUniqId;
    hTimeDiff->Draw();
    gCanvas->Modified();
    gCanvas->Update();
    gCanvas->Print(Form("time_%d.png",gImgid));
    hRefCh->Draw();
    gCanvas->Modified();
    gCanvas->Update();
    gCanvas->Print(Form("refch_%d.png",gImgid));
  
    drawDigi("m,p,v\n",2,-2,-2);
    cDigi->Print(Form("digi_%d.png",gImgid));
    gImgid++;
  }
}

void savePic(TCanvas *c, TString dir, TString path, TString link){
  c->Modified();
  c->Update();
  c->Print(dir+path);
  
  gSystem->Exec("cd "+ dir +"&& rm -f last_"+link+" &&  ln -s "+path+" last_"+link);
  //gSystem->Unlink(link);
  //gSystem->Symlink(path, link);
}


void HldUnpacker::DecodePos(Int_t startPos, Int_t endPos) {

  fHldFile.clear();
  fHldFile.seekg(startPos,ios::beg);
  Reset();
  
  PrtEvent event;
  Int_t e = 0;
  while(fHldFile.tellg() < endPos){
    if(++e%100000==0) std::cout<<"event # "<< e <<std::endl;
    if(!ReadEvent(&event, kTRUE))  break;
  }
  
  TString rand = randstr(5);
  std::stringstream strm;
  strm << time(NULL);
  TString id, unixtime = strm.str();
  if(fMode==3) id = unixtime;
  else id = Form("%d",gImgid++);

  TString dir = "../prtonline/data/";
  ofstream  file;
  file.open(dir+"timeline.csv", std::ios::app);
  file<< unixtime+Form(",%d,%d \n",fTotalHits,fMcpHits);
  file.close();

  file.open(dir+"last_timeline");
  file<< "time,total,mcp \n" + unixtime+Form(",%d,%d \n",fTotalHits,fMcpHits);
  file.close();
  
  
  hTimeDiff->Draw();
  savePic(gCanvas,dir,"pics/time_"+id+".png", "time");
  hRefCh->Draw();
  savePic(gCanvas,dir,"pics/refch_"+id+".png","refch");
  
  file.open(dir+"pics/digi_"+id+".csv");
  file<< drawDigi("m,p,v\n",2,-2,-2);
  file.close();
  savePic(cDigi,dir,"pics/digi_"+id+".png", "digi");
  
}

vector<string>  gOldFiles0;
void HldUnpacker::DecodeOnline(string inHld){
  
  Int_t startPos(0), endPos(0);
  fHldFile.close();
  fHldFile.clear();
  fHldFile.open(inHld.c_str(), ifstream::in | ifstream::binary);
  fHldFile.clear();
  
  gOldFiles0.push_back(inHld);

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
      hit.SetLeadTime(hit.GetLeadTime()-fTriggerTime);
      hit.SetTotTime(hit.GetLeadTime()-fTrailingTime[ch]);
      if(fMode==0) event->AddHit(hit);

      if(fMode!=0){
	hTimeDiff->Fill(hit.GetTotTime());
	if(hit.GetMcpId()<15){
	  fhDigi[hit.GetMcpId()]->Fill(map_col[ch],map_row[ch]);
	  fMcpHits++;
	}
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

      //if(trbAddress< 10000 && map_tdc[trbAddress]!=0) {
      if(trbAddress==8193){
	std::cout<<"map_tdc[trbAddress] "<<map_tdc[trbAddress] <<std::endl;
	std::cout<<RED<<hex<<fTrbData[i]  <<CYAN <<"  a "<<trbAddress << dec<<YELLOW<<"  w "<< trbWords<<std::endl;
      }
      // }
    }

  UInt_t tdcWords(0), tdcAddress(0);
  for(UInt_t i=0;i<fTrbData.size();i++){
    UInt_t word = __builtin_bswap32(fTrbData[i]);
    //std::cout<<"word "<< hex<< word  << dec<<std::endl;
      
      
    trbAddress = word & 0xFFFF;
    trbWords   = word>>16;

    if(trbAddress< 10000 && map_tdc[trbAddress]>=0) tdcWords  = trbWords;

    if(tdcWords>0){
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
	  if(fVerbose)
	    cout << "Epoch Counter reset since channel has changed" << endl;
	  epochCounter = 0;
	}

	fineTime      = (word>>12) & 0x3FF; // TDC fine time is represented by 10 bits
	edge	        = (word>>11) & 0x1;   // TDC edge indicator: 1->rising edge, 0->falling edge
	coarseTime    = word & 0x7FF;       // TDC coarse time is represented by 11 bits
	Double_t time = 5*(epochCounter*pow(2.0,11) + coarseTime)-(fineTime-31)*0.0102;

	tdcLastChannelNo = (Int_t)tdcChannel;

	{
	  Int_t ch, timeLe,timeTe, timeTot;
	  if(tdcChannel>48) continue;
	  if(tdcChannel==0){
	    fRefTime[map_tdc[trbAddress]]=time;
	    if(fMode!=0) hRefCh->Fill(map_tdc[trbAddress]);
	    continue;
	  }

	  ch = 48*map_tdc[trbAddress]+tdcChannel-1;
	
	  if(edge==0){
	    fTrailingTime[ch]=time;
	    continue;
	  }
	  if(ch==fTriggerChannel) fTriggerTime = time;	   
	    
	  PrtHit hit;
	  hit.SetTdc(tdcChannel);
	  hit.SetTrb(trbAddress);
	  hit.SetLeadTime(time);
	  hit.SetChannel(ch);
	  if(ch<maxmch){
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

  // std::cout<<"DATA  "<<data <<" "<<fDataBytes<<std::endl;
  if(!data) return kTRUE;
  if((startdata-data)%8){
    fHldFile.ignore(4);
    data -= 4;
  }
  ReadSubEvent(data);

  return kTRUE;
}
