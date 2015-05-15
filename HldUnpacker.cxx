#include "HldUnpacker.h"

#include <TH1F.h>
#include <TCanvas.h>
#include "../prttools/prttools.C"

#include <bitset>



const Int_t tdcmax(10000);
const Int_t nmcp(15), npix(64);
const Int_t maxmch(nmcp*npix);
Int_t tdcmap[tdcmax]={0};
Int_t map_mpc[nmcp][npix];
Int_t map_mcp[maxmch];
Int_t map_pix[maxmch];
Int_t map_row[maxmch];
Int_t map_col[maxmch];

const Int_t tdcnum(41);
const Int_t maxch(tdcnum*48);
TString tdcsid[tdcnum] ={"2000","2001","2002","2003","2004","2005","2006","2007","2008","2009",
			 "200a","200b","200c","200d","200e","200f","2010","2011","2012","2013",
			 "2014","2015","2016","2018","2019","201a","201c","2020","2023","2024",
			 "2025","2026","2027","2028","2029","202a","202b","202c","202d","202e","202f"
};


TH1F* hTimeDiff = new TH1F("hTimeDiff","hTimeDiff;time [ns];entries [#]",100,-200,200);

void CreateMap(){
  
  Int_t seqid =-1;
  for(Int_t i=0; i<tdcnum; i++){
    Int_t dec = TString::BaseConvert(tdcsid[i],16,10).Atoi();
    tdcmap[dec]=++seqid;
  }
  
  for(Int_t ch=0; ch<maxmch; ch++){
    Int_t mcp = ch/64;
    Int_t pix = ch%64;	
    Int_t col = pix/2 - 8*(pix/16);
    Int_t row = pix%2 + 2*(pix/16);
    pix = col+8*row;
      
    map_mpc[mcp][pix]=ch;
    map_mcp[ch] = mcp;
    map_pix[ch] = pix;
    map_row[ch] = row;
    map_col[ch] = col;
  }
}

UInt_t SwapBigEndian(UInt_t nBigEndianNumber){
  return __builtin_bswap32(nBigEndianNumber);
}

HldUnpacker::HldUnpacker(string hldFName, string tdcFName,
			 UInt_t subEventId, UInt_t ctsAddress,
			 UInt_t mode,UInt_t verbose) : fMode(mode),fVerbose(verbose){
  fRootName = "tt.root";
  fTriggerChannel = 1776;
  fTrailingTime.resize(maxch);
  fTdcAddresses.reserve(NO_OF_TDC_ADDRESSES);

  TrbSettings.nSubEventId = 0; // set subevent ID to 0 (should be 0x8C00 for TRBv3)
  TrbSettings.nTdcRefChannel = 0;
  TrbSettings.nTdcAddress.clear();
  TrbSettings.nTdcAddress.reserve(NO_OF_TDC_ADDRESSES);
  TrbSettings.nSubEventId = subEventId;
  TrbSettings.nCtsAddress = ctsAddress;

  fEvtIndex.clear();
  fEvtIndex.reserve(10000000);
  
  fHldFile.open(hldFName.c_str(), ifstream::in | ifstream::binary);
  if(fHldFile.fail()) exit(-1);
  if(SetHubAddresses("hub.list")<1) exit (-1);
  if(SetTdcAddresses(tdcFName)<1) exit (-1);
  
  IndexEvents();
}

void HldUnpacker::Decode(Long_t startEvent, Long_t endEvent) {

  TFile *file;
  TTree *tree;
  PrtEvent event;
  
  fHldFile.clear();
  RewindFile();
  
  fHldFile.seekg(fEvtIndex.at(startEvent),ios::beg);

  if(fMode==0){
    file = new TFile(fRootName.c_str(),"RECREATE");
    tree = new TTree("data","dirc@gsi hld unpacker",2);  
    tree->Branch("PrtEvent","PrtEvent",&event,128000,2);
  }
  
  if(endEvent==0) endEvent = fEvtIndex.size();
  std::cout<<"# of events  "<< endEvent<<std::endl;
  
  CreateMap();
  initDigi(0);
  for(Long_t e = startEvent; e<endEvent; e++){
    if(e%10000==0) std::cout<<"event # "<< e <<std::endl;
    //if(e>1000) break;
    
    if(fMode==0) event = PrtEvent();
    
    if(ReadEvent(&event, kTRUE)){
      if(event.GetHitSize()>0) tree->Fill();
    }
  }
  if(fMode==0) tree->Write();

  TCanvas *c = new TCanvas("c","c",0,0,800,400);
  hTimeDiff->Draw();
  c->Modified();
  c->Update();
  c->Print("time.png");
  SetRootPalette(1);
  drawDigi("m,p,v\n",2,-2,-2);
  cDigi->Print("digi.png");
}

void HldUnpacker::IndexEvents(){
  RewindFile();
  while(fHldFile.good()){
    Int_t tEvtIndex = fHldFile.tellg();
    if(ReadEvent(NULL, kFALSE))  fEvtIndex.push_back(tEvtIndex);
  }
}

Bool_t HldUnpacker::ReadEvent(PrtEvent *event, Bool_t all){
  UInt_t ehHSize = sizeof(HLD_HEADER);
  // read header of the event
  fHldFile.read((char*)&fEventHeader,ehHSize);
  if(fHldFile.gcount() != ehHSize) return kFALSE;
  
  
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
    	   << "  Decoding: \t" << hex << fEventHeader.nDecoding << dec
    	   << "  ID: \t\t" << hex << fEventHeader.nId << dec
    	   << "  Seq Nr: \t" << fEventHeader.nSeqNr 
    	   << "  Date: \t\t" << hex << fEventHeader.nDate << dec 
    	   << "  Time: \t\t" << hex << fEventHeader.nTime << dec 
    	   << "  Run: \t\t" << hex << fEventHeader.nRun << dec
    	   << "  Pad: \t\t" << hex << fEventHeader.nPad << dec <<endl;
    }
    
    fHitArray.clear();
    //    std::fill(fTrailingTime.begin(), fTrailingTime.end(), 0);
    ReadSubEvent(fDataBytes);

    for(Int_t i=0; i<fHitArray.size(); i++){
      PrtHit hit;
      hit = fHitArray[i];
      Int_t ch = hit.GetChannel();
      hit.SetLeadTime(hit.GetLeadTime()-fTriggerTime);
      hit.SetTotTime(hit.GetLeadTime()-fTrailingTime[ch]);
      if(fMode==0) event->AddHit(hit);
      hTimeDiff->Fill(hit.GetTotTime());
     
      if(hit.GetMcpId()<15)
	fhDigi[hit.GetMcpId()]->Fill(map_col[ch],map_row[ch]);
      
    }
    if(fMode==0) event->SetTime(10);
  }

  //skip empty bytes at the end of event
  UInt_t skipbytes = baseEventSize * (size_t)((fEventHeader.nSize-1)/baseEventSize + 1) - fEventHeader.nSize;
  if(skipbytes>0) fHldFile.ignore(skipbytes);
  
  return kTRUE;
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

    // convert header words from big Endian to little Endian type

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
    transform(fTrbData.begin(),fTrbData.end(),fTrbData.begin(),SwapBigEndian);

    // for(UInt_t i=0;i<fTrbData.size();i=i+4){
    //   std::cout<<std::bitset<32>(fTrbData[i]) <<"  "<<std::bitset<32>(fTrbData[i+1]) <<"  "<<std::bitset<32>(fTrbData[i+2]) <<"  "<<std::bitset<32>(fTrbData[i+3]) <<std::endl;
    // }
    // std::cout<<"============================ " <<  fTrbData.size() <<std::endl;
    
    if(false)
    for(UInt_t i=0;i<fTrbData.size();i=i+1){
      UInt_t word = fTrbData[i];
      trbAddress = word & 0xFFFF;
      trbWords   = word>>16;

      //if(trbAddress< 10000 && tdcmap[trbAddress]!=0) {
      if(trbAddress==8193){
	std::cout<<"tdcmap[trbAddress] "<<tdcmap[trbAddress] <<std::endl;
	std::cout<<RED<<hex<<fTrbData[i]  <<CYAN <<"  a "<<trbAddress << dec<<YELLOW<<"  w "<< trbWords<<std::endl;
      }
	// }
    }

    UInt_t tdcWords(0), tdcAddress(0);
    for(UInt_t i=0;i<fTrbData.size();i++){
      UInt_t word = fTrbData[i];
      //std::cout<<"word "<< hex<< word  << dec<<std::endl;
      
      
      trbAddress = word & 0xFFFF;
      trbWords   = word>>16;

      if(trbAddress< 10000 && tdcmap[trbAddress]>=0) tdcWords  = trbWords;

      if(tdcWords>0){
	for(UInt_t t=0; t<tdcWords; t++){
	  word = fTrbData[i+1+t];
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
	    if(tdcChannel==0 || tdcChannel>48) continue;
	    ch = 48*tdcmap[trbAddress]+tdcChannel-1;
	
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

UInt_t HexStringToInt(string str){
  if(str.empty()) return (0);
  stringstream cConverter(str);
  UInt_t nValue;
  cConverter >> hex >> nValue;
  return nValue;
}

Int_t HldUnpacker::SetHubAddresses(string filename){
  ifstream UserInputFile(filename.c_str(),ifstream::in);
  while(UserInputFile.good()){
    string cCurrentLine;
    getline(UserInputFile,cCurrentLine);
    if(cCurrentLine.empty()) continue;
    vector<string> tokens = LineParser(cCurrentLine,' ');
    fHubAddresses.push_back(tokens.at(0));
  }
  UserInputFile.close();
  TrbSettings.nHubAddress.resize(fHubAddresses.size());
  transform(fHubAddresses.begin(),fHubAddresses.end(),TrbSettings.nHubAddress.begin(),HexStringToInt);
  return(fHubAddresses.size());
}

Int_t HldUnpacker::SetTdcAddresses(string filename){
  ifstream file(filename.c_str(),ifstream::in);
  while(file.good()){
    string line;
    getline(file,line);
    if(line.empty()) continue;
    vector<string> tokens = LineParser(line,' ');
    fTdcAddresses.push_back(tokens.at(0));  
  }
  file.close();

  TrbSettings.nTdcAddress.resize(fTdcAddresses.size());
  transform(fTdcAddresses.begin(),fTdcAddresses.end(),TrbSettings.nTdcAddress.begin(),HexStringToInt);
  return fTdcAddresses.size();
}

std::vector<string> HldUnpacker::LineParser(string line, char delimiter){
  std::vector<string> cTokens; 
  cTokens.reserve(10);
  if(line.empty()) return cTokens;

  string cBuffer;
  stringstream cParsingLine(line);
  while(std::getline(cParsingLine,cBuffer,delimiter)){ 
    if(!cBuffer.empty()) cTokens.push_back(cBuffer);
    cBuffer.clear(); 
  }
  return cTokens;
}

