/* file : logic_debugger.h
 *
 * debugger for logic parser
 *
 * create by duan 
 *
 * 2017.5.2
 */

#ifndef _LOGIC_DEBUGGER_H_
#define _LOGIC_DEBUGGER_H_

#include "logic_parser/logicParser.h"
#include "ndapplib/ndsingleton.h"

#include <vector>
#include <string>

struct BreakPointInfo
{
	bool tempBreak;
	std::string functionName;
	std::string nodeName;

	BreakPointInfo(const char *func, const char *node,bool bTemp) :tempBreak(bTemp),functionName(func), nodeName(node)
	{

	}

	bool operator<(const BreakPointInfo &r)const
	{
		if (functionName < r.functionName && nodeName < r.nodeName) {
			return true;
		}
		return false;
	}

	bool operator==(const BreakPointInfo &r)const
	{
		if (0 == ndstricmp(functionName.c_str(), r.functionName.c_str()) && 
			0 == ndstricmp(nodeName.c_str(), r.nodeName.c_str()))
		{
			return true;
		}
		return false;
	}
	
};
typedef std::vector<BreakPointInfo> breakPoint_vct;
//////////////////////////////////////////////////////////////////////////

// processHeaderInfo[LOGIC_MAX_PROCESS] stored in share memory name "apoLogicDebugHeader"

#if defined(__ND_WIN__)
#define PROCESS_NAME_SIZE 64
#define LOGIC_MAX_PROCESS 10
#define LOGIC_DEBUGER_HEADER_NAME "apoLogicDebugHeader"
#else
#define PROCESS_NAME_SIZE 128
#define LOGIC_MAX_PROCESS 10
#define LOGIC_DEBUGER_HEADER_NAME "/tmp/apoLogicDebugHeader"
#endif

enum parserRunStat
{
	E_RUN_DBG_STAT_UNKNOWN = -1,
	E_RUN_DBG_STAT_TERMINATE= 0,
	E_RUN_DBG_STAT_RUNNING,
	E_RUN_DBG_STAT_WAIT_CMD,
	
};

enum parserDebugInputCmd
{
	E_DBG_INPUT_CMD_NONE,
	E_DBG_INPUT_CMD_NEW_BREAKPOINT,
	E_DBG_INPUT_CMD_STEP_RUN,
	E_DBG_INPUT_CMD_CONTINUE,
	E_DBG_INPUT_CMD_RUN_TO_CURSOR,
	E_DBG_INPUT_CMD_RUN_OUT,
	E_DBG_INPUT_CMD_DEL_BREAKPOINT,
	E_DBG_INPUT_CMD_TERMINATED,
	E_DBG_INPUT_CMD_ADD_TEMP_BREAKPOINT,
	E_DBG_INPUT_CMD_ADD_BREAKPOINT_BATCH,

	E_DBG_INPUT_CMD_ATTACHED,
	E_DBG_INPUT_CMD_DEATTACHED,
	E_DBG_INPUT_CMD_CONSOLE_ACK,


	E_DBG_OUTPUT_CMD_HIT_BREAKPOINT,
	E_DBG_OUTPUT_CMD_STEP,
	E_DBG_OUTPUT_CMD_TERMINATED,
	E_DBG_OUTPUT_CMD_HANDLE_ACK,
	E_DBG_OUTPUT_CMD_SCRIPT_RUN_OK,
	
	E_DBG_OUTPUT_CMD_DEATTACHED,

	E_DBG_OUTPUT_CMD_CONSOLE_INPUT,

};

struct processHeaderInfo 
{
	NDUINT32 processId;		//nd_processid()
	NDUINT32 debuggerProcessId; 
	NDUINT32 isDebugMode;	// the process is run debug (1) code or release code(0)
	ndatomic_t runStat;		//0 common/not-run , 1 running, 2 wait-debug, 3 terminate 
	ndatomic_t inputCmd;	//0 none , 1 input break point, 2 run-step, 3 run-continue , 4 run-to-current-node
	ndatomic_t srvHostCmd;	//debug host thread host cmd flag

	ndatomic_t outputCmd;	// parse output command
	NDUINT32 cmdEncodeType;
	char processName[PROCESS_NAME_SIZE];
	char shareMemName[PROCESS_NAME_SIZE];
	char semName[PROCESS_NAME_SIZE];
	char semClient[PROCESS_NAME_SIZE];

	char semCMDth[PROCESS_NAME_SIZE];	//the command receiver thread would block this.
	char scriptModule[128];

	char cmdBuf[4096];
};

struct LogicRunningProcess
{
	NDUINT32 pId;
	std::string name;
	std::string scriptModule;
	LogicRunningProcess(NDUINT32 pid, const char *inname, const char *module) :pId(pid), name(inname), scriptModule(module)
	{}
};

typedef std::vector<LogicRunningProcess> Process_vct_t;

class LOGIC_PARSER_CLASS ShareProcessLists
{
public:
	ShareProcessLists();
	~ShareProcessLists();


	static ShareProcessLists*get_Instant();
	static void destroy_Instant();

	int Create(const char*name=NULL);
	void Destroy(int flag = 0);

	processHeaderInfo *createProcessInfo(NDUINT32 processId, const char *procName,const char *moduleName, bool isDebug);
	void delProcess(NDUINT32 processId);

	processHeaderInfo *getProcessInfo(NDUINT32 processId);

	bool getProcesses(Process_vct_t &proVct);

public:

};
// 
// class LOGIC_PARSER_CLASS LogicRemoteDebuggerCommonBase
// {
// 
// };
class LOGIC_PARSER_CLASS LogicDebugClient
{
public:
	LogicDebugClient();
	virtual ~LogicDebugClient();

	//command for client
	int localStart(LogicParserEngine *parser, int argc, const char *argv[], int encodeType = ND_ENCODE_TYPE);
	int Attach(NDUINT32 processId, int cmdEncodeType );
	int Deattach();

	int postEndDebug();
	
	int cmdStep(){ return inputCmd(E_DBG_INPUT_CMD_STEP_RUN, E_RUN_DBG_STAT_WAIT_CMD); }
	int cmdContinue(){ return inputCmd(E_DBG_INPUT_CMD_CONTINUE, E_RUN_DBG_STAT_WAIT_CMD); }
	int cmdRunOut(){ return inputCmd(E_DBG_INPUT_CMD_RUN_OUT, E_RUN_DBG_STAT_WAIT_CMD); }
	int cmdRunTo(const char *func, const char *node){ return inputCmd(E_DBG_INPUT_CMD_RUN_TO_CURSOR, func, node); }
	int cmdStopDebug() { return inputCmd(E_DBG_INPUT_CMD_TERMINATED); }
	int cmdTerminate();

	int cmdAddBreakPoint(const char *funcName, const char *nodeName, bool isTemp = false);
	int cmdDelBreakPoint(const char *funcName, const char *nodeName);
	int cmdAddBreakPointBatch(ndxml *breakPointXml);

	virtual bool onEnterStep(const char *function, const char *node);
	virtual void onTerminate();
	virtual void onCommandOk();
	virtual void onScriptRunOk();
	virtual bool onConsoleInput(std::string &text);

	ndxml *getParserInfo();

	static bool getRunningProcess(Process_vct_t &processes);

	int waitEvent();
protected:

	ndsem_t getHostSem();
	ndsem_t getClientSem();

	int inputCmd(int cmdId, parserRunStat needRunStat= E_RUN_DBG_STAT_UNKNOWN);
	int inputCmd(int cmdId, const char *func, const char *nodeName);
	int inputToCmdThread(int cmdId, const char *func, const char *nodeName);

	NDUINT32 m_aimProcessId;
	bool m_isAttached;
	ndsem_t m_cmdSemToSrvHost;	//wait client cmd , that debugger-host thread wait it
	ndth_handle m_thCmdRecver;
};

//////////////////////////////////////////////////////////////////////////

//debug host
class LOGIC_PARSER_CLASS LocalDebugger
{
	friend class LogicDebugClient;
public:
	LocalDebugger(LogicParserEngine *parser,LogicObjectBase *owner=NULL);
	~LocalDebugger();

	int runCmdline(int argc, const char *argv[], int encodeType = ND_ENCODE_TYPE); 
	int runHost();
	void stopHost();

	int addBreakPoint(const char *funcName, const char *nodeName,bool isTemp=false);
	int delBreakPoint(const char *funcName, const char *nodeName);
	bool addBreakpointBatch(ndxml *breakpoints);
	void clearBreakpoint();
	
	bool requestConsoleInput(std::string &inText);
	//run on parser
	int onEnterStep(LogicParserEngine *parser,const char *funcName, const char *nodeName);
	int ScriptRunOk(LogicParserEngine *parser);

	ndxml_root & getParserInfo();
	int getLastError() { return m_parser->getErrno(); }

	LogicParserEngine *getParser() { return m_parser; }
	void setParser(LogicParserEngine *parser) { m_parser = parser; }
	int getEncodeType() { return m_encodeType; }

	//void setClient(LogicDebugClient *client) { m_client = client; }
	ndsem_t getClientSem() { return m_cliSem; }
	int waitClientCmd();
	breakPoint_vct getBreakPoints() { return m_breakPoints; }

private:

	bool preStartDebug(NDUINT32 processId, bool withDebugInfo=true);
	bool postEndDebug();
	bool startCMDRecvTh();

	int waitEvent(const char *funcName, const char *nodeName);
	int ntfClient(int cmd, const char *funcName, const char *nodeName);

	int m_encodeType;
	bool m_bStep;
	bool m_bRunOut;		//run leave function-and-break
	bool m_getConsoleInput;

	bool isBreakPoint(const char *func, const char *node, bool bTrytoDel=false);
	bool makeParserInfo();
	bool getParamFromInput(std::string &funcName, std::string &nodeName);
	bool getInputBreakpoints();
	bool OutPutInfo(ndxml *dataInfo);

	LogicParserEngine *m_parser;
	ndxml_root m_parserInfo;
	breakPoint_vct m_breakPoints;

	//LogicDebugClient *m_client;

	//debug runtime object
	nd_filemap_t m_outPutMem;
	ndsem_t m_RunningSem;
	ndsem_t m_cliSem; // the client thread wait this sem
	ndsem_t m_cmdSem;	//wait client cmd
	ndth_handle m_thHost;
};


#endif
