void draw_scaler(const char* filename)
{
  TString RecName[4]={"Si#1","Si#2","Ge#1","Ge#2"};
  TString FwdName[4]={"Fwd#Out","Fwd#In","Fwd#Up","Fwd#Down"};

  TFile* f=new TFile(filename);
  TTree* tree=(TTree*)f->Get("EmsEvent");

  KoaEmsEvent* ems=nullptr;
  KoaRawEvent* koala=nullptr;
  // tree->SetBranchAddress("KoalaEvent",&koala);
  tree->SetBranchAddress("EmsEvent",&ems);

  TGraph  *gHitRateRec[4];
  TGraph  *gHitRateFwd[4];
  TGraph  *gHitRateCommonOr;
  TGraph  *gHitRateGeOverlap[2];
  TGraph  *gHitRateSiRear[2];
    for(int i=0;i<4;i++){
      gHitRateRec[i] = new TGraph();
      gHitRateRec[i]->SetNameTitle(Form("gHitRateRec_%d",i+1),Form("%s : Hit Rate",RecName[i].Data()));
      gHitRateRec[i]->SetLineWidth(2);
      gHitRateRec[i]->SetLineColor(1+i);
      gHitRateRec[i]->SetMarkerColor(1+i);
    }
    for(int i=0;i<4;i++){
      gHitRateFwd[i] = new TGraph();
      gHitRateFwd[i]->SetNameTitle(Form("gHitRateFwd_%d",i+1),Form("%s : Hit Rate",FwdName[i].Data()));
      gHitRateFwd[i]->SetLineWidth(2);
      gHitRateFwd[i]->SetLineColor(1+i);
      gHitRateFwd[i]->SetMarkerColor(1+i);
    }
    gHitRateCommonOr = new TGraph();
    gHitRateCommonOr->SetNameTitle("gHitRateCommonOr","Trigger Rate");
    gHitRateCommonOr->SetLineWidth(2);
    gHitRateCommonOr->SetLineColor(6);
    gHitRateCommonOr->SetMarkerColor(6);
    //
    for(int i=0;i<2;i++){
      gHitRateGeOverlap[i] = new TGraph();
      gHitRateGeOverlap[i]->SetNameTitle(Form("gHitRateGeOverlap_%d",i+1),Form("%s OverlapArea (5 strips): Hit Rate",RecName[i+2].Data()));
      gHitRateGeOverlap[i]->SetLineWidth(2);
      gHitRateGeOverlap[i]->SetLineColor(1+i);
      gHitRateGeOverlap[i]->SetMarkerColor(1+i);
    }
    for(int i=0;i<2;i++){
      gHitRateSiRear[i] = new TGraph();
      gHitRateSiRear[i]->SetNameTitle(Form("gHitRateSiRear_%d",i+1),Form("%s RearSide: Hit Rate",RecName[i].Data()));
      gHitRateSiRear[i]->SetLineWidth(2);
      gHitRateSiRear[i]->SetLineColor(1+i);
      gHitRateSiRear[i]->SetMarkerColor(1+i);
    }

    Long64_t rec[4],fwd[4];
    Long64_t commonor;
    Long64_t geoverlap[2],sirear[2];
    Long64_t second,usecond;

    Long64_t entries=tree->GetEntries();
    Int_t npoints;
    double diff_time;
    for(int index=0;index<entries;index++){
      tree->GetEntry(index);
      //
      if(index){
        diff_time=ems->fSecond-second;
        diff_time+=(ems->fUSecond-usecond)*1e-6;
        for(int i=0;i<4;i++){
          npoints=gHitRateRec[i]->GetN();
          gHitRateRec[i]->SetPoint(npoints,ems->fSecond,(ems->fScalerRec[i]-rec[i])/diff_time);

          npoints=gHitRateFwd[i]->GetN();
          gHitRateFwd[i]->SetPoint(npoints,ems->fSecond,(ems->fScalerFwd[i]-fwd[i])/diff_time);
        }
        for(int i=0;i<2;i++){
          npoints=gHitRateGeOverlap[i]->GetN();
          gHitRateGeOverlap[i]->SetPoint(npoints,ems->fSecond,(ems->fScalerGeOverlap[i]-geoverlap[i])/diff_time);

          npoints=gHitRateSiRear[i]->GetN();
          gHitRateSiRear[i]->SetPoint(npoints,ems->fSecond,(ems->fScalerSiRear[i]-sirear[i])/diff_time);
        }
        npoints=gHitRateCommonOr->GetN();
        gHitRateCommonOr->SetPoint(npoints,ems->fSecond,(ems->fScalerCommonOr-commonor)/diff_time);
      }

      for(int j=0;j<4;j++){
        rec[j]=ems->fScalerRec[j];
        fwd[j]=ems->fScalerFwd[j];
      }
      for(int j=0;j<2;j++){
        geoverlap[j]=ems->fScalerGeOverlap[j];
        sirear[j]=ems->fScalerSiRear[j];
      }
      commonor=ems->fScalerCommonOr;
      second=ems->fSecond;
      usecond=ems->fUSecond;
    }
    //

    TFile* fnew=new TFile("scalers.root","recreate");
    for(int i=0;i<4;i++){
      gHitRateRec[i]->Write();
      gHitRateFwd[i]->Write();
    }
    for(int i=0;i<2;i++){
      gHitRateGeOverlap[i]->Write();
      gHitRateSiRear[i]->Write();
    }
    gHitRateCommonOr->Write();
    delete fnew;

    delete f;


  
}
