// -------------------------------------------------------------------------
// -----                      ERQTelescopeSiPoint header file                -----
// -----                  Created data  developerName                  -----
// -------------------------------------------------------------------------


/**  ERQTelescopeSiPoint.h
 **/


#ifndef ERQTelescopeSiPoint_H
#define ERQTelescopeSiPoint_H


#include "TObject.h"
#include "TVector3.h"

#include "FairMCPoint.h"


class ERQTelescopeSiPoint : public FairMCPoint
{

 public:

  /** Default constructor **/
  ERQTelescopeSiPoint();


  /** Constructor with arguments
   *@param EventID  Index of Event
   *@param trackID  Index of MCTrack
   *@param mot0trackID Index of Mother MCTrack
   *@param posIn    Ccoordinates at entrance of point [cm]
   *@param posOut   Coordinates at exit of point [cm]
   *@param momIn    Momentum of track at entrance [GeV]
   *@param momOut   Momentum of track at exit [GeV]
   *@param tof      Time since event start [ns]
   *@param length   Track length since creation [cm]
   *@param eLoss    Energy deposit [KeV]
   **/
  ERQTelescopeSiPoint(Int_t eventID, Int_t trackID,
		  Int_t mot0trackID,
		  Double_t mass,
		  TVector3 posIn,
		  TVector3 posOut, TVector3 momIn, TVector3 momOut,
		  Double_t tof, Double_t length, Double_t eLoss, Int_t N_Station ,Int_t X_Strip, Int_t Y_Strip);


  /** Copy constructor **/
  ERQTelescopeSiPoint(const ERQTelescopeSiPoint&);


  /** Destructor **/
  virtual ~ERQTelescopeSiPoint();


  ERQTelescopeSiPoint& operator=(const ERQTelescopeSiPoint&) { return *this; }


  /** Accessors **/
  Int_t GetEventID()            const { return fEventID; }
  Int_t GetMot0TrackID()        const { return fMot0TrackID; }
  Double_t GetXIn()             const { return fX; }
  Double_t GetYIn()             const { return fY; }
  Double_t GetZIn()             const { return fZ; }
  Double_t GetXOut()            const { return fX_out; }
  Double_t GetYOut()            const { return fY_out; }
  Double_t GetZOut()            const { return fZ_out; }
  Double_t GetPxOut()           const { return fPx_out; }
  Double_t GetPyOut()           const { return fPy_out; }
  Double_t GetPzOut()           const { return fPz_out; }
  Double_t GetMass()            const { return fMass; }

  void PositionIn(TVector3& pos)  { pos.SetXYZ(fX, fY, fZ); }
  void PositionOut(TVector3& pos) { pos.SetXYZ(fX_out,fY_out,fZ_out); }
  void MomentumOut(TVector3& mom) { mom.SetXYZ(fPx_out,fPy_out,fPz_out); }


  /** Point coordinates at given z from linear extrapolation **/
  Double_t GetX(Double_t z) const;
  Double_t GetY(Double_t z) const;
  // Int_t Sector() const {return fSector;}
  // Int_t Sensor() const {return fSensor;}

  Int_t          N_Station() {return fN_Station;}     // Si
  Int_t          X_Strip()   {return fX_Strip;}
  Int_t          Y_Strip()   {return fY_Strip;}

  /** Check for distance between in and out **/
  Bool_t IsUsable() const;

  /** Output to screen **/
  virtual void Print(const Option_t* opt = 0) const;

 protected:

  Int_t fEventID;
  Int_t fMot0TrackID;
  Double_t fMass;
  Double32_t fX_out,  fY_out,  fZ_out;
  Double32_t fPx_out, fPy_out, fPz_out;
  Int_t          fN_Station;     // Si
  Int_t          fX_Strip;
  Int_t          fY_Strip;
  ClassDef(ERQTelescopeSiPoint,1)
};
#endif
