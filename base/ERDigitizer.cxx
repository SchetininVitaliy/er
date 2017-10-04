#include "ERDigitizer.h"

#include <vector>

#include "TVector3.h"
#include "TGeoMatrix.h"
#include "TMath.h"
#include "TRandom3.h"

#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"

#include "ERPoint.h"

#include <iostream>
#include <algorithm>
using namespace std;

#include "ERDetectorList.h"

// ----------------------------------------------------------------------------
ERDigitizer::ERDigitizer()
  : FairTask("ER common digitization")
{
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
ERDigitizer::ERDigitizer(TString name, Int_t verbose)
  : FairTask(name, verbose)
{
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
ERDigitizer::~ERDigitizer()
{
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
void ERDigitizer::SetParContainers()
{
  // Get run and runtime database
  FairRunAna* run = FairRunAna::Instance();
  if ( ! run ) Fatal("SetParContainers", "No analysis run");

  FairRuntimeDb* rtdb = run->GetRuntimeDb();
  if ( ! rtdb ) Fatal("SetParContainers", "No runtime database");
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
InitStatus ERDigitizer::Init()
{
  // Get input array
  FairRootManager* ioman = FairRootManager::Instance();
  if ( ! ioman ) Fatal("Init", "No FairRootManager");

  TList* allBranchNames = ioman->GetBranchNameList();
  TIter nextBranch(allBranchNames);
  TObjString* bName;
  vector<TString> pointBranches;
  while (bName = (TObjString*)nextBranch()){
    if (bName->GetString().Contains(fName) && bName->GetString().Contains("Point")){
      pointBranches.push_back(bName->GetString());
    }
  }

  for (Int_t iBranch = 0; iBranch < pointBranches.size(); iBranch++){
    TString pBranch = pointBranches[iBranch];
    fSenVolPoints[pBranch] = (TClonesArray*) ioman->GetObject(pBranch);
    fSenVolDigis[pBranch] = new TClonesArray("ERDigi");
    ioman->Register(pBranch+"Digi", fName, fSenVolDigis[pBranch], kTRUE);
  }
  return kSUCCESS;
}
// -------------------------------------------------------------------------

// -----   Public method Exec   --------------------------------------------
void ERDigitizer::Exec(Option_t* opt){

  Reset();


  for(const auto &itSenVol: fSenVolPoints){
    TClonesArray* senVolPoints = itSenVol.second;
    //Sort the points by volNb
    map<Int_t, vector<Int_t>> points;
    for (Int_t iPoint = 0; iPoint < senVolPoints->GetEntriesFast(); iPoint++){
      ERPoint* point = (ERPoint*)senVolPoints->At(iPoint);
      points[point->GetVolNb()].push_back(iPoint);
    }

    for(const auto &itVol: points){
      fEdep = 0.; //sum edep in vol
      fTime = numeric_limits<float>::max(); // min time in vol
      
      for (const auto itPoint: itVol.second){
        ERPoint* point = (ERPoint*)(senVolPoints->At(itPoint));
        fEdep += point->GetEnergyLoss();

        if (point->GetTime() < fTime){
          fTime = point->GetTime();
        }
      }
      /*
      edep = gRandom->Gaus(edep, fElossSigma);

      if (edep < fElossThreshold)
        continue;

      time = gRandom->Gaus(time, fTimeSigma);
      */
      fVolNb = itVol.first;
      ERDigi *digi = AddDigi(fSenVolDigis[itSenVol.first]);
      /*
      for (const auto itPoint: itSensor.second){
        digi->AddLink(FairLink("RTelescopePoint", itPoint));
      }*/
    }
  }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void ERDigitizer::Reset()
{
  for(const auto &itSenVolDigi: fSenVolDigis){
    itSenVolDigi.second->Clear();
  }
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
void ERDigitizer::Finish()
{   

}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
ERDigi* ERDigitizer::AddDigi(TClonesArray* digis)
{
  ERDigi *digi = new((*digis)[digis->GetEntriesFast()])
              ERDigi(digis->GetEntriesFast(), fEdep, fTime, fVolNb);
  return digi;
}
// ----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ClassImp(ERDigitizer)