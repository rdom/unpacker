#include <iostream>
#include <sstream>
#include <algorithm>

#include "HldUnpacker.h"

using namespace std;

namespace {
  void PrintUsage() {
    cout<<"Usage: "<<endl;
    cout<<"    -i   inputFile.hld   "<<endl;
    cout<<"    -o   outputFile.root "<<endl;
    cout<<"    -s   start event "<<endl;
    cout<<"    -e   end event "<<endl;
    cout<<"    -m   mode; 0 - create root tree (default); 1 - create plots; 3 - online"<<endl;
    cout<<"    -v   verbose level "<<endl;
  }
}

vector<string>  gOldFiles;

int main(int argc, const char ** argv){

  string tdcAddresses("tdc.list");
  string hubAddresses("hub.list");
  string inFile("../data/dd15110143027.hld");
  string outFile("dd.root");
  int s(0),e(0),mode(0),verbose(0);
  
  for (int i=1; i<argc; i=i+2 ) {
    if ( string(argv[i]) == "-i" )      inFile   = argv[i+1];
    else if ( string(argv[i]) == "-o" ) outFile  = argv[i+1];
    else if ( string(argv[i]) == "-v" ) verbose  = atoi(argv[i+1]);
    else if ( string(argv[i]) == "-s" ) s  = atoi(argv[i+1]);
    else if ( string(argv[i]) == "-e" ) e  = atoi(argv[i+1]);
    else if ( string(argv[i]) == "-m" ) mode  = atoi(argv[i+1]);
    else if ( string(argv[i]) == "-v" ) verbose  = atoi(argv[i+1]);
    else {
      PrintUsage();
      return 1;
    }
  }

  HldUnpacker u(inFile,outFile,tdcAddresses,0x8100,0x7999,mode,verbose);
  
  if(mode<3) u.Decode(s,e);
  else{

    while(1){
      system("rm -f  newfile.tmp && ls /data.local/may2015/ce*.hld -ltr | grep ^- | tail -1 | awk '{ print $(NF) }' | tr -d '\n' > newfile.tmp");
      ifstream t("newfile.tmp");
      inFile = string((istreambuf_iterator<char>(t)), istreambuf_iterator<char>()); 

      if (std::find(gOldFiles.begin(), gOldFiles.end(), inFile) != gOldFiles.end()){
	cout<<"No new files  "<<endl;	
      }else{
	cout<<"Reading  "<< inFile<<endl;
	gOldFiles.push_back(inFile);
	u.DecodeOnline(inFile);
      }
      sleep(1);
    }
  }
  
  return 0;
}

