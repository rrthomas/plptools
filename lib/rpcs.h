#ifndef _rpcs_h_
#define _rpcs_h_

#include "psitime.h"
#include "rfsv.h"
#include "Enum.h"

class ppsocket;
class bufferStore;
class bufferArray;

/**
 * Remote procedure call services via PLP
 *
 * rpcs provides an interface for communicating
 * with the Psion's remote procedure call
 * service. The generic facilities for both,
 * EPOC and SIBO are implemented here, while
 * the facilities, unique to each of those
 * variants are implemented in
 * @ref rpcs32 or @ref rpcs16 respectively.
 * These normally are instantiated by using
 * @ref rpcsfactory .
 */
class rpcs {
	public:
		/**
		 * The known machine types.
		 */
		enum machs {
			PSI_MACH_UNKNOWN = 0,
			PSI_MACH_PC = 1,
			PSI_MACH_MC = 2,
			PSI_MACH_HC = 3,
			PSI_MACH_S3 = 4,
			PSI_MACH_S3A = 5,
			PSI_MACH_WORKABOUT = 6,
			PSI_MACH_SIENNA = 7,
			PSI_MACH_S3C = 8,
			PSI_MACH_S5 = 32,
			PSI_MACH_WINC = 33,
			// TODO: Code for 5mx
		};

		/**
		 * The known interface languages.
		 */
		enum languages {
			PSI_LANG_TEST	= 0,
			PSI_LANG_en_GB	= 1,
			PSI_LANG_fr_FR	= 2,
			PSI_LANG_de_DE	= 3,
			PSI_LANG_es_ES	= 4,
			PSI_LANG_it_IT	= 5,
			PSI_LANG_sv_SE	= 6,
			PSI_LANG_da_DK	= 7,
			PSI_LANG_no_NO	= 8,
			PSI_LANG_fi_FI	= 9,
			PSI_LANG_en_US	= 10,
			PSI_LANG_fr_CH	= 11,
			PSI_LANG_de_CH	= 12,
			PSI_LANG_pt_PT	= 13,
			PSI_LANG_tr_TR	= 14,
			PSI_LANG_is_IS	= 15,
			PSI_LANG_ru_RU	= 16,
			PSI_LANG_hu_HU	= 17,
			PSI_LANG_nl_NL	= 18,
			PSI_LANG_nl_BE	= 19,
			PSI_LANG_en_AU	= 20,
			PSI_LANG_fr_BE	= 21,
			PSI_LANG_de_AT	= 22,
			PSI_LANG_en_NZ	= 23,
			PSI_LANG_fr_CA	= 24,
			PSI_LANG_cs_CZ	= 25,
			PSI_LANG_sk_SK	= 26,
			PSI_LANG_pl_PL	= 27,
			PSI_LANG_sl_SI	= 28,
		};

		/**
		 * The known battery states.
		 */
		enum batterystates {
			PSI_BATT_DEAD = 0,
			PSI_BATT_VERYLOW = 1,
			PSI_BATT_LOW = 2,
			PSI_BATT_GOOD = 3,
		};

		/**
 		* This struct holds the data returned
 		* by @ref rpcs::getMachineInfo.
 		*/
		typedef struct machineInfo_t {
			Enum<machs> machineType;
			char machineName[17];
			unsigned long long machineUID;
			unsigned long countryCode;
			Enum<languages> uiLanguage;

			unsigned short romMajor;
			unsigned short romMinor;
			unsigned short romBuild;
			unsigned long romSize;
			bool romProgrammable;

			unsigned long ramSize;
			unsigned long ramFree;
			unsigned long ramMaxFree;
			unsigned long ramDiskSize;

			unsigned long registrySize;
			unsigned long displayWidth;
			unsigned long displayHeight;

			psi_timeval time;
			psi_timezone tz;

			psi_timeval mainBatteryInsertionTime;
			Enum<batterystates> mainBatteryStatus;
			psi_timeval mainBatteryUsedTime;
			unsigned long mainBatteryCurrent;
			unsigned long mainBatteryUsedPower;
			unsigned long mainBatteryVoltage;
			unsigned long mainBatteryMaxVoltage;

			Enum<batterystates> backupBatteryStatus;
			unsigned long backupBatteryVoltage;
			unsigned long backupBatteryMaxVoltage;
			psi_timeval backupBatteryUsedTime;

			bool externalPower;
		} machineInfo;

		/**
		 * Provides a virtual destructor.
		 */
		virtual ~rpcs() {};

		/**
		 * Initializes a connection to the remote
		 * machine.
		 */
		void reset();

		/**
		 * Attempts to re-establish a remote
		 * connection by first closing the socket,
		 * then connecting again to the ncpd daemon
		 * and finally calling @ref reset.
		 */
		void reconnect();

		/**
		 * Retrieves the current status of the
		 * connection.
		 *
		 * @returns The connection status.
		 */
		Enum<rfsv::errs> getStatus();

		/**
		 * Retrieves the version of the NCP protocol
		 * on the remote side.
		 *
		 * This function is working with both SIBO and EPOC
		 * devices.
		 *
		 * @param major The major part of the NCP version.
		 * 	Valid only if returned with no error.
		 * @param minor The minor part of the NCP version.
		 * 	Valid only if returned with no error.
		 *
		 * @returns A psion error code. 0 = Ok.
		 */
		Enum<rfsv::errs> getNCPversion(int &major, int &minor);

		/**
		 * Starts execution of a program on the remote machine.
		 *
		 * This function is working with both SIBO and EPOC
		 * devices.
		 *
		 * @param program The full path of the executable
		 * 	on the remote machine
		 * @param args The arguments for this program, separated
		 * 	by space.
		 *
		 * @returns A psion error code. 0 = Ok.
		 */
		Enum<rfsv::errs> execProgram(const char *program, const char *args);

		/**
		 * Requests termination of a program running on the
		 * remote machine.
		 *
		 * This function is working with both SIBO and EPOC
		 * devices.
		 *
		 * @param program
		 *
		 * @returns A psion error code. 0 = Ok.
		 */
		Enum<rfsv::errs> stopProgram(const char *);

		Enum<rfsv::errs> queryProgram(const char *);
		Enum<rfsv::errs> formatOpen(const char *, int &, int &);
		Enum<rfsv::errs> formatRead(int);
		Enum<rfsv::errs> getUniqueID(const char *, long &);

		/**
		 * Retrieve owner information of the remote machine.
		 *
		 * This function is working with both SIBO and EPOC
		 * devices.
		 *
		 * @param owner A bufferArray, containing the lines
		 * 	of the owner info upon return.
		 *
		 * @returns A psion error code. 0 = Ok.
		 */
		Enum<rfsv::errs> getOwnerInfo(bufferArray &);

		/**
		 * Retrieves the type of machine on the remote side
		 * as defined in @ref #machs.
		 *
		 * This function is working with both SIBO and EPOC
		 * devices
		 *
		 * @param type The code describing the type of machine
		 * 	on the remote side is stored here on return.
		 *
		 * @returns A psion error code. 0 = Ok.
		 */
		Enum<rfsv::errs> getMachineType(Enum<machs> &type);

		/**
		 * Retrieves the name of a process, having a
		 * given file opened on the remote side.
		 *
		 * This function is working with both SIBO and EPOC
		 * devices
		 *
		 * @param name The full path of a file to be checked
		 * 	for beeing used by a program.
		 * @param buf A buffer which gets filled with the
		 * 	program's name.
		 * @param maxlen The maximum capacity of the buffer.
		 */
		Enum<rfsv::errs> fuser(const char *name, char *buf, int maxlen);

		/**
		 * Requests the remote server to terminate.
		 *
		 * This function is working with both SIBO and EPOC
		 * devices. There is usually no need to call this
		 * function, because the remote server is automatically
		 * stopped on disconnect.
		 *
		 * @returns A psion error code. 0 = Ok.
		 */
		Enum<rfsv::errs> quitServer(void);

		// API different on SIBO and EPOC
		virtual Enum<rfsv::errs> queryDrive(const char, bufferArray &) = 0;
		/**
		 * Retrieves the command line of a running process.
		 *
		 * This function works with EPOC only. Using it with SIBO
		 * machines, returns always an error code E_PSI_NOT_SIBO.
		 *
		 * @param process Name of process. Format: processname.$pid
		 * @param ret The program name and arguments are returned here.
		 *
		 * @return Psion error code. 0 = Ok.
		 */
		virtual Enum<rfsv::errs> getCmdLine(const char *process, bufferStore &ret) = 0; 
		/**
		 * Retrieve general Information about the connected
		 * machine.
		 *
		 * This function works with EPOC only. Using it with SIBO
		 * machines, returns always an error code E_PSI_NOT_SIBO.
		 *
		 * @param machineInfo The struct holding all information on return.
		 * @return Psion error code. 0 = Ok.
		 */
		virtual Enum<rfsv::errs> getMachineInfo(machineInfo &) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> closeHandle(int) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> regOpenIter(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> regReadIter(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> regWrite(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> regRead(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> regDelete(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> setTime(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> configOpen(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> configRead(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> configWrite(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> queryOpen(void) { return rfsv::E_PSI_NOT_SIBO;}
		virtual Enum<rfsv::errs> queryRead(void) { return rfsv::E_PSI_NOT_SIBO;}

	protected:
		/**
		 * The socket, used for communication
		 * with ncpd.
		 */
		ppsocket *skt;

		/**
		 * The current status of the connection.
		 */
		Enum<rfsv::errs> status;

		/**
		 * The possible commands.
		 */
		enum commands {
			QUERY_NCP        = 0x00,
			EXEC_PROG        = 0x01,
			QUERY_DRIVE      = 0x02,
			STOP_PROG        = 0x03,
			QUERY_PROG       = 0x04,
			FORMAT_OPEN      = 0x05,
			FORMAT_READ      = 0x06,
			GET_UNIQUEID     = 0x07,
			GET_OWNERINFO    = 0x08,
			GET_MACHINETYPE  = 0x09,
			GET_CMDLINE      = 0x0a,
			FUSER            = 0x0b,
			GET_MACHINE_INFO = 0x64,
			REG_OPEN_ITER    = 0x66,
			REG_READ_ITER    = 0x67,
			REG_WRITE        = 0x68,
			REG_READ         = 0x69,
			REG_DELETE       = 0x6a,
			SET_TIME         = 0x6b,
			CONFIG_OPEN      = 0x6c,
			CONFIG_READ      = 0x6d,
			CONFIG_WRITE     = 0x6e,
			QUERY_OPEN       = 0x6f,
			QUERY_READ       = 0x70,
			QUIT_SERVER      = 0xff
		};

		/**
		 * Sends a command to the remote side.
		 *
		 * If communication fails, a reconnect is triggered
		 * and a second attempt to transmit the request
		 * is attempted. If that second attempt fails,
		 * the function returns an error an sets rpcs::status
		 * to E_PSI_FILE_DISC.
		 *
		 * @param cc The command to execute on the remote side.
		 * @param data Additional data for this command.
		 *
		 * @returns true on success, false on failure.
		 */
		bool sendCommand(enum commands cc, bufferStore &data);
		Enum<rfsv::errs> getResponse(bufferStore &data, bool statusIsFirstByte);
		const char *getConnectName();
};

#endif
