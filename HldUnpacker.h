#ifndef TRBUNPACKER_H
#define TRBUNPACKER_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <locale>
#include <string>
#include <vector>
#include <sstream>

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
  HldUnpacker(string hldFName, string tdcFName, UInt_t subEventId, UInt_t ctsAddress, UInt_t verbose=0); 
  ~HldUnpacker(){};
  
  void SetOutFile(string var){fRootName = var; }
  void Decode(Long_t startEvent, Long_t endEvent);  
  Bool_t ReadEvent(PrtEvent* event, Bool_t all);
  Bool_t ReadSubEvent(UInt_t data);
  
  Long_t GetHldEntries() const { return ((Long_t) fEvtIndex.size()); };

  std::vector<string> LineParser(string line, char delimiter);
  
private:
  ifstream fHldFile;
  HLD_HEADER fEventHeader;
  SUB_HEADER fSubEventHeader;
  SUB_TRAILER fSubEventTrailer;
  
  void IndexEvents();
  Bool_t CheckHubAddress(UInt_t& nUserHubAddress);
  Bool_t CheckTdcAddress(UInt_t& nUserTdcAddress);
  
  void RewindFile() { fHldFile.seekg(0,ios::beg); }; // set file get pointer to beginning of HLD file
  Int_t SetHubAddresses(string adressesFile);
  Int_t SetTdcAddresses(string adressesFile);

  string fRootName;
  vector<Double_t> fTrailingTime;
  vector<PrtHit> fHitArray;
  vector<string> fHubAddresses;
  vector<string> fTdcAddresses; // vector containing TDC addresses as strings
  vector<Int_t>  fEvtIndex;     // vector containing raw file positions of events

  Int_t fTriggerChannel;
  Double_t fTriggerTime;
  TRB_SETUP TrbSettings;
  UInt_t fCurrentSize;
  UInt_t fVerbose;
  UInt_t dataBytes;
};

#endif
