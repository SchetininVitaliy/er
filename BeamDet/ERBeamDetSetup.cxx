#include "ERBeamDetSetup.h"

#include <iostream>

#include "TGeoTube.h"
#include "TError.h"
#include "TMath.h"
#include "TGeoManager.h"
#include "TGeoMatrix.h"
#include "TROOT.h"
#include <Riostream.h>
#include <TDOMParser.h>
#include <TXMLAttr.h>
#include <TXMLNode.h>
#include <TList.h>

#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"
#include "FairGeoLoader.h"
#include "FairGeoMedium.h"
#include "FairGeoInterface.h"
#include "FairGeoBuilder.h"
#include "FairGeoMedia.h"

using namespace std;


ERBeamDetSetup* ERBeamDetSetup::fInstance = NULL;
Double_t        ERBeamDetSetup::fTargetR = 0;
Double_t        ERBeamDetSetup::fDistanceBetweenTOF = 0;
Double_t        ERBeamDetSetup::fDistanceBetweenMWPC = 0;
map<Int_t, map<Int_t, map<Int_t, ERBeamDetWire*>>>ERBeamDetSetup::fWires;
// ----- ToF parameters -----------------------------------------------------
Double_t ERBeamDetSetup::fPlasticX = 100;
Double_t ERBeamDetSetup::fPlasticY = 100;
Double_t ERBeamDetSetup::fPlasticZ = 0.01;
TString  ERBeamDetSetup::fPlasticMedia = "BC408";
// --------------------------------------------------------------------------
// ----- MWPC parameters ----------------------------------------------------
Double_t ERBeamDetSetup::fGasVolX = 5.;
Double_t ERBeamDetSetup::fGasVolY = 5.;
Double_t ERBeamDetSetup::fGasVolZ = 8.2;
Double_t ERBeamDetSetup::fGasStripX = 0.125;
Double_t ERBeamDetSetup::fGasStripY = 5.;
Double_t ERBeamDetSetup::fGasStripZ = 0.6; //cm
Double_t ERBeamDetSetup::fDistBetweenXandY = 1.;
Double_t ERBeamDetSetup::fCathodeThickness = 5 * 1e-4;
Double_t ERBeamDetSetup::fKaptonWindowThickness = 12.5 * 1e-4;
Double_t ERBeamDetSetup::fAnodeWireDiameter  = 20 * 1e-4;
TString  ERBeamDetSetup::fKaptonWindowMedia = "kapton";
TString  ERBeamDetSetup::fCathodeMedia = "aluminium";
TString  ERBeamDetSetup::fAnodeWireMedia = "tungsten";
TString  ERBeamDetSetup::fGasMedia = "CF4_CH4";
// --------------------------------------------------------------------------
// ------ fPosition of detector's parts relative to zero ---------------------
Double_t ERBeamDetSetup::fPositionToF1 = -1550.;
Double_t ERBeamDetSetup::fPositionToF2 = -50.;
Double_t ERBeamDetSetup::fPositionMWPC1 = -40.;
Double_t ERBeamDetSetup::fPositionMWPC2 = -8.;
// -------- fTarget parameters -----------------------------------------------
Double_t ERBeamDetSetup::fTargetH2R = 2.;   //cm
Double_t ERBeamDetSetup::fTargetH2Z = 0.4;   //cm
Double_t ERBeamDetSetup::fTargetShellThickness = 20 * 1e-4;

TString  ERBeamDetSetup::fParamsXmlFileName = "equip.xml";
TString  ERBeamDetSetup::fToFType1 = "ToF1";
TString  ERBeamDetSetup::fToFType2 = "ToF1";
TString  ERBeamDetSetup::fMWPCType1 = "MWPC1";
TString  ERBeamDetSetup::fMWPCType2 = "MWPC1";

Bool_t   ERBeamDetSetup::fFirstToFIsSet = false;
Bool_t   ERBeamDetSetup::fFirstMWPCIsSet = false;


ERBeamDetSetup::ERBeamDetSetup() {

  //-----------------------------------------------------------------------
 // gSystem->Load("libXMLParser.so");
  ParseXmlParameters();
  PrintDetectorParameters();
  std::cout << "ERBeamDetSetup initialized! "<< std::endl;
}
//-------------------------------------------------------------------------
void ERBeamDetSetup::AddMWPC(TString type, Double_t position)
{
  if(!fFirstMWPCIsSet) {
    fMWPCType1 = type;
    fPositionMWPC1 =  position;
    fFirstMWPCIsSet = true;
  }
  else {
    fMWPCType2 = type;
    fPositionMWPC2 =  position;
  }
}
void ERBeamDetSetup::AddToF(TString type, Double_t position)
{
  if(!fFirstMWPCIsSet) {
    fToFType1 = type;
    fPositionToF1 =  position;
    fFirstToFIsSet = true;
  }
  else {
    fToFType2 = type;
    fPositionToF2 =  position;
  }
}
//-------------------------------------------------------------------------
void ERBeamDetSetup::GetGeoParamsFromParContainer() {
  // --- Catch absence of TGeoManager
  if ( ! gGeoManager ) {
    std::cerr << "ERBeamDetSetup: cannot initialise without TGeoManager!"<< std::endl;
  }
 
  gGeoManager->CdTop();
  TGeoNode* cave = gGeoManager->GetCurrentNode();
  TGeoNode* beamDet  = NULL;
  for (Int_t iNode = 0; iNode < cave->GetNdaughters(); iNode++) {
    TString name = cave->GetDaughter(iNode)->GetName();
    if ( name.Contains("BeamDet", TString::kIgnoreCase) ) {
      beamDet = cave->GetDaughter(iNode);
      break;
    }
  }

  // ---- Getting MWPC geometry parameters---------------------------------
  TGeoNode*   mwpc = NULL;
  TGeoNode*   mwpcStation = NULL;
  Double_t    mwpcStationZ;
  Double_t    mwpcStationZ1;
  Double_t    mwpcStationZ2;
  TGeoMatrix* mwpcLocalPos;
  Double_t    mwpcMasterPos[3];
  TGeoNode*   plane = NULL;
  TGeoNode*   wire = NULL;

  for (Int_t iNode = 0; iNode < beamDet->GetNdaughters(); iNode++) {
    TString name = beamDet->GetDaughter(iNode)->GetName();
    if (name.Contains("MWPC", TString::kIgnoreCase) ) {
      mwpc = beamDet->GetDaughter(iNode);
      Int_t mwpcNb = iNode - 2;
      mwpcStationZ = mwpc->GetMatrix()->GetTranslation()[2]; 
      (name.Contains("1", TString::kIgnoreCase)) ? mwpcStationZ1 = mwpcStationZ 
                                                 : mwpcStationZ2 = mwpcStationZ;
      mwpcStation = mwpc->GetDaughter(0);
      //--------------------------------------------------------------------
      for (Int_t planeNb = 0; planeNb < mwpcStation->GetNdaughters(); planeNb++) {
        plane = mwpcStation->GetDaughter(planeNb);

        for (Int_t wireNb = 0; wireNb < plane->GetNdaughters(); wireNb++) {
          wire = plane->GetDaughter(wireNb);
          Double_t x = wire->GetMatrix()->GetTranslation()[0];
          Double_t y = wire->GetMatrix()->GetTranslation()[1];
          (planeNb == 0) ? fWires[mwpcNb][planeNb].insert(std::make_pair(wireNb, new ERBeamDetWire(x, y, mwpcStationZ)))
                         : fWires[mwpcNb][planeNb].insert(std::make_pair(wireNb, new ERBeamDetWire(y, x, mwpcStationZ)));
          std::cout << "Wire " << wireNb << " fPosition (" << fWires[mwpcNb][planeNb][wireNb]->fX << ", " 
                                                          << fWires[mwpcNb][planeNb][wireNb]->fY << ", " 
                                                          << mwpcStationZ << ") cm" << endl;
        }
      } 
    }
  }
  // Stations located simmetrically relative to local center
  fDistanceBetweenMWPC = TMath::Abs(mwpcStationZ1 - mwpcStationZ2);
  cout << "The distance between MWPC stations: " << fDistanceBetweenMWPC << " cm;" << endl;
  //-----------------------------------------------------------------------
  // ---- Getting tofPlastic geometry parameters ---------------------------------
  TGeoNode* tofPlastic = NULL;
  Double_t tofPlastic1Pos, tofPlastic2Pos;
  for (Int_t iNode = 0; iNode < beamDet->GetNdaughters(); iNode++) {
    TString name = beamDet->GetDaughter(iNode)->GetName();
    if ( name.Contains("plastic", TString::kIgnoreCase) ) {
      tofPlastic = beamDet->GetDaughter(iNode);
      if (name.Contains("1", TString::kIgnoreCase)) {
        tofPlastic1Pos = tofPlastic->GetMatrix()->GetTranslation()[2];
      }
      else {
        tofPlastic2Pos = tofPlastic->GetMatrix()->GetTranslation()[2];
      }
    }
  }
  fDistanceBetweenTOF = TMath::Abs(tofPlastic1Pos - tofPlastic2Pos);
  std::cout<< "The distance between plastics: " << fDistanceBetweenTOF << " cm;" << std::endl;
  //-----------------------------------------------------------------------
  // ---- Getting fTarget geometry parameters ------------------------------
  TGeoNode* fTarget = NULL;
  for (Int_t iNode = 0; iNode < beamDet->GetNdaughters(); iNode++) {
    TString name = beamDet->GetDaughter(iNode)->GetName();
    if ( name.Contains("Target", TString::kIgnoreCase) ) {
      fTarget = beamDet->GetDaughter(iNode);
      TGeoNode* shell = fTarget->GetDaughter(0);
      TGeoNode* h2 = shell->GetDaughter(0);
      TGeoTube* h2Tube = (TGeoTube*)h2->GetVolume()->GetShape();
      fTargetR = h2Tube->GetRmax();
      std::cout<< "Target radius " << fTargetR << " cm;" << std::endl;
      break;
    }
  } 
  std::cout << "ERBeamDetSetup: read parameters from parContainer! "<< std::endl; 
}
//--------------------------------------------------------------------------------------------------
ERBeamDetSetup* ERBeamDetSetup::Instance(){
  if (fInstance == NULL)
    return new ERBeamDetSetup();
  else
    return fInstance;
}
//--------------------------------------------------------------------------------------------------
Int_t ERBeamDetSetup::SetParContainers(){
      // Get run and runtime database
  FairRun* run = FairRun::Instance();
  if ( ! run ) Fatal("SetParContainers", "No analysis run");

  FairRuntimeDb* rtdb = run->GetRuntimeDb();
  if ( ! rtdb ) Fatal("SetParContainers", "No runtime database");

}
//--------------------------------------------------------------------------------------------------
Double_t ERBeamDetSetup::WireX(Int_t mwpcNb, Int_t planeNb, Int_t wireNb){
  return fWires[mwpcNb][planeNb][wireNb]->fX;
}
//--------------------------------------------------------------------------------------------------
Double_t ERBeamDetSetup::WireY(Int_t mwpcNb, Int_t planeNb, Int_t wireNb){
  return fWires[mwpcNb][planeNb][wireNb]->fY;
}
//--------------------------------------------------------------------------------------------------
Double_t ERBeamDetSetup::WireZ(Int_t mwpcNb, Int_t planeNb, Int_t wireNb){
  return fWires[mwpcNb][planeNb][wireNb]->fZ;
}
//--------------------------------------------------------------------------------------------------
void ERBeamDetSetup::GetToFParameters(TXMLNode *node) {
  node = node->GetNextNode();
  //cout << "Pasrsing ToF " << node->GetNodeName() << endl;
  TList *attrList = node->GetAttributes();
  TXMLAttr *attr = 0;
  TIter next(attrList);
  while ((attr=(TXMLAttr*)next())) {
    if (!strcasecmp("id", attr->GetName())) { 
      break;
    }
  }
  if(!strcasecmp(fToFType1, attr->GetValue())) {
    cout << "Tof value " << attr->GetValue() << endl;
    for( node = node->GetChildren(); node; node = node->GetNextNode()) {
      if(!strcasecmp(node->GetNodeName(), "plasticGeometry")) {
        attrList = node->GetAttributes();
        attr = 0;
        TIter nextPlasticAttr(attrList);
        while ((attr=(TXMLAttr*)nextPlasticAttr())) {
          if (!strcasecmp("X", attr->GetName())) {
            fPlasticX = atof(attr->GetValue());
          }
          if (!strcasecmp("Y", attr->GetName())) {
            fPlasticY = atof(attr->GetValue());
          }
          if (!strcasecmp("Z", attr->GetName())) {
            fPlasticZ = atof(attr->GetValue());
          }
        }
      }
      if(!strcasecmp(node->GetNodeName(), "plasticMedia")) {
        fPlasticMedia = node->GetText();
      }
    }
  }
}
//--------------------------------------------------------------------------------------------------
void ERBeamDetSetup::GetMWPCParameters(TXMLNode *node) {
  node = node->GetNextNode();
  //cout << "Pasrsing MWPC " << node->GetNodeName() << endl;
  TList *attrList = node->GetAttributes();
  TXMLAttr *attr = 0;
  TIter next(attrList);
  while ((attr=(TXMLAttr*)next())) {
    if (!strcasecmp("id", attr->GetName())) {
      break;
    }
  }
  if(!strcasecmp(fMWPCType1, attr->GetValue())) {
    for( node = node->GetChildren(); node; node = node->GetNextNode()) {
      if(!strcasecmp(node->GetNodeName(), "gasVolGeometry")) {
        attrList = node->GetAttributes();
        attr = 0;
        TIter nextGasVolAttr(attrList);
        while ((attr=(TXMLAttr*)nextGasVolAttr())) {
          if (!strcasecmp("X", attr->GetName())) {
            fGasVolX = atof(attr->GetValue());
          }
          if (!strcasecmp("Y", attr->GetName())) {
            fGasVolY = atof(attr->GetValue());
          }
          if (!strcasecmp("Z", attr->GetName())) {
            fGasVolZ = atof(attr->GetValue());
          }
        }
      }
      if(!strcasecmp(node->GetNodeName(), "gasStripGeometry")) {
        attrList = node->GetAttributes();
        attr = 0;
        TIter nextGasStripAttr(attrList);
        while ((attr=(TXMLAttr*)nextGasStripAttr())) {
          if (!strcasecmp("X", attr->GetName())) {
            fGasStripX = atof(attr->GetValue());
          }
          if (!strcasecmp("Y", attr->GetName())) {
            fGasStripY = atof(attr->GetValue());
          }
          if (!strcasecmp("Z", attr->GetName())) {
            fGasStripZ = atof(attr->GetValue());
          }
        }
      }
      if(!strcasecmp(node->GetNodeName(), "distBetweenXandYStrips")) {
        fDistBetweenXandY = atof(node->GetText());
      }
      if(!strcasecmp(node->GetNodeName(), "aluminiumThickness")) {
        fCathodeThickness = atof(node->GetText());
      }
      if(!strcasecmp(node->GetNodeName(), "kaptonThickness")) {
        fKaptonWindowThickness = atof(node->GetText());
      }
      if(!strcasecmp(node->GetNodeName(), "wireDiameter")) {
        fAnodeWireDiameter = atof(node->GetText());
      }
      if(!strcasecmp(node->GetNodeName(), "kaptonMedia")) {
        fKaptonWindowMedia = node->GetText();
      }
      if(!strcasecmp(node->GetNodeName(), "aluminiumMedia")) {
        fCathodeMedia = node->GetText();
      }
      if(!strcasecmp(node->GetNodeName(), "tungstenMedia")) {
        fAnodeWireMedia = node->GetText();
      }
      if(!strcasecmp(node->GetNodeName(), "gasMedia")) {
        fGasMedia = node->GetText();
      }
    }
  }
}
//--------------------------------------------------------------------------------------------------
Double_t ERBeamDetSetup::PrintDetectorParameters(void) {
  cout << "------------------------------------------------" << endl;
  cout << "Detector's parameters from " << fParamsXmlFileName << endl;
  cout << "ToF " << fToFType1 << " parameters:" << endl  
       << "   X = " << fPlasticX
       << "; Y = " << fPlasticY 
       << "; Z = " << fPlasticZ << endl
       << "   media = " << fPlasticMedia << endl;
  cout << "MWPC " << fMWPCType1 << " parameters: " << endl
       << "   GasVolX = " << fGasVolX
       << "; GasVolY = " << fGasVolY 
       << "; GasVolZ = " << fGasVolZ << endl
       << "   GasStripX = " << fGasStripX
       << "; GasStripY = " << fGasStripY 
       << "; GasStripZ = " << fGasStripZ << endl
       << "   Distance between X & Y strips = " << fDistBetweenXandY << endl
       << "   Cathode thickness = " << fCathodeThickness << endl
       << "   KaptonWindow thickness = " << fKaptonWindowThickness << endl
       << "   Wire diameter = " << fAnodeWireDiameter << endl
       << "   KaptonWindow media = " << fKaptonWindowMedia << endl
       << "   Cathode media = " << fCathodeMedia << endl
       << "   AnodeWire media = " << fAnodeWireMedia << endl
       << "   gasStrip media = " << fGasMedia << endl;
}
//--------------------------------------------------------------------------------------------------
void ERBeamDetSetup::ParseXmlParameters() {
  TDOMParser *domParser;//
  //gROOT->ProcessLine(".O 0"); 
  domParser = new TDOMParser;
  //domParser->SetValidate(false); // do not validate with DTD
  Int_t parsecode = domParser->ParseFile(fParamsXmlFileName);
  if (parsecode < 0) {
     cerr << domParser->GetParseCodeMessage(parsecode) << endl;
 //    return -1;
  }
  TXMLNode *rootNode = domParser->GetXMLDocument()->GetRootNode();
  TXMLNode *detPartNode = rootNode->GetChildren();
  TXMLNode *curNode;

  for ( ; detPartNode; detPartNode = detPartNode->GetNextNode()) { // detector's part
    if(!strcasecmp(detPartNode->GetNodeName(), "ToFTypes")) {
     // cout << "Cmp ToF " << detPartNode->GetNodeName() << endl;
      GetToFParameters(detPartNode->GetChildren());
    }
    if(!strcasecmp(detPartNode->GetNodeName(), "MWPCTypes")) {
     // cout << "Cmp MWPC " << detPartNode->GetNodeName() << endl;
      GetMWPCParameters(detPartNode->GetChildren());
    }
  }
  //return 0;
}
//--------------------------------------------------------------------------------------------------
void ERBeamDetSetup::ConstructGeometry() {
  // ----- BeamDet parameters -------------------------------------------------
  Double_t transTargetX = 0.;
  Double_t transTargetY = 0.; 
  Double_t transTargetZ = 0.;
  // --------------------------------------------------------------------------
  // Create a global translation
  Float_t global_X = 0.;
  Float_t global_Y = 0.;
  Float_t global_Z = 0.;
  //Create gloabal Rotation
  TGeoRotation *fGlobalRotation = new TGeoRotation();
  fGlobalRotation->RotateX(0.);
  fGlobalRotation->RotateY(0.);
  fGlobalRotation->RotateZ(0.);
  // Create a zero rotation
  TGeoRotation *fZeroRotation = new TGeoRotation();
  fZeroRotation->RotateX(0.);
  fZeroRotation->RotateY(0.);
  fZeroRotation->RotateZ(0.);
  // Create a 90 degree rotation around Z axis
  TGeoRotation *f90ZRotation = new TGeoRotation();
  f90ZRotation->RotateX(0.);
  f90ZRotation->RotateY(0.);
  f90ZRotation->RotateZ(90.);
  // Create a 90 degree rotation around X axis
  TGeoRotation *f90XRotation = new TGeoRotation();
  f90XRotation->RotateX(90.);
  f90XRotation->RotateY(0.);
  f90XRotation->RotateZ(0.);

  TGeoManager*   gGeoMan = NULL;
  // -------   Load media from media file   -----------------------------------
  FairGeoLoader*    geoLoad = FairGeoLoader::Instance();//new FairGeoLoader("TGeo","FairGeoLoader");
  FairGeoInterface* geoFace = geoLoad->getGeoInterface();
  TString geoPath = gSystem->Getenv("VMCWORKDIR");
  TString medFile = geoPath + "/geometry/media.geo";
  geoFace->setMediaFile(medFile);
  geoFace->readMedia();
  gGeoMan = gGeoManager;
  // --------------------------------------------------------------------------
  // -------   Geometry file name (output)   ----------------------------------
  TString geoFileName = geoPath + "/geometry/beamdet.temp.root";
  // --------------------------------------------------------------------------
  // -----------------   Get and create the required media    -----------------
  FairGeoMedia*   geoMedia = geoFace->getMedia();
  FairGeoBuilder* geoBuild = geoLoad->getGeoBuilder();

  // ----- Create media for ToF -----------------------------------------------
  FairGeoMedium* mPlastic      = geoMedia->getMedium(fPlasticMedia);
  if ( ! mPlastic ) Fatal("Main", "FairMedium Plastic not found");
  geoBuild->createMedium(mPlastic);
  TGeoMedium* pMedPlastic = gGeoMan->GetMedium(fPlasticMedia);
  if ( ! pMedPlastic ) Fatal("Main", "Medium Plastic not found");
  // --------------------------------------------------------------------------
  // ----- Create media for MWPC ----------------------------------------------
  FairGeoMedium* mCF4      = geoMedia->getMedium(fGasMedia);
  if ( ! mCF4 ) Fatal("Main", "FairMedium for gasStrip not found");
  geoBuild->createMedium(mCF4);
  TGeoMedium* pMedCF4 = gGeoMan->GetMedium(fGasMedia);
  if ( ! pMedCF4 ) Fatal("Main", "Medium for gasStrip not found");

  FairGeoMedium* mKaptonWindow      = geoMedia->getMedium(fKaptonWindowMedia);
  if ( ! mKaptonWindow ) Fatal("Main", "FairMedium kapton not found");
  geoBuild->createMedium(mKaptonWindow);
  TGeoMedium* pMedKaptonWindow = gGeoMan->GetMedium(fKaptonWindowMedia);
  if ( ! pMedKaptonWindow ) Fatal("Main", "Medium kapton not found");

  FairGeoMedium* mCathode      = geoMedia->getMedium(fCathodeMedia);
  if ( ! mCathode ) Fatal("Main", "FairMedium aluminium not found");
  geoBuild->createMedium(mCathode);
  TGeoMedium* pMedCathode = gGeoMan->GetMedium(fCathodeMedia);
  if ( ! pMedCathode ) Fatal("Main", "Medium aluminium not found");

  FairGeoMedium* mAnodeWire      = geoMedia->getMedium(fAnodeWireMedia);
  if ( ! mAnodeWire ) Fatal("Main", "FairMedium tungsten not found");
  geoBuild->createMedium(mAnodeWire);
  TGeoMedium* pMedAnodeWire = gGeoMan->GetMedium(fAnodeWireMedia);
  if ( ! pMedAnodeWire ) Fatal("Main", "Medium tungsten not found");
  // --------------------------------------------------------------------------
  // ------ Create media for fTarget -------------------------------------------
  FairGeoMedium* mH2      = geoMedia->getMedium("H2");
  if ( ! mH2 ) Fatal("Main", "FairMedium H2 not found");
  geoBuild->createMedium(mH2);
  TGeoMedium* pH2 = gGeoMan->GetMedium("H2");
  if ( ! pH2 ) Fatal("Main", "Medium H2 not found"); 

  FairGeoMedium* mSteel      = geoMedia->getMedium("Steel");
  if ( ! mSteel ) Fatal("Main", "FairMedium Steel not found");
  geoBuild->createMedium(mSteel);
  TGeoMedium* pSteel = gGeoMan->GetMedium("Steel");
  if ( ! pSteel ) Fatal("Main", "Medium vacuum not found");
  // --------------------------------------------------------------------------
  // ------ Create vacuum media -----------------------------------------------
  FairGeoMedium* vacuum      = geoMedia->getMedium("vacuum");
  if ( ! vacuum ) Fatal("Main", "FairMedium vacuum not found");
  geoBuild->createMedium(vacuum);
  TGeoMedium* pMed0 = gGeoMan->GetMedium("vacuum");
  if ( ! pMed0 ) Fatal("Main", "Medium vacuum not found");
  // --------------------------------------------------------------------------
  //------------------------- VOLUMES -----------------------------------------

  // --------------   Create geometry and top volume  -------------------------
  gGeoMan = (TGeoManager*)gROOT->FindObject("FAIRGeom");
  gGeoMan->SetName("BeamDetGeom");
  TGeoVolume* top   = new TGeoVolumeAssembly("TOP");
  //gGeoMan->SetTopVolume(top);

  TGeoVolume* beamdet = new TGeoVolumeAssembly("beamdet");
  //TGeoVolume* MWPC    = new TGeoVolumeAssembly("MWPC");
  TGeoVolume* target  = new TGeoVolumeAssembly("target");

  // --------------------------------------------------------------------------
  // ---------------- fTarget --------------------------------------------------
  fTargetH2Z /= 2.;

  Double_t fTargetShellR = fTargetH2R + fTargetShellThickness;
  Double_t fTargetShellZ = fTargetH2Z + fTargetShellThickness;

  TGeoVolume *targetH2 = gGeoManager->MakeTube("targetH2", pH2, 0, fTargetH2R, fTargetH2Z);
  TGeoVolume *targetShell = gGeoManager->MakeTube("targetShell", pSteel, 0, fTargetShellR, fTargetShellZ);
  // --------------------------------------------------------------------------
  // ----------------- MWPC ---------------------------------------------------
  fGasVolX /= 2.;
  fGasVolY /= 2.;
  fGasVolZ /= 2.;
  TGeoVolume* gasVol = gGeoManager->MakeBox("MWPCVol", pMedCF4, fGasVolX, fGasVolY, fGasVolZ);
  TGeoVolume* MWPC = gGeoManager->MakeBox("MWPC", pMedKaptonWindow, fGasVolX, fGasVolY, fGasVolZ + fKaptonWindowThickness);

  fGasStripX /= 2.0;
  fGasStripY /= 2.0;
  fGasStripZ /= 2.0;
  TGeoVolume* gasStrip = gGeoManager->MakeBox("gasStrip", pMedCF4, fGasStripX, fGasStripY, fGasStripZ);
  TGeoVolume* gasPlane = gGeoManager->MakeBox("gasPlane", pMedCF4, fGasVolX, fGasVolY, fGasStripZ + fCathodeThickness);
  TGeoVolume* tungstenWire = gGeoManager->MakeTube("tungstenWire", pMedAnodeWire, 0, fAnodeWireDiameter / 2, fGasStripY);
  // --------------------------------------------------------------------------
  // ---------------- ToF -----------------------------------------------------
  fPlasticX /= 2.0;
  fPlasticY /= 2.0;
  fPlasticZ /= 2.0;
  TGeoVolume* plastic = gGeoManager->MakeBox("plastic", pMedPlastic, fPlasticX, fPlasticY, fPlasticZ);
  // --------------------------------------------------------------------------
  //------------------ STRUCTURE  ---------------------------------------------
 // gasStrip->AddNode(tungstenWire, 1, new TGeoCombiTrans(0, 0, 0, f90XRotation));

  Int_t gasCount = fGasVolX / (2 * fGasStripX);

  Double_t gasPosX;

  for(Int_t i_gas = 1; i_gas <= 2*gasCount; i_gas++)
  {
    gasPosX = fGasVolX - fGasStripX * 2 * (i_gas - 1) - fGasStripX;
    gasPlane->AddNode(gasStrip, i_gas, new TGeoCombiTrans(gasPosX, 0, 0, fZeroRotation));
  }

  gasVol->AddNode(gasPlane, 1, new TGeoCombiTrans(0, 0, -fDistBetweenXandY / 2, fZeroRotation));
  gasVol->AddNode(gasPlane, 2, new TGeoCombiTrans(0, 0, fDistBetweenXandY / 2, f90ZRotation));

  MWPC->AddNode(gasVol, 1, new TGeoCombiTrans(0, 0, 0, fZeroRotation));

  beamdet->AddNode(plastic, 1, new TGeoCombiTrans(global_X, global_Y, fPositionToF1, fGlobalRotation));
  beamdet->AddNode(plastic, 2, new TGeoCombiTrans(global_X, global_Y, fPositionToF2, fGlobalRotation));
  beamdet->AddNode(MWPC, 1, new TGeoCombiTrans(global_X, global_Y, fPositionMWPC1, fGlobalRotation));
  beamdet->AddNode(MWPC, 2, new TGeoCombiTrans(global_X, global_Y, fPositionMWPC2, fGlobalRotation));


  targetShell->AddNode(targetH2, 1, new TGeoCombiTrans(.0, .0, .0, fZeroRotation));
  target->AddNode(targetShell, 1, new TGeoCombiTrans(.0,.0,.0, fZeroRotation));

  beamdet->AddNode(target, 1, new TGeoCombiTrans(transTargetX, transTargetY, transTargetZ, fZeroRotation));
  top->AddNode(beamdet, 1, new TGeoCombiTrans(global_X ,global_Y, global_Z, fGlobalRotation));
   // ---------------   Finish   -----------------------------------------------
  //gGeoMan->CloseGeometry();
  //gGeoMan->CheckOverlaps(0.001);
  //gGeoMan->PrintOverlaps();
  //gGeoMan->Test();

  //gGeoManager = gGeoMan;

  TFile* geoFile = new TFile(geoFileName, "RECREATE");
  top->Write();
  geoFile->Close();
  // --------------------------------------------------------------------------
}
//--------------------------------------------------------------------------------------------------
ClassImp(ERBeamDetSetup)