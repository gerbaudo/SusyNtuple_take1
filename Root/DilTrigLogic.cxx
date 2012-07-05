// Trigger methods to handle Susy Dilepton trigger logic

#include "SusyNtuple/DilTrigLogic.h"

/*--------------------------------------------------------------------------------*/
// Constructor
/*--------------------------------------------------------------------------------*/
DilTrigLogic::DilTrigLogic(bool isMC) :
  m_latestLogic(false)
{
  
  //if( isMC ) loadTriggerMaps();
  

}
/*--------------------------------------------------------------------------------*/
// Destructor
/*--------------------------------------------------------------------------------*/
DilTrigLogic::~DilTrigLogic()
{
  
  if( m_trigTool_mu18 )           delete m_trigTool_mu18;
  if( m_trigTool_mu18Med )        delete m_trigTool_mu18Med;
  if( m_trigTool_mu10L_not18 )    delete m_trigTool_mu18;
  if( m_trigTool_mu10L_not18Med ) delete m_trigTool_mu18;
  if( m_trigTool_mu18 )           delete m_trigTool_mu18;
  
}

/*--------------------------------------------------------------------------------*/
// Does event pass susy dilepton trigger logic?
/*--------------------------------------------------------------------------------*/
bool DilTrigLogic::passDilTrig(LeptonVector leptons, Event* evt)
{
  
  DataStream stream = evt->stream;

  // If unknown stream, then return
  if( stream == Stream_Unknown){
    cout<<"Error: Stream is unknown! Returning false from passDilTrig."<<endl;
    return false;
  }
  
  // Set Vars for easy access
  clear();
  setLeptons(leptons);
  uint evtTrigFlags = evt->trigFlags;
  
  // Check:
  DiLepEvtType ET = getDiLepEvtType(leptons);

  // EE
  if( (stream == Stream_Egamma || stream == Stream_MC) && (ET == ET_ee) )
    return passEE(m_elecs, evtTrigFlags);

  // MM
  if( (stream == Stream_Muons || stream == Stream_MC)  && (ET ==ET_mm) )
    return passMM(m_muons, evtTrigFlags);

  // EM
  if( ET == ET_em )
    return passEM(m_elecs, m_muons, evtTrigFlags, stream);
  
  // Must not have matched if we get here
  return false;
  
}

/*--------------------------------------------------------------------------------*/
// Methods for dilepton event types
/*--------------------------------------------------------------------------------*/
bool DilTrigLogic::passEE(ElectronVector elecs, uint evtFlag)
{

  if(elecs.size() != 2) return false;
  
  std::sort(elecs.begin(), elecs.end(), comparePt);

  // Handle two elec trigger effectively
  float ePt0  = elecs.at(0)->Pt();
  float ePt1  = elecs.at(1)->Pt();
  uint flag0  = elecs.at(0)->trigFlags;
  uint flag1  = elecs.at(1)->trigFlags;
  

  return passEETrigRegion(ePt0, ePt1, flag0, flag1, evtFlag);

}
/*--------------------------------------------------------------------------------*/
bool DilTrigLogic::passMM(MuonVector muons, uint evtFlag)
{
 
  if(muons.size() != 2) return false;

  std::sort(muons.begin(), muons.end(), comparePt);

  float mPt0 = muons.at(0)->Pt();
  float mPt1 = muons.at(1)->Pt();
  uint flag0 = muons.at(0)->trigFlags;
  uint flag1 = muons.at(1)->trigFlags;

  return passMMTrigRegion(mPt0, mPt1, flag0, flag1, evtFlag);
  
}
/*--------------------------------------------------------------------------------*/
bool DilTrigLogic::passEM(ElectronVector elecs, MuonVector muons, uint evtFlag, DataStream stream)
{

  if(elecs.size() != 1 && muons.size() != 1) return false;
  
  // Gather necessary vars
  float ePt  = elecs.at(0)->Pt();
  float mPt  = muons.at(0)->Pt();
  uint eFlag = elecs.at(0)->trigFlags; 
  uint mFlag = muons.at(0)->trigFlags; 
  
  return passEMTrigRegion(ePt, mPt, eFlag, mFlag, evtFlag, stream);

}

/*--------------------------------------------------------------------------------*/
// Pass Lepton trigger based on region
/*--------------------------------------------------------------------------------*/
bool DilTrigLogic::passEETrigRegion(float pt0, float pt1, uint flag0, uint flag1, uint evtFlag)
{

  // Region A
  if( 14 < pt0 && 14 < pt1 ){
    bool evtPass = (evtFlag & TRIG_2e12Tvh_loose1);
    bool match   = (flag0 & TRIG_e12Tvh_loose1) || (flag1 & TRIG_e12Tvh_loose1);
    return evtPass && match;
  }
  
  // Region B
  if( 25 < pt0 && 10 < pt1 && pt1 < 14 ){
    bool evtPass      = (evtFlag & TRIG_e24vh_medium1_e7_medium1);
    bool match        = (flag0 & TRIG_e24vh_medium1_e7_medium1) && (flag1 & TRIG_e24vh_medium1_e7_medium1);
    bool matchLeading = (flag0 & TRIG_e24vh_medium1); 
    return evtPass && match && matchLeading;
  }
  

  // Not in region:
  return false;

}
/*--------------------------------------------------------------------------------*/
bool DilTrigLogic::passMMTrigRegion(float pt0, float pt1, uint flag0, uint flag1, uint evtFlag)
{

  // Region A
  if( 18 < pt0 && 18 < pt1 ){
    bool evtPass   = (evtFlag & TRIG_mu18_tight_mu8_EFFS);
    bool match2lep = (flag0 & TRIG_mu18_tight_mu8_EFFS) && (flag1 & TRIG_mu18_tight_mu8_EFFS);
    bool match1lep = (flag0 & TRIG_mu18_tight) || (flag1 & TRIG_mu18_tight);
    return evtPass && match2lep && match1lep;
  }

  // Region B
  if(18 < pt0 && 14 < pt1 && pt1 < 18){
    bool evtPass      = (evtFlag & TRIG_mu18_tight_mu8_EFFS) || (evtFlag & TRIG_2mu13);
    bool match2lep0   = (flag0 & TRIG_mu13) && (flag1 & TRIG_mu13);
    bool match2lep1   = (flag0 & TRIG_mu18_tight_mu8_EFFS) && (flag1 & TRIG_mu18_tight_mu8_EFFS);
    bool matchLeading = (flag0 & TRIG_mu18_tight);
    return evtPass && ( match2lep0 || (match2lep1 && matchLeading) );
  }
  
  // Region C
  if( 18 < pt0 && 8 < pt1 && pt1 < 14 ){
    bool evtPass      = (evtFlag & TRIG_mu18_tight_mu8_EFFS);
    bool match2lep    = (flag0 & TRIG_mu18_tight_mu8_EFFS) && (flag1 & TRIG_mu18_tight_mu8_EFFS);
    bool matchLeading = (flag0 & TRIG_mu18_tight);
    return evtPass && match2lep && matchLeading;
  }

  // Region D
  if( 14 < pt0 && pt0 < 20 && 14 < pt1 ){
    bool evtPass   = (evtFlag & TRIG_2mu13);
    bool match2lep = (flag0 & TRIG_mu13) && (flag1 & TRIG_mu13);
    return evtPass && match2lep;
  }

  // Not in region:
  return false;

}
/*--------------------------------------------------------------------------------*/
bool DilTrigLogic::passEMTrigRegion(float ept, float mpt, uint eflag, uint mflag, 
				    uint evtFlag, DataStream stream)
{

  // From the twiki it appears we order based on lepton pt and require the stream
  // based from this information. This will need to be cross-checked... Hopefully 
  //cutflow will illuminate any issues.

  bool isEM = ept > mpt && ( stream == Stream_Egamma || stream == Stream_MC );  

  // Region A
  if( 14 < ept && 8 < mpt && isEM){
    bool evtPass = (evtFlag & TRIG_e12Tvh_medium1_mu8);
    bool ePass   = (eflag & TRIG_e12Tvh_medium1);
    bool mPass   = (mflag & TRIG_mu8);
    return evtPass && ePass && mPass;
  }

  // Region B
  if( 10 < ept && ept < 14 && 18 < mpt && isEM){
    bool evtPass = (evtFlag & TRIG_mu18_tight_e7_medium1);
    bool ePass   = (eflag & TRIG_e7_medium1); // **CHECK: e7 vs e7T ???
    bool mPass   = (mflag & TRIG_mu18_tight);
    return evtPass && ePass && mPass;
  }
  
  // Not in region:
  return false;
  
  
}

/*--------------------------------------------------------------------------------*/
// Monte Carlo Weighting
/*--------------------------------------------------------------------------------*/

//******************************************************//
// This hasn't been verified or updated. Commented out
// Until 2012 strategy developed. Will return once 
// mc-reweighting becomes an issue.
//******************************************************//

/*
void DilTrigLogic::loadTriggerMaps()
{
  TFile* eleTrigFile = new TFile("$ROOTCOREDIR/data/DGTriggerReweight/electron_maps.root");
  TFile* muoTrigFile = new TFile("$ROOTCOREDIR/data/DGTriggerReweight/muon_triggermaps.root");
  
  // TODO: dilepton triggers   
  m_elTrigWeightMap[TRIG_e20_medium]     = loadTrigWeighter(eleTrigFile, "e20_medium");
  m_elTrigWeightMap[TRIG_e22_medium]     = loadTrigWeighter(eleTrigFile, "e22_medium");
  m_elTrigWeightMap[TRIG_e22vh_medium1]  = loadTrigWeighter(eleTrigFile, "e22vh_medium1");
  m_elTrigWeightMap[TRIG_2e12_medium]    = loadTrigWeighter(eleTrigFile, "e12_medium");
  m_elTrigWeightMap[TRIG_2e12T_medium]   = loadTrigWeighter(eleTrigFile, "e12T_medium");
  m_elTrigWeightMap[TRIG_2e12Tvh_medium] = loadTrigWeighter(eleTrigFile, "e12Tvh_medium");

  m_muTrigWeightMap[TRIG_mu18]           = loadTrigWeighter(muoTrigFile, "mu18");
  m_muTrigWeightMap[TRIG_mu18_medium]    = loadTrigWeighter(muoTrigFile, "mu18_medium");
  m_muTrigWeightMap[TRIG_2mu10_loose]    = loadTrigWeighter(muoTrigFile, "mu10_loose");
}
*/

/*--------------------------------------------------------------------------------*/
/*
APReweightND* DilTrigLogic::loadTrigWeighter(TFile* f, TString chain)
{
  TString numName = "ths_"+chain+"_num";
  TString denName = "ths_"+chain+"_den";
  // muon file currently contains a typo                                                                                                                                       
  if (chain.Contains("mu")) numName = "ths_"+chain+"_nom";

  // Does this memory get cleaned up when the file closes?                                                                                                                     
  THnSparseD* num = (THnSparseD*) f->Get( numName );
  THnSparseD* den = (THnSparseD*) f->Get( denName );
  if(!num || !den){
    cout << "ERROR loading trig maps for chain " << chain << endl;
    abort();
  }
  return new APReweightND( den, num, true );
}
 */
/*--------------------------------------------------------------------------------*/
 /*
APReweightND* DilTrigLogic::getEleTrigWeighter(uint trigFlag)
{
  map<int, APReweightND*>::iterator itr = m_elTrigWeightMap.find(trigFlag);
  if(itr==m_elTrigWeightMap.end()){
    cout << "ERROR - Electron trigger reweighter " << trigFlag << " doesn't exist" << endl;
    return 0;
  }
  return itr->second;
}
 */
/*--------------------------------------------------------------------------------*/
/*
APReweightND* DilTrigLogic::getMuoTrigWeighter(uint trigFlag)
{
  map<int, APReweightND*>::iterator itr = m_muTrigWeightMap.find(trigFlag);
  if(itr==m_muTrigWeightMap.end()){
    cout << "ERROR - Muon trigger reweighter " << trigFlag << " doesn't exist" << endl;
    return 0;
  }
  return itr->second;
}
*/


/*--------------------------------------------------------------------------------*/
// Set Leptons for trigger
/*--------------------------------------------------------------------------------*/
void DilTrigLogic::setLeptons(LeptonVector leps){
  for(uint i=0; i<leps.size(); ++i){
    const Lepton* lep = leps.at(i);
    if(lep->isEle()) m_elecs.push_back( (Electron*) lep );
    else             m_muons.push_back( (Muon*)     lep );
  }
}

/*--------------------------------------------------------------------------------*/
// Debug flag
/*--------------------------------------------------------------------------------*/
void DilTrigLogic::debugFlag(uint flag){

  cout << "\tEF_e7_medium1               " << (flag & TRIG_e7_medium1)               << endl;
  cout << "\tEF_e12Tvh_medium1           " << (flag & TRIG_e12Tvh_medium1)           << endl;
  cout << "\tEF_e24vh_medium1            " << (flag & TRIG_e24vh_medium1)            << endl;
  cout << "\tEF_e24vhi_medium1           " << (flag & TRIG_e24vhi_medium1)           << endl;
  cout << "\tEF_2e12Tvh_loose1           " << (flag & TRIG_2e12Tvh_loose1)           << endl;
  cout << "\tEF_e24vh_medium1_e7_medium1 " << (flag & TRIG_e24vh_medium1_e7_medium1) << endl;
  cout << "\tEF_mu8                      " << (flag & TRIG_mu8)                      << endl;
  cout << "\tEF_mu18_tight               " << (flag & TRIG_mu18_tight)               << endl;
  cout << "\tEF_mu24i_tight              " << (flag & TRIG_mu24i_tight)              << endl;
  cout << "\tEF_2mu13                    " << (flag & TRIG_2mu13)                    << endl;
  cout << "\tEF_mu18_tight_mu8_EFFS      " << (flag & TRIG_mu18_tight_mu8_EFFS)      << endl;
  cout << "\tEF_e12Tvh_medium1_mu8       " << (flag & TRIG_e12Tvh_medium1_mu8)       << endl;
  cout << "\tEF_mu18_tight_e7_medium1    " << (flag & TRIG_mu18_tight_e7_medium1)    << endl;

}

