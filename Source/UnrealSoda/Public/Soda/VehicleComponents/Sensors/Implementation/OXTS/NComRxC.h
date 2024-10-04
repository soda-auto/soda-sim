//============================================================================================================
//!
//! \file NComRxC.h
//!
//! \brief NCom C decoder header.
//!
//============================================================================================================


#ifndef NCOMRXC_H
#define NCOMRXC_H


#define NCOMRXC_DEV_ID "111026"  //!< Development Identification.


//============================================================================================================
// Provide multiple compiler support for standard integer types for 8, 16, 32, and 64 bit quantities.
//
// Wish to use standard integer types for 8, 16, 32, and 64 bit quantities. Not all compilers support these
// types. Our aim is to use (u)intN_t for low level byte conversions and counters, i.e. places where the size
// of the type is very important. In more general code the regular types are used, where it is assumed that
// {char, short, int, long long} <--> {8, 16, 32, 64} bit types for both 32 and 64 bit x86 systems.
//
// For x86 systems long can either be 32 or 64 bits so its (direct) use is avoided. The definitions below
// should be modified according to compiler, but with preference to include cstdint for C++ or stdint.h for
// C if provided.
//
// It seems that MSVC does not support the usage of 8 bit integers in printf.


#define __STDC_LIMIT_MACROS
#define	__STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <cstdint>
#include <cinttypes>
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;


//============================================================================================================
//! \brief Matrix element definition allows an easy change from float to double numerical types.
//!
//! All matrix manipulation should be done with this type.

typedef double MatElement;  // Use double numbers for the matrix type.


//============================================================================================================
//! \brief This type is used for all references to matrices.
//!
//! Using this type will prevent the user from needing to worry about matrix dimensions.

typedef struct
{
    MatElement *m; //!< Pointer to the matrix data.
    long r;        //!< Rows of the matrix.
    long c;        //!< Columns of the matrix.
    long tr;       //!< Number of allocated rows.
    long tc;       //!< Number of allocated columns.
} Mat;


//============================================================================================================
//! \brief Macro providing easy access to matrix elements.

#define oxts_mat_e(A,r,c) ((A)->m[c+(r)*(int)(A)->tc])


//============================================================================================================
//! \brief Definition of empty (unallocated) matrix.

#define EMPTY_MAT {NULL, 0, 0, 0, 0}


//============================================================================================================
// Useful constants

#define MAX_INN_AGE          (10000)
#define DEV_ID_STRLEN            (9)
#define OS_SCRIPT_ID_STRLEN     (11)
#define BASE_STATION_ID_STRLEN   (5)
#define OMNISTAR_SERIAL_STRLEN  (15)
#define NCOMRX_BUFFER_SIZE     (512)


//============================================================================================================
//! \brief Response types used by the decoder.
//!
//! In the future, we may wish to distinguish between invalid or incomplete incoming data.

typedef enum
{
	COM_NO_UPDATE,       //!< No update (invalid or incomplete data).
	COM_NEW_UPDATE,      //!< Successful update.
} ComResponse;


//============================================================================================================
//! \brief Various output packet states.

typedef enum
{
	OUTPUT_PACKET_INVALID,     //!< An invalid output packet (only on invalidation).
	OUTPUT_PACKET_EMPTY,       //!< An empty output packet.
	OUTPUT_PACKET_REGULAR,     //!< A regular output packet.
	OUTPUT_PACKET_STATUS,      //!< Status only output packet.
	OUTPUT_PACKET_IN1DOWN,     //!< Trigger output packet (falling edge of input).
	OUTPUT_PACKET_IN1UP,       //!< Trigger2 output packet (rising edge of input).
	OUTPUT_PACKET_OUT1,        //!< Digital output packet.
	OUTPUT_PACKET_INTERPOLATED //!< Interpolated output packet.
} OutputPacketType;


//============================================================================================================
//! \brief Navigation status.

typedef enum
{
	NAVIGATION_STATUS_NOTHING,
	NAVIGATION_STATUS_RAWIMU,
	NAVIGATION_STATUS_INIT,
	NAVIGATION_STATUS_LOCKING,
	NAVIGATION_STATUS_LOCKED,
	NAVIGATION_STATUS_UNLOCKED,
	NAVIGATION_STATUS_EXPIRED,
	NAVIGATION_STATUS_RESERVED_07,
	NAVIGATION_STATUS_RESERVED_08,
	NAVIGATION_STATUS_RESERVED_09,
	NAVIGATION_STATUS_STATUSONLY,
	NAVIGATION_STATUS_RESERVED_11,
	NAVIGATION_STATUS_RESERVED_12,
	NAVIGATION_STATUS_RESERVED_13,
	NAVIGATION_STATUS_RESERVED_14,
	NAVIGATION_STATUS_RESERVED_15,
	NAVIGATION_STATUS_RESERVED_16,
	NAVIGATION_STATUS_RESERVED_17,
	NAVIGATION_STATUS_RESERVED_18,
	NAVIGATION_STATUS_RESERVED_19,
	NAVIGATION_STATUS_TRIGINIT,
	NAVIGATION_STATUS_TRIGLOCKING,
	NAVIGATION_STATUS_TRIGLOCKED,
	NAVIGATION_STATUS_UNKNOWN
} NavigationStatus;


//============================================================================================================
//! \brief Definitions of IMU types.

typedef enum
{
	IMU_TYPE_SIIMUA      = 0,
	IMU_TYPE_IMU200      = 1,
	IMU_TYPE_IMU2        = 2,
	IMU_TYPE_IMU2X       = 3,
	IMU_TYPE_IMU3        = 4,
	IMU_TYPE_IMU3X       = 5
} IMUType;


//============================================================================================================
//! \brief Definitions of GPS types.

typedef enum
{
	GPS_TYPE_BEELINE     = 0,
	GPS_TYPE_OEM4        = 1,
	GPS_TYPE_NONE        = 2,
	GPS_TYPE_OEMV        = 3,
	GPS_TYPE_LEA4        = 4,
	GPS_TYPE_GENERIC     = 5,
	GPS_TYPE_TRIMBLE5700 = 6,
	GPS_TYPE_AGGPS132    = 7
} GPSType;


//============================================================================================================
//! \brief Indexes into the packet.

typedef enum
{
	PI_SYNC           =  0,
	PI_TIME           =  1,
	PI_ACCEL_X        =  3,
	PI_ACCEL_Y        =  6,
	PI_ACCEL_Z        =  9,
	PI_ANG_RATE_X     = 12,
	PI_ANG_RATE_Y     = 15,
	PI_ANG_RATE_Z     = 18,
	PI_INS_NAV_MODE   = 21,
	PI_CHECKSUM_1     = 22,
	PI_POS_LAT        = 23,
	PI_POS_LON        = 31,
	PI_POS_ALT        = 39,
	PI_VEL_N          = 43,
	PI_VEL_E          = 46,
	PI_VEL_D          = 49,
	PI_ORIEN_H        = 52,
	PI_ORIEN_P        = 55,
	PI_ORIEN_R        = 58,
	PI_CHECKSUM_2     = 61,
	PI_CHANNEL_INDEX  = 62,
	PI_CHANNEL_STATUS = 63,
	PI_CHECKSUM_3     = 71
} PacketIndexes;


//============================================================================================================
//! \brief Indexes into the packet.
//!
//! Range is [start, stop).

typedef enum
{
	PCSR_CHECKSUM_1_START =  1,
	PCSR_CHECKSUM_1_STOP  = 22,
	PCSR_CHECKSUM_2_START = 22,
	PCSR_CHECKSUM_2_STOP  = 61,
	PCSR_CHECKSUM_3_START = 61,
	PCSR_CHECKSUM_3_STOP  = 71
} PacketChecksumRanges;



//############################################################################################################
//##                                                                                                        ##
//##  Filt2ndOrder                                                                                          ##
//##                                                                                                        ##
//############################################################################################################


//============================================================================================================
//! \brief Internally used 2nd order filter.
//!
//! \note Used as a 'private' member of NComRxC.
//!
//! The intention is that the user need not be concerned with this structure. The role of this structure is
//! to store the state variables required for the implementation of discrete 2nd order filter functionality.

typedef struct
{
	// Principal design parameters
	double mFreqSample;        //!< Input sampling frequency. [Hz]
	double mFreqCutoff;        //!< Filter cut-off frequency. [Hz]
	double mZeta;              //!< Filter damping ratio. [-]

	// Derived filter coefficients
	double mA0, mA1, mA2;      //!< Transfer function numerator coefficients.
	double mB0, mB1, mB2;      //!< Transfer function denominator coefficients.

	// Internal state variables
	double mT0, mT1, mT2;      //!< Time stamps of past inputs.
	double mX0, mX1, mX2;      //!< Past inputs.
	double mU0, mU1, mU2;      //!< Past outputs.
	int    mOutputValid;       //!< Is current output valid?

} Filt2ndOrder;




//############################################################################################################
//##                                                                                                        ##
//##  NComRxCInternal                                                                                       ##
//##                                                                                                        ##
//############################################################################################################


//============================================================================================================
//! \brief Internally used variables used in the decoding of a series NCom data packets.
//!
//! \note Used as a 'private' member of NComRxC.
//!
//! The intention is that the user need not be concerned with this structure. The role of this structure is
//! to provide work space and retain state information over a sequence of incoming packets. Basic decoding
//! statistics are also included in this structure since they are property of the decoder rather than the
//! decoded data. These statistics may be accessed (indirectly) using the functions NComNumChars,
//! NComSkippedChars and NComNumPackets defined for NComRxC.

typedef struct
{
	// Some hard coded options.
	int mHoldDistWhenSlow;  //!< Distance hold when Slow.

	// Buffer items.
	unsigned char  mCurPkt[NCOMRX_BUFFER_SIZE]; //!< Holds the incoming data.
	unsigned char *mCurStatus;                  //!< A pointer to mCurPkt+CHANNEL_STATUS
	int mCurLen;                                //!< Length of data in buffer.
	int mPktProcessed;                          //!< Flag indicates processed packet.

	// Byte counters.
	uint64_t mNumChars;     //!< Number of bytes.
	uint64_t mSkippedChars; //!< Number of skipped bytes.
	uint64_t mNumPackets;   //!< Number of packets.

	// For time (comes from Status and each cycle and needs combining).
	int32_t mMilliSecs;
	int32_t mMinutes;

	// To identify new triggers.
	unsigned char mTrigCount;
	unsigned char mTrig2Count;
	unsigned char mDigitalOutCount;

	// To compute distance travelled.
	int    mPrevDist2dValid;
	double mPrevDist2dTime;
	double mPrevDist2dSpeed;
	double mPrevDist2d;
	int    mPrevDist3dValid;
	double mPrevDist3dTime;
	double mPrevDist3dSpeed;
	double mPrevDist3d;

	// Storage for wheel speed information required to persist over several packets.
	int mIsOldWSpeedTimeValid;       double mOldWSpeedTime;
	int mIsOldWSpeedCountValid;      double mOldWSpeedCount;

	// Storage for reference frame information required to persist over several packets.
	int mIsAccurateRefLatValid;      double mAccurateRefLat;
	int mIsAccurateRefLonValid;      double mAccurateRefLon;
	int mIsAccurateRefAltValid;      double mAccurateRefAlt;
	int mIsAccurateRefHeadingValid;  double mAccurateRefHeading;

	// Work space for matrix calculations.
	Mat E;           //!< Euler angles.
	Mat Ch;          //!< DirCos for World to Level (Heading).
	Mat Cb;          //!< DirCos for Body to Level  (Roll/Pitch).
	Mat Ab;          //!< Accels Body.
	Mat Al;          //!< Accels level.
	Mat Wb;          //!< Ang Rate Body.
	Mat Wl;          //!< Ang Rate level.
	Mat Vn;          //!< Velocities NED.
	Mat Vl;          //!< Velocities level.
	Mat Yb;          //!< Ang Accel Body.
	Mat Yl;          //!< Ang Accel level.
	int mMatrixHold; //!< Prevents matrix calculations when 1 (say for slow computers), normally 0.

	// Filters for linear acceleration.
	Filt2ndOrder FiltForAx;      //!< Filter for X-axis linear acceleration.
	Filt2ndOrder FiltForAy;      //!< Filter for Y-axis linear acceleration.
	Filt2ndOrder FiltForAz;      //!< Filter for Z-axis linear acceleration.

	// State variables for differentiation of angular rate into angular acceleration.
	double mPrevWx;      //!< Previous value of Wx. [deg/s]
	double mPrevWy;      //!< Previous value of Wy. [deg/s]
	double mPrevWz;      //!< Previous value of Wz. [deg/s]
	double mPrevWbTime;  //!< Time corresponding to previous values of Wx, Wy and Wz. [s]

	// Filters for angular acceleration.
	Filt2ndOrder FiltForYx;      //!< Filter for X-axis angular acceleration.
	Filt2ndOrder FiltForYy;      //!< Filter for Y-axis angular acceleration.
	Filt2ndOrder FiltForYz;      //!< Filter for Z-axis angular acceleration.

} NComRxCInternal;




//############################################################################################################
//##                                                                                                        ##
//##  NComRxCGps                                                                                            ##
//##                                                                                                        ##
//############################################################################################################


//============================================================================================================
//! \brief Structure to hold GPS information
//!
//! \note Used as a 'public' member of NComRxC.
//!
//! Note there are validity flags for everything. Data members are set to 'zero' and invalid when created.
//!
//! This structure collects most of the information for the primary, secondary and external GPS receivers.

typedef struct
{
	//--------------------------------------------------------------------------------------------------------
	// GPS Information

	// System information

	int mIsTypeValid;                         uint8_t mType;                     //!< Type of card fitted/connected. [enum]
	int mIsFormatValid;                       uint8_t mFormat;                   //!< Data format of card fitted/connected. [enum]

	int mIsRawRateValid;                      uint8_t mRawRate;                  //!< Raw update rate. [enum]
	int mIsPosRateValid;                      uint8_t mPosRate;                  //!< Position update rate. [enum]
	int mIsVelRateValid;                      uint8_t mVelRate;                  //!< Velocity update rate. [enum]

	int mIsAntStatusValid;                    uint8_t mAntStatus;                //!< Antenna status. [enum]
	int mIsAntPowerValid;                     uint8_t mAntPower;                 //!< Antenna power. [enum]
	int mIsPosModeValid;                      uint8_t mPosMode;                  //!< Position mode. [enum]

	int mIsSerBaudValid;                      uint8_t mSerBaud;                  //!< Receiver baud. [enum]

	// Status

	int mIsNumSatsValid;                          int mNumSats;                  //!< Number of satellites tracked.

	int mIsCpuUsedValid;                       double mCpuUsed;                  //!< CPU usage. [%]
	int mIsCoreNoiseValid;                     double mCoreNoise;                //!< Core noise. [%]
	int mIsCoreTempValid;                      double mCoreTemp;                 //!< Core temperature. [deg C]
	int mIsSupplyVoltValid;                    double mSupplyVolt;               //!< Supply voltage. [V]

	// Received data statistics

	int mIsCharsValid;                       uint32_t mChars;                    //!< Number of bytes.
	int mIsCharsSkippedValid;                uint32_t mCharsSkipped;             //!< Number of invalid bytes.
	int mIsPktsValid;                        uint32_t mPkts;                     //!< Number of valid packets.
	int mIsOldPktsValid;                     uint32_t mOldPkts;                  //!< Number of out of date packets.

} NComRxCGps;



#ifdef __cplusplus
extern "C"
{
#endif


//============================================================================================================
// NComRxCGps access functions to return a textual description of values (such enumerated types) used in
// NComRxCGps. For information about function NComGpsGet<Thing>String see comments regarding <Thing> defined
// in the NComRxCGps structure.

//------------------------------------------------------------------------------------------------------------
// GPS Information

// System information

extern const char *NComGpsGetTypeString                     (const NComRxCGps *Com);
extern const char *NComGpsGetFormatString                   (const NComRxCGps *Com);

extern const char *NComGpsGetRawRateString                  (const NComRxCGps *Com);
extern const char *NComGpsGetPosRateString                  (const NComRxCGps *Com);
extern const char *NComGpsGetVelRateString                  (const NComRxCGps *Com);

extern const char *NComGpsGetAntStatusString                (const NComRxCGps *Com);
extern const char *NComGpsGetAntPowerString                 (const NComRxCGps *Com);
extern const char *NComGpsGetPosModeString                  (const NComRxCGps *Com);

extern const char *NComGpsGetSerBaudString                  (const NComRxCGps *Com);


//============================================================================================================
// General function declarations.

extern NComRxCGps *NComGpsCreate();
extern void        NComGpsDestroy(NComRxCGps *Com);
extern void        NComGpsCopy(NComRxCGps *ComDestination, const NComRxCGps *ComSource);


#ifdef __cplusplus
}
#endif




//############################################################################################################
//##                                                                                                        ##
//##  NComRxC                                                                                               ##
//##                                                                                                        ##
//############################################################################################################


//============================================================================================================
//! \brief Structure to hold measurements and information that have been decoded from NCom data packet(s).
//!
//! Note there are validity flags for everything. Most data members are set to 'zero' and invalid when created.
//!
//! The data member "mInternal" should be assumed to be private as this contains decoder state and work space
//! information.

typedef struct
{
	//--------------------------------------------------------------------------------------------------------
	// Other structures.

	NComRxCGps *mGpsPrimary;      //!< Primary internal GPS receiver.
	NComRxCGps *mGpsSecondary;    //!< Secondary internal GPS receiver.
	NComRxCGps *mGpsExternal;     //!< External GPS receiver.

	//--------------------------------------------------------------------------------------------------------
	// General information

	// Status

	int mIsOutputPacketTypeValid;             uint8_t mOutputPacketType;         //!< Type of output packet from the decoder. [enum]
	int mIsInsNavModeValid;                   uint8_t mInsNavMode;               //!< Navigation system mode. [enum]

	// System information

	int mIsSerialNumberValid;                     int mSerialNumber;             //!< Unit serial number.
	int mIsDevIdValid;                           char mDevId[DEV_ID_STRLEN+1];                    //!< Development ID.

	int mIsOsVersion1Valid;                       int mOsVersion1;               //!< Operating system major version.
	int mIsOsVersion2Valid;                       int mOsVersion2;               //!< Operating system minor version.
	int mIsOsVersion3Valid;                       int mOsVersion3;               //!< Operating system revision version.
	int mIsOsScriptIdValid;                      char mOsScriptId[OS_SCRIPT_ID_STRLEN+1];               //!< Operating system boot up script ID.

	int mIsImuTypeValid;                      uint8_t mImuType;                  //!< Type of IMU fitted. [enum]
	int mIsCpuPcbTypeValid;                   uint8_t mCpuPcbType;               //!< Type of CPU PCB fitted. [enum]
	int mIsInterPcbTypeValid;                 uint8_t mInterPcbType;             //!< Type of interconnection PCB fitted. [enum]
	int mIsFrontPcbTypeValid;                 uint8_t mFrontPcbType;             //!< Type of front panel PCB fitted. [enum]
	int mIsInterSwIdValid;                    uint8_t mInterSwId;                //!< Software ID of interconnection PCB. [enum]
	int mIsHwConfigValid;                     uint8_t mHwConfig;                 //!< Reserved for testing. [enum]

	int mIsDiskSpaceValid;                   uint64_t mDiskSpace;                //!< Remaining disk space. [B]
	int mIsFileSizeValid;                    uint64_t mFileSize;                 //!< Size of current raw data file. [B]
	int mIsUpTimeValid;                      uint32_t mUpTime;                   //!< Up time. [s]
	int mIsDualPortRamStatusValid;            uint8_t mDualPortRamStatus;        //!< Dual port RAM interface status. [enum]

	// IMU information

	int mIsUmacStatusValid;                   uint8_t mUmacStatus;               //!< UMAC (ABD) status. [enum]

	// Global Navigation Satellite System (GNSS) information

	int mIsGnssGpsEnabledValid;                  int mGnssGpsEnabled;           //!< Global Positioning System (GPS) enabled.
	int mIsGnssGlonassEnabledValid;              int mGnssGlonassEnabled;       //!< GLObal NAvigation Satellite System (GLONASS) enabled.
	int mIsGnssGalileoEnabledValid;              int mGnssGalileoEnabled;       //!< Galileo enabled.

	int mIsPsrDiffEnabledValid;                  int mPsrDiffEnabled;           //!< Pseudo-range differential.
	int mIsSBASEnabledValid;                     int mSBASEnabled;              //!< Satellite Based Augmentation System (SBAS).
	int mIsOmniVBSEnabledValid;                  int mOmniVBSEnabled;           //!< OmniSTAR Virtual Base Station (VBS).
	int mIsOmniHPEnabledValid;                   int mOmniHPEnabled;            //!< OmniSTAR High Performance (HP).
	int mIsL1DiffEnabledValid;                   int mL1DiffEnabled;            //!< L1 carrier-phase differential (RT20).
	int mIsL1L2DiffEnabledValid;                 int mL1L2DiffEnabled;          //!< L1/L2 carrier-phase differential (RT2).

	int mIsRawRngEnabledValid;                   int mRawRngEnabled;            //!< Raw pseudo-range output.
	int mIsRawDopEnabledValid;                   int mRawDopEnabled;            //!< Raw Doppler output.
	int mIsRawL1EnabledValid;                    int mRawL1Enabled;             //!< Raw L1 output.
	int mIsRawL2EnabledValid;                    int mRawL2Enabled;             //!< Raw L2 output.
	int mIsRawL5EnabledValid;                    int mRawL5Enabled;             //!< Raw L5 output.

	int mIsGpsPosModeValid;                   uint8_t mGpsPosMode;               //!< Position mode. [enum]
	int mIsGpsVelModeValid;                   uint8_t mGpsVelMode;               //!< Velocity mode. [enum]
	int mIsGpsAttModeValid;                   uint8_t mGpsAttMode;               //!< Attitude mode. [enum]

	int mIsPDOPValid;                          double mPDOP;                     //!< Positional dilution of precision. [-]
	int mIsHDOPValid;                          double mHDOP;                     //!< Horizontal dilution of precision. [-]
	int mIsVDOPValid;                          double mVDOP;                     //!< Vertical dilution of precision. [-]

	int mIsGpsNumObsValid;                        int mGpsNumObs;                //!< Number of satellites.
	int mIsUndulationValid;                    double mUndulation;               //!< Difference between ellipsoidal altitude and geoidal altitude. [m]
	int mIsGpsDiffAgeValid;                    double mGpsDiffAge;               //!< Differential corrections age to GPS. [s]
	int mIsBaseStationIdValid;                   char mBaseStationId[BASE_STATION_ID_STRLEN+1];            //!< Differential base station ID.

	// Dual antenna computation status

	int mIsHeadQualityValid;                  uint8_t mHeadQuality;              //!< Dual antenna Heading quality. [enum]
	int mIsHeadSearchTypeValid;               uint8_t mHeadSearchType;           //!< Dual antenna Heading search type. [enum]
	int mIsHeadSearchStatusValid;             uint8_t mHeadSearchStatus;         //!< Dual antenna Heading search status. [enum]
	int mIsHeadSearchReadyValid;              uint8_t mHeadSearchReady;          //!< Dual antenna Heading search ready. [enum]

	int mIsHeadSearchInitValid;                   int mHeadSearchInit;           //!< Initial number of ambiguities in the heading search.
	int mIsHeadSearchNumValid;                    int mHeadSearchNum;            //!< Remaining number of ambiguities in the heading search.
	int mIsHeadSearchTimeValid;                   int mHeadSearchTime;           //!< Heading Search Duration. [s]
	int mIsHeadSearchConstrValid;                 int mHeadSearchConstr;         //!< Number of constraints applied in the Heading Search.

	int mIsHeadSearchMasterValid;                 int mHeadSearchMaster;         //!< Master Satellite PRN in the Heading Search.
	int mIsHeadSearchSlave1Valid;                 int mHeadSearchSlave1;         //!< Slave 1 Satellite PRN in the Heading Search.
	int mIsHeadSearchSlave2Valid;                 int mHeadSearchSlave2;         //!< Slave 2 Satellite PRN in the Heading Search.
	int mIsHeadSearchSlave3Valid;                 int mHeadSearchSlave3;         //!< Slave 3 Satellite PRN in the Heading Search.

	// OmniSTAR information

	int mIsOmniStarSerialValid;                  char mOmniStarSerial[OMNISTAR_SERIAL_STRLEN+1];           //!< OmniSTAR serial string.
	int mIsOmniStarFreqValid;                  double mOmniStarFreq;             //!< OmniSTAR frequency. [Hz]
	int mIsOmniStarSNRValid;                   double mOmniStarSNR;              //!< OmniSTAR signal to noise ratio. [dB]
	int mIsOmniStarLockTimeValid;              double mOmniStarLockTime;         //!< OmniSTAR lock time. [s]

	int mIsOmniStatusVbsExpiredValid;            int mOmniStatusVbsExpired;     //!< Virtual Base Station status: Expired.
	int mIsOmniStatusVbsOutOfRegionValid;        int mOmniStatusVbsOutOfRegion; //!< Virtual Base Station status: Out of region.
	int mIsOmniStatusVbsNoRemoteSitesValid;       int mOmniStatusVbsNoRemoteSites;//!< Virtual Base Station status: No remote sites.

	int mIsOmniStatusHpExpiredValid;             int mOmniStatusHpExpired;      //!< High Performance status: Expired.
	int mIsOmniStatusHpOutOfRegionValid;         int mOmniStatusHpOutOfRegion;  //!< High Performance status: Out of region.
	int mIsOmniStatusHpNoRemoteSitesValid;       int mOmniStatusHpNoRemoteSites;//!< High Performance status: No remote sites.
	int mIsOmniStatusHpNotConvergedValid;        int mOmniStatusHpNotConverged; //!< High Performance status: Not converged.
	int mIsOmniStatusHpKeyInvalidValid;          int mOmniStatusHpKeyInvalid;   //!< High Performance status: Key is invalid.

	//--------------------------------------------------------------------------------------------------------
	// General user options

	// General options

	int mIsOptionLevelValid;                  uint8_t mOptionLevel;              //!< Vehicle approximately level during initialisation? [enum]
	int mIsOptionVibrationValid;              uint8_t mOptionVibration;          //!< Vibration level. [enum]
	int mIsOptionGpsAccValid;                 uint8_t mOptionGpsAcc;             //!< GPS environment. [enum]
	int mIsOptionUdpValid;                    uint8_t mOptionUdp;                //!< Packet format transmitted over Ethernet. [enum]
	int mIsOptionSer1Valid;                   uint8_t mOptionSer1;               //!< Packet format transmitted over serial port 1. [enum]
	int mIsOptionSer2Valid;                   uint8_t mOptionSer2;               //!< Packet format transmitted over serial port 2. [enum]
	int mIsOptionSer3Valid;                   uint8_t mOptionSer3;               //!< Packet format transmitted over serial port 3. [enum]
	int mIsOptionHeadingValid;                uint8_t mOptionHeading;            //!< Dual antenna heading initialisation mode. [enum]

	int mIsOptionInitSpeedValid;          int mIsOptionInitSpeedConfig;               double mOptionInitSpeed;          //!< Initialisation speed. [m s^(-1)]
	int mIsOptionTopSpeedValid;           int mIsOptionTopSpeedConfig;                double mOptionTopSpeed;           //!< Maximum vehicle speed. [m s^(-1)]

	// Output baud rate settings

	int mIsOptionSer1BaudValid;               uint8_t mOptionSer1Baud;           //!< Serial port 1 baud. [enum]
	int mIsOptionSer2BaudValid;               uint8_t mOptionSer2Baud;           //!< Serial port 2 baud. [enum]
	int mIsOptionSer3BaudValid;               uint8_t mOptionSer3Baud;           //!< Serial port 3 baud. [enum]

	int mIsOptionCanBaudValid;                uint8_t mOptionCanBaud;            //!< Controller area network (CAN) bus baud rate. [enum]

	//--------------------------------------------------------------------------------------------------------
	// General measurements

	// Timing

	int mIsTimeValid;                          double mTime;                     //!< Seconds from GPS time zero (0h 1980-01-06). [s]

	int mIsTimeWeekCountValid;               uint32_t mTimeWeekCount;            //!< GPS time format, week counter. [week]
	int mIsTimeWeekSecondValid;                double mTimeWeekSecond;           //!< GPS time format, seconds into week. [s]
	int mIsTimeUtcOffsetValid;                    int mTimeUtcOffset;            //!< Offset between Coordinated Universal Time (UTC) and GPS time. [s]

	// Position

	int mIsLatValid;                      int mIsLatApprox;                           double mLat;                      //!< Latitude. [deg]
	int mIsLonValid;                      int mIsLonApprox;                           double mLon;                      //!< Longitude. [deg]
	int mIsAltValid;                      int mIsAltApprox;                           double mAlt;                      //!< Altitude. [m]

	int mIsNorthAccValid;                      double mNorthAcc;                 //!< North accuracy. [m]
	int mIsEastAccValid;                       double mEastAcc;                  //!< East accuracy. [m]
	int mIsAltAccValid;                        double mAltAcc;                   //!< Altitude accuracy. [m]

	// Distance

	int mIsDist2dValid;                        double mDist2d;                   //!< Distance travelled in horizontal directions. [m]
	int mIsDist3dValid;                        double mDist3d;                   //!< Distance travelled in all directions. [m]

	// Velocity

	int mIsVnValid;                       int mIsVnApprox;                            double mVn;                       //!< North velocity. [m s^(-1)]
	int mIsVeValid;                       int mIsVeApprox;                            double mVe;                       //!< East velocity. [m s^(-1)]
	int mIsVdValid;                       int mIsVdApprox;                            double mVd;                       //!< Downward velocity. [m s^(-1)]

	int mIsVfValid;                            double mVf;                       //!< Forward velocity. [m s^(-1)]
	int mIsVlValid;                            double mVl;                       //!< Lateral velocity. [m s^(-1)]

	int mIsVnAccValid;                         double mVnAcc;                    //!< North velocity accuracy. [m s^(-1)]
	int mIsVeAccValid;                         double mVeAcc;                    //!< East velocity accuracy. [m s^(-1)]
	int mIsVdAccValid;                         double mVdAcc;                    //!< Down velocity accuracy. [m s^(-1)]

	// Speed

	int mIsSpeed2dValid;                       double mSpeed2d;                  //!< Speed in horizonal directions. [m s^(-1)]
	int mIsSpeed3dValid;                       double mSpeed3d;                  //!< Speed in all directions. [m s^(-1)]

	// Acceleration

	int mIsAxValid;                            double mAx;                       //!< Acceleration along the X axis. [m s^(-2)]
	int mIsAyValid;                            double mAy;                       //!< Acceleration along the Y axis. [m s^(-2)]
	int mIsAzValid;                            double mAz;                       //!< Acceleration along the Z axis. [m s^(-2)]

	int mIsAfValid;                            double mAf;                       //!< Acceleration forward. [m s^(-2)]
	int mIsAlValid;                            double mAl;                       //!< Acceleration laterally. [m s^(-2)]
	int mIsAdValid;                            double mAd;                       //!< Acceleration downward. [m s^(-2)]

	// Filtered acceleration

	int mIsFiltAxValid;                        double mFiltAx;                   //!< Filtered acceleration along the X axis. [m s^(-2)]
	int mIsFiltAyValid;                        double mFiltAy;                   //!< Filtered acceleration along the Y axis. [m s^(-2)]
	int mIsFiltAzValid;                        double mFiltAz;                   //!< Filtered acceleration along the Z axis. [m s^(-2)]

	int mIsFiltAfValid;                        double mFiltAf;                   //!< Filtered acceleration forward. [m s^(-2)]
	int mIsFiltAlValid;                        double mFiltAl;                   //!< Filtered acceleration laterally. [m s^(-2)]
	int mIsFiltAdValid;                        double mFiltAd;                   //!< Filtered acceleration downward. [m s^(-2)]

	// Orientation

	int mIsHeadingValid;                  int mIsHeadingApprox;                       double mHeading;                  //!< Heading. [deg]
	int mIsPitchValid;                    int mIsPitchApprox;                         double mPitch;                    //!< Pitch. [deg]
	int mIsRollValid;                     int mIsRollApprox;                          double mRoll;                     //!< Roll. [deg]

	int mIsHeadingAccValid;                    double mHeadingAcc;               //!< Heading accuracy. [deg]
	int mIsPitchAccValid;                      double mPitchAcc;                 //!< Pitch accuracy. [deg]
	int mIsRollAccValid;                       double mRollAcc;                  //!< Roll accuracy. [deg]

	// Special

	int mIsTrackValid;                         double mTrack;                    //!< Track angle. [deg]
	int mIsSlipValid;                          double mSlip;                     //!< Slip angle. [deg]
	int mIsCurvatureValid;                     double mCurvature;                //!< Curvature. [m^(-1)]

	// Angular rate

	int mIsWxValid;                            double mWx;                       //!< Angular rate about the X axis. [deg s^(-1)]
	int mIsWyValid;                            double mWy;                       //!< Angular rate about the Y axis. [deg s^(-1)]
	int mIsWzValid;                            double mWz;                       //!< Angular rate about the Z axis. [deg s^(-1)]

	int mIsWfValid;                            double mWf;                       //!< Angular rate about the forward axis. [deg s^(-1)]
	int mIsWlValid;                            double mWl;                       //!< Angular rate about the lateral axis. [deg s^(-1)]
	int mIsWdValid;                            double mWd;                       //!< Angular rate about the down axis. [deg s^(-1)]

	// Angular acceleration

	int mIsYxValid;                            double mYx;                       //!< Angular acceleration about the X axis. [deg s^(-2)]
	int mIsYyValid;                            double mYy;                       //!< Angular acceleration about the Y axis. [deg s^(-2)]
	int mIsYzValid;                            double mYz;                       //!< Angular acceleration about the Z axis. [deg s^(-2)]

	int mIsYfValid;                            double mYf;                       //!< Angular acceleration about the forward axis. [deg s^(-2)]
	int mIsYlValid;                            double mYl;                       //!< Angular acceleration about the lateral axis. [deg s^(-2)]
	int mIsYdValid;                            double mYd;                       //!< Angular acceleration about the down axis. [deg s^(-2)]

	// Filtered angular acceleration

	int mIsFiltYxValid;                        double mFiltYx;                   //!< Filtered angular acceleration about the X axis. [deg s^(-2)]
	int mIsFiltYyValid;                        double mFiltYy;                   //!< Filtered angular acceleration about the Y axis. [deg s^(-2)]
	int mIsFiltYzValid;                        double mFiltYz;                   //!< Filtered angular acceleration about the Z axis. [deg s^(-2)]

	int mIsFiltYfValid;                        double mFiltYf;                   //!< Filtered angular acceleration about the forward axis. [deg s^(-2)]
	int mIsFiltYlValid;                        double mFiltYl;                   //!< Filtered angular acceleration about the lateral axis. [deg s^(-2)]
	int mIsFiltYdValid;                        double mFiltYd;                   //!< Filtered angular acceleration about the down axis. [deg s^(-2)]

	// Filter characteristics

	int mIsLinAccFiltFreqValid;                double mLinAccFiltFreq;           //!< Cut-off frequency of linear acceleration low-pass filter. [Hz]
	int mIsLinAccFiltZetaValid;                double mLinAccFiltZeta;           //!< Damping ratio of linear acceleration low-pass filter. [-]
	int mIsLinAccFiltFixed;
	int mHasLinAccFiltChanged;
	int mIsLinAccFiltOff;

	int mIsAngAccFiltFreqValid;                double mAngAccFiltFreq;           //!< Cut-off frequency of angular acceleration low-pass filter. [Hz]
	int mIsAngAccFiltZetaValid;                double mAngAccFiltZeta;           //!< Damping ratio of angular acceleration low-pass filter. [-]
	int mIsAngAccFiltFixed;
	int mHasAngAccFiltChanged;
	int mIsAngAccFiltOff;

	//--------------------------------------------------------------------------------------------------------
	// Model particulars

	// Innovations (discrepancy between GPS and IMU)

	int mInnPosXAge;                           double mInnPosX;                  //!< Innovation in latitude. [-]
	int mInnPosYAge;                           double mInnPosY;                  //!< Innovation in longitude. [-]
	int mInnPosZAge;                           double mInnPosZ;                  //!< Innovation in altitude. [-]

	int mInnVelXAge;                           double mInnVelX;                  //!< Innovation in north velocity. [-]
	int mInnVelYAge;                           double mInnVelY;                  //!< Innovation in east velocity. [-]
	int mInnVelZAge;                           double mInnVelZ;                  //!< Innovation in down velocity. [-]

	int mInnHeadingAge;                        double mInnHeading;               //!< Innovation in heading. [-]
	int mInnPitchAge;                          double mInnPitch;                 //!< Innovation in pitch. [-]

	// Gyroscope bias and scale factor

	int mIsWxBiasValid;                        double mWxBias;                   //!< X gyroscope bias. [deg s^(-1)]
	int mIsWyBiasValid;                        double mWyBias;                   //!< Y gyroscope bias. [deg s^(-1)]
	int mIsWzBiasValid;                        double mWzBias;                   //!< Z gyroscope bias. [deg s^(-1)]

	int mIsWxBiasAccValid;                     double mWxBiasAcc;                //!< X gyroscope bias accuracy. [deg s^(-1)]
	int mIsWyBiasAccValid;                     double mWyBiasAcc;                //!< Y gyroscope bias accuracy. [deg s^(-1)]
	int mIsWzBiasAccValid;                     double mWzBiasAcc;                //!< Z gyroscope bias accuracy. [deg s^(-1)]

	int mIsWxSfValid;                          double mWxSf;                     //!< X gyroscope scale factor deviation. [-]
	int mIsWySfValid;                          double mWySf;                     //!< Y gyroscope scale factor deviation. [-]
	int mIsWzSfValid;                          double mWzSf;                     //!< Z gyroscope scale factor deviation. [-]

	int mIsWxSfAccValid;                       double mWxSfAcc;                  //!< X gyroscope scale factor deviation accuracy. [-]
	int mIsWySfAccValid;                       double mWySfAcc;                  //!< Y gyroscope scale factor deviation accuracy. [-]
	int mIsWzSfAccValid;                       double mWzSfAcc;                  //!< Z gyroscope scale factor deviation accuracy. [-]

	// Accelerometer bias and scale factor

	int mIsAxBiasValid;                        double mAxBias;                   //!< X accelerometer bias. [m s^(-2)]
	int mIsAyBiasValid;                        double mAyBias;                   //!< Y accelerometer bias. [m s^(-2)]
	int mIsAzBiasValid;                        double mAzBias;                   //!< Z accelerometer bias. [m s^(-2)]

	int mIsAxBiasAccValid;                     double mAxBiasAcc;                //!< X accelerometer bias accuracy. [m s^(-2)]
	int mIsAyBiasAccValid;                     double mAyBiasAcc;                //!< Y accelerometer bias accuracy. [m s^(-2)]
	int mIsAzBiasAccValid;                     double mAzBiasAcc;                //!< Z accelerometer bias accuracy. [m s^(-2)]

	int mIsAxSfValid;                          double mAxSf;                     //!< X accelerometer scale factor deviation. [-]
	int mIsAySfValid;                          double mAySf;                     //!< Y accelerometer scale factor deviation. [-]
	int mIsAzSfValid;                          double mAzSf;                     //!< Z accelerometer scale factor deviation. [-]

	int mIsAxSfAccValid;                       double mAxSfAcc;                  //!< X accelerometer scale factor deviation accuracy. [-]
	int mIsAySfAccValid;                       double mAySfAcc;                  //!< Y accelerometer scale factor deviation accuracy. [-]
	int mIsAzSfAccValid;                       double mAzSfAcc;                  //!< Z accelerometer scale factor deviation accuracy. [-]

	// GPS antenna position

	int mIsGAPxValid;                          double mGAPx;                     //!< Primary GPS antenna position X offset. [m]
	int mIsGAPyValid;                          double mGAPy;                     //!< Primary GPS antenna position Y offset. [m]
	int mIsGAPzValid;                          double mGAPz;                     //!< Primary GPS antenna position Z offset. [m]

	int mIsGAPxAccValid;                       double mGAPxAcc;                  //!< Primary GPS antenna position X offset accuracy. [m]
	int mIsGAPyAccValid;                       double mGAPyAcc;                  //!< Primary GPS antenna position Y offset accuracy. [m]
	int mIsGAPzAccValid;                       double mGAPzAcc;                  //!< Primary GPS antenna position Z offset accuracy. [m]

	int mIsAtHValid;                           double mAtH;                      //!< Secondary GPS antenna relative heading. [deg]
	int mIsAtPValid;                           double mAtP;                      //!< Secondary GPS antenna relative pitch. [deg]

	int mIsAtHAccValid;                        double mAtHAcc;                   //!< Secondary GPS antenna relative heading accuracy. [deg]
	int mIsAtPAccValid;                        double mAtPAcc;                   //!< Secondary GPS antenna relative pitch accuracy. [deg]

	int mIsBaseLineLengthValid;           int mIsBaseLineLengthConfig;                double mBaseLineLength;           //!< Distance between GPS antennas. [m]
	int mIsBaseLineLengthAccValid;        int mIsBaseLineLengthAccConfig;             double mBaseLineLengthAcc;        //!< Distance between GPS antennas accuracy. [m]

	//--------------------------------------------------------------------------------------------------------
	// Statistics

	// IMU hardware status event counters

	int mIsImuMissedPktsValid;               uint32_t mImuMissedPkts;            //!< Number of IMU hardware missed packets.
	int mIsImuResetCountValid;               uint32_t mImuResetCount;            //!< Number of IMU hardware resets.
	int mIsImuErrorCountValid;               uint32_t mImuErrorCount;            //!< Number of IMU hardware errors.

	// GPS successive rejected aiding updates

	int mIsGPSPosRejectValid;                uint32_t mGPSPosReject;             //!< Number of successive GPS position updates rejected.
	int mIsGPSVelRejectValid;                uint32_t mGPSVelReject;             //!< Number of successive GPS velocity updates rejected.
	int mIsGPSAttRejectValid;                uint32_t mGPSAttReject;             //!< Number of successive GPS attitude updates rejected.

	// Received data statistics

	int mIsImuCharsValid;                    uint32_t mImuChars;                 //!< IMU number of bytes.
	int mIsImuCharsSkippedValid;             uint32_t mImuCharsSkipped;          //!< IMU number of invalid bytes.
	int mIsImuPktsValid;                     uint32_t mImuPkts;                  //!< IMU number of valid packets.

	int mIsCmdCharsValid;                    uint32_t mCmdChars;                 //!< Command number of bytes.
	int mIsCmdCharsSkippedValid;             uint32_t mCmdCharsSkipped;          //!< Command number of invalid bytes.
	int mIsCmdPktsValid;                     uint32_t mCmdPkts;                  //!< Command number of valid packets.
	int mIsCmdErrorsValid;                   uint32_t mCmdErrors;                //!< Command number of errors.

	//--------------------------------------------------------------------------------------------------------
	// Transformation Euler angles

	// Orientation of vehicle-frame relative to IMU-frame

	int mIsImu2VehHeadingValid;                double mImu2VehHeading;           //!< Heading. [deg]
	int mIsImu2VehPitchValid;                  double mImu2VehPitch;             //!< Pitch. [deg]
	int mIsImu2VehRollValid;                   double mImu2VehRoll;              //!< Roll. [deg]

	//--------------------------------------------------------------------------------------------------------
	// Miscellaneous items

	// Triggers

	int mIsTrigTimeValid;                 int mIsTrigTimeNew;                      double mTrigTime;                 //!< Time of last trigger falling edge. [s]
	int mIsTrig2TimeValid;                int mIsTrig2TimeNew;                     double mTrig2Time;                //!< Time of last trigger rising edge. [s]
	int mIsDigitalOutTimeValid;           int mIsDigitalOutTimeNew;                double mDigitalOutTime;           //!< Time of last digital output. [s]

	// Remote lever arm option

	int mIsRemoteLeverArmXValid;               double mRemoteLeverArmX;          //!< Remote lever arm X position. [m]
	int mIsRemoteLeverArmYValid;               double mRemoteLeverArmY;          //!< Remote lever arm Y position. [m]
	int mIsRemoteLeverArmZValid;               double mRemoteLeverArmZ;          //!< Remote lever arm Z position. [m]

	// Local reference frame (definition)

	int mIsRefLatValid;                        double mRefLat;                   //!< Reference frame latitude. [deg]
	int mIsRefLonValid;                        double mRefLon;                   //!< Reference frame longitude. [deg]
	int mIsRefAltValid;                        double mRefAlt;                   //!< Reference frame altitude. [m]
	int mIsRefHeadingValid;                    double mRefHeading;               //!< Reference frame heading. [deg]

	//--------------------------------------------------------------------------------------------------------
	// Zero velocity and advanced slip

	// Innovations

	int mInnZeroVelXAge;                       double mInnZeroVelX;              //!< Discrepancy from zero of north velocity. [-]
	int mInnZeroVelYAge;                       double mInnZeroVelY;              //!< Discrepancy from zero of east velocity. [-]
	int mInnZeroVelZAge;                       double mInnZeroVelZ;              //!< Discrepancy from zero of downward velocity. [-]

	int mInnNoSlipHAge;                        double mInnNoSlipH;               //!< Discrepancy of slip angle at rear wheels. [-]

	// Lever arm options

	int mIsZeroVelLeverArmXValid;              double mZeroVelLeverArmX;         //!< Zero velocity position X offset. [m]
	int mIsZeroVelLeverArmYValid;              double mZeroVelLeverArmY;         //!< Zero velocity position Y offset. [m]
	int mIsZeroVelLeverArmZValid;              double mZeroVelLeverArmZ;         //!< Zero velocity position Z offset. [m]

	int mIsZeroVelLeverArmXAccValid;           double mZeroVelLeverArmXAcc;      //!< Zero velocity position X offset accuracy. [m]
	int mIsZeroVelLeverArmYAccValid;           double mZeroVelLeverArmYAcc;      //!< Zero velocity position Y offset accuracy. [m]
	int mIsZeroVelLeverArmZAccValid;           double mZeroVelLeverArmZAcc;      //!< Zero velocity position Z offset accuracy. [m]

	int mIsNoSlipLeverArmXValid;               double mNoSlipLeverArmX;          //!< Rear wheel position X offset. [m]
	int mIsNoSlipLeverArmYValid;               double mNoSlipLeverArmY;          //!< Rear wheel position Y offset. [m]
	int mIsNoSlipLeverArmZValid;               double mNoSlipLeverArmZ;          //!< Rear wheel position Z offset. [m]

	int mIsNoSlipLeverArmXAccValid;            double mNoSlipLeverArmXAcc;       //!< Rear wheel position X offset accuracy. [m]
	int mIsNoSlipLeverArmYAccValid;            double mNoSlipLeverArmYAcc;       //!< Rear wheel position Y offset accuracy. [m]
	int mIsNoSlipLeverArmZAccValid;            double mNoSlipLeverArmZAcc;       //!< Rear wheel position Z offset accuracy. [m]

	// Heading alignment

	int mIsHeadingMisAlignValid;               double mHeadingMisAlign;          //!< Estimated heading offset between unit and vehicle. [deg]
	int mIsHeadingMisAlignAccValid;            double mHeadingMisAlignAcc;       //!< Estimated accuracy of heading offset between unit and vehicle. [deg]

	// User Options

	int mIsOptionSZVDelayValid;           int mIsOptionSZVDelayConfig;                double mOptionSZVDelay;           //!< Garage mode setting. [s]
	int mIsOptionSZVPeriodValid;          int mIsOptionSZVPeriodConfig;               double mOptionSZVPeriod;          //!< Garage mode setting. [s]

	int mIsOptionNSDelayValid;            int mIsOptionNSDelayConfig;                 double mOptionNSDelay;            //!< Advanced slip setting. [s]
	int mIsOptionNSPeriodValid;           int mIsOptionNSPeriodConfig;                double mOptionNSPeriod;           //!< Advanced slip setting. [s]
	int mIsOptionNSAngleStdValid;         int mIsOptionNSAngleStdConfig;              double mOptionNSAngleStd;         //!< Advanced slip setting. [deg]
	int mIsOptionNSHAccelValid;           int mIsOptionNSHAccelConfig;                double mOptionNSHAccel;           //!< Advanced slip setting. [m s^(-2)]
	int mIsOptionNSVAccelValid;           int mIsOptionNSVAccelConfig;                double mOptionNSVAccel;           //!< Advanced slip setting. [m s^(-2)]
	int mIsOptionNSSpeedValid;            int mIsOptionNSSpeedConfig;                 double mOptionNSSpeed;            //!< Advanced slip setting. [m s^(-1)]
	int mIsOptionNSRadiusValid;           int mIsOptionNSRadiusConfig;                double mOptionNSRadius;           //!< Advanced slip setting. [m]

	//--------------------------------------------------------------------------------------------------------
	// Wheel speed input

	// Innovations

	int mInnWSpeedAge;                         double mInnWSpeed;                //!< Wheel speed innovation. [-]

	// Wheel speed lever arm option

	int mIsWSpeedLeverArmXValid;               double mWSpeedLeverArmX;          //!< Wheel speed position X offset. [m]
	int mIsWSpeedLeverArmYValid;               double mWSpeedLeverArmY;          //!< Wheel speed position Y offset. [m]
	int mIsWSpeedLeverArmZValid;               double mWSpeedLeverArmZ;          //!< Wheel speed position Z offset. [m]

	int mIsWSpeedLeverArmXAccValid;            double mWSpeedLeverArmXAcc;       //!< Wheel speed position X offset accuracy. [m]
	int mIsWSpeedLeverArmYAccValid;            double mWSpeedLeverArmYAcc;       //!< Wheel speed position Y offset accuracy. [m]
	int mIsWSpeedLeverArmZAccValid;            double mWSpeedLeverArmZAcc;       //!< Wheel speed position Z offset accuracy. [m]

	// Wheel speed input model

	int mIsWSpeedScaleValid;              int mIsWSpeedScaleConfig;                   double mWSpeedScale;              //!< Wheel speed scale factor. [pulse m^(-1)]
	int mIsWSpeedScaleStdValid;           int mIsWSpeedScaleStdConfig;                double mWSpeedScaleStd;           //!< Wheel speed scale factor accuracy. [%]

	int mIsOptionWSpeedDelayValid;        int mIsOptionWSpeedDelayConfig;             double mOptionWSpeedDelay;        //!< Wheel speed setting. [s]
	int mIsOptionWSpeedZVDelayValid;      int mIsOptionWSpeedZVDelayConfig;           double mOptionWSpeedZVDelay;      //!< Wheel speed setting. [s]
	int mIsOptionWSpeedNoiseStdValid;     int mIsOptionWSpeedNoiseStdConfig;          double mOptionWSpeedNoiseStd;     //!< Wheel speed setting.

	// Wheel speed tacho measurements

	int mIsWSpeedTimeValid;                    double mWSpeedTime;               //!< Time of last wheel speed measurement. [s]
	int mIsWSpeedCountValid;                   double mWSpeedCount;              //!< Count at last wheel speed measurement.
	int mIsWSpeedTimeUnchangedValid;           double mWSpeedTimeUnchanged;      //!< Time since last count. [s]
	int mIsWSpeedFreqValid;                    double mWSpeedFreq;               //!< Wheel speed frequency. [Hz]

	//--------------------------------------------------------------------------------------------------------
	// Heading lock

	// Innovations

	int mInnHeadingHAge;                       double mInnHeadingH;              //!< Heading lock innovation. [-]

	// User options

	int mIsOptionHLDelayValid;            int mIsOptionHLDelayConfig;                 double mOptionHLDelay;            //!< Heading lock setting. [s]
	int mIsOptionHLPeriodValid;           int mIsOptionHLPeriodConfig;                double mOptionHLPeriod;           //!< Heading lock setting. [s]
	int mIsOptionHLAngleStdValid;         int mIsOptionHLAngleStdConfig;              double mOptionHLAngleStd;         //!< Heading lock setting. [deg]

	int mIsOptionStatDelayValid;          int mIsOptionStatDelayConfig;               double mOptionStatDelay;          //!< Heading lock setting. [s]
	int mIsOptionStatSpeedValid;          int mIsOptionStatSpeedConfig;               double mOptionStatSpeed;          //!< Heading lock setting. [m s^(-1)]

	//--------------------------------------------------------------------------------------------------------
	// For use in testing

	// Reserved for testing

	int mIsTimeMismatchValid;                     int mTimeMismatch;             //!< Reserved for testing.
	int mIsImuTimeDiffValid;                      int mImuTimeDiff;              //!< Reserved for testing.
	int mIsImuTimeMarginValid;                    int mImuTimeMargin;            //!< Reserved for testing.
	int mIsImuLoopTimeValid;                      int mImuLoopTime;              //!< Reserved for testing.
	int mIsOpLoopTimeValid;                       int mOpLoopTime;               //!< Reserved for testing.

	int mIsBnsLagValid;                           int mBnsLag;                   //!< Reserved for testing.
	int mIsBnsLagFiltValid;                    double mBnsLagFilt;               //!< Reserved for testing.

	//--------------------------------------------------------------------------------------------------------
	// Decoder private area.

	NComRxCInternal *mInternal;  //!< Private decoder state and work space.

} NComRxC;



#ifdef __cplusplus
extern "C"
{
#endif


//============================================================================================================
// NComRxC access functions to return a textual description of values (such enumerated types) used in NComRxC.
// For information about function NComGet<Thing>String see comments regarding <Thing> defined in the NComRxC
// structure.

//------------------------------------------------------------------------------------------------------------
// General information

// Status

extern const char *NComGetOutputPacketTypeString         (const NComRxC *Com);
extern const char *NComGetInsNavModeString               (const NComRxC *Com);

// System information

extern const char *NComGetImuTypeString                  (const NComRxC *Com);
extern const char *NComGetCpuPcbTypeString               (const NComRxC *Com);
extern const char *NComGetInterPcbTypeString             (const NComRxC *Com);
extern const char *NComGetFrontPcbTypeString             (const NComRxC *Com);
extern const char *NComGetInterSwIdString                (const NComRxC *Com);
extern const char *NComGetHwConfigString                 (const NComRxC *Com);

extern const char *NComGetDualPortRamStatusString        (const NComRxC *Com);

// IMU information

extern const char *NComGetUmacStatusString               (const NComRxC *Com);

// Global Navigation Satellite System (GNSS) information

extern const char *NComGetGpsPosModeString               (const NComRxC *Com);
extern const char *NComGetGpsVelModeString               (const NComRxC *Com);
extern const char *NComGetGpsAttModeString               (const NComRxC *Com);

// Dual antenna computation status

extern const char *NComGetHeadQualityString              (const NComRxC *Com);
extern const char *NComGetHeadSearchTypeString           (const NComRxC *Com);
extern const char *NComGetHeadSearchStatusString         (const NComRxC *Com);
extern const char *NComGetHeadSearchReadyString          (const NComRxC *Com);

//------------------------------------------------------------------------------------------------------------
// General user options

// General options

extern const char *NComGetOptionLevelString              (const NComRxC *Com);
extern const char *NComGetOptionVibrationString          (const NComRxC *Com);
extern const char *NComGetOptionGpsAccString             (const NComRxC *Com);
extern const char *NComGetOptionUdpString                (const NComRxC *Com);
extern const char *NComGetOptionSer1String               (const NComRxC *Com);
extern const char *NComGetOptionSer2String               (const NComRxC *Com);
extern const char *NComGetOptionSer3String               (const NComRxC *Com);
extern const char *NComGetOptionHeadingString            (const NComRxC *Com);

// Output baud rate settings

extern const char *NComGetOptionSer1BaudString           (const NComRxC *Com);
extern const char *NComGetOptionSer2BaudString           (const NComRxC *Com);
extern const char *NComGetOptionSer3BaudString           (const NComRxC *Com);

extern const char *NComGetOptionCanBaudString            (const NComRxC *Com);


//============================================================================================================
// General function declarations.

extern void          NComInvalidate(NComRxC *Com);
extern void          NComReport(const NComRxC *Com, const char *file_name, int append);
extern NComRxC      *NComCreateNComRxC();
extern void          NComDestroyNComRxC(NComRxC *Com);
extern void          NComCopy(NComRxC *ComDestination, const NComRxC *ComSource);
extern const char   *NComDecoderVersionString();
extern const char   *NComFileDefaultExt(const NComRxC *Com);
extern const char   *NComFileFilter(const NComRxC *Com);
extern const int32_t NComBaudRate(const NComRxC *Com);
extern ComResponse   NComNewChar(NComRxC *Com, unsigned char c);
extern ComResponse   NComNewChars(NComRxC *Com, const unsigned char *data, int num);
extern uint64_t      NComNumChars(const NComRxC *Com);
extern uint64_t      NComSkippedChars(const NComRxC *Com);
extern uint64_t      NComNumPackets(const NComRxC *Com);
extern void          NComUpdateInnAge(NComRxC *Com);
extern void          NComInterpolate(NComRxC *Com, double a, const NComRxC *A, double b, const NComRxC *B);


#ifdef __cplusplus
}
#endif




//============================================================================================================
// Backwards compatibility.
//
// This decoder is not quite compatible with some older versions, hence the change to the type NComRxC from
// NComRx. The changes are slight, however, so one may wish to continue using the older type NComRx. To do so
// enable the following type definition.

// typedef NComRxC NComRx;


#endif
