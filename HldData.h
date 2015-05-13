// -----------------------------------------
// PrtHit.h
//
// Author  : R.Dzhygadlo at gsi.de
// -----------------------------------------

#ifndef PrtHit_h
#define PrtHit_h 1

#include <vector>

#include "TObject.h"
#include "TVector3.h"

struct TRB_SETUP{
  UInt_t nSubEventId; // subevent ID (should be 0x8c00)
  UInt_t nTdcRefChannel; // TRBv3 TDC reference channel
  UInt_t nCtsAddress; // TRB address of Central Trigger System (TCS), should be 0x0002
  std::vector<UInt_t> nHubAddress; // vector of TRBv3 addresses of HUBs (can be empty)
  std::vector<UInt_t> nTdcAddress; // vector of TRBv3 addresses of TDC endpoints
};

struct HLD_HEADER { // HLD header description
  UInt_t nSize; // size of event in 4-byte words
  UInt_t nDecoding;
  UInt_t nId;
  UInt_t nSeqNr; // sequential trigger number, use to identfy event
  UInt_t nDate;
  UInt_t nTime;
  UInt_t nRun;
  UInt_t nPad;
};

struct SUB_HEADER{
  UInt_t nSize; // subevent size in bytes
  UInt_t nDecoding; // subevent decoding settings
  UInt_t nEventId; // subevent ID (should be 0x8c00 for TRBv3)
  UInt_t nTrigger; // subevent trigger number
};

struct SUB_TRAILER{
  UInt_t nSebHeader;
  UInt_t nSebError; // this is the subevent builder error code (must be 0x00000001 otherwise event is corrupted)
};

struct TDC_HEADER{ // TRBv3 TDC header information
  UInt_t nRandomBits; // random code, generated individually for each event
  UInt_t nErrorBits; // TDC errors are indicated here (0 in case of no errors)
};


class PrtHit : public TObject {

public:   
 
  //Constructor
  PrtHit(){};
  ~PrtHit(){};
 
  // Accessors 
  Int_t GetTrb() { return fTrb;}
  Int_t GetTdc() { return fTdc;}
  Int_t GetChannel() { return fChannel;}
  Int_t GetMcpId()       { return fMcpId; }
  Int_t GetPixelId()     { return fPixelId; }
  Double_t GetLeadTime() { return fLeadTime; } 
  Double_t GetTotTime() { return fTotTime; } 
    
  // Mutators  
  void SetTrb(Int_t val) { fTrb = val; }
  void SetTdc(Int_t val) { fTdc = val; }
  void SetChannel(Int_t val) { fChannel=val; }
  void SetMcpId(Int_t val)   { fMcpId = val; }
  void SetPixelId(Int_t val) { fPixelId = val; }
  void SetLeadTime(Double_t val) { fLeadTime=val; } 
  void SetTotTime(Double_t val) { fTotTime=val; } 

protected:
  
  Int_t fTrb;
  Int_t fTdc;
  Int_t fChannel;
  Int_t fMcpId;
  Int_t fPixelId;
  Double_t fLeadTime;    
  Double_t fTotTime;  

  ClassDef(PrtHit,3)
};

class PrtEvent: public TObject  {

public:

  PrtEvent(){fEventTime=0;fState=0;}
  ~PrtEvent(){}; 

  void AddHit(PrtHit hit)  { fHitArray.push_back(hit); }
  PrtHit GetHit(Int_t ind) { return fHitArray[ind]; }
 
  // Accessors 
  Long_t GetTime()   const { return fEventTime; }
  UInt_t GetState() const { return fState; } 
  UInt_t GetHitSize() const { return fHitArray.size(); }
  
  // Mutators
  void SetTime(Long_t val)   { fEventTime = val; }
  void SetState(UInt_t val) { fState = val; }

private: 
  Long_t fEventTime;
  std::vector<PrtHit> fHitArray;
  UInt_t fState; 
 
  ClassDef(PrtEvent, 3);
};

#endif



