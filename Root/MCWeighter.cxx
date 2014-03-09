#include <iostream>
#include "TSystem.h"
#include "TFile.h"
#include "TKey.h"
#include "TTree.h"
#include "TObjArray.h"
#include "TChainElement.h"
#include "TH1F.h"
#include "SusyNtuple/MCWeighter.h"

using namespace std;
using namespace Susy;

/*--------------------------------------------------------------------------------*/
// Constructor
/*--------------------------------------------------------------------------------*/
MCWeighter::MCWeighter(TTree* tree, string xsecDir) : 
        m_useProcSumw(true),
        m_sumwMethod(Sumw_MAP),
        m_xsecMethod(Xsec_ST),
        m_xsecDB(gSystem->ExpandPathName(xsecDir.c_str()))
{
  if(tree) buildSumwMap(tree);
}

/*--------------------------------------------------------------------------------*/
// Destructor
/*--------------------------------------------------------------------------------*/
MCWeighter::~MCWeighter()
{}

/*--------------------------------------------------------------------------------*/
// Build a map of MCID -> sumw.
// This method will loop over the input files associated with the TChain. The MCID 
// in the first entry of the tree will be used, so one CANNOT use this if multiple 
// datasets are combined into one SusyNt tree file! The generator weighted cutflow 
// histograms will then be used to calculate the total sumw for each MCID. Each 
// dataset used here must be complete, they CANNOT be spread out across multiple jobs.
// However, one can have more than one (complete) dataset in the chain, which is why 
// we use the map.
/*--------------------------------------------------------------------------------*/
void MCWeighter::buildSumwMap(TTree* tree)
{
  if(tree->InheritsFrom("TChain")){
    buildSumwMapFromChain(dynamic_cast<TChain*>(tree));
  }
  else buildSumwMapFromTree(tree);

  // Dump the map values
  cout << endl << "On-the-fly sumw computation:" << endl;
  dumpSumwMap();
}
/*--------------------------------------------------------------------------------*/
void MCWeighter::buildSumwMapFromTree(TTree* tree)
{
  TFile* f = tree->GetCurrentFile();

  // Setup branch for accessing the MCID in the tree
  Event* evt = 0;
  tree->SetBranchStatus("*", 0);
  tree->SetBranchStatus("mcChannel", 1);
  tree->SetBranchAddress("event", &evt);
  tree->GetEntry(0);
  // General key, default process number (0)
  SumwMapKey genKey(evt->mcChannel, 0);
  if(m_sumwMap.find(genKey) == m_sumwMap.end()) m_sumwMap[genKey] = 0;

  // Get the generator weighted histogram
  TH1F* hGenCF = (TH1F*) f->Get("genCutFlow");

  // TODO: Check if bin 2 can be used for everything yet
  // This bin counts events after susy propagators have been removed
  int sumwBin = hGenCF->GetXaxis()->FindBin("SusyProp Veto");
  m_sumwMap[genKey] += hGenCF->GetBinContent(sumwBin);
  // Bin 1 is all initial events
  // Bin 2 is events after susy propagators have been removed
  // (relevant for the simplified models only)
  //if(!isSimplifiedModel) m_sumwMap[genKey] += hGenCF->GetBinContent(1);
  //else m_sumwMap[genKey] += hGenCF->GetBinContent(2);

  // Find the histograms per process
  TIter next(f->GetListOfKeys());
  while(TKey* tkey = (TKey*) next()){
    // Test to see if object is a cutflow histogram
    TObject* obj = tkey->ReadObj();
    if(obj->InheritsFrom("TH1")){
      string histoName = obj->GetName();

      // Histo is named procCutFlowXYZ where XYZ is the process number
      string prefix = "procCutFlow";
      if(histoName.find(prefix) != string::npos){
        TH1F* hProcCF = (TH1F*) obj;

        // Extract the process ID (XYZ) from the histo name (procCutFlowXYZ)
        string procString = histoName.substr(prefix.size(), string::npos);
        // Make sure the string is an int
        if(!isInt(procString)){
          cerr << "MCWeighter::buildSumwMap - ERROR - proc string from procCutFlow "
               << "histo is not an integer! Histo name: " << histoName
               << " proc string: " << procString << endl;
          abort();
        }
        stringstream stream;
        stream << procString;
        int proc;
        stream >> proc;
        // Skip the default with proc = -1 or 0
        if(proc != -1 && proc != 0){
          SumwMapKey procKey(evt->mcChannel, proc);
          if(m_sumwMap.find(procKey) == m_sumwMap.end()) m_sumwMap[procKey] = 0;
          m_sumwMap[procKey] += hProcCF->GetBinContent(sumwBin);
          //if(!isSimplifiedModel) m_sumwMap[procKey] += hProcCF->GetBinContent(1);
          //else m_sumwMap[procKey] += hProcCF->GetBinContent(2);
        }
      } // Is a proc cutflow
    } // Object is a histo
  } // Loop over TKeys in TFile
}
/*--------------------------------------------------------------------------------*/
void MCWeighter::buildSumwMapFromChain(TChain* chain)
{
  //cout << "MCWeighter::buildSumwMap" << endl;

  // Loop over files in the chain
  TObjArray* fileElements = chain->GetListOfFiles();
  TIter next(fileElements);
  TChainElement* chainElement = 0;
  while((chainElement = (TChainElement*)next())){
    TString fileTitle = chainElement->GetTitle();
    TFile* f = TFile::Open(fileTitle.Data());

    // Get the tree, for extracting mcid
    TTree* tree = (TTree*) f->Get("susyNt");

    buildSumwMapFromTree(tree);

    /*
    // Setup branch for accessing the MCID in the tree
    Event* evt = 0;
    tree->SetBranchStatus("*", 0);
    tree->SetBranchStatus("mcChannel", 1);
    tree->SetBranchAddress("event", &evt);
    tree->GetEntry(0);
    // General key, default process number (0)
    SumwMapKey genKey(evt->mcChannel, 0);
    if(m_sumwMap.find(genKey) == m_sumwMap.end()) m_sumwMap[genKey] = 0;

    // Get the generator weighted histogram
    TH1F* hGenCF = (TH1F*) f->Get("genCutFlow");

    // TODO: Check if bin 2 can be used for everything yet
    // This bin counts events after susy propagators have been removed
    int sumwBin = hGenCF->GetXaxis()->FindBin("SusyProp Veto");
    m_sumwMap[genKey] += hGenCF->GetBinContent(sumwBin);
    // Bin 1 is all initial events
    // Bin 2 is events after susy propagators have been removed
    // (relevant for the simplified models only)
    //if(!isSimplifiedModel) m_sumwMap[genKey] += hGenCF->GetBinContent(1);
    //else m_sumwMap[genKey] += hGenCF->GetBinContent(2);

    // Find the histograms per process
    TIter next(f->GetListOfKeys());
    while(TKey* tkey = (TKey*) next()){
      // Test to see if object is a cutflow histogram
      TObject* obj = tkey->ReadObj();
      if(obj->InheritsFrom("TH1")){
        string histoName = obj->GetName();

        // Histo is named procCutFlowXYZ where XYZ is the process number
        string prefix = "procCutFlow";
        if(histoName.find(prefix) != string::npos){
          TH1F* hProcCF = (TH1F*) obj;

          // Extract the process ID (XYZ) from the histo name (procCutFlowXYZ)
          string procString = histoName.substr(prefix.size(), string::npos);
          // Make sure the string is an int
          if(!isInt(procString)){
            cerr << "MCWeighter::buildSumwMap - ERROR - proc string from procCutFlow "
                 << "histo is not an integer! Histo name: " << histoName
                 << " proc string: " << procString << endl;
            abort();
          }
          stringstream stream;
          stream << procString;
          int proc;
          stream >> proc;
          // Skip the default with proc = -1 or 0
          if(proc != -1 && proc != 0){
            SumwMapKey procKey(evt->mcChannel, proc);
            if(m_sumwMap.find(procKey) == m_sumwMap.end()) m_sumwMap[procKey] = 0;
            m_sumwMap[procKey] += hProcCF->GetBinContent(sumwBin);
            //if(!isSimplifiedModel) m_sumwMap[procKey] += hProcCF->GetBinContent(1);
            //else m_sumwMap[procKey] += hProcCF->GetBinContent(2);
          }
        } // Is a proc cutflow
      } // Object is a histo
    } // Loop over TKeys in TFile
    */

    f->Close();
    delete f;
  } // Loop over TChain elements

}

/*--------------------------------------------------------------------------------*/
void MCWeighter::dumpSumwMap()
{
  // Dump out the MCIDs and calculated sumw
  SumwMap::iterator sumwMapIter;
  cout.precision(8);
  for(sumwMapIter = m_sumwMap.begin(); sumwMapIter != m_sumwMap.end(); sumwMapIter++){
    cout << "mcid: " << sumwMapIter->first.first
         << " proc: " << sumwMapIter->first.second
         << " sumw: " << sumwMapIter->second
         << endl;
  }
  cout.precision(6);
}

/*--------------------------------------------------------------------------------*/
// Get event weight, combine gen, pileup, xsec, and lumi weights
// Default weight uses A-D lumi
// You can supply a different luminosity, 
// but the pileup weights will still correspond to A-D
/*--------------------------------------------------------------------------------*/
float MCWeighter::getMCWeight(const Event* evt, float lumi, WeightSys sys)
{
  if(!evt->isMC) return 1;
  else{
    float sumw = getSumw(evt);
    float xsec = getXsecTimesEff(evt, sys);
    float pupw = getPileupWeight(evt, sys);
    return evt->w * pupw * xsec * lumi / sumw;
  }
}

/*--------------------------------------------------------------------------------*/
// Get the sumw for this event
/*--------------------------------------------------------------------------------*/
float MCWeighter::getSumw(const Event* evt)
{
  float sumw = evt->sumw;
  if(m_sumwMethod==Sumw_MAP){
    // Map key is pair(mcid, proc)
    unsigned int mcid = evt->mcChannel;
    int procID = m_useProcSumw? evt->susyFinalState : 0;
    // Correct for procID == -1
    if(procID < 0) procID = 0;
    SumwMapKey key(mcid, procID);
    SumwMap::const_iterator sumwMapIter = m_sumwMap.find(key);
    if(sumwMapIter != m_sumwMap.end()) sumw = sumwMapIter->second;
    else{
      cerr << "MCWeighter::getEventWeight - ERROR - requesting to use sumw map but "
           << "mcid " << mcid << " proc " << procID << " not found!" << endl;
      cerr << "Here's what's currently in the map:" << endl;
      dumpSumwMap();
      abort();
    }
  }
  return sumw;
}

/*--------------------------------------------------------------------------------*/
// Get the SUSYTools cross section for this sample
/*--------------------------------------------------------------------------------*/
SUSY::CrossSectionDB::Process MCWeighter::getCrossSection(const Event* evt)
{
  using namespace SUSY;
  if(evt->isMC){
    // SUSYTools expects 0 as default value, but we have existing tags with default of -1
    int proc = evt->susyFinalState > 0? evt->susyFinalState : 0;
    // Temporary bugfix for Wh nohadtau in n0150
    #warning "Temporary bugfix for Wh nohadtau in n0150"
    if(evt->mcChannel >= 177501 && evt->mcChannel <= 177528) proc = 125;
    const intpair k(evt->mcChannel, proc);
    // Check to see if we've cached this process yet.
    XSecMap::const_iterator iter = m_xsecCache.find(k);
    if(iter != m_xsecCache.end()){
      return iter->second;
    }
    else{
      // Hasn't been cached yet, load it from the DB
      CrossSectionDB::Process p = m_xsecDB.process(evt->mcChannel, proc);
      m_xsecCache[k] = p;
      return p;
    }
  }
  cerr << "MCWeighter::getCrossSection - WARNING - xsec not found in SUSYTools." << endl;
  return CrossSectionDB::Process();
}

/*--------------------------------------------------------------------------------*/
float MCWeighter::getXsecTimesEff(const Event* evt, MCWeighter::WeightSys sys)
{
  float xsec = evt->xsec;
  if(m_xsecMethod==Xsec_ST){
    SUSY::CrossSectionDB::Process p = getCrossSection(evt);
    xsec = p.xsect() * p.kfactor() * p.efficiency();
    if(sys==MCWeighter::Sys_XSEC_UP)
      xsec *= (1. + p.relunc());
    else if(sys==MCWeighter::Sys_XSEC_DN)
      xsec *= (1. - p.relunc());
  }
  return xsec;
}

/*--------------------------------------------------------------------------------*/
// Get the pileup weight for this event
/*--------------------------------------------------------------------------------*/
float MCWeighter::getPileupWeight(const Event* evt, MCWeighter::WeightSys sys)
{
  if(sys==MCWeighter::Sys_PILEUP_UP) return evt->wPileup_up;
  else if(sys==MCWeighter::Sys_PILEUP_DN) return evt->wPileup_dn;
  else return evt->wPileup;
}

/*--------------------------------------------------------------------------------*/
// Utils for checking that a string is an int. See
// http://stackoverflow.com/questions/2844817/how-do-i-check-if-a-c-string-is-an-int
/*--------------------------------------------------------------------------------*/
std::string MCWeighter::rmLeadingTrailingWhitespaces(const std::string& str)
{
  using std::string;
  size_t startpos = str.find_first_not_of(" \t");
  size_t endpos = str.find_last_not_of(" \t");
  if(( string::npos == startpos ) || ( string::npos == endpos)) return string("");
  else return str.substr(startpos, endpos-startpos+1);
}
/*--------------------------------------------------------------------------------*/
bool MCWeighter::isInt(const std::string& s)
{
  std::string rs(rmLeadingTrailingWhitespaces(s));
  if(rs.empty() || ((!isdigit(rs[0])) && (rs[0] != '-') && (rs[0] != '+'))) return false ;
  char * p ;
  strtol(rs.c_str(), &p, 10) ;
  return (*p == 0) ;
}

