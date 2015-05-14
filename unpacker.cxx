#include <iostream>
#include "HldUnpacker.h"

using namespace std;

namespace {
  void PrintUsage() {
    cout<<"Usage: "<<endl;
    cout<<"    -i   inputFile.hld   "<<endl;
    cout<<"    -o   outputFile.root "<<endl;
    cout<<"    -v   verbose level "<<endl;
  }
}

int main(int argc, const char ** argv){

  string tdcAddresses("tdc.list");
  string hubAddresses("hub.list");
  string inFile("../data/dd15110143027.hld");
  string outFile("dd.root");
  int verbose(0);
  
  for (int i=1; i<argc; i=i+2 ) {
    if ( string(argv[i]) == "-i" )      inFile   = argv[i+1];
    else if ( string(argv[i]) == "-o" ) outFile  = argv[i+1];
    else if ( string(argv[i]) == "-v" ) verbose  = atoi(argv[i+1]);
    else {
      PrintUsage();
      return 1;
    }
  }

  HldUnpacker u(inFile,tdcAddresses,0x8100,0x7999,verbose);
  
  u.SetOutFile(outFile);
  u.Decode(0,0);

  return 0;
}

