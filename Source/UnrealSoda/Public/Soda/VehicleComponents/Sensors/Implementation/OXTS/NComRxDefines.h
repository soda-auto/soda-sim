//
// Created by slovak on 7/19/16.
//

#ifndef PROJECT_NCOMRXDEFINES_H
#define PROJECT_NCOMRXDEFINES_H

//============================================================================================================
// Definitions.

// General constants.
#define EARTH_RADIUS        (6371000)       //!< Approximate radius of earth.
#define NOUTPUT_PACKET_LENGTH  (72)       //!< NCom packet length.
#define NCOM_SYNC           (0xE7)        //!< NCom sync byte.
#define PKT_PERIOD          (0.01)        //!< 10ms updates.
#define TIME2SEC            (1e-3)        //!< Units of 1 ms.
#define FINETIME2SEC        (4e-6)        //!< Units of 4 us.
#define TIMECYCLE           (60000)       //!< Units of TIME2SEC (i.e. 60 seconds).
#define WEEK2CYCLES         (10080)       //!< Time cycles in a week.
#define ACC2MPS2            (1e-4)        //!< Units of 0.1 mm/s^2.
#define RATE2RPS            (1e-5)        //!< Units of 0.01 mrad/s.
#define VEL2MPS             (1e-4)        //!< Units of 0.1 mm/s.
#define ANG2RAD             (1e-6)        //!< Units of 0.001 mrad.
#define INNFACTOR           (0.1)         //!< Resolution of 0.1.
#define POSA2M              (1e-3)        //!< Units of 1 mm.
#define VELA2MPS            (1e-3)        //!< Units of 1 mm/s.
#define ANGA2RAD            (1e-5)        //!< Units of 0.01 mrad.
#define GB2RPS              (5e-6)        //!< Units of 0.005 mrad/s.
#define AB2MPS2             (1e-4)        //!< Units of 0.1 mm/s^2.
#define GSFACTOR            (1e-6)        //!< Units of 1 ppm.
#define ASFACTOR            (1e-6)        //!< Units of 1 ppm.
#define GBA2RPS             (1e-6)        //!< Units of 0.001 mrad/s.
#define ABA2MPS2            (1e-5)        //!< Units of 0.01 mm/s^2.
#define GSAFACTOR           (1e-6)        //!< Units of 1 ppm.
#define ASAFACTOR           (1e-6)        //!< Units of 1 ppm.
#define GPSPOS2M            (1e-3)        //!< Units of 1 mm.
#define GPSATT2RAD          (1e-4)        //!< Units of 0.1 mrad.
#define GPSPOSA2M           (1e-4)        //!< Units of 0.1 mm.
#define GPSATTA2RAD         (1e-5)        //!< Units of 0.01 mrad.
#define INNFACTOR           (0.1)         //!< Resolution of 0.1.
#define DIFFAGE2SEC         (1e-2)        //!< Units of 0.01 s.
#define REFPOS2M            (0.0012)      //!< Units of 1.2 mm.
#define REFANG2RAD          (1e-4)        //!< Units of 0.1 mrad.
#define OUTPOS2M            (1e-3)        //!< Units of 1 mm.
#define ZVPOS2M             (1e-3)        //!< Units of 1 mm.
#define ZVPOSA2M            (1e-4)        //!< Units of 0.1 mm.
#define NSPOS2M             (1e-3)        //!< Units of 1 mm.
#define NSPOSA2M            (1e-4)        //!< Units of 0.1 mm.
#define ALIGN2RAD           (1e-4)        //!< Units of 0.1 mrad.
#define ALIGNA2RAD          (1e-5)        //!< Units of 0.01 mrad.
#define SZVDELAY2S          (1.0)         //!< Units of 1.0 s.
#define SZVPERIOD2S         (0.1)         //!< Units of 0.1 s.
#define TOPSPEED2MPS        (0.5)         //!< Units of 0.5 m/s.
#define NSDELAY2S           (0.1)         //!< Units of 0.1 s.
#define NSPERIOD2S          (0.02)        //!< Units of 0.02 s.
#define NSACCEL2MPS2        (0.04)        //!< Units of 0.04 m/s^2.
#define NSSPEED2MPS         (0.1)         //!< Units of 0.1 m/s.
#define NSRADIUS2M          (0.5)         //!< Units of 0.5 m.
#define INITSPEED2MPS       (0.1)         //!< Units of 0.1 m/s.
#define HLDELAY2S           (1.0)         //!< Units of 1.0 s.
#define HLPERIOD2S          (0.1)         //!< Units of 0.1 s.
#define STATDELAY2S         (1.0)         //!< Units of 1.0 s.
#define STATSPEED2MPS       (0.01)        //!< Units of 1.0 cm/s.
#define WSPOS2M             (1e-3)        //!< Units of 1 mm.
#define WSPOSA2M            (1e-4)        //!< Units of 0.1 mm.
#define WSSF2PPM            (0.1)         //!< Units of 0.1 pulse per metre (ppm).
#define WSSFA2PC            (0.002)       //!< Units of 0.002% of scale factor.
#define WSDELAY2S           (0.1)         //!< Units of 0.1 s.
#define WSNOISE2CNT         (0.1)         //!< Units of 0.1 count for wheel speed noise.
#define UNDUL2M             (0.005)       //!< Units of 5 mm.
#define DOPFACTOR           (0.1)         //!< Resolution of 0.1.
#define OMNISTAR_MIN_FREQ   (1.52e9)      //!< (Hz) i.e. 1520.0 MHz.
#define OMNIFREQ2HZ         (1000.0)      //!< Resolution of 1 kHz.
#define SNR2DB              (0.2)         //!< Resolution of 0.2 dB.
#define LTIME2SEC           (1.0)         //!< Resolution of 1.0 s.
#define TEMPK_OFFSET        (203.15)      //!< Temperature offset in degrees K.
#define ABSZERO_TEMPC       (-273.15)     //!< Absolute zero (i.e. 0 deg K) in deg C.

// For more accurate and complete local coordinates
#define FINEANG2RAD (1.74532925199433e-9) //!< Units of 0.1 udeg.
#define ALT2M               (1e-3)        //!< Units of 1 mm.

// For GPS supply voltage
#define SUPPLYV2V           (0.1)         //!< Units of 0.1 V.

// Mathematical constant definitions
#ifndef M_PI
#define M_PI (3.1415926535897932384626433832795)  //!< Pi.
#endif
#define DEG2RAD             (M_PI/180.0)  //!< Convert degrees to radians.
#define RAD2DEG             (180.0/M_PI)  //!< Convert radians to degrees.
#define POS_INT_24          (8388607)     //!< Maximum value of a two's complement 24 bit integer.
#define NEG_INT_24          (-8388607)    //!< Minimum value of a two's complement 24 bit integer.
#define INV_INT_24          (-8388608)    //!< Represents an invalid two's complement 24 bit integer.

#define NCOM_COUNT_TOO_OLD  (150)         //!< Cycle counter for data too old.
#define NCOM_STDCNT_MAX     (0xFF)        //!< Definition for the RTBNS accuracy counter.
#define MIN_HORZ_SPEED      (0.07)        //!< 0.07 m/s hold distance.
#define MIN_VERT_SPEED      (0.07)        //!< 0.07 m/s hold distance.
#define SPEED_HOLD_FACTOR   (2.0)         //!< Hold distance when speed within 2 sigma of 0.
#define MINUTES_IN_WEEK     (10080)       //!< Number of minutes in a week.

#endif //PROJECT_NCOMRXDEFINES_H
