#include <iostream>
#include <sstream>
#include <algorithm>

#include "HldUnpacker.h"

using namespace std;

namespace {
  void PrintUsage() {
    cout<<"Usage: "<<endl;
    cout<<"    -i   inputFile.hld   (or /d/may2015/ce*.hld im -m3 option is used)"<<endl;
    cout<<"    -o   outputFile.root "<<endl;
    cout<<"    -s   start event "<<endl;
    cout<<"    -e   end event "<<endl;
    cout<<"    -u   uniq id ( 0 default) "<<endl;
    cout<<"    -m   mode; 0 - create root tree (default); 1 - create plots; 3 - online"<<endl;
    cout<<"    -v   verbose level "<<endl;
  }
}

vector<string>  gOldFiles;

int main(int argc, const char ** argv){

  string tdcAddresses("tdc.list");
  string hubAddresses("hub.list");
  string inRegex("/d/may2015/ce*.hld");
  string inFile("../data/dd15110143027.hld");
  string outFile("dd.root");
  int s(0),e(0),mode(0),verbose(0),uniqid(0);
  
  for (int i=1; i<argc; i=i+2 ) {
    if ( string(argv[i]) == "-i" )      inFile   = argv[i+1];
    else if ( string(argv[i]) == "-o" ) outFile  = argv[i+1];
    else if ( string(argv[i]) == "-v" ) verbose  = atoi(argv[i+1]);
    else if ( string(argv[i]) == "-s" ) s  = atoi(argv[i+1]);
    else if ( string(argv[i]) == "-e" ) e  = atoi(argv[i+1]);
    else if ( string(argv[i]) == "-m" ) mode = atoi(argv[i+1]);
    else if ( string(argv[i]) == "-u" ) uniqid = atoi(argv[i+1]);
    else {
      PrintUsage();
         return 1;
    }
  }

  HldUnpacker u(inFile,outFile,tdcAddresses,0x8100,0x7999,mode,verbose,uniqid);
  
  if(mode<3) u.Decode(s,e);
  else{
    std::cout<<"inFile "<< inFile<<std::endl;
    
    if(inFile.find('*') != std::string::npos) inRegex = inFile;
    string shellcmd = "rm -f  newfile.tmp && ls ";
    shellcmd.append(inRegex);
    shellcmd.append(" -ltr | grep ^- | tail -1 | awk '{ print $(NF) }' | tr -d '\n' > newfile.tmp");
    while(1){
      system(shellcmd.c_str());
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

