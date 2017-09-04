#ifndef hld_unpacker_h
#define hld_unpacker_h

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <bitset>

#include "TClonesArray.h"
#include "TFile.h"
#include "TObject.h"
#include "TTree.h"

#include "HldData.h"

// #include "PrtEvent.h"
// #include "PrtHit.h"

#define SIZE_OF_DATAWORD 4
#define NO_OF_TDC_ADDRESSES 16

#define NO_ERR_BITS 8
#define SEB_ERROR_CODE 0x00000001
#define TRB_HEADER_MARKER 0xC0
#define TDC_HEADER_MARKER 1 // 0b001
#define TDC_EPOCH_MARKER  3 // 0b011
#define TDC_DEBUG_MARKER  2 // 0b010

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

using namespace std;

class HldUnpacker {

public:
  HldUnpacker(string hldFName, string outFile, string tdcFName, UInt_t subEventId, UInt_t ctsAddress, UInt_t mode=0, UInt_t freq =0, UInt_t verbose=0, UInt_t uniqid=0); 
  ~HldUnpacker(){};

  Int_t IndexEvents();  
  void Decode(Int_t startEvent, Int_t endEvent);
  void DecodePos(Int_t startPos, Int_t endPos);
  void DecodeOnline(string hldFName);
  void Reset();
  void Report(Int_t flag);
  Bool_t ReadEvent(PrtEvent* event, Bool_t all);
  Bool_t ReadSubEvent(UInt_t data);
  Bool_t GoodHeader(HLD_HEADER header);
  void SetDataRegex(string regex){fDataRegex = regex;}

private:
  ifstream fHldFile;
  HLD_HEADER fEventHeader;
  SUB_HEADER fSubEventHeader;
  SUB_TRAILER fSubEventTrailer;
  
  string fRootName;
  vector<Double_t> fTrailingTime;
  vector<Double_t> fRefTime;
  vector<PrtHit> fHitArray;
  vector<Int_t>  fEvtIndex;
  string fDataRegex;

  Int_t fTriggerChannel;
  Double_t fTriggerTime;
  Double_t fTriggerRefTime;
  UInt_t fCurrentSize;
  UInt_t fMode;
  UInt_t fFreq;
  UInt_t fVerbose;
  UInt_t fDataBytes;
  UInt_t fUniqId;
  UInt_t fEvents;
  UInt_t fFlippedEntry;
  
  // for online monitoring
  UInt_t fTotalHits;
  UInt_t fMcpHits;
  
};

#endif
