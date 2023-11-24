// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include <iostream>
#include "dyncar_bicycle.h"

//-/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//---/////////////////////////////////////////////////
#ifdef MEX_DEBUG_PRINTF

#include "mex.h"
#include <stdio.h>
class mystream : public std::streambuf
{
protected:
	virtual std::streamsize xsputn(const char *s, std::streamsize n) { mexPrintf("%.*s", n, s); return n; }
	virtual int overflow(int c = EOF) { if (c != EOF) { mexPrintf("%.1s", &c); } return 1; }
};
class scoped_redirect_cout
{
public:
	scoped_redirect_cout() { old_buf = std::cout.rdbuf(); std::cout.rdbuf(&mout); }
	~scoped_redirect_cout() { std::cout.rdbuf(old_buf); }
private:
	mystream mout;
	std::streambuf *old_buf;
};
scoped_redirect_cout mycout_redirect;
#endif // MEX_DEBUG_PRINTF

//-////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-//////////////////////////////////////////

// DYNAMIC

// CAR

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DynamicCar::~DynamicCar() {}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DynamicCar::DynamicCar(Eigen::Vector3d init_navig) {

fl_first_call = 1;
fl_first_it = 1;
  //==========================================

  // Parameters of culculations
  culc_par.fl_drag_taking = 1;
  culc_par.fl_Long_Circ = 3;
  culc_par.fl_static_load = 0;
  culc_par.fl_RunKut = 0;
  culc_par.nsteps = 1000;
  culc_par.fl_act_low = 2;
  culc_par.implicit_solver = 0;
  culc_par.num_impl_nonlin_it = 4;
  culc_par.corr_impl_step = 0;
  //----------------------------------------
  // default kinematic and dynamic parameters
  car_par.a = 1.73;
  car_par.b = 1.168;  // 1.35;
  car_par.L = car_par.a + car_par.b;
  car_par.max_steer = 22.0 * M_PI / 180.0;  // 52.0*M_PI/180.0;

  //-----------------------------
  car_par.mu = 1.0;
  car_par.max_accel = 8.0;  // 50.0
  car_par.CGvert = 0.4;
  car_par.mass = 1196.0;
  car_par.moment_inertia = 1260.0;

  //------------------------------------
  car_par.Cy_f = 226000.0;
  car_par.Cy_r = 240000.0;
  car_par.Cx_f = 376000.0;
  car_par.Cx_r = 284000.0;

  //------------------------------------
  car_par.drive_distrib = 0.0;  // 0.5 = AWD, 1.0 = FWD, 0.0 = RWD
  car_par.frontWheelRad = 0.31;
  car_par.rearWheelRad = 0.35;
  car_par.maxPowerEngine = 135000.0;
  car_par.maxTorqueEngine = 200.0 * 6.25;
  car_par.n_engines = 2.0;
  car_par.Om_maxTorque = 192.68;
  car_par.Om_nullTorque = 201.06;
  //
  car_par.maxBrakePressure = 60.0;
  car_par.maxGradBrakePressure = 250.0;
  car_par.kBrake_f = 70.0;
  car_par.kBrake_r = 70.0;
  car_par.n_brakes = 1.0;
  car_par.maxGradSteer = 8.22;

  //------------------------------------
  car_par.rollingResistance = 600.0;
  car_par.rollingResistanceSpeedDepended = 0.0;
  car_par.Cxx = 1.0;

  //-----------------------------------------------

  car_par.fWIn = 1.42;
  car_par.rWIn = 2.06;


  //-----------------------------------------------
  // initial state ------------------------------//

  fl_reverse = 0;

  car_state.state = 0;

  car_state.xc = init_navig(0);
  car_state.yc = init_navig(1);
  car_state.psi = init_navig(2);

  nav_to_cog(0);

  car_state.Vx = 0.0;
  car_state.Vy = 0.0;
  car_state.Om = 0.0;

  car_state.accel = 0.0;
  car_state.Vdes = 0.0;

  car_state.steer_alpha = 0.0;
  car_state.steer_alpha_req = 0.0;

  car_state.dVxdt = 0.0;
  car_state.dVydt =  0.0;
  car_state.dOmdt =  0.0;

  car_state.slipAng_f = 0.0;
  car_state.slipAng_r = 0.0;


  car_state.Nr = 9.89*car_par.mass*car_par.a/car_par.L;
  car_state.Nf = 9.89*car_par.mass*car_par.b/car_par.L;

  car_state.FYf = 0.0;
  car_state.FYr = 0.0;

  car_state.FXf = 0.0;
  car_state.FXr = 0.0;

  car_state.FXf_req = 0.0;
  car_state.FXr_req = 0.0;

  car_state.B_pressure = 0.0;
  car_state.B_pressure_req = 0.0;

  car_state.Nbrake_f = 0.0;
  car_state.Nbrake_r = 0.0;
  car_state.Nmot_f = 0.0;
  car_state.Nmot_r = 0.0;

  car_state.addedFx = 0.0;
  car_state.addedFy = 0.0;
  car_state.addedN = 0.0;

  car_state.OmWf = 0.0;
  car_state.OmWr = 0.0;

  car_state.evTrqW_f = 0.0;
  car_state.evTrqW_r = 0.0;

  car_state.flDW_f = 1;
  car_state.flDW_r = 1;

  car_state.long_slip_f = 0.0;
  car_state.long_slip_r = 0.0;

  car_state.initialized = 0;
  car_state.callcount = 0;
  //===================================================

  //------------------------------------

  fl_out = 0;

  fl_fileDyn = 0;

  in_time = 0.0;
  d_time = 0.0;

  fl_MPClog = 0;

  timeMPClog = 0.0;
}

//--------------------------------------------------------------------------------------

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DynamicCar::DynamicCar(DynamicPar& params, double x, double y, double psi, double v) {


  fl_first_call = 1;
  //==========================================

  culc_par = params.culc_par;
  car_par = params.car_par;

  //-----------------------------------------------
  // initial state ------------------------------//

  fl_reverse = 0;

  car_state.state = 0;

  //car_state.xc = init_navig(0);
  //car_state.yc = init_navig(1);
  //car_state.psi = init_navig(2);
  //nav_to_cog(0);

  car_state.Vx = 0.0;
  car_state.Vy = 0.0;
  car_state.Om = 0.0;

  car_state.accel = 0.0;
  car_state.Vdes = 0.0;

  car_state.steer_alpha = 0.0;
  car_state.steer_alpha_req = 0.0;

  car_state.dVxdt = 0.0;
  car_state.dVydt =  0.0;
  car_state.dOmdt =  0.0;

  car_state.Nr = 9.89*car_par.mass*car_par.a/car_par.L;
  car_state.Nf = 9.89*car_par.mass*car_par.b/car_par.L;

  car_state.slipAng_f = 0.0;
  car_state.slipAng_r = 0.0;

  car_state.FYf = 0.0;
  car_state.FYr = 0.0;

  car_state.FXf = 0.0;
  car_state.FXr = 0.0;

  car_state.FXf_req = 0.0;
  car_state.FXr_req = 0.0;

  car_state.B_pressure = 0.0;
  car_state.B_pressure_req = 0.0;

  car_state.Nbrake_f = 0.0;
  car_state.Nbrake_r = 0.0;
  car_state.Nmot_f = 0.0;
  car_state.Nmot_r = 0.0;

  car_state.addedFx = 0.0;
  car_state.addedFy = 0.0;
  car_state.addedN = 0.0;

  car_state.OmWf = 0.0;
  car_state.OmWr = 0.0;

  car_state.evTrqW_f = 0.0;
  car_state.evTrqW_r = 0.0;

  car_state.flDW_f = 1;
  car_state.flDW_r = 1;

  car_state.long_slip_f = 0.0;
  car_state.long_slip_r = 0.0;

  car_state.initialized = 1;
  car_state.callcount = 0;
  //===================================================
  init_nav(x, y, psi, v);
  //------------------------------------

  fl_out = 0;

  fl_fileDyn = 0;

  in_time = 0.0;
  d_time = 0.0;

  fl_MPClog = 0;

  timeMPClog = 0.0;
}

//--------------------------------------------------------------------------------------

//-//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//-//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//+++++++++++++++++
void DynamicCar::newFXreq() {
  if (fabs(car_state.accel) < 0.00001) car_state.accel = 0.0;

  if (fabs(car_state.accel) > car_par.max_accel)
    car_state.accel =
        car_par.max_accel * (car_state.accel / fabs(car_state.accel));

  car_state.FXreq = car_par.mass * car_state.accel;

  // FXreq being force which one requests from car

  if ((fl_AccBrkMode > 0) || (culc_par.fl_act_low < 2)) {
    if (car_par.drive_distrib < 0.0) car_par.drive_distrib = 0.0;

    if (fabs(car_par.drive_distrib) > 1.0) car_par.drive_distrib = 1.0;

    if (car_par.drive_distrib < 0.01) {
      car_state.FXr_act = car_state.FXreq;
      car_state.FXf_act = 0.0;

    } else if (car_par.drive_distrib > 0.99) {
      car_state.FXf_act = car_state.FXreq;
      car_state.FXr_act = 0.0;
    } else {
      car_state.FXf_act = car_par.drive_distrib * car_state.FXreq;
      car_state.FXr_act = (1.0 - car_par.drive_distrib) * car_state.FXreq;
    }

    car_state.B_pressure_req = 0.0;
  } else {
    car_state.FXf_act = 0.0;
    car_state.FXr_act = 0.0;

    car_state.B_pressure_req = (fabs(car_state.FXreq) / car_par.n_brakes) /
                               ((car_par.kBrake_f / car_par.frontWheelRad) +
                                (car_par.kBrake_r / car_par.rearWheelRad));

    if (car_state.B_pressure_req > car_par.maxBrakePressure)
      car_state.B_pressure_req = car_par.maxBrakePressure;
  }
}
//-----------------------------------------------------------------------------------------
//-//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//+++++++++++++++++

void DynamicCar::getDrag() {
  car_state.Fdrag = 0.0;

  if (culc_par.fl_drag_taking > 0) {
    car_state.Fdrag =
                car_par.Cxx * car_state.Vx * car_state.Vx;

    if (car_state.Vx < 0.0) car_state.Fdrag *= -1.0;
  }
}

//-----------------------------------------------------------------------------------------

//-//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//+++++++++++++++++

void DynamicCar::getDrag_PM() {
  car_state.Fdrag_PM = 0.0;

  return;

  // turn resistance   //-----------------------------------------

  double acc_turn =
      (car_par.b / car_par.L) * car_state.Vdes * (car_state.Vdes / car_par.L) *
      tan(car_state.steer_alpha_req) * sin(car_state.steer_alpha_req);

  if (fl_reverse > 0) acc_turn *= -1.0;

  car_state.accel += acc_turn;

  if (culc_par.fl_drag_taking > 0) {
    car_state.Fdrag_PM =
        car_par.rollingResistance +
        car_par.rollingResistanceSpeedDepended * fabs(car_state.Vdes) +
        car_par.Cxx * car_state.Vdes * car_state.Vdes;

    if (fl_reverse > 0) car_state.Fdrag_PM *= -1.0;

    car_state.accel += (car_state.Fdrag_PM / car_par.mass);

    // one has to take into account different behaviours of Fdrag in the cases
    // of acceleration and braking. !! Fdrag_PM is usually culculated by PM.
    // Here is located such evaluation. After Fdrag_PM having evaluated the
    // car_state.accel should be corrected
  }
}
//-----------------------------------------------------------------------------------------

//-//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//+++++++++++++++++
void DynamicCar::getVerticalLoads() {

    double Trq_tyres = (car_state.FXr + car_state.addedFx +
                        car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha))*car_par.CGvert;
    double weight = 9.89*car_par.mass;

    if(culc_par.fl_static_load > 0)
        Trq_tyres = 0.0;
    else if (culc_par.fl_static_load < 0)
        Trq_tyres = car_state.dVxdt*car_par.mass*car_par.CGvert;

    car_state.Nr = (weight*car_par.a + Trq_tyres)/car_par.L;

    if(car_state.Nr <= 0.0)
        car_state.Nr = 0.001;

    car_state.Nf = (weight*car_par.b - Trq_tyres)/car_par.L;

    if(car_state.Nf <= 0.0)
        car_state.Nf = 0.001;
}

//-----------------------------------------------------------------------------------------

//-//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//+++++++++++++++++
void DynamicCar::evolutionControl(double dt) {
    if (fabs(car_state.steer_alpha_req) >= car_par.max_steer)
      car_state.steer_alpha_req =
          (car_par.max_steer - 0.00001) *
          (car_state.steer_alpha_req / fabs(car_state.steer_alpha_req));

    car_state.FXf_req = car_state.FXf_act;

    car_state.FXr_req = car_state.FXr_act;

    double Fbrake_f = 0.0;

    double Fbrake_r = 0.0;


    if (culc_par.fl_act_low < 1) {  // For simplifying

      car_state.steer_alpha = car_state.steer_alpha_req;

    } else {
      double steer_change = car_par.maxGradSteer * dt;

      if((culc_par.fl_RunKut > 0)&&(car_state.state == 0))
          steer_change = 0.0;

      if (fabs(car_state.steer_alpha_req - car_state.steer_alpha) <= steer_change)
        car_state.steer_alpha = car_state.steer_alpha_req;
      else {
        double dir_ch = 1.0;

        if (car_state.steer_alpha > car_state.steer_alpha_req) dir_ch = -1.0;

        car_state.steer_alpha += steer_change * dir_ch;
      }

      if (culc_par.fl_act_low > 1) {
        double press_change = car_par.maxGradBrakePressure * dt;

        if (fabs(car_state.B_pressure_req - car_state.B_pressure) <= press_change)
          car_state.B_pressure = car_state.B_pressure_req;
        else {
          double dir_ch = 1.0;

          if (car_state.B_pressure > car_state.B_pressure_req) dir_ch = -1.0;

          car_state.B_pressure += press_change * dir_ch;
        }

        Fbrake_f = car_par.n_brakes * car_state.B_pressure *
                          car_par.kBrake_f / car_par.frontWheelRad;
        Fbrake_r = car_par.n_brakes * car_state.B_pressure *
                          car_par.kBrake_r / car_par.rearWheelRad;



   //     if (fl_reverse < 1) {
   //       Fbrake_f *= -1.0;
  //        Fbrake_r *= -1.0;
  //      }

        if (fl_AccBrkMode > 0) {
          // rear engine

          double Fmax =
              car_par.n_engines * car_par.maxTorqueEngine / car_par.rearWheelRad;

          if (fabs(car_state.FXr_act) > Fmax)
            car_state.FXr_req =
                car_state.FXr_act * Fmax / fabs(car_state.FXr_act);

          double VWheel = fabs(car_state.Vx);

          double Power_req = fabs(car_state.FXr_req) * VWheel;

          if (Power_req > car_par.n_engines * car_par.maxPowerEngine)
            car_state.FXr_req =
                car_state.FXr_req *
                (car_par.n_engines * car_par.maxPowerEngine / Power_req);

          // engine curve
          double OmWheel = VWheel / car_par.rearWheelRad;

          if (OmWheel >= car_par.Om_nullTorque) {
            //          std::cout << "-> FXr_req:" << car_state.FXr_req //
            //                    << "-> OmWheel:" << OmWheel //
            //                    << "-> Om_nullTorque:" << car_par.Om_nullTorque
            //                    //
            //                    << std::endl;
            car_state.FXr_req = 0.0;
        //    std::cout << "-> FXr_req:" << car_state.FXr_req << std::endl;
          } else if (OmWheel > car_par.Om_maxTorque) {
            double F1 = car_par.n_engines * car_par.maxPowerEngine /
                        (car_par.Om_maxTorque * car_par.rearWheelRad);

            Fmax = F1 * (car_par.Om_nullTorque - OmWheel) /
                   (car_par.Om_nullTorque - car_par.Om_maxTorque);

            if (fabs(car_state.FXr_req) > Fmax) {
              if (car_state.FXr_req < 0.0)
                car_state.FXr_req = (-1.0) * Fmax;
              else
                car_state.FXr_req = Fmax;
            }
          }

          // front engine

          Fmax =
              car_par.n_engines * car_par.maxTorqueEngine / car_par.frontWheelRad;

          if (fabs(car_state.FXf_act) > Fmax)
            car_state.FXf_req =
                car_state.FXf_act * Fmax / fabs(car_state.FXf_act);

          VWheel = fabs(car_state.Vx / cos(car_state.steer_alpha));

          Power_req = fabs(car_state.FXf_req) * VWheel;

          if (Power_req > car_par.n_engines * car_par.maxPowerEngine)
            car_state.FXf_req =
                car_state.FXf_req *
                (car_par.n_engines * car_par.maxPowerEngine / Power_req);

          // engine curve
          OmWheel = VWheel / car_par.frontWheelRad;

          if (OmWheel >= car_par.Om_nullTorque)
            car_state.FXf_req = 0.0;
          else if (OmWheel > car_par.Om_maxTorque) {
            double F1 = car_par.n_engines * car_par.maxPowerEngine /
                        (car_par.Om_maxTorque * car_par.frontWheelRad);

            Fmax = F1 * (car_par.Om_nullTorque - OmWheel) /
                   (car_par.Om_nullTorque - car_par.Om_maxTorque);

            if (fabs(car_state.FXf_req) > Fmax) {
              if (car_state.FXf_req < 0.0)
                car_state.FXf_req = (-1.0) * Fmax;
              else
                car_state.FXf_req = Fmax;
            }
          }
        }

    //    car_state.FXf_req += Fbrake_f;
    //    car_state.FXr_req += Fbrake_r;
      }
    }


    car_state.Nbrake_f = Fbrake_f*car_par.frontWheelRad;
    car_state.Nbrake_r = Fbrake_r*car_par.rearWheelRad;

    car_state.Nmot_f = car_state.FXf_req*car_par.frontWheelRad;
    car_state.Nmot_r = car_state.FXr_req*car_par.rearWheelRad;



    double drFrc_f = car_par.rollingResistance*car_par.drive_distrib;
    double drFrc_r = car_par.rollingResistance*(1.0 - car_par.drive_distrib);

    if(culc_par.fl_drag_taking == 0){

        drFrc_f = 0.0;
        drFrc_r = 0.0;


    }


    Fbrake_f += drFrc_f;
    Fbrake_r += drFrc_r;

    if(car_state.state == 0){


       if(fabs(car_state.FXf_req + car_state.FXr_req) - Fbrake_f - Fbrake_r > 1.0){


          if(fl_reverse < 1){

              Fbrake_f *= -1.0;
              Fbrake_r *= -1.0;
          }

          car_state.FXf_req += Fbrake_f;
          car_state.FXr_req += Fbrake_r;

          car_state.state = 1;
          fl_AccBrk_fact = 1.0;

       }
       else
         fl_AccBrk_fact = -1.0;


    }
    else{

            if(fabs(car_state.FXf_req + car_state.FXr_req) - Fbrake_f - Fbrake_r > 0.0)
              fl_AccBrk_fact = 1.0;
            else
              fl_AccBrk_fact = -1.0;



            if(culc_par.fl_Long_Circ < 2){

               car_state.OmWr = car_state.Vx/car_par.rearWheelRad;

               double Vxf = car_state.Vx;
               double Vyf = car_state.Vy + car_state.Om * car_par.a;

               double cs = cos(car_state.steer_alpha);
               double sn = sin(car_state.steer_alpha);

               double Vxw = Vxf * cs + Vyf * sn;
               double Vyw = Vyf * cs - Vxf * sn;

               car_state.OmWf = Vxw/car_par.frontWheelRad;

            }

            double fct_br_f = 1.0;

            double fct_br_r = 1.0;

            if(fl_reverse == 0){


                fct_br_f = -1.0;
                fct_br_r = -1.0;

                if(car_state.OmWf < 0.0)
                 fct_br_f = 1.0;


                if(car_state.OmWr < 0.0)
                 fct_br_r = 1.0;

            }
            else{

                if(car_state.OmWf > 0.0)
                 fct_br_f = -1.0;


                if(car_state.OmWr > 0.0)
                 fct_br_r = -1.0;

            }

            Fbrake_f *= fct_br_f;
            Fbrake_r *= fct_br_r;

            car_state.FXf_req += Fbrake_f;
            car_state.FXr_req += Fbrake_r;



    }



}

//-----------------------------------------------------------------------------------------

//-//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//+++++++++++++++++

void DynamicCar::newLongForce() {
  // verical force distribution:

  getVerticalLoads();

  // tyre force distribution:

  // REAR:

  double FXBound_r = car_par.mu * car_state.Nr;

  if (culc_par.fl_Long_Circ < 0) {
    if (fabs(car_state.FYr) >= FXBound_r)
      FXBound_r = 0.0;
    else
      FXBound_r = sqrt(FXBound_r * FXBound_r - car_state.FYr * car_state.FYr);
  }

  if (fabs(car_state.FXr_req) < 0.001)
    car_state.FXr = 0.0;
  else if(culc_par.fl_Long_Circ <= 0){

    if (fabs(car_state.FXr_req) > FXBound_r) {
      car_state.FXr = FXBound_r;

      if (car_state.FXr_req < 0.0) car_state.FXr = (-1.0) * car_state.FXr;

    } else
      car_state.FXr = car_state.FXr_req;
  }
  else car_state.FXr = car_state.FXr_req;

  // FORWARD:

  double FXBound_f = car_par.mu * car_state.Nf;

  if (culc_par.fl_Long_Circ < 0) {
    if (fabs(car_state.FYf) >= FXBound_f)
      FXBound_f = 0.0;
    else
      FXBound_f = sqrt(FXBound_f * FXBound_f - car_state.FYf * car_state.FYf);
  }

  if (fabs(car_state.FXf_req) < 0.001)
    car_state.FXf = 0.0;
  else if(culc_par.fl_Long_Circ <= 0){
    if (fabs(car_state.FXf_req) > FXBound_f) {
      car_state.FXf = FXBound_f;

      if (car_state.FXf_req < 0.0) car_state.FXf = (-1.0) * car_state.FXf;
    } else
      car_state.FXf = car_state.FXf_req;
  }
  else car_state.FXf = car_state.FXf_req;

}
//--------------------------------------------------------------------------------------------------------------------

//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::newLatForce() {
  getLatSlip();
  getVerticalLoads();

  // FORWARD

  car_state.FYf_req =
      brushModel(car_state.slipAng_f, car_state.Nf, car_par.Cy_f);

  if (culc_par.fl_Long_Circ < 0) {
    double FBound = car_par.mu * car_state.Nf;

    if (fabs(car_state.FXf) >= FBound)
      FBound = 0.0;
    else
      FBound = sqrt(FBound * FBound - car_state.FXf * car_state.FXf);

    if (fabs(car_state.FYf_req) > FBound) {
      car_state.FYf = FBound;

      if (car_state.FYf_req < 0.0) car_state.FYf = (-1.0) * FBound;
    } else
      car_state.FYf = car_state.FYf_req;
  } else
    car_state.FYf = car_state.FYf_req;

  // REAR

  car_state.FYr_req =
      brushModel(car_state.slipAng_r, car_state.Nr, car_par.Cy_r);

  if (culc_par.fl_Long_Circ < 0) {
    double FBound = car_par.mu * car_state.Nr;

    if (fabs(car_state.FXr) >= FBound)
      FBound = 0.0;
    else
      FBound = sqrt(FBound * FBound - car_state.FXr * car_state.FXr);

    if (fabs(car_state.FYr_req) > FBound) {
      car_state.FYr = FBound;

      if (car_state.FYr_req < 0.0) car_state.FYr = (-1.0) * FBound;
    } else
      car_state.FYr = car_state.FYr_req;
  } else
    car_state.FYr = car_state.FYr_req;

}

//--------------------------------------------------------------------------------------------------------------------

//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::getLatSlip() {
  // Rear:

  double Vxr = car_state.Vx;
  double Vyr = car_state.Vy - car_state.Om * car_par.b;

  if (Vxr < 0.0) Vxr = (-1.0) * Vxr;

  car_state.slipAng_r = atan2(Vyr, Vxr);

 // if (Vxr < 0.001) car_state.slipAng_r = 0.0;

  double Vxf = car_state.Vx;
  double Vyf = car_state.Vy + car_state.Om * car_par.a;

  double cs = cos(car_state.steer_alpha);
  double sn = sin(car_state.steer_alpha);

  double Vxw = Vxf * cs + Vyf * sn;
  double Vyw = Vyf * cs - Vxf * sn;

  if (Vxw < 0.0) Vxw = (-1.0) * Vxw;

  car_state.slipAng_f = atan2(Vyw, Vxw);

 // if (Vxw < 0.001) car_state.slipAng_f = 0.0;
}

//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
double DynamicCar::brushModel_solver(double FY, int fl_forwardWheel,
                                     int& fl_OK) {
  // TO DO: It should be changed to lookup-table;

  double Fslip = fabs(FY);

  double Ctyre = car_par.Cy_f;

  double Fz = car_par.mass * 9.89 / car_par.L;

  if (fl_forwardWheel > 0) {
    Fz = Fz * car_par.b;

  } else {
    Ctyre = car_par.Cy_r;
    Fz = Fz * car_par.a;
  }

  fl_OK = 1.0;

  double alpha_slide = car_par.mu * ((3.0 * Fz) / Ctyre);

  double slip = 0.0;

  if (Fslip >= car_par.mu * Fz) {
    slip = alpha_slide;

    if (FY > 0) slip = slip * (-1.0);

    return slip;
  }

  if (Fslip <= Ctyre * 0.0001)
    slip = Fslip / Ctyre;
  else {
    slip = alpha_slide * 0.5;

    int iter = 0;

    do {
      iter++;

      fl_OK = iter;

      double F = -brushModel(slip, Fz, Ctyre) - Fslip;

      if (fabs(F) < 1.0) break;

      if (iter > 8) {
        fl_OK = 0;
        break;
      }

      if (true)
        if (slip > 0.98 * alpha_slide) {
          if (F < 0.0) {
            double F1 =
                -brushModel(slip + 0.5 * (alpha_slide - slip), Fz, Ctyre) -
                Fslip;

            if (fabs(F1) < fabs(F)) slip += 0.5 * (alpha_slide - slip);

            break;

          } else {
            slip = 0.98 * alpha_slide;
          }
        }

      double slip_rel = slip / alpha_slide;

      double dF = Ctyre * (1.0 - 2.0 * slip_rel + slip_rel * slip_rel);

      slip += -F / dF;

      if (slip < 0.0) slip = 0.0;

      if (slip >= alpha_slide) slip = 0.99 * alpha_slide;

    } while (true);
  }

  if (FY > 0) slip = slip * (-1.0);

  return slip;
}
//-------------------------------------------------------------------------------------------------------------------

//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
double DynamicCar::brushModel(double slip_angle, double Fz, double Ctyre) {
  double Fslip = 0.0;

  double alpha_slide = car_par.mu * ((3.0 * Fz) / Ctyre);

  if (fabs(slip_angle) >= alpha_slide) {
    Fslip = car_par.mu * Fz;

    if (slip_angle > 0.0) Fslip = Fslip * (-1.0);

  } else {
    double tal = tan(slip_angle);
    double slip_rel = fabs(tal) / alpha_slide;

    Fslip = -Ctyre * tal * (1.0 - slip_rel + (1.0 / 3.0) * slip_rel * slip_rel);
  }

  return Fslip;
}
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
double DynamicCar::brushModel_combined(double slip_angle, double Fz) {
  double Fslip = 0.0;

  double alpha_slide = car_par.mu * (3.0 * Fz);

  if (fabs(slip_angle) >= alpha_slide) {
    Fslip = car_par.mu * Fz;

  } else {
    double slip_rel = fabs(slip_angle) / alpha_slide;

    Fslip = slip_angle * (1.0 - slip_rel + (1.0 / 3.0) * slip_rel * slip_rel);
  }

  return Fslip;
}
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
double DynamicCar::brushModel_combined_diff_N(double slip_angle, double Fz) {
  double dFslip = 0.0;

  double alpha_slide = car_par.mu * (3.0 * Fz);

  if (fabs(slip_angle) < alpha_slide) {

    double slip_rel = fabs(slip_angle) / alpha_slide;
    double slip_rel2 = slip_rel*slip_rel;
    double slip_rel3 = slip_rel2*slip_rel;

    dFslip = 3.0*car_par.mu* (slip_rel2 - (2.0 / 3.0) * slip_rel3);
  }
  else
    dFslip = car_par.mu;

  return dFslip;
}
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
double DynamicCar::brushModel_combined_diff(double slip_angle, double Fz) {
  double dFslip = 0.0;

  double alpha_slide = car_par.mu * (3.0 * Fz);


  if (fabs(slip_angle) < alpha_slide)
  {
    double slip_rel = fabs(slip_angle) / alpha_slide;

    dFslip = (1.0 - 2.0*slip_rel + slip_rel * slip_rel);
  }

  return dFslip;
}
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::combinedSlip() {
  if (culc_par.fl_Long_Circ < 2) {
    newLatForce();   //
    newLongForce();  // Here Longitude and Lateral tyre forces being evaluated;
    // these functions use (or don't use) cyrcle of tyre force (rough model).

    if(culc_par.fl_Long_Circ == 1){

        double f_r = sqrt(car_state.FXr*car_state.FXr + car_state.FYr*car_state.FYr);

        double fmax_r = car_state.Nr*car_par.mu;

        if(f_r > fmax_r){

            double df = fmax_r/f_r;

            car_state.FXr *= df;
            car_state.FYr *= df;
        }

        double f_f = sqrt(car_state.FXf*car_state.FXf + car_state.FYf*car_state.FYf);
        double fmax_f = car_state.Nf*car_par.mu;

        if(f_f > fmax_f){

             double df = fmax_f/f_f;

             car_state.FXf *= df;
             car_state.FYf *= df;
        }
    }

  } else {
    getVerticalLoads();

    double Vxr = car_state.Vx;
    double WVr = car_state.OmWr * car_par.rearWheelRad;
    double Vyr = car_state.Vy - car_state.Om * car_par.b;

    if (culc_par.fl_Long_Circ < 3)
      if ((car_par.drive_distrib > 0.99) && (fl_AccBrkMode > 0)) {
        WVr = Vxr;
        car_state.OmWr = Vxr / car_par.rearWheelRad;
      }

    double DWr = WVr - Vxr;

    double normK_r = fabs(WVr) + 0.0001;

    car_state.long_slip_r = DWr / normK_r;

    if (Vxr < 0.0) Vxr = (-1.0) * Vxr;

    car_state.slipAng_r = atan2(Vyr, Vxr);

    double sig_xr = car_par.Cx_r * car_state.long_slip_r;
    double sig_yr = -car_par.Cy_r * Vyr / normK_r;

    double gama_r = sqrt(sig_xr * sig_xr + sig_yr * sig_yr);

    double Fslip_r = brushModel_combined(gama_r, car_state.Nr);

    car_state.FXr = (sig_xr / (gama_r + 0.0001)) * Fslip_r;
    car_state.FYr = (sig_yr / (gama_r + 0.0001)) * Fslip_r;

    double Vxf = car_state.Vx;
    double Vyf = car_state.Vy + car_state.Om * car_par.a;

    double cs = cos(car_state.steer_alpha);
    double sn = sin(car_state.steer_alpha);

    double Vxw = Vxf * cs + Vyf * sn;
    double Vyw = Vyf * cs - Vxf * sn;

    double WVf = car_state.OmWf * car_par.frontWheelRad;

    if (culc_par.fl_Long_Circ < 3)
      if ((car_par.drive_distrib < 0.01) && (fl_AccBrkMode > 0)) {
        WVf = Vxw;
        car_state.OmWf = Vxw / car_par.frontWheelRad;
      }

    double DWf = WVf - Vxw;

    double normK_f = fabs(WVf) + 0.0001;

    car_state.long_slip_f = DWf / normK_f;

    if (Vxw < 0.0) Vxw = (-1.0) * Vxw;

    car_state.slipAng_f = atan2(Vyw, Vxw);

    double sig_xf = car_par.Cx_f * car_state.long_slip_f;
    double sig_yf = -car_par.Cy_f * Vyw / normK_f;

    double gama_f = sqrt(sig_xf * sig_xf + sig_yf * sig_yf);

    double Fslip_f = brushModel_combined(gama_f, car_state.Nf);

    car_state.FXf = (sig_xf / (gama_f + 0.0001)) * Fslip_f;
    car_state.FYf = (sig_yf / (gama_f + 0.0001)) * Fslip_f;
  }


}

//---------------------------------------------------------------------

//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////

void DynamicCar::getAccBrkMode() {
  fl_AccBrkMode = 1.0;  // acceleration mode; -1.0 - braking mode
  fl_AccBrk_fact = 1.0;

  if (fabs(car_state.accel) < 0.00001) return;

  double fl = 1000.0;

  if (fl_reverse > 0) fl = -1000.0;

  if (fl * car_state.accel < 0.0) fl_AccBrkMode = -1.0;

  fl_AccBrk_fact = fl_AccBrkMode;
}
//---------------------------------------------------------------------
//-////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-/////////////////////////////////////
void DynamicCar::addToMPClog(double dtk) {

  //combinedSlip();

#if 0
     fileMPClog << timeMPClog
                << "     "
                << car_state.steer_alpha << "  "
                << car_state.FXf_req << "  " << car_state.FXr_req << "  "
                << car_state.Vx << "  " << car_state.Vy << "  " <<
                car_state.Om << "  " << car_state.x << "  " << car_state.y <<
                "  "<< car_state.psi << "  "
                << car_state.OmWf << "  " << car_state.OmWr << "  " <<
                car_state.dVxdt << "  " << car_state.dVydt << "  " <<
                car_state.dOmdt << "  "
                << car_state.Vx*cos(car_state.psi) -
                car_state.Vy*sin(car_state.psi) << "  "
                << car_state.Vx*sin(car_state.psi) +
                car_state.Vy*cos(car_state.psi) << "  "
                << car_state.Om << "  "
                << car_state.dOmWfdt << "  " << car_state.dOmWrdt << "  "
                << std::endl;
#endif

  timeMPClog += dtk;
}
//---------------------------------------------------------------------
//-////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-/////////////////////////////////////
void DynamicCar::addToLog(double dt) {

  in_time += dt;
  d_time += dt;

#if 0
   if(d_time >= 0.01){

       double FY = (1.0/car_par.L)*car_par.mass*car_state.Vx*car_state.Om;

       double FYr_st = car_par.a*FY;
       double FYf_st = car_par.b*FY;

       int flOKr = 1;
       double al_r = brushModel_solver(FYr_st,0,flOKr);
       if(flOKr < 1)
           al_r = 0.0;

       int flOKf = 1;
       double al_f = brushModel_solver(FYf_st,1,flOKf);
       if(flOKf < 1)
           al_f = 0.0;

       double Vdes = car_state.Vdes;

       if(fl_reverse > 0)
           Vdes = -Vdes;



      fileDyn << in_time << "      " << (180.0/M_PI)*car_state.Om
              //<< "   " <<   (180.0/M_PI)*(tan(car_state.steer_alpha +car_state.slipAng_f) - tan(car_state.slipAng_r))*car_state.Vx/car_par.L
              << "   " <<   (180.0/M_PI)*(tan(car_state.steer_alpha + al_f) - tan(al_r))*car_state.Vx/car_par.L
              << "   " <<(180.0/M_PI)*tan(car_state.steer_alpha)*car_state.Vx/car_par.L
              << "      "
              << car_state.slipAng_r*(180.0/M_PI) << "   " <<  al_r*(180.0/M_PI)
              << "      "
              << car_state.slipAng_f*(180.0/M_PI) << "   " <<  al_f*(180.0/M_PI)
              << "      "
              << car_state.steer_alpha*(180.0/M_PI) << "   " << car_state.steer_alpha_req*(180.0/M_PI)
              << "      "
              << car_state.Vx << "   " << Vdes << "     "
              << car_state.x << "   " << car_state.y << "   " << car_state.psi
              << "      " << car_state.Vy << "   " << car_state.Vx*(car_par.b*tan(car_state.steer_alpha)/car_par.L + tan(al_r))
              << "      " << (180.0/M_PI)*atan2(car_state.Vy,car_state.Vx) << "   "
              << (180.0/M_PI)*atan2(car_state.Vx*(car_par.b*tan(car_state.steer_alpha)/car_par.L + tan(al_r)),car_state.Vx)
              << "      " << car_state.FXr_req << " "
              << car_state.FXr << "  " << car_state.FYr << " " << fabs(car_state.Nr*car_par.mu)
              << " " << fabs(car_state.Nr*car_par.mu) - sqrt(car_state.FXr*car_state.FXr + car_state.FYr*car_state.FYr)
              << "      " << car_state.FXf << "  " << car_state.FYf<< " " << fabs(car_state.Nf*car_par.mu)
              << " " << fabs(car_state.Nf*car_par.mu) - sqrt(car_state.FXf*car_state.FXf + car_state.FYf*car_state.FYf)
              << "      " << car_state.state << "      " << car_state.B_pressure
              << "      " << car_state.long_slip_f*(180.0/M_PI) << "  " << car_state.long_slip_r*(180.0/M_PI)
              << "      " << car_state.dVxdt
              << "       " << car_state.Om*car_state.Vy + (car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                         + car_state.FXr)/car_par.mass - car_state.Fdrag/car_par.mass
              << "    " << car_state.Om*car_state.Vx << "   " << 9.89 - sqrt(car_state.dVxdt*car_state.dVxdt
                                                                                + car_state.Om*car_state.Vx*car_state.Om*car_state.Vx)

              << "            " << car_state.accel - (car_state.Fdrag_PM/car_par.mass)
              << "         " << car_state.OmWr*car_par.rearWheelRad << "   " << car_state.Vx
              << "         " << car_state.OmWf*car_par.frontWheelRad
              << "         " << car_state.Nbrake_r/car_par.rearWheelRad << "    " << car_state.Nbrake_f/car_par.frontWheelRad
              << std::endl;


       d_time = 0.0;

       if(fl_out > 0)
         std::cout<< "intime = " << in_time << std::endl;

    }

#endif

}

//---------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////

void DynamicCar::dynamicNav_ev_Base_test(double steer, double accel_des, double dt,
                               int reverse_fl, double speed_des) {
  // +++   Here is passed the required control for actuators; evolution of
  // actuators is defined in evolutionControl()

  car_state.steer_alpha_req = steer;
  car_state.accel =
      accel_des;  // with sign corresponding car system of coordinate

  fl_reverse = reverse_fl;
  car_state.Vdes = speed_des;  // For Fdrag_PM  being evaluated

  getAccBrkMode();  // getting choice of fl_AccBrkMode

  getDrag_PM();  // correction of car_state.accel to take into account FDrag_PM
                 // (evaluation of drag force for PM is located here !);

  newFXreq();  // distribution of required forces to engines (forces being
               // distributed as requested values in corresponding to drive
               // type)

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  double dtk = dt / double(culc_par.nsteps);

  int nsteps = 1;

  do {
    if (fl_MPClog > 0) addToMPClog(dtk);

    // integrNavkinematictest(dtk);
    integrNav(dtk);

    // t = t + dtk;

    nsteps++;

  } while (nsteps <= culc_par.nsteps);

  if (fl_fileDyn > 0) addToLog(dt);

  car_state.callcount += 1;
}

//-------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////

void DynamicCar::dynamicNav_ev(double steer, double accel_des, double dt,
                               int reverse_fl, double speed_des,
                               double addFx,double addFy,double addN) {
  // +++   Here is passed the required control for actuators; evolution of
  // actuators is defined in evolutionControl()

  car_state.steer_alpha_req = steer;
  car_state.accel =
      accel_des;  // with sign corresponding car system of coordinate

  fl_reverse = reverse_fl;
  car_state.Vdes = speed_des;  // For Fdrag_PM  being evaluated

  getAccBrkMode();  // getting choice of fl_AccBrkMode

  getDrag_PM();  // correction of car_state.accel to take into account FDrag_PM
                 // (evaluation of drag force for PM is located here !);

  newFXreq();  // distribution of required forces to engines (forces being
               // distributed as requested values in corresponding to drive
               // type)

  evolutionControl(dt);

 // std::cout<< "N_r = " << car_state.Nmot_r << "    "<< "N_f = " << car_state.Nmot_f << "    "<< "Brk_f = " << car_state.Nbrake_f << "    "
 //             << "Brk_r = " << car_state.Nbrake_r << std::endl;

  dynamicNav_ev_Base(dt,car_state.steer_alpha,car_state.Nmot_f,car_state.Nmot_r,car_state.Nbrake_f,car_state.Nbrake_r,
                     addFx, addFy, addN);
}

//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////

void DynamicCar::dynamicNav_ev_Base(double dt,double steer,double NmotF,double NmotR,double NBrkF,double NBrkR,
                                    double addFx,double addFy,double addN){
  // +++   Here is passed the required control

  // For loging if call of dynamicNav_ev_Base() occuring not from dynamicNav_ev() :
    car_state.steer_alpha = steer;

    car_state.Nmot_f = NmotF;
    car_state.Nmot_r = NmotR;
    car_state.Nbrake_f = NBrkF;
    car_state.Nbrake_r = NBrkR;

    car_state.addedFx = addFx;
    car_state.addedFy = addFy;
    car_state.addedN = addN;


  double dtk = dt / double(culc_par.nsteps);

  int nsteps = 1;

  do {
    if (fl_MPClog > 0) addToMPClog(dtk);

    if(culc_par.implicit_solver == 0)
       integrNavBase(dtk,NmotF,NmotR,NBrkF,NBrkR);
    else
       integrNavBase_implicit(dtk,NmotF,NmotR,NBrkF,NBrkR);

    // t = t + dtk;

    nsteps++;

  } while (nsteps <= culc_par.nsteps);

  if (fl_fileDyn > 0) addToLog(dt);

  car_state.callcount += 1;
}

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::integrNav(double dt){

    //dVxdt = Om*Vy + (FXr + FXf - Fdrag)/m;
    //dVydt = -Om*Vx + (FYf*cos(steer) + FXf*sin(steer) + FYr)/m;
    //dOmdt = (-FYr*b + (FYf*cos(steer) +FXf*sin(steer))*a)/mI;
    //dpsidt = Om;
    //dxdt = Vx*cos(psi)-Vy*sin(psi);
    //dydt = Vx*sin(psi)+Vy*cos(psi);


 //   std::cout << "Vx = " << car_state.Vx << "    =====================================" << std::endl;

    if((culc_par.fl_RunKut < 1)||(car_state.state == 0))
        evolutionControl(dt);


    if(car_state.state == 0)
        return;



    // in the case of Run-Kut the actuators have initial state on this step;

    combinedSlip();
    getDrag();


    double kx1 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
    double ky1 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
    double kpsi1 = car_state.Om;
    double kVx1 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                                                                            -car_state.Fdrag)/car_par.mass);
    double kVy1 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +
                                                   car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
    double kOm1 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                      car_state.FXf*sin(car_state.steer_alpha)*car_par.a)/car_par.moment_inertia;

    double kOmWf1 = car_par.frontWheelRad*(car_state.FXf_req - car_state.FXf)/car_par.fWIn;
    double kOmWr1 = car_par.rearWheelRad*(car_state.FXr_req - car_state.FXr)/car_par.rWIn;


    double x0 = car_state.x;
    double y0 = car_state.y;
    double psi0 = car_state.psi;
    double Vx0 = car_state.Vx;
    double Vy0 = car_state.Vy;
    double Om0 = car_state.Om;

    double OmWf0 = car_state.OmWf;
    double OmWr0 = car_state.OmWr;


    if(culc_par.fl_RunKut > 0){


        car_state.x = x0 + 0.5*kx1*dt;
        car_state.y = y0 + 0.5*ky1*dt;
        car_state.psi = psi0 + 0.5*kpsi1*dt;
        car_state.Vx = Vx0 + 0.5*kVx1*dt;
        car_state.Vy = Vy0 + 0.5*kVy1*dt;
        car_state.Om = Om0 + 0.5*kOm1*dt;

        if(culc_par.fl_Long_Circ > 1){

            car_state.OmWf = OmWf0 + 0.5*kOmWf1*dt;
            car_state.OmWr = OmWr0 + 0.5*kOmWr1*dt;

        }

        evolutionControl(0.5*dt);

        combinedSlip();
        getDrag();

        double kx2 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
        double ky2 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
        double kpsi2 = car_state.Om;
        double kVx2 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                    - car_state.Fdrag)/car_par.mass);

        double kVy2 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +
                                                   car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
        double kOm2 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                      car_state.FXf*sin(car_state.steer_alpha)*car_par.a)/car_par.moment_inertia;


        double kOmWf2 = car_par.frontWheelRad*(car_state.FXf_req - car_state.FXf)/car_par.fWIn;
        double kOmWr2 = car_par.rearWheelRad*(car_state.FXr_req - car_state.FXr)/car_par.rWIn;



        car_state.x = x0 + 0.5*kx2*dt;
        car_state.y = y0 + 0.5*ky2*dt;
        car_state.psi = psi0 + 0.5*kpsi2*dt;
        car_state.Vx = Vx0 + 0.5*kVx2*dt;
        car_state.Vy = Vy0 + 0.5*kVy2*dt;
        car_state.Om = Om0 + 0.5*kOm2*dt;

        if(culc_par.fl_Long_Circ > 1){

            car_state.OmWf = OmWf0 + 0.5*kOmWf2*dt;
            car_state.OmWr = OmWr0 + 0.5*kOmWr2*dt;
        }

        // actuators have the same state (t+0.5*dt)

        combinedSlip();
        getDrag();

        double kx3 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
        double ky3 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
        double kpsi3 = car_state.Om;
        double kVx3 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                                        -car_state.Fdrag)/car_par.mass);

        double kVy3 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +
                                                   car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
        double kOm3 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                      car_state.FXf*sin(car_state.steer_alpha)*car_par.a)/car_par.moment_inertia;

        double kOmWf3 = car_par.frontWheelRad*(car_state.FXf_req - car_state.FXf)/car_par.fWIn;
        double kOmWr3 = car_par.rearWheelRad*(car_state.FXr_req - car_state.FXr)/car_par.rWIn;


        car_state.x = x0 + kx3*dt;
        car_state.y = y0 + ky3*dt;
        car_state.psi = psi0 + kpsi3*dt;
        car_state.Vx = Vx0 + kVx3*dt;
        car_state.Vy = Vy0 + kVy3*dt;
        car_state.Om = Om0 + kOm3*dt;

        if(culc_par.fl_Long_Circ > 1){

            car_state.OmWf = OmWf0 + kOmWf3*dt;
            car_state.OmWr = OmWr0 + kOmWr3*dt;
        }


        evolutionControl(0.5*dt); // actuators have changed in corresponding to changing time from t to t + dt
        combinedSlip();
        getDrag();

        double kx4 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
        double ky4 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
        double kpsi4 = car_state.Om;
        double kVx4 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                    - car_state.Fdrag)/car_par.mass);

        double kVy4 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +
                                                   car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
        double kOm4 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                      car_state.FXf*sin(car_state.steer_alpha)*car_par.a)/car_par.moment_inertia;

        double kOmWf4 = car_par.frontWheelRad*(car_state.FXf_req - car_state.FXf)/car_par.fWIn;
        double kOmWr4 = car_par.rearWheelRad*(car_state.FXr_req - car_state.FXr)/car_par.rWIn;


        // Runge modifications:

        kx1 = (kx1 + 2*kx2 + 2*kx3 + kx4)/6.0;
        ky1 = (ky1 + 2*ky2 + 2*ky3 + ky4)/6.0;
        kpsi1 = (kpsi1 + 2*kpsi2 + 2*kpsi3 + kpsi4)/6.0;
        kVx1 = (kVx1 + 2*kVx2 + 2*kVx3 + kVx4)/6.0;
        kVy1 = (kVy1 + 2*kVy2 + 2*kVy3 + kVy4)/6.0;
        kOm1 = (kOm1 + 2*kOm2 + 2*kOm3 + kOm4)/6.0;

        kOmWf1 = (kOmWf1 + 2*kOmWf2 + 2*kOmWf3 + kOmWf4)/6.0;
        kOmWr1 = (kOmWr1 + 2*kOmWr2 + 2*kOmWr3 + kOmWr4)/6.0;

    }
    else{

        car_state.dVxdt = kVx1;
        car_state.dOmWrdt = kOmWr1;

        double dti = analize_dt(dt);

        while(dt > dti){

            car_state.x = x0 + kx1*dti;
            car_state.y = y0 + ky1*dti;
            car_state.psi = psi0 + kpsi1*dti;
            car_state.Vx = Vx0 + kVx1*dti;
            car_state.Vy = Vy0 + kVy1*dti;
            car_state.Om = Om0 + kOm1*dti;

            if(culc_par.fl_Long_Circ > 1){
                car_state.OmWf = OmWf0 + kOmWf1*dti;
                car_state.OmWr = OmWr0 + kOmWr1*dti;
            }

            if(dt > dti)
                dt = dt - dti;
            else
                dti = dt;

            combinedSlip();
            getDrag();

            kx1 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
            ky1 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
            kpsi1 = car_state.Om;
            kVx1 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                        - car_state.Fdrag)/car_par.mass);

            kVy1 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +
                                                           car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
            kOm1 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                              car_state.FXf*sin(car_state.steer_alpha)*car_par.a)/car_par.moment_inertia;

            kOmWf1 = car_par.frontWheelRad*(car_state.FXf_req - car_state.FXf)/car_par.fWIn;
            kOmWr1 = car_par.rearWheelRad*(car_state.FXr_req - car_state.FXr)/car_par.rWIn;


            x0 = car_state.x;
            y0 = car_state.y;
            psi0 = car_state.psi;
            Vx0 = car_state.Vx;
            Vy0 = car_state.Vy;
            Om0 = car_state.Om;

            OmWf0 = car_state.OmWf;
            OmWr0 = car_state.OmWr;
        }

    }




    car_state.x = x0 + kx1*dt;
    car_state.y = y0 + ky1*dt;
    car_state.psi = psi0 + kpsi1*dt;
    car_state.Vx = Vx0 + kVx1*dt;
    car_state.Vy = Vy0 + kVy1*dt;
    car_state.Om = Om0 + kOm1*dt;

    if(culc_par.fl_Long_Circ > 1){
        car_state.OmWf = OmWf0 + kOmWf1*dt;
        car_state.OmWr = OmWr0 + kOmWr1*dt;
    }

    car_state.dVxdt = kVx1;
    car_state.dVydt = kVy1;
    car_state.dOmdt = kOm1;
    car_state.dOmWfdt = kOmWf1;
    car_state.dOmWrdt = kOmWr1;



    if(fl_AccBrk_fact < 0.0){

            double Vvh = sqrt(car_state.Vx*car_state.Vx + car_state.Vy*car_state.Vy);

            if(Vvh < 0.01)
            {


                car_state.Vx = 0.0;
                car_state.Vy = 0.0;
                car_state.Om = 0.0;

                car_state.OmWf = 0.0;
                car_state.OmWr = 0.0;

                car_state.long_slip_r = 0.0;
                car_state.long_slip_f = 0.0;

                car_state.slipAng_r = 0.0;
                car_state.slipAng_f = 0.0;

                car_state.dVxdt = 0.0;
                car_state.dVydt = 0.0;
                car_state.dOmdt = 0.0;
                car_state.dOmWfdt = 0.0;
                car_state.dOmWrdt = 0.0;

                car_state.state = 0;


            }



    }


    if(culc_par.fl_Long_Circ > 1)
       combinedSlip();

    nav_to_cog(1);



}
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DynamicCar::evalTireDissForces(double NmotF,double NmotR,double NBrkF,double NBrkR){

    car_state.flDW_f = 1;
    car_state.flDW_r = 1;
  // Tire Forces (Combined Slip)

    double Vxw;
    double Vyw; // velocity components at the point of front wheel in the inertial coordinate system of front wheel

    getVerticalLoads();

    double Vxr = car_state.Vx;
    double WVr = car_state.OmWr * car_par.rearWheelRad;
    double Vyr = car_state.Vy - car_state.Om * car_par.b;

    if (culc_par.fl_Long_Circ < 3)
      if ((car_par.drive_distrib > 0.99) && (NBrkR < 0.001))
      {
        WVr = Vxr;
        car_state.OmWr = Vxr / car_par.rearWheelRad;
        NBrkR = 0.0;
         car_state.flDW_r = -1;
      }

    double DWr = WVr - Vxr;

    double normK_r = fabs(WVr) + 0.0001;

    car_state.long_slip_r = DWr / normK_r;

    if (Vxr < 0.0) Vxr = (-1.0) * Vxr;

    car_state.slipAng_r = atan2(Vyr, Vxr);

    double sig_xr = car_par.Cx_r * car_state.long_slip_r;
    double sig_yr = -car_par.Cy_r * Vyr / normK_r;

    double gama_r = sqrt(sig_xr * sig_xr + sig_yr * sig_yr);

    double Fslip_r = brushModel_combined(gama_r, car_state.Nr);

    car_state.FXr = (sig_xr / (gama_r + 0.0001)) * Fslip_r;
    car_state.FYr = (sig_yr / (gama_r + 0.0001)) * Fslip_r;

    double Vxf = car_state.Vx;
    double Vyf = car_state.Vy + car_state.Om * car_par.a;

    double cs = cos(car_state.steer_alpha);
    double sn = sin(car_state.steer_alpha);

    Vxw = Vxf * cs + Vyf * sn;
    Vyw = Vyf * cs - Vxf * sn;

    double WVf = car_state.OmWf * car_par.frontWheelRad;

    if (culc_par.fl_Long_Circ < 3)
      if ((car_par.drive_distrib < 0.01) && (NBrkF < 0.001)) {
        WVf = Vxw;
        car_state.OmWf = Vxw / car_par.frontWheelRad;
        NBrkF = 0.0;
        car_state.flDW_f = -1;
      }

    double DWf = WVf - Vxw;

    double normK_f = fabs(WVf) + 0.0001;

    car_state.long_slip_f = DWf / normK_f;

    if (Vxw < 0.0) Vxw = (-1.0) * Vxw;

    car_state.slipAng_f = atan2(Vyw, Vxw);

    double sig_xf = car_par.Cx_f * car_state.long_slip_f;
    double sig_yf = -car_par.Cy_f * Vyw / normK_f;

    double gama_f = sqrt(sig_xf * sig_xf + sig_yf * sig_yf);

    double Fslip_f = brushModel_combined(gama_f, car_state.Nf);

    car_state.FXf = (sig_xf / (gama_f + 0.0001)) * Fslip_f;
    car_state.FYf = (sig_yf / (gama_f + 0.0001)) * Fslip_f;

  // Dissipative Forces

    car_state.evTrqW_f = 0.0;
    car_state.evTrqW_r = 0.0;



    if(car_par.drive_distrib < 0.01)
        NBrkR += car_par.rollingResistance*car_par.rearWheelRad;
    else if(car_par.drive_distrib > 0.99)
        NBrkF += car_par.rollingResistance*car_par.frontWheelRad;
    else{

        NBrkR += car_par.rollingResistance*car_par.rearWheelRad*(1.0 - car_par.drive_distrib);
        NBrkF += car_par.rollingResistance*car_par.frontWheelRad*car_par.drive_distrib;
      }

    // Rear

    if(car_state.flDW_r > -1){

      if(fabs(NmotR- car_state.FXr*car_par.rearWheelRad) > NBrkR)
        car_state.flDW_r = 1;
      else
        car_state.flDW_r = 0;

      if(car_state.OmWr > 0.0 || ((car_state.OmWr == 0.0)&&(car_state.Vx > 0.0)))
          car_state.evTrqW_r = NmotR - NBrkR - car_state.FXr*car_par.rearWheelRad;
      else if(car_state.OmWr < 0.0 || ((car_state.OmWr == 0.0)&&(car_state.Vx < 0.0)))
          car_state.evTrqW_r = NmotR + NBrkR - car_state.FXr*car_par.rearWheelRad;
      else if((car_state.OmWr == 0.0)&&(car_state.Vx == 0.0)){

        if(fabs(NmotR) < NBrkR)
          car_state.flDW_r = -1;
        else{

            if(NmotR > 0.0)
              car_state.evTrqW_r = NmotR - NBrkR;
            else
              car_state.evTrqW_r = NmotR + NBrkR;

        }
      }
    }

    // Front

    if(car_state.flDW_f > -1){

      if(fabs(NmotF- car_state.FXf*car_par.frontWheelRad) > NBrkF)
        car_state.flDW_f = 1;
      else
        car_state.flDW_f = 0;

      if(car_state.OmWf > 0.0 || ((car_state.OmWf == 0.0)&&(Vxw > 0.0)))
          car_state.evTrqW_f = NmotF - NBrkF - car_state.FXf*car_par.frontWheelRad;
      else if(car_state.OmWf < 0.0 || ((car_state.OmWf == 0.0)&&(Vxw < 0.0)))
          car_state.evTrqW_f = NmotF + NBrkF - car_state.FXf*car_par.frontWheelRad;
      else if((car_state.OmWf == 0.0)&&(Vxw == 0.0)){

        if(fabs(NmotF) < NBrkF)
          car_state.flDW_f = -1;
        else{

            if(NmotF > 0.0)
              car_state.evTrqW_f = NmotF - NBrkF;
            else
              car_state.evTrqW_f = NmotF + NBrkF;

        }
      }
    }


}
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::evWheel(double dt,double omW_0,double&omW, double Trq, int fl_Trq,double Iin){

  double dOmW = Trq/Iin;

  if(fl_Trq < 0){

      omW = omW_0;
      return;
   }

  if(omW_0 == 0.0){

      if(fl_Trq < 1)
        omW = omW_0;
     else
        omW = omW_0 + dOmW*dt;

  }
  else{

      omW = omW_0 + dOmW*dt;

      if(fl_Trq < 1)
          if((omW < 0.0 && omW_0 > 0.0)||(omW > 0.0 && omW_0 < 0.0))
              omW = 0.0;

  }




}

//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::integrNavBase(double dt,double NmotF,double NmotR,double NBrkF,double NBrkR){

    //dVxdt = Om*Vy + (FXr + FXf - Fdrag)/m;
    //dVydt = -Om*Vx + (FYf*cos(steer) + FXf*sin(steer) + FYr)/m;
    //dOmdt = (-FYr*b + (FYf*cos(steer) +FXf*sin(steer))*a)/mI;
    //dpsidt = Om;
    //dxdt = Vx*cos(psi)-Vy*sin(psi);
    //dydt = Vx*sin(psi)+Vy*cos(psi);


  //  car_state.steer_alpha = steer;


    if(fabs(NmotF + NmotR) > NBrkF + NBrkR + car_par.rollingResistance*
     (car_par.drive_distrib*car_par.frontWheelRad + (1.0 - car_par.drive_distrib)*car_par.rearWheelRad))
      fl_AccBrk_fact = 1;
    else
      fl_AccBrk_fact = -1;


    if(fl_AccBrk_fact < 0)
      if(sqrt(car_state.addedFx*car_state.addedFx + car_state.addedFy*car_state.addedFy) > 100.0) fl_AccBrk_fact = 1;


    if(car_state.state == 0){

        if(fl_AccBrk_fact == 1)
          car_state.state = 1;
        else
          return;
     }




    // in the case of Run-Kut the actuators have initial state on this step;


    evalTireDissForces(NmotF,NmotR,NBrkF,NBrkR);
    getDrag();


    double kx1 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
    double ky1 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
    double kpsi1 = car_state.Om;
    double kVx1 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                                                                 +car_state.addedFx -car_state.Fdrag)/car_par.mass);
    double kVy1 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +car_state.addedFy+
                                                   car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
    double kOm1 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                      car_state.FXf*sin(car_state.steer_alpha)*car_par.a + car_state.addedN)/car_par.moment_inertia;

    double kOmWf1 = car_state.evTrqW_f;
    double kOmWr1 = car_state.evTrqW_r;

    int flD_r = car_state.flDW_r;
    int flD_f = car_state.flDW_f;


    double x0 = car_state.x;
    double y0 = car_state.y;
    double psi0 = car_state.psi;
    double Vx0 = car_state.Vx;
    double Vy0 = car_state.Vy;
    double Om0 = car_state.Om;

    double OmWf0 = car_state.OmWf;
    double OmWr0 = car_state.OmWr;


    if(culc_par.fl_RunKut > 0){


        car_state.x = x0 + 0.5*kx1*dt;
        car_state.y = y0 + 0.5*ky1*dt;
        car_state.psi = psi0 + 0.5*kpsi1*dt;
        car_state.Vx = Vx0 + 0.5*kVx1*dt;
        car_state.Vy = Vy0 + 0.5*kVy1*dt;
        car_state.Om = Om0 + 0.5*kOm1*dt;

        evWheel(0.5*dt,OmWf0,car_state.OmWf,car_state.evTrqW_f,car_state.flDW_f,car_par.fWIn);
        evWheel(0.5*dt,OmWr0,car_state.OmWr,car_state.evTrqW_r,car_state.flDW_r,car_par.rWIn);

        evalTireDissForces(NmotF,NmotR,NBrkF,NBrkR);
        getDrag();


        double kx2 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
        double ky2 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
        double kpsi2 = car_state.Om;
        double kVx2 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                                                                     +car_state.addedFx -car_state.Fdrag)/car_par.mass);
        double kVy2 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +car_state.addedFy+
                                                       car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
        double kOm2 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                          car_state.FXf*sin(car_state.steer_alpha)*car_par.a + car_state.addedN)/car_par.moment_inertia;


        double kOmWf2 = car_state.evTrqW_f;
        double kOmWr2 = car_state.evTrqW_r;



        car_state.x = x0 + 0.5*kx2*dt;
        car_state.y = y0 + 0.5*ky2*dt;
        car_state.psi = psi0 + 0.5*kpsi2*dt;
        car_state.Vx = Vx0 + 0.5*kVx2*dt;
        car_state.Vy = Vy0 + 0.5*kVy2*dt;
        car_state.Om = Om0 + 0.5*kOm2*dt;

        evWheel(0.5*dt,OmWf0,car_state.OmWf,car_state.evTrqW_f,car_state.flDW_f,car_par.fWIn);
        evWheel(0.5*dt,OmWr0,car_state.OmWr,car_state.evTrqW_r,car_state.flDW_r,car_par.rWIn);

        //

        evalTireDissForces(NmotF,NmotR,NBrkF,NBrkR);
        getDrag();

        double kx3 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
        double ky3 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
        double kpsi3 = car_state.Om;
        double kVx3 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                                                                     +car_state.addedFx -car_state.Fdrag)/car_par.mass);
        double kVy3 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +car_state.addedFy+
                                                       car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
        double kOm3 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                          car_state.FXf*sin(car_state.steer_alpha)*car_par.a + car_state.addedN)/car_par.moment_inertia;


        double kOmWf3 = car_state.evTrqW_f;
        double kOmWr3 = car_state.evTrqW_r;


        car_state.x = x0 + kx3*dt;
        car_state.y = y0 + ky3*dt;
        car_state.psi = psi0 + kpsi3*dt;
        car_state.Vx = Vx0 + kVx3*dt;
        car_state.Vy = Vy0 + kVy3*dt;
        car_state.Om = Om0 + kOm3*dt;

        evWheel(dt,OmWf0,car_state.OmWf,car_state.evTrqW_f,car_state.flDW_f,car_par.fWIn);
        evWheel(dt,OmWr0,car_state.OmWr,car_state.evTrqW_r,car_state.flDW_r,car_par.rWIn);


        evalTireDissForces(NmotF,NmotR,NBrkF,NBrkR);
        getDrag();

        double kx4 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
        double ky4 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
        double kpsi4 = car_state.Om;
        double kVx4 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                                                                     +car_state.addedFx -car_state.Fdrag)/car_par.mass);
        double kVy4 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +car_state.addedFy+
                                                       car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
        double kOm4 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                          car_state.FXf*sin(car_state.steer_alpha)*car_par.a + car_state.addedN)/car_par.moment_inertia;

        double kOmWf4 = car_state.evTrqW_f;
        double kOmWr4 = car_state.evTrqW_r;


        // Runge modifications:

        kx1 = (kx1 + 2*kx2 + 2*kx3 + kx4)/6.0;
        ky1 = (ky1 + 2*ky2 + 2*ky3 + ky4)/6.0;
        kpsi1 = (kpsi1 + 2*kpsi2 + 2*kpsi3 + kpsi4)/6.0;
        kVx1 = (kVx1 + 2*kVx2 + 2*kVx3 + kVx4)/6.0;
        kVy1 = (kVy1 + 2*kVy2 + 2*kVy3 + kVy4)/6.0;
        kOm1 = (kOm1 + 2*kOm2 + 2*kOm3 + kOm4)/6.0;

        kOmWf1 = (kOmWf1 + 2*kOmWf2 + 2*kOmWf3 + kOmWf4)/6.0;
        kOmWr1 = (kOmWr1 + 2*kOmWr2 + 2*kOmWr3 + kOmWr4)/6.0;

    }
    else{

        car_state.dVxdt = kVx1;
        car_state.dOmWrdt = kOmWr1;

        double dti = analize_dt(dt);

        while(dt > dti){

            car_state.x = x0 + kx1*dti;
            car_state.y = y0 + ky1*dti;
            car_state.psi = psi0 + kpsi1*dti;
            car_state.Vx = Vx0 + kVx1*dti;
            car_state.Vy = Vy0 + kVy1*dti;
            car_state.Om = Om0 + kOm1*dti;

            evWheel(dti,OmWf0,car_state.OmWf,kOmWf1,flD_f,car_par.fWIn);
            evWheel(dti,OmWr0,car_state.OmWr,kOmWr1,flD_r,car_par.rWIn);

            if(dt > dti)
                dt = dt - dti;
            else
                dti = dt;

            evalTireDissForces(NmotF,NmotR,NBrkF,NBrkR);
            getDrag();

            kx1 = (car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi));
            ky1 = (car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi));
            kpsi1 = car_state.Om;
            kVx1 = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha) - car_state.FYf*sin(car_state.steer_alpha)
                                                                                                         +car_state.addedFx -car_state.Fdrag)/car_par.mass);
            kVy1 = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) +car_state.addedFy+
                                                           car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
            kOm1 = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                              car_state.FXf*sin(car_state.steer_alpha)*car_par.a + car_state.addedN)/car_par.moment_inertia;


            kOmWf1 = car_state.evTrqW_f;
            kOmWr1 = car_state.evTrqW_r;

            flD_r = car_state.flDW_r;
            flD_f = car_state.flDW_f;

            x0 = car_state.x;
            y0 = car_state.y;
            psi0 = car_state.psi;
            Vx0 = car_state.Vx;
            Vy0 = car_state.Vy;
            Om0 = car_state.Om;

            OmWf0 = car_state.OmWf;
            OmWr0 = car_state.OmWr;
        }

    }




    car_state.x = x0 + kx1*dt;
    car_state.y = y0 + ky1*dt;
    car_state.psi = psi0 + kpsi1*dt;
    car_state.Vx = Vx0 + kVx1*dt;
    car_state.Vy = Vy0 + kVy1*dt;
    car_state.Om = Om0 + kOm1*dt;

    evWheel(dt,OmWf0,car_state.OmWf,kOmWf1,flD_f,car_par.fWIn);
    evWheel(dt,OmWr0,car_state.OmWr,kOmWr1,flD_r,car_par.rWIn);

    car_state.dVxdt = kVx1;
    car_state.dVydt = kVy1;
    car_state.dOmdt = kOm1;
    car_state.dOmWfdt = kOmWf1;
    car_state.dOmWrdt = kOmWr1;



    if(fl_AccBrk_fact < 0.0){

            double Vvh = sqrt(car_state.Vx*car_state.Vx + car_state.Vy*car_state.Vy+ 3.0*car_state.Om*car_state.Om);

            if(Vvh < 0.01)
            {


                car_state.Vx = 0.0;
                car_state.Vy = 0.0;
                car_state.Om = 0.0;

                car_state.OmWf = 0.0;
                car_state.OmWr = 0.0;

                car_state.long_slip_r = 0.0;
                car_state.long_slip_f = 0.0;

                car_state.slipAng_r = 0.0;
                car_state.slipAng_f = 0.0;

                car_state.dVxdt = 0.0;
                car_state.dVydt = 0.0;
                car_state.dOmdt = 0.0;
                car_state.dOmWfdt = 0.0;
                car_state.dOmWrdt = 0.0;

                car_state.state = 0;


            }



    }

    evalTireDissForces(NmotF,NmotR,NBrkF,NBrkR);

    nav_to_cog(1);

  //  std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! state = " << car_state.state << std::endl;


}
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::integrNavBase_implicit(double dt,double NmotF,double NmotR,double NBrkF,double NBrkR){

    //dVxdt = Om*Vy + (FXr + FXf - Fdrag)/m;
    //dVydt = -Om*Vx + (FYf*cos(steer) + FXf*sin(steer) + FYr)/m;
    //dOmdt = (-FYr*b + (FYf*cos(steer) +FXf*sin(steer))*a)/mI;
    //dpsidt = Om;
    //dxdt = Vx*cos(psi)-Vy*sin(psi);
    //dydt = Vx*sin(psi)+Vy*cos(psi);


  //  car_state.steer_alpha = steer;

  // Getting current temporal layer fixed =================

  car_state_0.x = car_state.x;
  car_state_0.y = car_state.y;
  car_state_0.psi = car_state.psi;
  car_state_0.Vx = car_state.Vx;
  car_state_0.Vy = car_state.Vy;
  car_state_0.Om = car_state.Om;

  car_state_0.OmWf = car_state.OmWf;
  car_state_0.OmWr = car_state.OmWr;

  solverDat.fl_first_call = 1;

  //=======================================================

  int nonlin_it = culc_par.num_impl_nonlin_it;

  bool show_errors = false;
  std::vector<double> errors_on_it(nonlin_it,-1.0);
  int k_err = 0;
  //+++++++++++++++++++++++++++++++++++++++ Nonlinear Itterations ++++++++++++++++++++++++++++++++++ +++++++++++++++++++++++++++++++++++

  // Analizing of total dissipation ---------------------------------------------

  if(fabs(NmotF + NmotR) > NBrkF + NBrkR + car_par.rollingResistance*
   (car_par.drive_distrib*car_par.frontWheelRad + (1.0 - car_par.drive_distrib)*car_par.rearWheelRad))
    fl_AccBrk_fact = 1.0;
  else
    fl_AccBrk_fact = -1.0;

  if(fl_AccBrk_fact < 0)
    if(sqrt(car_state.addedFx*car_state.addedFx + car_state.addedFy*car_state.addedFy) > 100.0) fl_AccBrk_fact = 1;


  if(car_state.state == 0){

      if(fl_AccBrk_fact > 0.0)
        car_state.state = 1;

   }

 // fl_first_it = 1;
  while(nonlin_it > 0 && car_state.state >0){


   // if(false)
    if(fl_AccBrk_fact < 0.0){

            double Vvh = sqrt(car_state.Vx*car_state.Vx + car_state.Vy*car_state.Vy + 3.0*car_state.Om*car_state.Om);

            if(Vvh < 0.01)
            {


                car_state.Vx = 0.0;
                car_state.Vy = 0.0;
                car_state.Om = 0.0;

                car_state.OmWf = 0.0;
                car_state.OmWr = 0.0;

                car_state.long_slip_r = 0.0;
                car_state.long_slip_f = 0.0;

                car_state.slipAng_r = 0.0;
                car_state.slipAng_f = 0.0;

                car_state.dVxdt = 0.0;
                car_state.dVydt = 0.0;
                car_state.dOmdt = 0.0;
                car_state.dOmWfdt = 0.0;
                car_state.dOmWrdt = 0.0;

                car_state.state = 0;

                break;
            }

    }

    // Nonlinear system and step
    std::vector<double> FdF;

    FdF = linearizatedSystem(NmotF,NmotR,NBrkF,NBrkR,dt);


    int fl_r = car_state.flDW_r;
    int fl_f = car_state.flDW_f;

    if(show_errors)
      std::cout << fl_r << " ---<>--- " << fl_f << " --- <  > ----" ;

    // update


    double step = 1.0;

    if(culc_par.corr_impl_step > 0 && fl_first_it < 1){

        int it = 4;

        double step1 = step;
        double step0 = 0.0;

        step = step1;
        std::vector<double> mFF;
        mFF = updateState(step,dt,NmotF,NmotR,NBrkF,NBrkR,fl_r,fl_f,1);

        while(it > 0){

          bool fl_it = true;

          step = updStep(step0,step1,FdF[0],mFF[0],FdF[1],mFF[1],fl_it);

          if(!fl_it)
            break;

          if(it > 1){
            std::vector<double> mF1;
            mF1 = updateState(step,dt,NmotF,NmotR,NBrkF,NBrkR,fl_r,fl_f,1);

            if((mF1[1] < 0.0)&&(mF1[0] < FdF[0])){

              step0 = step;
              FdF = mF1;
            }
            else{

              step1 = step;
              mFF = mF1;
            }
          }
          it--;
        }

        std::cout << step << "  ";
        FdF = updateState(step,dt,NmotF,NmotR,NBrkF,NBrkR,fl_r,fl_f,2);
     }
    else{

        if(show_errors)
          FdF = updateState(step,dt,NmotF,NmotR,NBrkF,NBrkR,fl_r,fl_f,2);
        else
          updateState(step,dt,NmotF,NmotR,NBrkF,NBrkR,fl_r,fl_f,0);
    }
    if(show_errors)
    {
      errors_on_it[k_err]= std::sqrt(FdF[0]);


      k_err++;
    }

    nonlin_it--;
    fl_first_it = 0;
 }

  if(show_errors)
  {
   std::cout<< "errors: ---------------      ";
   for(int k_e=0;k_e<culc_par.num_impl_nonlin_it;k_e++)
   std::cout << errors_on_it[k_e] << "     ";
   std::cout << "-------------------------------"<< std::endl;
  }
  //++++++++++++++++++++++++++ END of Nonlinear Itterations ++++++++++++++++++++++++++++++++++++++  +++++++++++++++++++++++++++++++

  car_state.psi = car_state_0.psi + 0.5*(car_state.Om + car_state_0.Om)*dt;
  car_state.x = car_state_0.x + 0.5*(car_state.Vx*cos(car_state.psi) - car_state.Vy*sin(car_state.psi))*dt
      + 0.5*(car_state_0.Vx*cos(car_state_0.psi) - car_state_0.Vy*sin(car_state_0.psi))*dt;
  car_state.y = car_state_0.y + 0.5*(car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi))*dt
      + 0.5*(car_state.Vx*sin(car_state.psi) + car_state.Vy*cos(car_state.psi))*dt;

  analyseDissForces(NmotF,NmotR,NBrkF,NBrkR);

  car_state.dVxdt = (car_state.Om*car_state.Vy + (car_state.FXr + car_state.FXf*cos(car_state.steer_alpha)
                                                  - car_state.FYf*sin(car_state.steer_alpha)+car_state.addedFx
                                                  -car_state.Fdrag)/car_par.mass);
  car_state.dVydt = (-car_state.Om*car_state.Vx + (car_state.FYr + car_state.FXf*sin(car_state.steer_alpha) + car_state.addedFy
                                                 + car_state.FYf*cos(car_state.steer_alpha) )/car_par.mass);
  car_state.dOmdt = (-car_state.FYr*car_par.b + car_state.FYf*cos(car_state.steer_alpha)*car_par.a +
                    car_state.FXf*sin(car_state.steer_alpha)*car_par.a + car_state.addedN)/car_par.moment_inertia;

  car_state.dOmWfdt = (car_state.evTrqW_f- car_state.FXf*car_par.frontWheelRad)/car_par.fWIn;
  car_state.dOmWrdt = (car_state.evTrqW_r- car_state.FXr*car_par.rearWheelRad)/car_par.rWIn;



  nav_to_cog(1);



}
//--------------------------------------------------------------------------------------------------------------------------------------------------
//-/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
double DynamicCar::updStep(double t0,double t1,double f0,double f1,double df0,double df1,bool& fl){

  double t = t1 - t0;

  if(fabs(t) < 0.01){

      fl = false;
      return t1;
  }

  double t2 = t*t;
  double t3 = t2*t;

  double s = (t0 + 2.0*t1)/3.0;

  //return s;

  if(df1 < 0.0){
      // f = c0 + c1*(s-t0) + c2*(s-t0)^2 + c3*(s-t0)^3;

      double c0 = f0;
      double c1 = df0;

      double A = f1 - c0 - c1*t;
      double B = df1 - df0;

      double c2 = (3.0*A - B*t)/t2;

      if(c2 < 0.0){
        double c3 = -(2.0*A - B*t)/t3;

        if(fabs(c3) > 0.000001){

          double D = c2*c2-3.0*c3*c1;
          if(D>=0){

             double cs = -1.0*(c2 + sqrt(D))/(3.0*c3);
             if((cs >= 0.01)&&(cs <= t1 - t0))
               s = t0 + cs;
          }
        }
      }
  }
  else{
      // f = c0 + c1*(s-t0) + c2*(s-t0)^2;

      double c0 = f0;
      double c1 = df0;
      double c2 = (f1 -f0 - c1*t)/t2;

      if(c2 > 0.000001){

          double cs = -c1/(2.0*c2);
          if((cs >= 0.01)&&(cs <= t1 - t0))
            s = t0 + cs;
      }
  }

  return s;
}
//==////////////////////////////////////////////////////////////////////////-------------------------------------------
std::vector<double> DynamicCar::updateState(double step,double dt,double NmotF,double NmotR,double NBrkF,double NBrkR,int fl_r,int fl_f,int fl){

  std::vector<double> FdF;
  FdF.resize(2,0.0);

  bool ky = true;

  double OmWr = car_state.OmWr;
  double OmWf = car_state.OmWf;
  double Nr = car_state.Nr;
  double Nf = car_state.Nf;
  double Om = car_state.Om;
  double Vx = car_state.Vx;
  double Vy = car_state.Vy;

  if((fl_r > -1)&&(fl_f > -1)){

      double W_r = car_state.OmWr + step*solverDat.dX(5);

      if((fl_r < 1)&& ky)
        if((car_state.OmWr < 0.0 && W_r > 0.0) ||(car_state.OmWr > 0.0 && W_r < 0.0)){

            if(fabs(W_r) > 0.0001)
                step = - car_state.OmWr/solverDat.dX(5);

            W_r = 0.0;
        }

      double W_f = car_state.OmWf + step*solverDat.dX(6);

      if((fl_f < 1)&& ky)
        if((car_state.OmWf < 0.0 && W_f > 0.0) ||(car_state.OmWf > 0.0 && W_f < 0.0)){

            if(fabs(W_f) > 0.0001){
                step = - car_state.OmWf/solverDat.dX(6);
                W_r = car_state.OmWr + step*solverDat.dX(5);
              }

            W_f = 0.0;
        }


      car_state.OmWr = W_r;
      car_state.OmWf = W_f;

  }else if((fl_f > -1)&&(fl_r == -1)){

    double W_f = car_state.OmWf + step*solverDat.dX(5);

    if((fl_f < 1)&& ky)
      if((car_state.OmWf < 0.0 && W_f > 0.0) ||(car_state.OmWf > 0.0 && W_f < 0.0)){

          if(fabs(W_f) > 0.0001)
              step = - car_state.OmWf/solverDat.dX(5);

          W_f = 0.0;
      }

    car_state.OmWf = W_f;

  }else if((fl_r > -1)&&(fl_f == -1)){

    double W_r = car_state.OmWr + step*solverDat.dX(5);

    if((fl_r < 1)&& ky )
      if((car_state.OmWr < 0.0 && W_r > 0.0) ||(car_state.OmWr > 0.0 && W_r < 0.0)){

          if(fabs(W_r) > 0.0001)
              step = - car_state.OmWr/solverDat.dX(5);

          W_r = 0.0;
      }

    car_state.OmWr = W_r;

  }

  car_state.Nr += step*solverDat.dX(0);
  car_state.Nf += step*solverDat.dX(1);
  car_state.Om += step*solverDat.dX(2);
  car_state.Vx += step*solverDat.dX(3);
  car_state.Vy += step*solverDat.dX(4);

  if(fl > 0){


      FdF = linearizatedSystem(NmotF,NmotR,NBrkF,NBrkR,dt,0);


      if(fl < 2){

          car_state.OmWr = OmWr;
          car_state.OmWf = OmWf;
          car_state.Nr = Nr;
          car_state.Nf = Nf;
          car_state.Om = Om;
          car_state.Vx = Vx;
          car_state.Vy = Vy;

          car_state.flDW_r = fl_r;
          car_state.flDW_f = fl_f;
      }


  }


  return FdF;


}
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<double> DynamicCar::linearizatedSystem(double NmotF,double NmotR,double NBrkF,double NBrkR,double dt1,int fl_dX){



 // dt1 = 1.0/dt1;
 // double dt = 1.0;

  double dt = dt1;
  dt1 = 1.0;

  double normF = -1.0;
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  car_state.flDW_f = 1;

  car_state.flDW_r = 1;

  // Dissipative Forces

    car_state.evTrqW_f = 0.0;


    if(car_par.drive_distrib < 0.01)
        NBrkR += car_par.rollingResistance*car_par.rearWheelRad;
    else if(car_par.drive_distrib > 0.99)
        NBrkF += car_par.rollingResistance*car_par.frontWheelRad;
    else{

        NBrkR += car_par.rollingResistance*car_par.rearWheelRad*(1.0 - car_par.drive_distrib);
        NBrkF += car_par.rollingResistance*car_par.frontWheelRad*car_par.drive_distrib;
      }

  // REAR wheel Forces ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  double Vxw;
  double Vyw;

  double Vxr = car_state.Vx;
  double WVr = car_state.OmWr * car_par.rearWheelRad;
  double Vyr = car_state.Vy - car_state.Om * car_par.b;

  if (culc_par.fl_Long_Circ < 3)
    if ((car_par.drive_distrib > 0.99) && (NBrkR < 0.001))
    {
      WVr = Vxr;
      car_state.OmWr = Vxr / car_par.rearWheelRad;
      NBrkR = 0.0;
       car_state.flDW_r = -1;
    }

  double DWr = WVr - Vxr;

  double normK_r = fabs(WVr) + 0.0001;

  car_state.long_slip_r = DWr / normK_r;

  if (Vxr < 0.0) Vxr = (-1.0) * Vxr;

  car_state.slipAng_r = atan2(Vyr, Vxr);

  double sig_xr = car_par.Cx_r * car_state.long_slip_r;
  double sig_yr = -car_par.Cy_r * Vyr / normK_r;

  double gama_r = sqrt(sig_xr * sig_xr + sig_yr * sig_yr + 0.0001);

  double Fslip_r = brushModel_combined(gama_r, car_state.Nr);


  car_state.FXr = (sig_xr / gama_r) * Fslip_r;
  car_state.FYr = (sig_yr / gama_r) * Fslip_r;



    // Rear Torque

    car_state.evTrqW_r = 0.0;

    if(car_state.flDW_r > -1){

      if(fabs(NmotR- car_state.FXr*car_par.rearWheelRad) > NBrkR)
        car_state.flDW_r = 1;
      else
        car_state.flDW_r = 0;

      if(car_state.flDW_r < 1 && car_state.OmWr == 0.0)
        car_state.flDW_r = -1;

      if(car_state.OmWr > 0.0 || ((car_state.OmWr == 0.0)&&(car_state.Vx > 0.0)))
          car_state.evTrqW_r = NmotR - NBrkR; //- car_state.FXr*car_par.rearWheelRad;
      else if(car_state.OmWr < 0.0 || ((car_state.OmWr == 0.0)&&(car_state.Vx < 0.0)))
          car_state.evTrqW_r = NmotR + NBrkR; // - car_state.FXr*car_par.rearWheelRad;
      else if((car_state.OmWr == 0.0)&&(car_state.Vx == 0.0)){

        if(fabs(NmotR) < NBrkR)
          car_state.flDW_r = -1;
        else{

            if(NmotR > 0.0)
              car_state.evTrqW_r = NmotR - NBrkR;
            else
              car_state.evTrqW_r = NmotR + NBrkR;

        }
      }
    }

    // for adjusting singularities

    double intention_r = 1.0;

    if(car_state.OmWr > 0.0)
      intention_r = 1.0;
    else if(car_state.OmWr < 0.0)
      intention_r = -1.0;
    else if(car_state.OmWr == 0.0){

         if(car_state.flDW_r > -1){

             if(car_state.evTrqW_r - car_state.FXr*car_par.rearWheelRad > 0.0)
               intention_r = 1.0;
             else if(car_state.evTrqW_r - car_state.FXr*car_par.rearWheelRad < 0.0)
               intention_r = -1.0;
             else
               intention_r = 0.0;

           }

      }


  //------------- Diffs

  if(solverDat.fl_first_call > 0)
  {
    double DFslip_r = brushModel_combined_diff(gama_r, car_state.Nr);
    double DFslip_rDN= brushModel_combined_diff_N(gama_r, car_state.Nr);

    double Fslip = Fslip_r/gama_r;
    double sig_x = sig_xr/gama_r;
    double sig_y = sig_yr/gama_r;

    solverDat.DFX_rDlon = car_par.Cx_r*( Fslip - Fslip*sig_x*sig_x + DFslip_r*sig_x*sig_x);
    solverDat.DFX_rDlat = car_par.Cy_r*sig_x*sig_y*(Fslip - DFslip_r);
    solverDat.DFY_rDlon =  car_par.Cx_r*sig_x*sig_y*(DFslip_r - Fslip);
    solverDat.DFY_rDlat = -car_par.Cy_r*( Fslip - Fslip*sig_y*sig_y + DFslip_r*sig_y*sig_y);
    solverDat.DFX_rDN = sig_x*DFslip_rDN;
    solverDat.DFY_rDN = sig_y*DFslip_rDN;

    double lat_r =  Vyr / normK_r;
    double lon_r = car_state.long_slip_r;

    solverDat.Dlat_rDVx = 0.0;
    solverDat.Dlat_rDVy = (1.0/normK_r);
    solverDat.Dlat_rDOm = - solverDat.Dlat_rDVy*car_par.b;
    solverDat.Dlat_rDOmWr = -(lat_r*solverDat.Dlat_rDVy)*car_par.rearWheelRad*intention_r;
    //if(car_state.OmWr < 0.0)
    //  solverDat.Dlat_rDOmWr *= -1.0;

    solverDat.Dlon_rDVx = -(1.0/normK_r);
    solverDat.Dlon_rDVy = 0.0;
    solverDat.Dlon_rDOm = 0.0;
    double dlon_rDOmWr1 = lon_r*solverDat.Dlon_rDVx*car_par.rearWheelRad*intention_r;
    //if(car_state.OmWr < 0.0)
    //  dlon_rDOmWr1 *= -1.0;
    solverDat.Dlon_rDOmWr =dlon_rDOmWr1 - solverDat.Dlon_rDVx*car_par.rearWheelRad;
  }



  // Front wheel forces+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  double Vxf = car_state.Vx;
  double Vyf = car_state.Vy + car_state.Om * car_par.a;

  double cs = cos(car_state.steer_alpha);
  double sn = sin(car_state.steer_alpha);

  Vxw = Vxf * cs + Vyf * sn;
  Vyw = Vyf * cs - Vxf * sn;

  double WVf = car_state.OmWf * car_par.frontWheelRad;

  if (culc_par.fl_Long_Circ < 3)
    if ((car_par.drive_distrib < 0.01) && (NBrkF < 0.001)) {
      WVf = Vxw;
      car_state.OmWf = Vxw / car_par.frontWheelRad;
      NBrkF = 0.0;
      car_state.flDW_f = -1;
    }


  double DWf = WVf - Vxw;

  double normK_f = fabs(WVf) + 0.0001;

  car_state.long_slip_f = DWf / normK_f;

  if (Vxw < 0.0) Vxw = (-1.0) * Vxw;

  car_state.slipAng_f = atan2(Vyw, Vxw);

  double sig_xf = car_par.Cx_f * car_state.long_slip_f;
  double sig_yf = -car_par.Cy_f * Vyw / normK_f;

  double gama_f = sqrt(sig_xf * sig_xf + sig_yf * sig_yf + 0.0001);

  double Fslip_f = brushModel_combined(gama_f, car_state.Nf);

  car_state.FXf = (sig_xf / gama_f) * Fslip_f;
  car_state.FYf = (sig_yf / gama_f) * Fslip_f;

  // Front Torque

  if(car_state.flDW_f > -1){

    if(fabs(NmotF- car_state.FXf*car_par.frontWheelRad) > NBrkF)
      car_state.flDW_f = 1;
    else
      car_state.flDW_f = 0;

    if(car_state.flDW_f < 1 && car_state.OmWf == 0.0)
      car_state.flDW_f = -1;

    if(car_state.OmWf > 0.0 || ((car_state.OmWf == 0.0)&&(Vxw > 0.0)))
        car_state.evTrqW_f = NmotF - NBrkF;// - car_state.FXf*car_par.frontWheelRad;
    else if(car_state.OmWf < 0.0 || ((car_state.OmWf == 0.0)&&(Vxw < 0.0)))
        car_state.evTrqW_f = NmotF + NBrkF;// - car_state.FXf*car_par.frontWheelRad;
    else if((car_state.OmWf == 0.0)&&(Vxw == 0.0)){

      if(fabs(NmotF) < NBrkF)
        car_state.flDW_f = -1;
      else{

          if(NmotF > 0.0)
            car_state.evTrqW_f = NmotF - NBrkF;
          else
            car_state.evTrqW_f = NmotF + NBrkF;

      }
    }
  }

  // for adjusting singularities

  double intention_f = 1.0;

  if(car_state.OmWf > 0.0)
    intention_f = 1.0;
  else if(car_state.OmWf < 0.0)
    intention_f = -1.0;
  else if(car_state.OmWf == 0.0){

       if(car_state.flDW_f > -1){

           if(car_state.evTrqW_f - car_state.FXf*car_par.frontWheelRad > 0.0)
             intention_f = 1.0;
           else if(car_state.evTrqW_f - car_state.FXf*car_par.frontWheelRad < 0.0)
             intention_f = -1.0;
           else
             intention_f = 0.0;

         }

    }

  //-------------------- Diffs
  if(solverDat.fl_first_call > 0)
  {
    double DFslip_f = brushModel_combined_diff(gama_f, car_state.Nf);
    double DFslip_fDN= brushModel_combined_diff_N(gama_f, car_state.Nf);

    double Fslip = Fslip_f/gama_f;
    double sig_x = sig_xf/gama_f;
    double sig_y = sig_yf/gama_f;

    solverDat.DFX_fDlon = car_par.Cx_f*( Fslip - Fslip*sig_x*sig_x + DFslip_f*sig_x*sig_x);
    solverDat.DFX_fDlat = car_par.Cy_f*sig_x*sig_y*(Fslip - DFslip_f);
    solverDat.DFY_fDlon =  car_par.Cx_f*sig_x*sig_y*(DFslip_f - Fslip);
    solverDat.DFY_fDlat = -car_par.Cy_f*( Fslip - Fslip*sig_y*sig_y + DFslip_f*sig_y*sig_y);
    solverDat.DFX_fDN = sig_x*DFslip_fDN;
    solverDat.DFY_fDN = sig_y*DFslip_fDN;

    double lat_f =  Vyw / normK_f;
    double lon_f = car_state.long_slip_f;

    double Dlat_fdvy = (1.0/normK_f);

    double dvyDVx= -sn;
    double dvyDVy = cs;
    double dvyDOm = cs*car_par.a;

    solverDat.Dlat_fDVx =Dlat_fdvy*dvyDVx;
    solverDat.Dlat_fDVy =Dlat_fdvy*dvyDVy;
    solverDat.Dlat_fDOm =Dlat_fdvy*dvyDOm;
    solverDat.Dlat_fDOmWf = -(lat_f*Dlat_fdvy)*car_par.frontWheelRad*intention_f;
    //if(car_state.OmWf < 0.0)
    //  solverDat.Dlat_fDOmWf *= -1.0;

    double Dlon_fdvx = -(1.0/normK_f);

    double dvxDVx = cs;
    double dvxDVy = sn;
    double dvxDOm = sn*car_par.a;

    solverDat.Dlon_fDVx = Dlon_fdvx*dvxDVx;
    solverDat.Dlon_fDVy = Dlon_fdvx*dvxDVy;
    solverDat.Dlon_fDOm = Dlon_fdvx*dvxDOm;
    double dlon_fDOmWf1 = lon_f*Dlon_fdvx*car_par.frontWheelRad*intention_f;
    //if(car_state.OmWf < 0.0)
    //  dlon_fDOmWf1 *= -1.0;
    solverDat.Dlon_fDOmWf =dlon_fDOmWf1 - Dlon_fdvx*car_par.frontWheelRad;
  }

  //-------------------------------------------------------------------------

  Eigen::VectorXd Y(4);
  //Y.resize(4);
  //Y.fill(0.0);

  Y(0) = car_state.FXr;
  Y(1) = car_state.FYr;
  Y(2) = car_state.FXf;
  Y(3) = car_state.FYf;

  Eigen::MatrixXd DYDZ = Eigen::MatrixXd::Zero(4,6);
  // Z = [lat_r,lon_r,lat_f,lon_f,Nr,Nf]';
  if(solverDat.fl_first_call > 0)
  {
    //DYDZ.resize(4,6);
    //DYDZ.fill(0.0);

    DYDZ(0,0)=solverDat.DFX_rDlat;
    DYDZ(0,1)=solverDat.DFX_rDlon;
    DYDZ(0,4)=solverDat.DFX_rDN;
    DYDZ(1,0)=solverDat.DFY_rDlat;
    DYDZ(1,1)=solverDat.DFY_rDlon;
    DYDZ(1,4)=solverDat.DFY_rDN;

    DYDZ(2,2)=solverDat.DFX_fDlat;
    DYDZ(2,3)=solverDat.DFX_fDlon;
    DYDZ(2,5)=solverDat.DFX_fDN;
    DYDZ(3,2)=solverDat.DFY_fDlat;
    DYDZ(3,3)=solverDat.DFY_fDlon;
    DYDZ(3,5)=solverDat.DFY_fDN;
  }

  Eigen::MatrixXd E;
  Eigen::VectorXd X;

  Eigen::MatrixXd DZDX;
  Eigen::VectorXd D;
  Eigen::MatrixXd G;

  if((car_state.flDW_r > -1)&&(car_state.flDW_f > -1)){

      solverDat.Nsys = 7;

      E = Eigen::MatrixXd::Identity(7,7);

      E(5,5) = dt1;
      E(6,6) = dt1;

      X = Eigen::VectorXd::Zero(7);


      X(5) = car_state.OmWr;
      X(6) = car_state.OmWf;

      if(solverDat.fl_first_call > 0)
      {

        DZDX = Eigen::MatrixXd::Zero(6,7);

      // Z = [lat_r,lon_r,lat_f,lon_f,Nr,Nf]';

        DZDX(0,5)= solverDat.Dlat_rDOmWr;
        DZDX(1,5)= solverDat.Dlon_rDOmWr;

        DZDX(2,6)= solverDat.Dlat_fDOmWf;
        DZDX(3,6)= solverDat.Dlon_fDOmWf;
      }

      D = Eigen::VectorXd::Zero(7);

      D(5) = -dt1*car_state_0.OmWr - dt*(car_state.evTrqW_r/car_par.rWIn);
      D(6) = -dt1*car_state_0.OmWf - dt*(car_state.evTrqW_f/car_par.fWIn);


      G = Eigen::MatrixXd::Zero(7,4);

      G(5,0) = dt*car_par.rearWheelRad/car_par.rWIn;
      G(6,2) = dt*car_par.frontWheelRad/car_par.fWIn;

  }
  else if((car_state.flDW_f == -1)&&(car_state.flDW_r > -1)){

      solverDat.Nsys = 6;

      E = Eigen::MatrixXd::Identity(6,6);

      E(5,5) = dt1;

      X = Eigen::VectorXd::Zero(6);

      X(5) = car_state.OmWr;

      if(solverDat.fl_first_call > 0)
      {

        DZDX = Eigen::MatrixXd::Zero(6,6);
        // Z = [lat_r,lon_r,lat_f,lon_f,Nr,Nf]';
        DZDX(0,5)= solverDat.Dlat_rDOmWr;
        DZDX(1,5)= solverDat.Dlon_rDOmWr;
      }

      D = Eigen::VectorXd::Zero(6);

      D(5) = -dt1*car_state_0.OmWr - dt*(car_state.evTrqW_r/car_par.rWIn);

      G = Eigen::MatrixXd::Zero(6,4);

      G(5,0) = dt*car_par.rearWheelRad/car_par.rWIn;

  }
  else if((car_state.flDW_f > -1)&&(car_state.flDW_r == -1)){

      solverDat.Nsys = 6;

      E = Eigen::MatrixXd::Identity(6,6);
      E(5,5) = dt1;

      X = Eigen::VectorXd::Zero(6);
      X(5) = car_state.OmWf;

      if(solverDat.fl_first_call > 0)
      {
        DZDX = Eigen::MatrixXd::Zero(6,6);
        // Z = [lat_r,lon_r,lat_f,lon_f,Nr,Nf]';
        DZDX(2,5)= solverDat.Dlat_fDOmWf;
        DZDX(3,5)= solverDat.Dlon_fDOmWf;
      }

      D = Eigen::VectorXd::Zero(6);
      D(5) = -dt1*car_state_0.OmWf - dt*(car_state.evTrqW_f/car_par.fWIn);

      G = Eigen::MatrixXd::Zero(6,4);
      G(5,2) = dt*car_par.frontWheelRad/car_par.fWIn;
  }
  else{

      solverDat.Nsys = 5;
      E = Eigen::MatrixXd::Identity(5,5);

      X = Eigen::VectorXd::Zero(5);

      if(solverDat.fl_first_call > 0)
      {
        DZDX = Eigen::MatrixXd::Zero(6,5);

      }
      // Z = [lat_r,lon_r,lat_f,lon_f,Nr,Nf]';

      D = Eigen::VectorXd::Zero(5);

      G = Eigen::MatrixXd::Zero(5,4);


  }

  X(0) = car_state.Nr;
  X(1) = car_state.Nf;
  X(2) = car_state.Om;
  X(3) = car_state.Vx;
  X(4) = car_state.Vy;

  if(solverDat.fl_first_call > 0)
  {
    DZDX(0,2)= solverDat.Dlat_rDOm;
    DZDX(0,3)= solverDat.Dlat_rDVx;
    DZDX(0,4)= solverDat.Dlat_rDVy;

    DZDX(1,2)= solverDat.Dlon_rDOm;
    DZDX(1,3)= solverDat.Dlon_rDVx;
    DZDX(1,4)= solverDat.Dlon_rDVy;

    DZDX(2,2)= solverDat.Dlat_fDOm;
    DZDX(2,3)= solverDat.Dlat_fDVx;
    DZDX(2,4)= solverDat.Dlat_fDVy;

    DZDX(3,2)= solverDat.Dlon_fDOm;
    DZDX(3,3)= solverDat.Dlon_fDVx;
    DZDX(3,4)= solverDat.Dlon_fDVy;

    DZDX(4,0) = 1.0;
    DZDX(5,1) = 1.0;
  }

  D(0) = -car_par.mass*9.89*car_par.a/car_par.L;
  D(1) = -car_par.mass*9.89*car_par.b/car_par.L;
  D(2) = -dt1*car_state_0.Om;
  D(3) = -dt1*car_state_0.Vx;
  D(4) = -dt1*car_state_0.Vy;

  double hc = car_par.CGvert/car_par.L;
  G(0,0) = -hc;
  G(0,2) = -hc*cs;
  G(0,3) = hc*sn;
  G(1,0) = hc;
  G(1,2) = hc*cs;
  G(1,3) = -hc*sn;
  double dt_inrt = dt/car_par.moment_inertia;
  G(2,1) = dt_inrt*car_par.b;
  G(2,2) = -dt_inrt*sn*car_par.a;
  G(2,3) = -dt_inrt*cs*car_par.a;
  double dt_mass = (dt/car_par.mass);
  G(3,0) = -dt_mass;
  G(3,2) = -dt_mass*cs;
  G(3,3) = dt_mass*sn;
  G(4,1) = -dt_mass;
  G(4,2) = -dt_mass*sn;
  G(4,3) = -dt_mass*cs;

  E(2,2) = dt1;
  E(3,3) = dt1;
  E(4,4) = dt1;

  solverDat.F = E*X + G*Y + D;

 // solverDat.F = X + G*Y + D;

  car_state.Fdrag = car_par.Cxx*X(3)*X(3);
  if(X(3) < 0.0)
    car_state.Fdrag *= -1.0;

  solverDat.F(3) = solverDat.F(3) + (dt/car_par.mass)*(car_state.Fdrag - car_state.addedFx);
  solverDat.F(4) = solverDat.F(4) - (dt/car_par.mass)*car_state.addedFy;
  solverDat.F(2) = solverDat.F(2) - (dt/car_par.moment_inertia)*car_state.addedN;
  solverDat.F(0) = solverDat.F(0) - hc*car_state.addedFx;
  solverDat.F(1) = solverDat.F(1) + hc*car_state.addedFx;

  solverDat.F(3) = solverDat.F(3) - X(2)*X(4)*dt;
  solverDat.F(4) = solverDat.F(4) + X(2)*X(3)*dt;

  if(solverDat.fl_first_call > 0)
  {
    solverDat.dFdX = E + G*(DYDZ*DZDX);

    solverDat.dFdX(3,2) = solverDat.dFdX(3,2) - X(4)*dt;
    solverDat.dFdX(3,4) = solverDat.dFdX(3,4) - X(2)*dt;
    solverDat.dFdX(4,2) = solverDat.dFdX(4,2) + X(3)*dt;
    solverDat.dFdX(4,3) = solverDat.dFdX(4,3) + X(2)*dt;

    double dFdrag = 2.0*car_par.Cxx*X(3);
    if(X(3) < 0.0)
      dFdrag *= -1.0;

    solverDat.dFdX(3,3) = solverDat.dFdX(3,3) + (dt/car_par.mass)*dFdrag;
  }

  //solverDat.dX.resize(solverDat.Nsys);
  if(false)
    {
      Eigen::VectorXd vec(solverDat.Nsys-2);
      for(int i=2;i<solverDat.Nsys;i++)
        vec(i-2) = X(i)+D(i)/dt1;

      std::cout << sqrt(solverDat.F.transpose()*solverDat.F) << "  1  "
          << sqrt((vec.transpose())*vec)<< " 2  " << dt*(car_state.evTrqW_r/car_par.rWIn)
          << " 3  " << dt*(car_state.evTrqW_f/car_par.fWIn) << std::endl;
    }

  normF = solverDat.F.transpose()*solverDat.F;

  if(fl_dX > 0){

      if(solverDat.fl_first_call > 0)
      {
        solverDat.dX = solverDat.dFdX.partialPivLu().solve(-solverDat.F);
        // solverDat.dX = solverDat.dFdX.colPivHouseholderQr().solve(-solverDat.F);
      }
      else
        solverDat.dX =-solverDat.F;
  }
 //   solverDat.fl_first_call--;

  double normdFdX = 0.0;

  if(solverDat.dX.rows() == solverDat.dFdX.cols())
    normdFdX = 2.0*solverDat.F.transpose()*(solverDat.dFdX*solverDat.dX);

  std::vector<double> out_dat(2,0.0);
  out_dat[0] = normF;
  out_dat[1] = normdFdX;

  return out_dat;


}

//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DynamicCar::treatSingularities(){

 // return;

  double treshsld = 0.0000001;

   if(car_state.flDW_r > -1)
    if(fabs(car_state.OmWr) < treshsld){

       if(car_state.flDW_r < 1){

           car_state.OmWr = 0.0;
           car_state.flDW_r = -1;


       }
       else{

           car_state.OmWr = treshsld;
           if(car_state.evTrqW_r- car_state.FXr*car_par.rearWheelRad < 0.0)
             car_state.OmWr = -treshsld;
         }

    }


   if(car_state.flDW_f > -1)
    if(fabs(car_state.OmWf) < treshsld){

       if(car_state.flDW_f < 1){

           car_state.OmWf = 0.0;
           car_state.flDW_f = -1;


       }
       else{

           car_state.OmWf = treshsld;
           if(car_state.evTrqW_f- car_state.FXf*car_par.frontWheelRad < 0.0)
             car_state.OmWf = -treshsld;
         }

    }


}
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DynamicCar::analyseDissForces(double NmotF,double NmotR,double NBrkF,double NBrkR){

    car_state.flDW_f = 1;
    car_state.flDW_r = 1;

  // Tire Forces (Combined Slip)

    double Vxw;
    double Vyw; // velocity components at the point of front wheel in the inertial coordinate system of front wheel

    double Vxr = car_state.Vx;
    double WVr = car_state.OmWr * car_par.rearWheelRad;
    double Vyr = car_state.Vy - car_state.Om * car_par.b;

    if (culc_par.fl_Long_Circ < 3)
      if ((car_par.drive_distrib > 0.99) && (NBrkR < 0.001))
      {
        WVr = Vxr;
        car_state.OmWr = Vxr / car_par.rearWheelRad;
        NBrkR = 0.0;
         car_state.flDW_r = -1;
      }

    double DWr = WVr - Vxr;

    double normK_r = fabs(WVr) + 0.0001;

    car_state.long_slip_r = DWr / normK_r;

    if (Vxr < 0.0) Vxr = (-1.0) * Vxr;

    car_state.slipAng_r = atan2(Vyr, Vxr);

    double sig_xr = car_par.Cx_r * car_state.long_slip_r;
    double sig_yr = -car_par.Cy_r * Vyr / normK_r;

    double gama_r = sqrt(sig_xr * sig_xr + sig_yr * sig_yr + 0.0001);

    double Fslip_r = brushModel_combined(gama_r, car_state.Nr);

    car_state.FXr = (sig_xr / gama_r) * Fslip_r;
    car_state.FYr = (sig_yr / gama_r) * Fslip_r;

    double Vxf = car_state.Vx;
    double Vyf = car_state.Vy + car_state.Om * car_par.a;

    double cs = cos(car_state.steer_alpha);
    double sn = sin(car_state.steer_alpha);

    Vxw = Vxf * cs + Vyf * sn;
    Vyw = Vyf * cs - Vxf * sn;

    double WVf = car_state.OmWf * car_par.frontWheelRad;

    if (culc_par.fl_Long_Circ < 3)
      if ((car_par.drive_distrib < 0.01) && (NBrkF < 0.001)) {
        WVf = Vxw;
        car_state.OmWf = Vxw / car_par.frontWheelRad;
        NBrkF = 0.0;
        car_state.flDW_f = -1;
      }

    double DWf = WVf - Vxw;

    double normK_f = fabs(WVf) + 0.0001;

    car_state.long_slip_f = DWf / normK_f;

    if (Vxw < 0.0) Vxw = (-1.0) * Vxw;

    car_state.slipAng_f = atan2(Vyw, Vxw);

    double sig_xf = car_par.Cx_f * car_state.long_slip_f;
    double sig_yf = -car_par.Cy_f * Vyw / normK_f;

    double gama_f = sqrt(sig_xf * sig_xf + sig_yf * sig_yf + 0.0001);

    double Fslip_f = brushModel_combined(gama_f, car_state.Nf);

    car_state.FXf = (sig_xf / gama_f) * Fslip_f;
    car_state.FYf = (sig_yf / gama_f) * Fslip_f;

  // Dissipative Forces

    car_state.evTrqW_f = 0.0;
    car_state.evTrqW_r = 0.0;

    if(car_par.drive_distrib < 0.01)
        NBrkR += car_par.rollingResistance*car_par.rearWheelRad;
    else if(car_par.drive_distrib > 0.99)
        NBrkF += car_par.rollingResistance*car_par.frontWheelRad;
    else{

        NBrkR += car_par.rollingResistance*car_par.rearWheelRad*(1.0 - car_par.drive_distrib);
        NBrkF += car_par.rollingResistance*car_par.frontWheelRad*car_par.drive_distrib;
      }

    // Rear

    if(car_state.flDW_r > -1){

      if(fabs(NmotR- car_state.FXr*car_par.rearWheelRad) > NBrkR)
        car_state.flDW_r = 1;
      else
        car_state.flDW_r = 0;

      if(car_state.OmWr > 0.0 || ((car_state.OmWr == 0.0)&&(car_state.Vx > 0.0)))
          car_state.evTrqW_r = NmotR - NBrkR; //- car_state.FXr*car_par.rearWheelRad;
      else if(car_state.OmWr < 0.0 || ((car_state.OmWr == 0.0)&&(car_state.Vx < 0.0)))
          car_state.evTrqW_r = NmotR + NBrkR; // - car_state.FXr*car_par.rearWheelRad;
      else if((car_state.OmWr == 0.0)&&(car_state.Vx == 0.0)){

        if(fabs(NmotR) < NBrkR)
          car_state.flDW_r = -1;
        else{

            if(NmotR > 0.0)
              car_state.evTrqW_r = NmotR - NBrkR;
            else
              car_state.evTrqW_r = NmotR + NBrkR;

        }
      }
    }

    // Front

    if(car_state.flDW_f > -1){

      if(fabs(NmotF- car_state.FXf*car_par.frontWheelRad) > NBrkF)
        car_state.flDW_f = 1;
      else
        car_state.flDW_f = 0;

      if(car_state.OmWf > 0.0 || ((car_state.OmWf == 0.0)&&(Vxw > 0.0)))
          car_state.evTrqW_f = NmotF - NBrkF;// - car_state.FXf*car_par.frontWheelRad;
      else if(car_state.OmWf < 0.0 || ((car_state.OmWf == 0.0)&&(Vxw < 0.0)))
          car_state.evTrqW_f = NmotF + NBrkF;// - car_state.FXf*car_par.frontWheelRad;
      else if((car_state.OmWf == 0.0)&&(Vxw == 0.0)){

        if(fabs(NmotF) < NBrkF)
          car_state.flDW_f = -1;
        else{

            if(NmotF > 0.0)
              car_state.evTrqW_f = NmotF - NBrkF;
            else
              car_state.evTrqW_f = NmotF + NBrkF;

        }
      }
    }

    // AERO Drag

    car_state.Fdrag = car_par.Cxx*car_state.Vx*car_state.Vx;

    if(car_state.Vx < 0.0)
      car_state.Fdrag *= -1.0;




}
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////

//-////////////////////////////////////
double DynamicCar::analize_dt(double t){



    double dt = t;

    if(culc_par.fl_RunKut >= 0)
      return dt;

    if(fl_first_call == 1){

        fl_first_call =0;

        return dt = 0.0000001;
    }


    if(culc_par.fl_RunKut < -1){

        if(car_par.drive_distrib > 0.01)
            return dt;

        if(car_state.OmWr < 0.0)
            return dt;

        if(car_state.Vx < 0.0)
            return dt;

        double VW = car_par.rearWheelRad*car_state.OmWr;
        double AW = car_par.rearWheelRad*car_state.dOmWrdt;

        double f = (-VW*car_state.dVxdt + AW*car_state.Vx + 0.001*(AW - car_state.dVxdt))/((VW +0.001)*(VW + 0.001));

        if(f <= 1.0)
            return dt;

        if(f >= 10000.0)
            dt = 0.000001;
        else
            dt = 0.01/f;

        dt = 0.0000001;
    }
    else{

        double Vxr = car_state.Vx;
        double WVr = car_state.OmWr*car_par.rearWheelRad;
        double Vyr = car_state.Vy - car_state.Om*car_par.b;

        double DWr = WVr - Vxr;

        double normK_r = fabs(WVr) + 0.001;

        double long_slip_r = fabs(DWr/normK_r);


        if(culc_par.fl_Long_Circ < 3)
            if((car_par.drive_distrib > 0.99)&&(fl_AccBrkMode > 0)){

               long_slip_r = 0.0;
            }

        if(Vxr < 0.0)
            Vxr = (-1.0)*Vxr;

        double slipAng_r = fabs(atan2(Vyr,Vxr+0.001));


        double Vxf = car_state.Vx;
        double Vyf = car_state.Vy + car_state.Om*car_par.a;

        double cs = cos(car_state.steer_alpha);
        double sn = sin(car_state.steer_alpha);

        double Vxw = Vxf*cs + Vyf*sn;
        double Vyw = Vyf*cs - Vxf*sn;



        double WVf = car_state.OmWf*car_par.frontWheelRad;



        double DWf = WVf - Vxw;

        double normK_f = fabs(WVf) + 0.001;

        double long_slip_f = fabs(DWf/normK_f);

        if(culc_par.fl_Long_Circ < 3)
            if((car_par.drive_distrib < 0.01)&&(fl_AccBrkMode > 0)){

                long_slip_f = 0.0;
            }


        if(Vxw < 0.0)
            Vxw = (-1.0)*Vxw;

        double slipAng_f = fabs(atan2(Vyw,Vxw + 0.001));

        double lim_angle = 2.0*(M_PI/180.0);

        if(((long_slip_f > lim_angle)||(slipAng_f > lim_angle))||((long_slip_r > lim_angle)||(slipAng_r > lim_angle)))
            dt = 0.0000001;
        else if(fabs(car_state.Vx) < 2.0)
            dt = 0.0000001;


    }

  //  std::cout << " ----------------------- dt correction ---------------------------" << std::endl;

    return dt;


}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::integrNavkinematictest(double dt) {
  // dVxdt = ax;
  // Vy = 0
  // Om = Vx*tan(steer_alpha)/L;
  // dpsidt = Om;
  // dxdt = Vx*cos(psi);
  // dydt = Vx*sin(psi);

  if (culc_par.fl_RunKut < 1) evolutionControl(dt);

  combinedSlip();

  car_state.Om = car_state.Vx * tan(car_state.steer_alpha) / car_par.L;
  car_state.Vy = car_state.Om * car_par.b;
  double kx1 =
      (car_state.Vx * cos(car_state.psi) - car_state.Vy * sin(car_state.psi));
  double ky1 =
      (car_state.Vx * sin(car_state.psi) + car_state.Vy * cos(car_state.psi));
  double kpsi1 = car_state.Om;
  double kVx1 = car_state.accel;

  double x0 = car_state.x;
  double y0 = car_state.y;
  double psi0 = car_state.psi;
  double Vx0 = car_state.Vx;

  if (culc_par.fl_RunKut > 0) {
    car_state.x = x0 + 0.5 * kx1 * dt;
    car_state.y = y0 + 0.5 * ky1 * dt;
    car_state.psi = psi0 + 0.5 * kpsi1 * dt;
    car_state.Vx = Vx0 + 0.5 * kVx1 * dt;

    car_state.dVxdt = car_state.accel;

    evolutionControl(0.5 * dt);

    car_state.Om = car_state.Vx * tan(car_state.steer_alpha) / car_par.L;
    car_state.Vy = car_state.Om * car_par.b;
    double kx2 =
        (car_state.Vx * cos(car_state.psi) - car_state.Vy * sin(car_state.psi));
    double ky2 =
        (car_state.Vx * sin(car_state.psi) + car_state.Vy * cos(car_state.psi));
    double kpsi2 = car_state.Om;
    double kVx2 = car_state.accel;

    car_state.x = x0 + 0.5 * kx2 * dt;
    car_state.y = y0 + 0.5 * ky2 * dt;
    car_state.psi = psi0 + 0.5 * kpsi2 * dt;
    car_state.Vx = Vx0 + 0.5 * kVx2 * dt;

    car_state.dVxdt = car_state.accel;

    // actuators have the same state (t+0.5*dt)

    car_state.Om = car_state.Vx * tan(car_state.steer_alpha) / car_par.L;
    car_state.Vy = car_state.Om * car_par.b;
    double kx3 =
        (car_state.Vx * cos(car_state.psi) - car_state.Vy * sin(car_state.psi));
    double ky3 =
        (car_state.Vx * sin(car_state.psi) + car_state.Vy * cos(car_state.psi));
    double kpsi3 = car_state.Om;
    double kVx3 = car_state.accel;

    car_state.x = x0 + 0.5 * kx3 * dt;
    car_state.y = y0 + 0.5 * ky3 * dt;
    car_state.psi = psi0 + 0.5 * kpsi3 * dt;
    car_state.Vx = Vx0 + 0.5 * kVx3 * dt;

    car_state.dVxdt = car_state.accel;

    evolutionControl(0.5 * dt);  // actuators have changed in corresponding to
                                 // changing time from t to t + dt
    combinedSlip();

    car_state.Om = car_state.Vx * tan(car_state.steer_alpha) / car_par.L;
    car_state.Vy = car_state.Om * car_par.b;
    double kx4 =
        (car_state.Vx * cos(car_state.psi) - car_state.Vy * sin(car_state.psi));
    double ky4 =
        (car_state.Vx * sin(car_state.psi) + car_state.Vy * cos(car_state.psi));
    double kpsi4 = car_state.Om;
    double kVx4 = car_state.accel;
    // Runge modifications:

    kx1 = (kx1 + 2 * kx2 + 2 * kx3 + kx4) / 6.0;
    ky1 = (ky1 + 2 * ky2 + 2 * ky3 + ky4) / 6.0;
    kpsi1 = (kpsi1 + 2 * kpsi2 + 2 * kpsi3 + kpsi4) / 6.0;
    kVx1 = (kVx1 + 2 * kVx2 + 2 * kVx3 + kVx4) / 6.0;
  }

  car_state.x = x0 + kx1 * dt;
  car_state.y = y0 + ky1 * dt;
  car_state.psi = psi0 + kpsi1 * dt;
  car_state.Vx = Vx0 + kVx1 * dt;
  car_state.Om = car_state.Vx * tan(car_state.steer_alpha) / car_par.L;
  car_state.Vy = car_state.Om * car_par.b;

  car_state.dVxdt = kVx1;

  if (((car_state.Vx < 0.0) && (fl_reverse == 0)) ||
      ((car_state.Vx > 0.0) && (fl_reverse > 0))) {
    car_state.x = x0;
    car_state.y = y0;
    car_state.psi = psi0;
    car_state.Vx = 0.0;
    car_state.Vy = 0.0;
    car_state.Om = 0.0;

    car_state.dVxdt = 0.0;
  }

  nav_to_cog(1);
}
//-------------------------------------------------------------------------------------------------------------------
//-///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-////////////////////////////////////
void DynamicCar::nav_to_cog(int Fl_reverse) {
  double cs = cos(car_state.psi);
  double sn = sin(car_state.psi);

  if (Fl_reverse == 0) {
    car_state.x = car_state.xc + car_par.b * cs;
    car_state.y = car_state.yc + car_par.b * sn;

  } else {
    car_state.xc = car_state.x - car_par.b * cs;
    car_state.yc = car_state.y - car_par.b * sn;
  }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//-/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//---/////////////////////////////////////////////////
void DynamicCar::init_nav(double x, double y, double psi, double v) {
  car_state.x = x;
  car_state.y = y;
  car_state.psi = psi;
  car_state.Vx = v;

  nav_to_cog(1);

  //---------------

  car_state.OmWf = car_state.Vx / car_par.frontWheelRad;
  car_state.OmWr = car_state.Vx / car_par.rearWheelRad;

  if (fabs(car_state.Vx) > 0.00001) car_state.state = 1;

  car_state.initialized += 1;

  car_state.Nr = 9.89*car_par.mass*car_par.a/car_par.L;
  car_state.Nf = 9.89*car_par.mass*car_par.b/car_par.L;


  //  std::cout << "V0 = " << car_state.Vx << std::endl;
}

void DynamicCar::reset_state() {
	fl_reverse = 0;
	car_state.state = 0;
	car_state.xc = 0;
	car_state.yc = 0;
	car_state.psi = 0;
	nav_to_cog(0);
	car_state.Vx = 0.0;
	car_state.Vy = 0.0;
	car_state.accel = 0.0;
	car_state.steer_alpha = 0.0;
	car_state.dVxdt = 0.0;
	//   car_state.dVydt =  0.0;
	//   car_state.dOmdt =  0.0;
	car_state.slipAng_f = 0.0;
	car_state.slipAng_r = 0.0;
	car_state.FYf = 0.0;
	car_state.FYr = 0.0;
	car_state.FXf = 0.0;
	car_state.FXr = 0.0;
	car_state.FXf_req = 0.0;
	car_state.FXr_req = 0.0;
	car_state.B_pressure = 0.0;
	car_state.B_pressure_req = 0.0;
	car_state.OmWf = 0.0;
	car_state.OmWr = 0.0;
	car_state.long_slip_f = 0.0;
	car_state.long_slip_r = 0.0;
	car_state.callcount = 0;
	car_state.Nr = 9.89*car_par.mass*car_par.a/car_par.L;
	car_state.Nf = 9.89*car_par.mass*car_par.b/car_par.L;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// /*

static DynamicCar dcar(Eigen::Vector3d::Zero(3));
static DynamicPar *dynpar_pt;

extern "C" void initModel(double x, double y, double psi, double v ) {
  dynpar_pt = new DynamicPar();
  dcar.culc_par = dynpar_pt->culc_par;
  dcar.car_par = dynpar_pt->car_par;
  dcar.reset_state();
  dcar.init_nav(x, y, psi, v);

  //dcar.car_par.Om_nullTorque = 201.0;
  //std::cout << "initModel(): " << dcar.car_par.Om_nullTorque << std::endl;

  std::cout << "Vehicle initModel() with params: " << std::endl;

  //output parameters
  std::cout << "fl_drag_taking: " << dcar.culc_par.fl_drag_taking << std::endl;
  std::cout << "fl_Long_Circ: " << dcar.culc_par.fl_Long_Circ << std::endl;
  std::cout << "fl_static_load: " << dcar.culc_par.fl_static_load << std::endl;
  std::cout << "fl_RunKut: " << dcar.culc_par.fl_RunKut << std::endl;
  std::cout << "implicit_solver: " << dcar.culc_par.implicit_solver << std::endl;
  std::cout << "num_impl_nonlin_it: " << dcar.culc_par.num_impl_nonlin_it << std::endl;
  std::cout << "nsteps: " << dcar.culc_par.nsteps << std::endl;
  std::cout << "fl_act_low: " << dcar.culc_par.fl_act_low << std::endl;
  std::cout << "a: " << dcar.car_par.a << std::endl;
  std::cout << "b: " << dcar.car_par.b << std::endl;
  std::cout << "L: " << dcar.car_par.L << std::endl;
  std::cout << "max_steer: " << dcar.car_par.max_steer << std::endl;
  std::cout << "mu: " << dcar.car_par.mu << std::endl;
  std::cout << "max_accel: " << dcar.car_par.max_accel << std::endl;
  std::cout << "CGvert: " << dcar.car_par.CGvert << std::endl;
  std::cout << "mass: " << dcar.car_par.mass << std::endl;
  std::cout << "moment_inertia: " << dcar.car_par.moment_inertia << std::endl;
  std::cout << "Cy_f: " << dcar.car_par.Cy_f << std::endl;
  std::cout << "Cy_r: " << dcar.car_par.Cy_r << std::endl;
  std::cout << "Cx_f: " << dcar.car_par.Cx_f << std::endl;
  std::cout << "Cx_r: " << dcar.car_par.Cx_r << std::endl;
  std::cout << "drive_distrib: " << dcar.car_par.drive_distrib << std::endl;
  std::cout << "frontWheelRad: " << dcar.car_par.frontWheelRad << std::endl;
  std::cout << "rearWheelRad: " << dcar.car_par.rearWheelRad << std::endl;
  std::cout << "maxPowerEngine: " << dcar.car_par.maxPowerEngine << std::endl;
  std::cout << "maxTorqueEngine: " << dcar.car_par.maxTorqueEngine << std::endl;
  std::cout << "n_engines: " << dcar.car_par.n_engines << std::endl;
  std::cout << "maxBrakePressure: " << dcar.car_par.maxBrakePressure << std::endl;
  std::cout << "maxGradBrakePressure: " << dcar.car_par.maxGradBrakePressure << std::endl;
  std::cout << "kBrake_f: " << dcar.car_par.kBrake_f << std::endl;
  std::cout << "kBrake_r: " << dcar.car_par.kBrake_r << std::endl;
  std::cout << "n_brakes: " << dcar.car_par.n_brakes << std::endl;
  std::cout << "maxGradSteer: " << dcar.car_par.maxGradSteer << std::endl;
  std::cout << "rollingResistance: " << dcar.car_par.rollingResistance << std::endl;
  std::cout << "rollingResistanceSpeedDepended: " << dcar.car_par.rollingResistanceSpeedDepended << std::endl;
  std::cout << "Cxx: " << dcar.car_par.Cxx << std::endl;
  std::cout << "fWIn: " << dcar.car_par.fWIn << std::endl;
  std::cout << "rWIn: " << dcar.car_par.rWIn << std::endl;
 }

extern "C" void stepFunc1(double steer, double accel_des, double dt,
			 int reverse_fl, double speed_des, double X_init, double Y_init, double Psi_init, double Vx_init,
						CarParameters *__params, int VehReset, car_state_t* __dcar) {
  double ax = accel_des / dcar.car_par.mass;

  if ((!dcar.car_state.initialized) | (VehReset))  {
      initModel(X_init, Y_init, Psi_init, Vx_init);
      mySetParamFunction(__params);
  }



  dcar.dynamicNav_ev(steer, ax, dt, reverse_fl, 0.0001);
  *__dcar = dcar.car_state;
  // std::cout << " X: " << dcar.car_state.x << std::endl;
  //  __dcar->lat_slip = (__dcar->slipAng_r + __dcar->slipAng_f) / 2.0;
}


extern "C" void myTerminateFunction() {
	if (dynpar_pt)
	{
		free(dynpar_pt);
		std::cout << "Vehicle terminated " << std::endl;
	}
}

extern "C" void mySetParamFunction(CarParameters *new_par) {
	dcar.car_par = *new_par;

	std::cout << "Setting new car params: " << std::endl;
	//output parameters
	std::cout << "a: " << dcar.car_par.a << std::endl;
	std::cout << "b: " << dcar.car_par.b << std::endl;
	std::cout << "L: " << dcar.car_par.L << std::endl;
	std::cout << "max_steer: " << dcar.car_par.max_steer << std::endl;
	std::cout << "mu: " << dcar.car_par.mu << std::endl;
	std::cout << "max_accel: " << dcar.car_par.max_accel << std::endl;
	std::cout << "CGvert: " << dcar.car_par.CGvert << std::endl;
	std::cout << "mass: " << dcar.car_par.mass << std::endl;
	std::cout << "moment_inertia: " << dcar.car_par.moment_inertia << std::endl;
	std::cout << "Cy_f: " << dcar.car_par.Cy_f << std::endl;
	std::cout << "Cy_r: " << dcar.car_par.Cy_r << std::endl;
	std::cout << "Cx_f: " << dcar.car_par.Cx_f << std::endl;
	std::cout << "Cx_r: " << dcar.car_par.Cx_r << std::endl;
	std::cout << "drive_distrib: " << dcar.car_par.drive_distrib << std::endl;
	std::cout << "frontWheelRad: " << dcar.car_par.frontWheelRad << std::endl;
	std::cout << "rearWheelRad: " << dcar.car_par.rearWheelRad << std::endl;
	std::cout << "maxPowerEngine: " << dcar.car_par.maxPowerEngine << std::endl;
	std::cout << "maxTorqueEngine: " << dcar.car_par.maxTorqueEngine << std::endl;
	std::cout << "n_engines: " << dcar.car_par.n_engines << std::endl;
	std::cout << "maxBrakePressure: " << dcar.car_par.maxBrakePressure << std::endl;
	std::cout << "maxGradBrakePressure: " << dcar.car_par.maxGradBrakePressure << std::endl;
	std::cout << "kBrake_f: " << dcar.car_par.kBrake_f << std::endl;
	std::cout << "kBrake_r: " << dcar.car_par.kBrake_r << std::endl;
	std::cout << "n_brakes: " << dcar.car_par.n_brakes << std::endl;
	std::cout << "maxGradSteer: " << dcar.car_par.maxGradSteer << std::endl;
	std::cout << "rollingResistance: " << dcar.car_par.rollingResistance << std::endl;
	std::cout << "rollingResistanceSpeedDepended: " << dcar.car_par.rollingResistanceSpeedDepended << std::endl;
	std::cout << "Cxx: " << dcar.car_par.Cxx << std::endl;
	std::cout << "fWIn: " << dcar.car_par.fWIn << std::endl;
	std::cout << "rWIn: " << dcar.car_par.rWIn << std::endl;
}

// */
