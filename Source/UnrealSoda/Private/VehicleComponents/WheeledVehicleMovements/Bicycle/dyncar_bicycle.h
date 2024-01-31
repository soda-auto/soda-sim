// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#ifndef DYNCAR_BICYCLE_H
#define DYNCAR_BICYCLE_H

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
//#define MEX_DEBUG_PRINTF
#include <math.h>

//#include <fstream>//+++++++++++++++++++++++++++
//#include <iostream>//+++++++++++++++++++++++++++++

#ifdef __cplusplus //================================
extern "C" {       //================================
#endif             //===============================

typedef struct __car_state {

  int state;   // is equal 1 when car is moving ; state = 0 when object DynamicCar being created

  double psi;  // yaw of the car // in world coordinate system
  double x;  //
  double y;  // x,y of the Center of Gravitaty (CG) // in world coordinate system

  double xc; // analogious coodinates x,y for Center of Rear axe of car (CR)
  double yc;  // in world coordinate system

  double Vx;   //Velocity of CG in coordinate system related to car:OX pointing along central axe of the car; OY pointing to left
  double Vy;  // So, Vx - longitudal speed, Vy - lateral speed of the car

  double Vdes; // Desired longitudal speed from Path Matcher (Control) // (only for logging)

  double Om;  // angular velocity of car in world coordinate system

  double dVxdt; // d(Vx)/dt
  double dVydt; // d(Vy)/dt

  double dOmdt; // d(Om)/dt

  double OmWr; // rotational speed of rear wheel
  double OmWf; // rotational speed of forward wheel

  double dOmWfdt; // d(OmWf)/dt
  double dOmWrdt; // d(OmWr)/dt

  // Diagnosis
  int callcount;
  int initialized;

  // Control data
  double steer_alpha_req; // required steering angle from Control
  double steer_alpha;  // steering angle (angle of turning of forward wheel)
  double accel;        // desired acceleration (decceleration) for car from Control


  //Forces

  double Nr;// Load on rear wheel
  double Nf;// Load on forward wheel

  double Fdrag; // drag force acting on car
  double Fdrag_PM; // evaluated drag force to have to be added to massa*accel force which is requested by Control

  double FXreq; // whole force which being requested from engines/brakes (= massa*accel + Fdrag_PM)

  double FXr_act;  // Requested Force for rear engines (actuators)
  double FXf_act;  // requested force for forward engines (distributed from Fxreq in correspondence to drive distribution)

  double FXr_req;  // Force for rear wheel (from engines and brakes)
  double FXf_req;  // force for forward wheel // These forces are requested from tyres in longitudal directional

  double FYr_req;  // these forces    are requested   // rear
  double FYf_req;  // from tyres in lateral direction // forward

  double FXr;      // real forces acting on tyres // rear
  double FXf;      // in longitudinal direction    // forward

  double FYr; // real forces acting on tyres // rear
  double FYf; // in lateral direction        // forward

  // brakes: Fbrake = car_par.n_brakes * car_state.B_pressure *(
  //                      (car_par.kBrake_f / car_par.frontWheelRad) +  (car_par.kBrake_f / car_par.frontWheelRad) )
  //

  double B_pressure; // Pressure for brakes
  double B_pressure_req; // Requested pressure

  double Nbrake_f;
  double Nbrake_r;
  double Nmot_f;
  double Nmot_r;

  double addedFx;
  double addedFy;
  double addedN;

  // wheels

  double slipAng_r; // lateral slip // rear
  double slipAng_f; // angles       // forward

  double long_slip_r; // longitudal slip // rear
  double long_slip_f; // angles          // forward

  double evTrqW_r;
  double evTrqW_f;

  int flDW_r;
  int flDW_f;

  double lat_slip;

} car_state_t;
typedef struct __CarParameters {
                //--- kinematic ----//
                double a;
                double b;
                double L;
                double max_steer;

                // --- corrected ------/
                double mu;
                double max_accel;  // should be bounded by mu*g

                //-- Dynamic ----------/
                double drive_distrib;  //  0 - RWD, 1 - FWD, 0.5 - AWD etc.
                double mass;
                double moment_inertia;
                double CGvert;  // high of CG

                // stiftness of tyres in lateral direction (for 2 wheels):
                double Cy_r;
                double Cy_f;

                // stiftness of tyres in longitudal direction (for 2 wheels):
                double Cx_r;
                double Cx_f;

                // actuators -----------/

                double rearWheelRad;
                double frontWheelRad;    // m
                double maxPowerEngine;   // for one engine, watt
                double maxTorqueEngine;  // for one engine, on wheels, N*m
                double n_engines;

                // torque curve
                double Om_maxTorque;
                double Om_nullTorque;         // for wheels
                double maxBrakePressure;      // Bar
                double maxGradBrakePressure;  // Bar/s
                double kBrake_f;              // N*m/Bar
                double kBrake_r;              //
                double n_brakes;
                double maxGradSteer;  // rad/s

                //------------------/

                // drag parameters --/

                double rollingResistance;               // [N] Rolling resistance //
                double rollingResistanceSpeedDepended;  // [N*s/m] Rolling resistance
                double Cxx;                             // Coefficient of aero drag

                // -- in old PM:
                // turningDrag   = m*b/(a+b)*abs(yawRate*vxCG)*abs(tan(delta));
                // aeroDrag      = C_xx * vxCG * vxCG;
                // inclinedDrag  = -m*g*sin(grade);  %negative sign due to GPS convention for
                // grade FxDrag = turningDrag + rollingResistance + aeroDrag + inclinedDrag;
                //-------------------/

                //-----------------/

                double fWIn;  // moment of inertia fo front train
                double rWIn;  // -''- rear
        }CarParameters;

#ifdef __cplusplus   //========================
};                   //======================
#endif              //=======================

#ifdef __cplusplus  //=======================

#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <Eigen/Eigen>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

// ------ Control structures
// --------------------------------------------------------------------------------------------//

struct CulcParameters {

  int fl_drag_taking; // if 1 then drag forces being taked into account
  int fl_Long_Circ;  // 0 || 1 || 2 ; 2 is default value (combined slip mode)
  int fl_static_load; // 1 for static case of loads on rear and forward wheel
  int fl_RunKut; // 1 for Runge Kutta integration; -1 for Local Grid Refinement (more expensive) // 0 is default (Eiler)
  int nsteps; // sampling for integration on [t,t+dt] interval // 400 is default for Eiler
  int fl_act_low; // Lawes for actuators;// 2 is default

  int implicit_solver;
  int num_impl_nonlin_it;
  int corr_impl_step;

};
/*
struct CarParameters {
  //--- kinematic ----//

  double a; // distance from CG to forward wheel
  double b; // distance from CG to rear wheel
  double L; // wheel base (= a+b)


  double max_steer; // max turning angle for forward wheel

  double max_accel;  // maximal value of requested acceleration/decceleration

  //-- Dynamic ----------/
  double drive_distrib;  //  drive distribution: 0 - RWD, 1 - FWD, 0.5 - AWD etc.
  double mass;           // mass of car
  double moment_inertia; // yaw moment of inertia for car
  double CGvert;         // high of CG
  double mu;             // mu (friction coefficient)

  // stiftness of tyres in lateral direction (for 2 wheels):
  double Cy_r; // rear
  double Cy_f; // forward

  // stiftness of tyres in longitudal direction (for 2 wheels):
  double Cx_r; // rear
  double Cx_f; // forward

  // actuators -----------/

  double rearWheelRad; // radiuses of // rear
  double frontWheelRad; // wheels,m   // forward

  double maxPowerEngine;   // for one engine, watt
  double maxTorqueEngine;  // for one engine, on wheels, N*m
  double n_engines;        // number of engines for rear and forward wheels (this numbers are the same for rear and forward wheel)

  // torque curve
  double Om_maxTorque; // parameters of            //
  double Om_nullTorque; // torque curve for engines//

  // Brakes parameters:
  double maxBrakePressure;      // Bar
  double maxGradBrakePressure;  // Bar/s
  double kBrake_f;              // N*m/Bar
  double kBrake_r;              //
  double n_brakes;


  double maxGradSteer;  // rad/s // speed of turning of forward wheel (speed of steering changes)

  //------------------/

  // drag parameters --/

  double rollingResistance;               // [N] Rolling resistance //
  double rollingResistanceSpeedDepended;  // [N*s/m] Rolling resistance
  double Cxx;                             // Coefficient of aero drag

  // -- in old PM:
  // turningDrag   = m*b/(a+b)*abs(yawRate*vxCG)*abs(tan(delta));
  // aeroDrag      = C_xx * vxCG * vxCG;
  // inclinedDrag  = -m*g*sin(grade);  %negative sign due to GPS convention for
  // grade FxDrag = turningDrag + rollingResistance + aeroDrag + inclinedDrag;
  //-------------------/

  //-----------------/

  double fWIn;  // moment of inertia fo front train
  double rWIn;  // -''- rear
};
*/
struct PM_data{

    struct {

        int ind_tr;

        double s;

        double e;

        double psi_des;

        double curve;

        std::vector<double> xy_des;

        double speed_des;

        double accel_des;

    } des_state;

    double dpsi;

    double steer_FBFFW;

    double accel_FBFFW;
};

struct ImplSolverData{

    Eigen::VectorXd F;

    Eigen::MatrixXd dFdX;

    Eigen::VectorXd dX;

    int Nsys;

    int fl_first_call;

    double DFX_rDlat;
    double DFX_rDlon;
    double DFX_rDN;
    double DFY_rDlat;
    double DFY_rDlon;
    double DFY_rDN;

    double DFX_fDlat;
    double DFX_fDlon;
    double DFX_fDN;
    double DFY_fDlat;
    double DFY_fDlon;
    double DFY_fDN;

    double Dlat_rDOm;
    double Dlat_rDVx;
    double Dlat_rDVy;
    double Dlat_rDOmWr;

    double Dlon_rDOm;
    double Dlon_rDVx;
    double Dlon_rDVy;
    double Dlon_rDOmWr;

    double Dlat_fDOm;
    double Dlat_fDVx;
    double Dlat_fDVy;
    double Dlat_fDOmWf;

    double Dlon_fDOm;
    double Dlon_fDVx;
    double Dlon_fDVy;
    double Dlon_fDOmWf;
};

struct DynamicPar {

  CulcParameters culc_par;

  CarParameters car_par;

  int fl_Dynamic;    //  not used
  int fl_Dynamic_PM; // not used

  DynamicPar() {
    fl_Dynamic = 1;
    fl_Dynamic_PM = 1;

    // Parameters of calculations
    culc_par.fl_drag_taking = 1;
    culc_par.fl_Long_Circ = 3;
    culc_par.fl_static_load = 0;
    culc_par.fl_RunKut = 0;
    culc_par.nsteps = 10;
    culc_par.fl_act_low = 2;

    culc_par.implicit_solver = 1;
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
  }
};

//----- Dynamic Car
//---------------------------------------------------------------------------------------------------//

class DynamicCar {
 protected:
  void newFXreq();

  void getDrag();

  void getVerticalLoads();

  void newLongForce();

  void newLatForce();

//  void analize_Fdrag(double& dVxdt);

  void evolutionControl(double dt);

  void combinedSlip();

  double brushModel_combined(double slip_angle, double Fz);
  double brushModel_combined_diff(double slip_angle, double Fz);
  double brushModel_combined_diff_N(double slip_angle, double Fz);

  void integrNav(double dt);

  void integrNavkinematictest(double dt);

  void addToMPClog(double dtk);

  void addToLog(double dt);

  double analize_dt(double t);

  void evalTireDissForces(double NmotF,double NmotR,double NBrkF,double NBrkR);
  void analyseDissForces(double NmotF,double NmotR,double NBrkF,double NBrkR);

  void treatSingularities();
  std::vector<double> linearizatedSystem(double NmotF, double NmotR, double NBrkF, double NBrkR, double dt, int fl_dX = 1);
  std::vector<double> updateState(double step, double dt, double NmotF, double NmotR, double NBrkF, double NBrkR, int fl_r, int fl_f, int fl = 0);
  double updStep(double t0, double t1, double f0, double f1, double df0, double df1, bool &fl);

  ImplSolverData solverDat;

  void evWheel(double dt,double omW_0,double&omW, double Trq, int fl_Trq,double Iin);

  void integrNavBase(double dt, double NmotF, double NmotR, double NBrkF, double NBrkR);
  void integrNavBase_implicit(double dt, double NmotF, double NmotR, double NBrkF, double NBrkR);

 public:
  DynamicCar(Eigen::Vector3d init_navig);

  DynamicCar(DynamicPar& params, double x, double y, double psi, double v = 0.0);

  virtual ~DynamicCar();

  int fl_first_call;
  int fl_first_it;

  double in_time;
  double d_time;
  int fl_fileDyn;

  int fl_out;

  double timeMPClog;
  int fl_MPClog;

//  std::ofstream fileDyn; // ++++++++++++++++++++++++++++++++ (!)

//  std::ofstream fileDynPM;

//  std::ofstream fileMPClog; // ++++++++++++++++++++++++++++++++ (!)


  void dynamicNav_ev(double steer, double accel_des, double dt, int reverse_fl,
                     double speed_des,double addFx = 0.0,double addFy = 0.0,
                     double addN = 0.0);

  void dynamicNav_ev_Base(double dt,double steer,double NmotF,double NmotR,double NBrkF,double NBrkR,double addFx = 0.0,double addFy = 0.0,
          double addN = 0.0);

  void dynamicNav_ev_Base_test(double steer, double accel_des, double dt,
                                 int reverse_fl, double speed_des);

  void init_nav(double x, double y, double psi, double v);
  void reset_state();

  int fl_reverse;
  double fl_AccBrkMode;
  double fl_AccBrk_fact;
  //-----------------------------

  void nav_to_cog(int fl_reverse);

  void getAccBrkMode();
  void getDrag_PM();

  double brushModel_solver(double FY, int fl_forwardWheel, int& fl_OK);

  double brushModel(double slip_angle, double Fz, double Ctyre);

  void getLatSlip();

  //-----------------------------

  CulcParameters culc_par;
  CarParameters car_par;

  car_state_t car_state;

  car_state_t car_state_0;

   PM_data current_PM_data;
};

// /*
#endif  // __cplusplus__

// /*
#ifdef __cplusplus
extern "C" {
#endif
void initModel(double x, double y, double psi, double v);
// void stepFunc(double steer, double accel_des, double dt, int reverse_fl,
//               double speed_des, car_state_t* __dcar);
void stepFunc1(double steer, double accel_des, double dt,
                int reverse_fl, double speed_des, double X_init, double Y_init, double Psi_init, double Vx_init,
                CarParameters *__params, int VehReset, car_state_t* __dcar);
void myTerminateFunction();
void mySetParamFunction(CarParameters *new_par);
#ifdef __cplusplus
};
#endif
// */ //==========================================

#endif  // DYNCAR_BICYCLE_H
