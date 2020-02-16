/*
[Bloc décrivant le format audio]
FormatBlocID(4 octets) : Identifiant «fmt »(0x66, 0x6D, 0x74, 0x20)
BlocSize(4 octets) : Nombre d'octets du bloc - 16  (0x10)

AudioFormat(2 octets) : Format du stockage dans le fichier(1: PCM, ...)
NbrCanaux(2 octets) : Nombre de canaux(de 1 à 6, cf.ci - dessous)
Frequence(4 octets) : Fréquence d'échantillonnage (en hertz) [Valeurs standardisées : 11 025, 22 050, 44 100 et éventuellement 48 000 et 96 000]
BytePerSec(4 octets) : Nombre d'octets à lire par seconde (c.-à-d., Frequence * BytePerBloc).
BytePerBloc(2 octets) : Nombre d'octets par bloc d'échantillonnage(c. - à - d., tous canaux confondus : NbrCanaux * BitsPerSample / 8).
BitsPerSample(2 octets) : Nombre de bits utilisés pour le codage de chaque échantillon(8, 16, 24)

[Bloc des données]
DataBlocID(4 octets) : Constante «data»(0x64, 0x61, 0x74, 0x61)
DataSize(4 octets) : Nombre d'octets des données (c.-à-d. "Data[]", c.-à-d. taille_du_fichier - taille_de_l'entête(qui fait 44 octets normalement).
DATAS[] : [Octets du Sample 1 du Canal 1] [Octets du Sample 1 du Canal 2] [Octets du Sample 2 du Canal 1] [Octets du Sample 2 du Canal 2]

* Les Canaux :
1 pour mono,
2 pour stéréo
3 pour gauche, droit et centre
4 pour face gauche, face droit, arrière gauche, arrière droit
5 pour gauche, centre, droit, surround(ambiant)
6 pour centre gauche, gauche, centre, centre droit, droit, surround(ambiant)

NOTES IMPORTANTES : Les octets des mots sont stockés sous la forme(c. - à - d., en "little endian")
[87654321][16..9][24..17][8..1][16..9][24..17][...
*/

#include <cstdint>
using byte_t = unsigned char;

enum class byte_swizzling_t : uint32_t
{
	big_endian       = 0x00010203u,
	little_endian    = 0x03020100u,
	pdp_endian       = 0x01000302u,
	honeywell_endian = 0x02030001u,
};
// SBTODO : cpu_swizzling, gpu_swizzling
static constexpr byte_swizzling_t cpu_swizzling = byte_swizzling_t::little_endian;
static constexpr byte_swizzling_t gpu_swizzling = byte_swizzling_t::little_endian;


//template< byte_swizzling_t swizzling = cpu_swizzling, size_t count >
//constexpr uint32_t make_cc();


template< byte_swizzling_t swizzling = cpu_swizzling >
constexpr uint32_t fourcc(const byte_t tag0, const byte_t tag1, const byte_t tag2, const byte_t tag3);

template<> constexpr uint32_t fourcc<byte_swizzling_t::little_endian   >(const byte_t tag0, const byte_t tag1, const byte_t tag2, const byte_t tag3) { return (tag3 << 24u) | (tag2 << 16u) | (tag1 << 8u) | (tag0 << 0u); };
template<> constexpr uint32_t fourcc<byte_swizzling_t::big_endian      >(const byte_t tag0, const byte_t tag1, const byte_t tag2, const byte_t tag3) { return (tag0 << 24u) | (tag1 << 16u) | (tag2 << 8u) | (tag3 << 0u); };
template<> constexpr uint32_t fourcc<byte_swizzling_t::pdp_endian      >(const byte_t tag0, const byte_t tag1, const byte_t tag2, const byte_t tag3) { return (tag1 << 24u) | (tag0 << 16u) | (tag3 << 8u) | (tag2 << 0u); };
template<> constexpr uint32_t fourcc<byte_swizzling_t::honeywell_endian>(const byte_t tag0, const byte_t tag1, const byte_t tag2, const byte_t tag3) { return (tag2 << 24u) | (tag3 << 16u) | (tag0 << 8u) | (tag1 << 0u); };
static_assert( fourcc<byte_swizzling_t::little_endian   >('\0', '\1', '\2', '\3') == static_cast<uint32_t>(byte_swizzling_t::little_endian   ), "Incorrect fourcc" );
static_assert( fourcc<byte_swizzling_t::big_endian      >('\0', '\1', '\2', '\3') == static_cast<uint32_t>(byte_swizzling_t::big_endian      ), "Incorrect fourcc" );
static_assert( fourcc<byte_swizzling_t::pdp_endian      >('\0', '\1', '\2', '\3') == static_cast<uint32_t>(byte_swizzling_t::pdp_endian      ), "Incorrect fourcc" );
static_assert( fourcc<byte_swizzling_t::honeywell_endian>('\0', '\1', '\2', '\3') == static_cast<uint32_t>(byte_swizzling_t::honeywell_endian), "Incorrect fourcc" );

constexpr uint32_t platform_endianness = fourcc('\0', '\1', '\2', '\3');
static_assert( platform_endianness == static_cast<uint32_t>(cpu_swizzling), "Wrong CPU endianness set" );


struct SBWavChunk
{
	uint32_t tag;      // big endian
	uint32_t dataSize; // little endian
};

struct SBWavRiffChunk : SBWavChunk
{
	constexpr SBWavRiffChunk() : SBWavChunk{ fourcc<byte_swizzling_t::big_endian>('R', 'I', 'F', 'F'), 4u } {}
	uint32_t formatID = fourcc<byte_swizzling_t::big_endian>( 'W', 'A', 'V', 'E' ); // big endian
};
static_assert(SBWavRiffChunk().tag == 0x52494646, "Wrong RIFF tag");
static_assert(SBWavRiffChunk().formatID == 0x57415645, "Wrong WAV tag");

enum class SBWavAudioCodec : uint16_t
{
	WAVE_FORMAT_UNKNOWN = 				  0x0000u,
	WAVE_FORMAT_PCM = 					  0x0001u,
	WAVE_FORMAT_ADPCM = 				  0x0002u,
	WAVE_FORMAT_IEEE_FLOAT = 			  0x0003u,
	WAVE_FORMAT_VSELP = 				  0x0004u,
	WAVE_FORMAT_IBM_CVSD = 				  0x0005u,
	WAVE_FORMAT_ALAW = 					  0x0006u,
	WAVE_FORMAT_MULAW = 				  0x0007u,

	WAVE_FORMAT_OKI_ADPCM = 			  0x0010u,
	WAVE_FORMAT_DVI_ADPCM = 			  0x0011u,
	WAVE_FORMAT_MEDIASPACE_ADPCM = 		  0x0012u,
	WAVE_FORMAT_SIERRA_ADPCM = 			  0x0013u,
	WAVE_FORMAT_G723_ADPCM = 			  0x0014u,
	WAVE_FORMAT_DIGISTD = 				  0x0015u,
	WAVE_FORMAT_DIGIFIX = 				  0x0016u,
	WAVE_FORMAT_DIALOGIC_OKI_ADPCM = 	  0x0017u,
	WAVE_FORMAT_MEDIAVISION_ADPCM = 	  0x0018u,
	WAVE_FORMAT_CU_CODEC = 				  0x0019u,

	WAVE_FORMAT_YAMAHA_ADPCM = 			  0x0020u,
	WAVE_FORMAT_SONARC = 				  0x0021u,
	WAVE_FORMAT_DSPGROUP_TRUESPEECH = 	  0x0022u,
	WAVE_FORMAT_ECHOSC1 = 				  0x0023u,
	WAVE_FORMAT_AUDIOFILE_AF36 = 		  0x0024u,
	WAVE_FORMAT_APTX = 					  0x0025u,
	WAVE_FORMAT_AUDIOFILE_AF10 = 		  0x0026u,
	WAVE_FORMAT_PROSODY_1612 = 			  0x0027u,
	WAVE_FORMAT_LRC = 					  0x0028u,

	WAVE_FORMAT_DOLBY_AC2 = 			  0x0030u,
	WAVE_FORMAT_GSM610 = 				  0x0031u,
	WAVE_FORMAT_MSNAUDIO = 				  0x0032u,
	WAVE_FORMAT_ANTEX_ADPCME = 			  0x0033u,
	WAVE_FORMAT_CONTROL_RES_VQLPC = 	  0x0034u,
	WAVE_FORMAT_DIGIREAL = 				  0x0035u,
	WAVE_FORMAT_DIGIADPCM = 			  0x0036u,
	WAVE_FORMAT_CONTROL_RES_CR10 = 		  0x0037u,
	WAVE_FORMAT_NMS_VBXADPCM = 			  0x0038u,
	WAVE_FORMAT_ROLAND_RDAC = 			  0x0039u,
	WAVE_FORMAT_ECHOSC3 = 				  0x003Au,
	WAVE_FORMAT_ROCKWELL_ADPCM = 		  0x003Bu,
	WAVE_FORMAT_ROCKWELL_DIGITALK = 	  0x003Cu,
	WAVE_FORMAT_XEBEC = 				  0x003Du,

	WAVE_FORMAT_G721_ADPCM = 			  0x0040u,
	WAVE_FORMAT_G728_CELP = 			  0x0041u,
	WAVE_FORMAT_MSG723 = 				  0x0042u,

	WAVE_FORMAT_MPEG = 					  0x0050u,

	WAVE_FORMAT_RT24 = 					  0x0052u,
	WAVE_FORMAT_PAC = 					  0x0053u,

	WAVE_FORMAT_MPEGLAYER3 = 			  0x0055u,

	WAVE_FORMAT_LUCENT_G723 = 			  0x0059u,

	WAVE_FORMAT_CIRRUS = 				  0x0060u,
	WAVE_FORMAT_ESPCM = 				  0x0061u,
	WAVE_FORMAT_VOXWARE = 				  0x0062u,
	WAVE_FORMAT_CANOPUS_ATRAC = 		  0x0063u,
	WAVE_FORMAT_G726_ADPCM = 			  0x0064u,
	WAVE_FORMAT_G722_ADPCM = 			  0x0065u,
	WAVE_FORMAT_DSAT = 					  0x0066u,
	WAVE_FORMAT_DSAT_DISPLAY = 			  0x0067u,

	WAVE_FORMAT_VOXWARE_BYTE_ALIGNED = 	  0x0069u,

	WAVE_FORMAT_VOXWARE_AC8 = 			  0x0070u,
	WAVE_FORMAT_VOXWARE_AC10 = 			  0x0071u,
	WAVE_FORMAT_VOXWARE_AC16 = 			  0x0072u,
	WAVE_FORMAT_VOXWARE_AC20 = 			  0x0073u,
	WAVE_FORMAT_VOXWARE_RT24 = 			  0x0074u,
	WAVE_FORMAT_VOXWARE_RT29 = 			  0x0075u,
	WAVE_FORMAT_VOXWARE_RT29HW = 		  0x0076u,
	WAVE_FORMAT_VOXWARE_VR12 = 			  0x0077u,
	WAVE_FORMAT_VOXWARE_VR18 = 			  0x0078u,
	WAVE_FORMAT_VOXWARE_TQ40 = 			  0x0079u,

	WAVE_FORMAT_SOFTSOUND = 			  0x0080u,
	WAVE_FORMAT_VOXWARE_TQ60 = 			  0x0081u,
	WAVE_FORMAT_MSRT24 = 				  0x0082u,
	WAVE_FORMAT_G729A = 				  0x0083u,
	WAVE_FORMAT_MVI_MV12 = 				  0x0084u,
	WAVE_FORMAT_DF_G726 = 				  0x0085u,
	WAVE_FORMAT_DF_GSM610 = 			  0x0086u,
	WAVE_FORMAT_ISIAUDIO = 				  0x0088u,
	WAVE_FORMAT_ONLIVE = 				  0x0089u,

	WAVE_FORMAT_SBC24 = 				  0x0091u,
	WAVE_FORMAT_DOLBY_AC3_SPDIF = 		  0x0092u,

	WAVE_FORMAT_ZYXEL_ADPCM = 			  0x0097u,
	WAVE_FORMAT_PHILIPS_LPCBB = 		  0x0098u,
	WAVE_FORMAT_PACKED = 				  0x0099u,

	WAVE_FORMAT_RHETOREX_ADPCM = 		  0x0100u,
	WAVE_FORMAT_IRAT = 					  0x0101u,

	WAVE_FORMAT_VIVO_G723 = 			  0x0111u,
	WAVE_FORMAT_VIVO_SIREN = 			  0x0112u,

	WAVE_FORMAT_DIGITAL_G723 = 			  0x0123u,

	WAVE_FORMAT_CREATIVE_ADPCM = 		  0x0200u,

	WAVE_FORMAT_CREATIVE_FASTSPEECH8 = 	  0x0202u,
	WAVE_FORMAT_CREATIVE_FASTSPEECH10 =   0x0203u,

	WAVE_FORMAT_QUARTERDECK = 			  0x0220u,

	WAVE_FORMAT_FM_TOWNS_SND = 			  0x0300u,

	WAVE_FORMAT_BTV_DIGITAL = 			  0x0400u,

	WAVE_FORMAT_VME_VMPCM = 			  0x0680u,

	WAVE_FORMAT_OLIGSM = 				  0x1000u,
	WAVE_FORMAT_OLIADPCM = 				  0x1001u,
	WAVE_FORMAT_OLICELP = 				  0x1002u,
	WAVE_FORMAT_OLISBC = 				  0x1003u,
	WAVE_FORMAT_OLIOPR = 				  0x1004u,

	WAVE_FORMAT_LH_CODEC = 				  0x1100u,

	WAVE_FORMAT_NORRIS = 				  0x1400u,
	WAVE_FORMAT_ISIAUDIO_2 = 			  0x1401u,

	WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS = 0x1500u,

	WAVE_FORMAT_DVM = 					  0x2000u,
};

struct SBWavFmtChunk : SBWavChunk
{
	constexpr SBWavFmtChunk(
			uint32_t tag = fourcc<byte_swizzling_t::big_endian>('f', 'm', 't', ' '), uint32_t dataSize = 16u,
			SBWavAudioCodec codecID = SBWavAudioCodec::WAVE_FORMAT_PCM,
			uint16_t     numChannels = 0,
			uint32_t     sampleRate = 0,
			uint32_t     byteRate = 0,      // sampleRate * numChannels * ( bitsPerSample / CHAR_BIT )
			uint16_t     blockAlign = 0,    // numChannels * ( bitsPerSample / CHAR_BIT )
			uint16_t     bitsPerSample = 0  // numChannels * ( bitsPerSample / CHAR_BIT )
		)
		: SBWavChunk{ tag, dataSize },
			codecID(codecID),
			numChannels(numChannels),
			sampleRate(sampleRate),
			byteRate(byteRate),      // sampleRate * numChannels * ( bitsPerSample / CHAR_BIT )
			blockAlign(blockAlign),    // numChannels * ( bitsPerSample / CHAR_BIT )
			bitsPerSample(bitsPerSample)  // numChannels * ( bitsPerSample / CHAR_BIT )
	{}
	SBWavAudioCodec codecID = SBWavAudioCodec::WAVE_FORMAT_PCM;
	uint16_t     numChannels = 0;   // little endian
	uint32_t     sampleRate = 0;    // little endian
	uint32_t     byteRate = 0;      // little endian; sampleRate * numChannels * ( bitsPerSample / CHAR_BIT )
	uint16_t     blockAlign = 0;    // little endian; numChannels * ( bitsPerSample / CHAR_BIT )
	uint16_t     bitsPerSample = 0; // little endian; numChannels * ( bitsPerSample / CHAR_BIT )
};
static_assert(SBWavFmtChunk().tag == 0x666d7420, "Wrong wav fmt tag");

struct SBWavFmtEXChunk : SBWavFmtChunk
{
	constexpr SBWavFmtEXChunk() : SBWavFmtChunk{ fourcc<byte_swizzling_t::big_endian>('f', 'm', 't', ' '), 18u, SBWavAudioCodec::WAVE_FORMAT_UNKNOWN } {}
	uint16_t     extraParamSize = 0; // little endian; doesn't exist for PCM
};
static_assert(SBWavFmtEXChunk().tag == 0x666d7420, "Wrong wav fmt ex tag");

struct SBWavDataChunk : SBWavChunk
{
	constexpr SBWavDataChunk() : SBWavChunk{ fourcc<byte_swizzling_t::big_endian>('d', 'a', 't', 'a'), 0u } {}
};
static_assert(SBWavDataChunk().tag == 0x64617461, "Wrong wav data tag");

