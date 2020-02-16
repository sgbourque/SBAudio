#pragma once

#include <vector>
#include <string>

#include "Windows.h"

struct SBAsioDevice
{
	struct CLSID
	{
		unsigned long  Data1;
		unsigned short Data2;
		unsigned short Data3;
		unsigned char  Data4[8];
	};

	CLSID			classID;
	std::wstring	path;
	std::wstring	name;

	operator bool() const
	{
		return !path.empty();
	}
};

enum class SBAsioDriverResult
{
	Error_Failed = -2,
	Error_Unitialized = -1,

	Success = 0,
	AlreadyExists = 1,
};

std::vector<SBAsioDevice> SB_EnumerateASIODevices();

SBAsioDriverResult SB_CreateAsioDriver(const SBAsioDevice& device);
SBAsioDriverResult SB_ReleaseAsioDriver(const SBAsioDevice& device);

void SB_ASIOInitialize();
void SB_ASIOShutdown();


enum class ASIOBool : long
{
	False = 0,
	True = 1
};

enum class ASIOError : long
{
	OK = 0,                       	// This value will be returned whenever the call succeeded
	SuccessFuture    = 0x3f4847a0,	// unique success return value for ASIOFuture calls
	NotPresent       = -1000,     	// hardware input or output is not present or available
	HWMalfunction,                	// hardware is malfunctioning (can be returned by any ASIO function)
	InvalidParameter,             	// input parameter invalid
	InvalidMode,                  	// hardware is in a bad mode or used in a bad mode
	SPNotAdvancing,               	// hardware is not running when sample position is inquired
	NoClock,                      	// sample clock or rate cannot be determined or is not present
	NoMemory,                     	// not enough memory for completing the request
};

enum class ASIOSampleType : long
{
	Int16_MSB = 0,
	Int24_MSB = 1,  		// used for 20 bits as well
	Int32_MSB = 2,
	Float32_MSB = 3,		// IEEE 754 32 bit float
	Float64_MSB = 4,		// IEEE 754 64 bit double float

	// these are used for 32 bit data buffer, with different alignment of the data inside
	// 32 bit PCI bus systems can be more easily used with these
	Int32_MSB16 = 8, 		// 32 bit data with 16 bit alignment
	Int32_MSB18 = 9, 		// 32 bit data with 18 bit alignment
	Int32_MSB20 = 10,		// 32 bit data with 20 bit alignment
	Int32_MSB24 = 11,		// 32 bit data with 24 bit alignment

	Int16_LSB = 16,
	Int24_LSB = 17, 		// used for 20 bits as well
	Int32_LSB = 18,
	Float32_LSB = 19,		// IEEE 754 32 bit float, as found on Intel x86 architecture
	Float64_LSB = 20, 		// IEEE 754 64 bit double float, as found on Intel x86 architecture

	// these are used for 32 bit data buffer, with different alignment of the data inside
	// 32 bit PCI bus systems can more easily used with these
	Int32_LSB16 = 24,		// 32 bit data with 18 bit alignment
	Int32_LSB18 = 25,		// 32 bit data with 18 bit alignment
	Int32_LSB20 = 26,		// 32 bit data with 20 bit alignment
	Int32_LSB24 = 27,		// 32 bit data with 24 bit alignment

	//	ASIO DSD format.
	DSD_Int8_LSB1 = 32,		// DSD 1 bit data, 8 samples per byte. First sample in Least significant bit.
	DSD_Int8_MSB1 = 33,		// DSD 1 bit data, 8 samples per byte. First sample in Most significant bit.
	DSD_Int8_NER8 = 40,		// DSD 8 bit data, 1 sample per byte. No Endianness required.
};

enum class ASIOFuture : long
{
	EnableTimeCodeRead = 1,	// no arguments
	DisableTimeCodeRead,		// no arguments
	SetInputMonitor,			// ASIOInputMonitor* in params
	Transport,					// ASIOTransportParameters* in params
	SetInputGain,				// ASIOChannelControls* in params, apply gain
	GetInputMeter,				// ASIOChannelControls* in params, fill meter
	SetOutputGain,				// ASIOChannelControls* in params, apply gain
	GetOutputMeter,			// ASIOChannelControls* in params, fill meter
	CanInputMonitor,			// no arguments for CanXXX selectors
	CanTimeInfo,
	CanTimeCode,
	CanTransport,
	CanInputGain,
	CanInputMeter,
	CanOutputGain,
	CanOutputMeter,
	OptionalOne,

	//	DSD support
	//	The following extensions are required to allow switching
	//	and control of the DSD subsystem.
	SetIoFormat = 0x23111961,		/* ASIOIoFormat * in params.			*/
	GetIoFormat = 0x23111983,		/* ASIOIoFormat * in params.			*/
	CanDoIoFormat = 0x23112004,		/* ASIOIoFormat * in params.			*/

	// Extension for drop out detection
	CanReportOverload = 0x24042012,	/* return ASE_SUCCESS if driver can detect and report overloads */

	GetInternalBufferSamples = 0x25042012	/* ASIOInternalBufferInfo * in params. Deliver size of driver internal buffering, return ASE_SUCCESS if supported */
};


using ASIOSampleRate = double;
using ASIOSamples = int64_t;
using ASIOTimeStamp = int64_t;

struct AsioTimeInfo
{
	double          speed;                  // absolute speed (1. = nominal)
	ASIOTimeStamp   systemTime;             // system time related to samplePosition, in nanoseconds
											// on mac, must be derived from Microseconds() (not UpTime()!)
											// on windows, must be derived from timeGetTime()
	ASIOSamples     samplePosition;
	ASIOSampleRate  sampleRate;             // current rate
	unsigned long flags;                    // (see below)
	char reserved[12];
};

struct ASIOTimeCode
{
	double          speed;                  // speed relation (fraction of nominal speed)
											// optional; set to 0. or 1. if not supported
	ASIOSamples     timeCodeSamples;        // time in samples
	unsigned long   flags;                  // some information flags (see below)
	char future[64];
};

struct ASIOTime                          // both input/output
{
	long reserved[4];                       // must be 0
	AsioTimeInfo     timeInfo;       // required
	ASIOTimeCode     timeCode;       // optional, evaluated if (timeCode.flags & kTcValid)
};

struct ASIOClockSource
{
	long index;					// as used for ASIOSetClockSource()
	long associatedChannel;		// for instance, S/PDIF or AES/EBU
	long associatedGroup;		// see channel groups (ASIOGetChannelInfo())
	ASIOBool isCurrentSource;	// ASIOTrue if this is the current clock source
	char name[32];				// for user selection
};

struct ASIOBufferInfo
{
	ASIOBool isInput;			// on input:  ASIOTrue: input, else output
	long channelNum;			// on input:  channel index
	void* buffers[2];			// on output: double buffer addresses
};

struct ASIOChannelInfo
{
	long channel;			// on input, channel index
	ASIOBool isInput;		// on input
	ASIOBool isActive;		// on exit
	long channelGroup;		// dto
	ASIOSampleType type;	// dto
	char name[32];			// dto
};

typedef struct ASIOCallbacks
{
	void (*bufferSwitch) (long doubleBufferIndex, ASIOBool directProcess);
	// bufferSwitch indicates that both input and output are to be processed.
	// the current buffer half index (0 for A, 1 for B) determines
	// - the output buffer that the host should start to fill. the other buffer
	//   will be passed to output hardware regardless of whether it got filled
	//   in time or not.
	// - the input buffer that is now filled with incoming data. Note that
	//   because of the synchronicity of i/o, the input always has at
	//   least one buffer latency in relation to the output.
	// directProcess suggests to the host whether it should immedeately
	// start processing (directProcess == ASIOTrue), or whether its process
	// should be deferred because the call comes from a very low level
	// (for instance, a high level priority interrupt), and direct processing
	// would cause timing instabilities for the rest of the system. If in doubt,
	// directProcess should be set to ASIOFalse.
	// Note: bufferSwitch may be called at interrupt time for highest efficiency.

	void (*sampleRateDidChange) (ASIOSampleRate sRate);
	// gets called when the AudioStreamIO detects a sample rate change
	// If sample rate is unknown, 0 is passed (for instance, clock loss
	// when externally synchronized).

	long (*asioMessage) (long selector, long value, void* message, double* opt);
	// generic callback for various purposes, see selectors below.
	// note this is only present if the asio version is 2 or higher

	ASIOTime* (*bufferSwitchTimeInfo) (ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
	// new callback with time info. makes ASIOGetSamplePosition() and various
	// calls to ASIOGetSampleRate obsolete,
	// and allows for timecode sync etc. to be preferred; will be used if
	// the driver calls asioMessage with selector ASIOFuture::SupportsTimeInfo.
} ASIOCallbacks;

struct IASIO : public IUnknown
{
	virtual ASIOBool init(void* sysHandle) = 0;
	virtual void getDriverName(char* name) = 0;
	virtual long getDriverVersion() = 0;
	virtual void getErrorMessage(char* string) = 0;
	virtual ASIOError start() = 0;
	virtual ASIOError stop() = 0;
	virtual ASIOError getChannels(long* numInputChannels, long* numOutputChannels) = 0;
	virtual ASIOError getLatencies(long* inputLatency, long* outputLatency) = 0;
	virtual ASIOError getBufferSize(long* minSize, long* maxSize, long* preferredSize, long* granularity) = 0;
	virtual ASIOError canSampleRate(ASIOSampleRate sampleRate) = 0;
	virtual ASIOError getSampleRate(ASIOSampleRate* sampleRate) = 0;
	virtual ASIOError setSampleRate(ASIOSampleRate sampleRate) = 0;
	virtual ASIOError getClockSources(ASIOClockSource* clocks, long* numSources) = 0;
	virtual ASIOError setClockSource(long reference) = 0;
	virtual ASIOError getSamplePosition(ASIOSamples* sPos, ASIOTimeStamp* tStamp) = 0;
	virtual ASIOError getChannelInfo(ASIOChannelInfo* info) = 0;
	virtual ASIOError createBuffers(ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks) = 0;
	virtual ASIOError disposeBuffers() = 0;
	virtual ASIOError controlPanel() = 0;
	virtual ASIOError future(ASIOFuture selector, void* opt) = 0;
	virtual ASIOError outputReady() = 0;
};

IASIO* SB_QueryInterface(const SBAsioDevice& device);

inline std::wstring SB_GetASIOErrorString(ASIOError error)
{
	switch (error)
	{
	case ASIOError::OK:               return L"no error";                      // = 0,				// This value will be returned whenever the call succeeded
	case ASIOError::SuccessFuture:    return L"success (future)";              // = 0x3f4847a0,	// unique success return value for ASIOFuture calls
	case ASIOError::NotPresent:       return L"not present";                   // = -1000,			// hardware input or output is not present or available
	case ASIOError::HWMalfunction:    return L"hardware malfunction";          //,					// hardware is malfunctioning (can be returned by any ASIO function)
	case ASIOError::InvalidParameter: return L"invalid parameter";             //,					// input parameter invalid
	case ASIOError::InvalidMode:      return L"invalid mode";                  //,					// hardware is in a bad mode or used in a bad mode
	case ASIOError::SPNotAdvancing:   return L"sample position not advancing"; //,					// hardware is not running when sample position is inquired
	case ASIOError::NoClock:          return L"no clock";                      //,					// sample clock or rate cannot be determined or is not present
	case ASIOError::NoMemory:         return L"no memory";                     //,					// not enough memory for completing the request
	default:                          return L"unknown";
	}
}
