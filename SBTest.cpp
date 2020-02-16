namespace SBLib
{
/*
Log
	Each output streams have their own filters (debug console, user console, file report).
	Passed the user threshold log merging will be attempted and a Warning will be emitted.
	If merge result is still over user threshold, report a SystemWarning and reports all log anyway (perf/mem/space warning).
	Passed the system threshold, reports a SystemError and breaks or emit an exception and truncates the log.

	Severity
		Info:          Generic non-critical information. Outputs if not filtered out.
		Warning:       Recoverable warning (glitches, incorrect behaviour, ).
		SystemWarning: Bad warning. The system can become irresponsive or be badly affected.
		Error:         Possibly recoverable error (bad/undefined behaviour).
		SystemError:   Sub-system error (access denied, disk full, non-critical out-of-memory, ). Can be ignored if either the sub-system can be reinitialized or ignored.
		FatalError:    Major system error, unrecoverable (critical out-of-memory, hardware/driver crash, data corruption, etc.).

	Filters works as callbacks set for every subchannel/stream/severity for filtering/processing.
	bool FilterCallback( std::iostream& out, uintptr_t channelID, severity_t severity, string message );

	Messages are only reconstructed and passed to the FilterCallback if the function was registered for given channelID/stream/severity setting.
	This is usually done though something equivalent to
		for ( out: registeredStreams )
			for ( channelID: registeredChannels )
				for ( severityID: registeredSeverity )
					Log::Register( out, channelID, severity, forwardCallback );
	It is thus possible to do something equivalent to
		Log::Register( debugConsole,
			{SystemChannel, GraphicsChannel, AudioChannels},
			{Warning, SystemWarning, Error, SystemError, FatalError},
			forwardCallback );
		Log::Register( fileReport,
			{SystemChannel, GraphicsChannel, AudioChannels, ScriptChannel, UserChannel},
			{Info, Warning, SystemWarning, Error, SystemError, FatalError},
			forwardCallback );
		Log::Register( userConsole,
			{ScriptChannel, UserChannel},
			{Info, Warning, SystemWarning, Error},
			processCallback );
		Log::Register( userConsole,
			{SystemChannel, GraphicsChannel, AudioChannels},
			{SystemWarning, Error},
			forwardCallback );
		Log::Register( userConsole,
			{SystemChannel, GraphicsChannel, AudioChannels, ScriptChannel, UserChannel},
			{SystemError, FatalError},
			breakCallback );

*/


#if 0

namespace Graphics
{

	struct ApplicationParameters
{
	const char* applicationName;
	uint32_t    applicationVersion;

	const char* engineName;
	uint32_t    engineVersion;
};

struct DeviceParameters
{
	enum class API : uint32_t
	{
		Unknown = 0x00000000,
		Vulkan  = 0x00000001,
		DX12    = 0x0001000C,
	};

	ApplicationParameters applicationParameters;
	API     	api;
	uint32_t	apiVersion;

	uint64_t	deviceFlags;    	// device capabilities to be tested on startup
	uint64_t	validationFlags;	// validation / debug layers to be used
};

class Device
{
public:
	using parameters_t = DeviceParameters;
	Device( parameters_t deviceParameters );
	virtual ~Device();

private:
	parameters_t deviceParameters;
	void* lowLevelDevice;
};

}

shared_ptr<Graphics::Device> Graphics::CreateDevice( Graphics::Device::parameters_t parameters );

#endif // 0
}
