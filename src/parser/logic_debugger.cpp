/* file : logic_debugger.cpp
 *
 * debugger for logic parser
 *
 * create by duan 
 *
 * 2017.5.2
 */

#include "logic_parser/logic_debugger.h"
#include "logic_parser/logicEngineRoot.h"

static nd_filemap_t g_debugHeader = { 0 };
ShareProcessLists::ShareProcessLists()
{
	//m_RunningSem = 0;
	//m_cliSem = 0;

}

ShareProcessLists::~ShareProcessLists()
{
}


ShareProcessLists*ShareProcessLists::get_Instant()
{
	return NDSingleton<ShareProcessLists>::Get();
}
void ShareProcessLists::destroy_Instant()
{
	NDSingleton<ShareProcessLists>::Destroy();
}


int ShareProcessLists::Create(const char*)
{
	size_t size = sizeof(processHeaderInfo) * LOGIC_MAX_PROCESS;
	int ret = nd_mem_share_create(LOGIC_DEBUGER_HEADER_NAME,  size, &g_debugHeader);
	if (-1 == ret) {
		return -1;
	}
	return 0;
}
void ShareProcessLists::Destroy(int)
{
	if (nd_mem_share_addr(&g_debugHeader)) {
		nd_mem_share_close(&g_debugHeader);
	}
	
}


processHeaderInfo *ShareProcessLists::createProcessInfo(NDUINT32 processId, const char *procName, const char *moduleName,bool isDebug)
{
	if (!nd_mem_share_addr(&g_debugHeader)) {
		nd_logerror("The program not be init success\n");
		return NULL;
	}

	processHeaderInfo* pAddr = NULL;
	processHeaderInfo *proHeader = (processHeaderInfo *)nd_mem_share_addr(&g_debugHeader);
	for (int i = 0; i < LOGIC_MAX_PROCESS; i++){
		if (proHeader[i].processId == processId){
			pAddr = &proHeader[i];
			break;
		}
		else if (0 == ndstricmp(proHeader[i].processName, procName) && 
			0 == ndstricmp(proHeader[i].scriptModule, moduleName)) {
			pAddr = &proHeader[i];
			pAddr->processId = processId;
			break;
		}
	}
	if (!pAddr)	{
		pAddr = getProcessInfo(0);
		if (!pAddr)	{
			return NULL;
		}
	}
	

	pAddr->processId = processId;
	pAddr->debuggerProcessId = 0;
	pAddr->isDebugMode = isDebug ? 1 : 0;

	ndstrncpy(pAddr->processName, procName, PROCESS_NAME_SIZE);
#if defined(__ND_WIN__)
	ndsnprintf(pAddr->shareMemName, PROCESS_NAME_SIZE, "%s_smm", procName);
#else
	ndsnprintf(pAddr->shareMemName, PROCESS_NAME_SIZE, "/tmp/%s_smm", procName);
#endif
	ndsnprintf(pAddr->semName, PROCESS_NAME_SIZE, "%s_sem", procName);
	ndsnprintf(pAddr->semClient, PROCESS_NAME_SIZE, "%s_semCli", procName);
	ndsnprintf(pAddr->semCMDth, PROCESS_NAME_SIZE, "%s_cmdSem", procName);
	pAddr->scriptModule[0] = 0;

	ndstrncpy(pAddr->scriptModule, moduleName, sizeof(pAddr->scriptModule));

	pAddr->runStat = E_RUN_DBG_STAT_TERMINATE;
	pAddr->inputCmd = 0;
	pAddr->outputCmd = 0;
	pAddr->srvHostCmd = 0;
	pAddr->cmdEncodeType = ND_ENCODE_TYPE;
	
	return pAddr;
}

void ShareProcessLists::delProcess(NDUINT32 processId)
{
	processHeaderInfo *proceInfo = getProcessInfo(processId);
	if (!proceInfo)	{
		return;
	}

	proceInfo->processId = 0;
}

processHeaderInfo *ShareProcessLists::getProcessInfo(NDUINT32 processId)
{
	if (!nd_mem_share_addr(&g_debugHeader)) {
		return NULL;
	}

	processHeaderInfo *proHeader = (processHeaderInfo *)nd_mem_share_addr(&g_debugHeader);
	for (int i = 0; i < LOGIC_MAX_PROCESS; i++){
		if (proHeader[i].processId == processId){
			return &proHeader[i];
		}
	}
	return NULL;
}

bool ShareProcessLists::getProcesses(Process_vct_t &proVct)
{
	if (!nd_mem_share_addr(&g_debugHeader)) {
		return false;
	}

	processHeaderInfo *proHeader = (processHeaderInfo *)nd_mem_share_addr(&g_debugHeader);
	for (int i = 0; i < LOGIC_MAX_PROCESS; i++){
		if (proHeader[i].processId)	{
			proVct.push_back(LogicRunningProcess(proHeader[i].processId, proHeader[i].processName,proHeader[i].scriptModule));
		}
	}
	return true;

}

//////////////////////////////////////////////////////////////////////////


//start an independent thread to run script, this used in local debug mode.
void* parseThreadRun(void* param)
{
	ParserCmdLine *pcmdInfo = (ParserCmdLine*)param;

	LogicEngineRoot *root = LogicEngineRoot::get_Instant();
	nd_assert(root);
	LocalDebugger &debugger = root->getGlobalDebugger();
	LogicParserEngine *parser = debugger.getParser();

	//pre debug 
	NDUINT32 processId = nd_processid();
	ShareProcessLists *pProcessRoot = ShareProcessLists::get_Instant();

	processHeaderInfo * proceInfo = pProcessRoot->getProcessInfo(processId);

	if (!proceInfo)	{
		delete pcmdInfo;
		return (void*)-1;
	}

	proceInfo->debuggerProcessId = nd_processid();
	proceInfo->runStat = 1;
	nd_atomic_set(&proceInfo->inputCmd, E_RUN_DBG_STAT_RUNNING);

	int ret = parser->runCmdline(pcmdInfo->argc, (const char**)pcmdInfo->argv, debugger.getEncodeType());

	delete pcmdInfo;

	proceInfo->runStat = E_RUN_DBG_STAT_TERMINATE;
	nd_atomic_set(&proceInfo->outputCmd, E_DBG_OUTPUT_CMD_TERMINATED);

	ndsem_t  sem = debugger.getClientSem();
	nd_sem_post(sem);
	//pProcessRoot->delProcess(processId);

	if (0 == ret)	{
		return (void*)0;
	}
	return (void*)(size_t)(parser->getErrno());
}

void* cmdReceiverThread(void* param)
{
	LocalDebugger &debugger = *(LocalDebugger*) param;	

	return (void*)(size_t)(debugger.waitClientCmd());

}


void* cliCmdReveiverTh(void* param)
{
	LogicDebugClient &debugger = *(LogicDebugClient*)param;

	while (true)	{
		int ret = debugger.waitEvent();
		if (ret == -1) {
			break;
		}
	}

	return (void*)0;
}

//////////////////////////////////////////////////////////////////////////

LogicDebugClient::LogicDebugClient()
{
	m_cmdSemToSrvHost = 0;
	m_aimProcessId = 0;
	m_isAttached = 0 ;
}
LogicDebugClient::~LogicDebugClient()
{
	if (m_isAttached) {
		Deattach();
	}
	if (m_cmdSemToSrvHost){
		nd_sem_destroy(m_cmdSemToSrvHost);
		m_cmdSemToSrvHost = NULL;
	}

}


int LogicDebugClient::localStart(LogicParserEngine *parser, int argc, const char *argv[], int encodeType)
{
 	m_aimProcessId = nd_processid();

	LocalDebugger & debuggerHost = LogicEngineRoot::get_Instant()->getGlobalDebugger();
	debuggerHost.setParser(parser);
	if (-1 == debuggerHost.runCmdline(argc, argv, encodeType)) {
		return -1;
	}

	if (-1 == waitEvent()) {
		return -1;
	}
	return 0;
}


int LogicDebugClient::Attach(NDUINT32 processId, int cmdEncodeType )
{
	processHeaderInfo *pAddr = ShareProcessLists::get_Instant()->getProcessInfo(processId);
	if (!pAddr)	{
		return -1;
	}
	if (pAddr->isDebugMode == 0) {
		nd_logmsg("The aim process is not surport debug mode!\n");
		return 1;
	}

	m_cmdSemToSrvHost = nd_sem_open(pAddr->semCMDth);
	if (!m_cmdSemToSrvHost)	{
		return -1;
	}

	LocalDebugger & debuggerHost = LogicEngineRoot::get_Instant()->getGlobalDebugger();

	if (!debuggerHost.preStartDebug(processId)) {
		return -1;
	}

	m_isAttached = true;
	m_aimProcessId = processId;
	pAddr->debuggerProcessId = nd_processid();
	pAddr->cmdEncodeType = cmdEncodeType;


	ndthread_t thId = 0;
	m_thCmdRecver = nd_createthread(cliCmdReveiverTh, (void*)this, &thId, 0);
	if (!m_thCmdRecver){
		return -1;
	}

	inputToCmdThread(E_DBG_INPUT_CMD_ATTACHED, NULL, NULL);

	return 0;
}

int LogicDebugClient::Deattach( )
{
	if (!m_aimProcessId) {
		return -1;
	}

	inputToCmdThread(E_DBG_INPUT_CMD_DEATTACHED, NULL, NULL);

	processHeaderInfo *pAddr = ShareProcessLists::get_Instant()->getProcessInfo(m_aimProcessId);
	if (!pAddr)	{
		//cmdContinue();
		return  -1 ;
	}
	
	pAddr->debuggerProcessId = 0;
	
	//notify script-host-thread running
	nd_atomic_set(&pAddr->inputCmd, E_DBG_INPUT_CMD_CONTINUE);
	ndsem_t sem = getHostSem();
	nd_sem_post(sem);
	
	
	if(m_thCmdRecver) {
		nd_atomic_set(&pAddr->outputCmd, E_DBG_OUTPUT_CMD_DEATTACHED);
		ndsem_t cliThSem = getClientSem();
		nd_sem_post(cliThSem);
		
		nd_waitthread(m_thCmdRecver) ;
		m_thCmdRecver = 0;
	}
	
	LocalDebugger & debuggerHost = LogicEngineRoot::get_Instant()->getGlobalDebugger();
	debuggerHost.postEndDebug();
	m_aimProcessId = 0;
	m_isAttached = false;
	if (m_cmdSemToSrvHost)	{
		nd_sem_destroy(m_cmdSemToSrvHost);
		m_cmdSemToSrvHost = NULL;
	}
	
	return 0;

}

int LogicDebugClient::cmdAddBreakPoint(const char *funcName, const char *nodeName, bool isTemp)
{
	if (m_isAttached) {
		return inputToCmdThread(isTemp ? E_DBG_INPUT_CMD_ADD_TEMP_BREAKPOINT : E_DBG_INPUT_CMD_NEW_BREAKPOINT, funcName, nodeName);
	}
	else {
		LocalDebugger & debuggerHost = LogicEngineRoot::get_Instant()->getGlobalDebugger();
		debuggerHost.addBreakPoint(funcName, nodeName,isTemp);
	}
	return 0;
}

int LogicDebugClient::cmdDelBreakPoint(const char *funcName, const char *nodeName)
{
	if (m_isAttached) {
		return inputToCmdThread(E_DBG_INPUT_CMD_DEL_BREAKPOINT, funcName, nodeName);
	}
	else {
		LocalDebugger & debuggerHost = LogicEngineRoot::get_Instant()->getGlobalDebugger();
		debuggerHost.delBreakPoint(funcName, nodeName);
	}
	return 0;
}


int LogicDebugClient::cmdAddBreakPointBatch(ndxml *breakPointXml)
{
	if (!breakPointXml)	{
		return -1;
	}
	if (m_isAttached){
		processHeaderInfo * proceInfo = ShareProcessLists::get_Instant()->getProcessInfo(m_aimProcessId);
		if (!proceInfo)	{
			return -1;
		}

		nd_atomic_set(&proceInfo->srvHostCmd, E_DBG_INPUT_CMD_ADD_BREAKPOINT_BATCH);

		ndxml_to_buf(breakPointXml, proceInfo->cmdBuf, sizeof(proceInfo->cmdBuf));
		//ndsnprintf(proceInfo->cmdBuf, sizeof(proceInfo->cmdBuf), "<node func=\"%s\" exenode=\"%s\" />", func, nodeName);

		nd_sem_post(m_cmdSemToSrvHost);
	}

	return 0;
}

int LogicDebugClient::postEndDebug()
{
	LocalDebugger & debuggerHost = LogicEngineRoot::get_Instant()->getGlobalDebugger();
	debuggerHost.postEndDebug();
	return 0;
}
int LogicDebugClient::cmdTerminate()
{
	if (m_isAttached){
		Deattach();
	}
	else {
		inputCmd(E_DBG_INPUT_CMD_RUN_OUT);
	}
	return 0;
}


int LogicDebugClient::waitEvent()
{
	int ret = 0;
	ShareProcessLists *processList = ShareProcessLists::get_Instant();
	processHeaderInfo * proceInfo = processList->getProcessInfo(m_aimProcessId);

	if (!proceInfo){
		return 0;
	}

	ndsem_t sem = getClientSem();
	ndatomic_t recvCmd = 0;
RE_WIAT:

	ret = nd_sem_wait(sem, INFINITE);
	if (0 != ret)	{
		nd_sem_destroy(sem);
		return -1;
	}
	recvCmd = nd_atomic_read(&proceInfo->outputCmd);
	proceInfo->outputCmd = E_DBG_INPUT_CMD_NONE;
	//handler event 
	switch (recvCmd)
	{
	case E_DBG_OUTPUT_CMD_HIT_BREAKPOINT:
	case E_DBG_OUTPUT_CMD_STEP:
	{
		ndxml *xml = ndxml_from_text(proceInfo->cmdBuf);
		if (xml){
			onEnterStep(ndxml_getattr_val(xml, "func"), ndxml_getattr_val(xml, "exenode"));
			ndxml_free(xml);
		}

		break;
	}
	case E_DBG_OUTPUT_CMD_SCRIPT_RUN_OK:
		onScriptRunOk();
		break;
	case E_DBG_OUTPUT_CMD_TERMINATED:
		onTerminate();
		
		if (m_isAttached){
			return -1;
		}
		break; 
	case E_DBG_OUTPUT_CMD_HANDLE_ACK:
		onCommandOk();
		goto RE_WIAT;
		break;
	case E_DBG_OUTPUT_CMD_DEATTACHED:
		return -1;
		break ;
	case E_DBG_OUTPUT_CMD_CONSOLE_INPUT:
	{
		std::string str1;
		if (onConsoleInput(str1)) {			
			nd_atomic_set(&proceInfo->inputCmd, E_DBG_INPUT_CMD_CONSOLE_ACK);
			strncpy(proceInfo->cmdBuf, str1.c_str(), sizeof(proceInfo->cmdBuf));
			ndsem_t sem = getHostSem();
			nd_sem_post(sem);
		}
		goto RE_WIAT;
		break;
	}
	default:
		goto RE_WIAT;
		break;
	}

	//nd_sem_destroy(sem);
	return 0;
}

int LogicDebugClient::inputCmd(int cmdId, parserRunStat needRunStat)
{
	processHeaderInfo * proceInfo = ShareProcessLists::get_Instant()->getProcessInfo(m_aimProcessId);
	if (!proceInfo)	{
		return -1;
	}
	if (needRunStat!= E_RUN_DBG_STAT_UNKNOWN) {
		if (needRunStat != proceInfo->runStat) {
			return 0;
		}
	}

	nd_atomic_set(&proceInfo->inputCmd, cmdId);
	ndsem_t sem = getHostSem();
	nd_sem_post(sem);

	if (!m_isAttached)	{
		return waitEvent();
	}
	return 0;
}

int LogicDebugClient::inputCmd(int cmdId, const char *func, const char *nodeName)
{
	processHeaderInfo * proceInfo = ShareProcessLists::get_Instant()->getProcessInfo(m_aimProcessId);
	if (!proceInfo)	{
		return -1;
	}

	nd_atomic_set(&proceInfo->inputCmd, cmdId);

	ndsnprintf(proceInfo->cmdBuf, sizeof(proceInfo->cmdBuf), "<node func=\"%s\" exenode=\"%s\" />", func, nodeName);

	ndsem_t sem = getHostSem();
	nd_sem_post(sem);

	if (!m_isAttached)	{
		return waitEvent();
	}
	return 0;
}

int LogicDebugClient::inputToCmdThread(int cmdId, const char *func, const char *nodeName)
{
	processHeaderInfo * proceInfo = ShareProcessLists::get_Instant()->getProcessInfo(m_aimProcessId);
	if (!proceInfo)	{
		return -1;
	}

	nd_atomic_set(&proceInfo->srvHostCmd, cmdId);
	ndsnprintf(proceInfo->cmdBuf, sizeof(proceInfo->cmdBuf), "<node func=\"%s\" exenode=\"%s\" />", func, nodeName);

	nd_sem_post(m_cmdSemToSrvHost);
// 
// 	if (!m_isAttached)	{
// 		return waitEvent();
// 	}
	return 0;

}


ndsem_t LogicDebugClient::getHostSem()
{
	LocalDebugger &debugger = LogicEngineRoot::get_Instant()->getGlobalDebugger();
	return debugger.m_RunningSem;

}
ndsem_t LogicDebugClient::getClientSem()
{

	LocalDebugger &debugger = LogicEngineRoot::get_Instant()->getGlobalDebugger();
	return debugger.m_cliSem;
}

bool LogicDebugClient::onEnterStep(const char *function, const char *node)
{
	nd_logdebug("run to step\n");
	return false;
}


void LogicDebugClient::onTerminate()
{
	nd_logdebug("terminate\n");

}

void LogicDebugClient::onCommandOk()
{
	nd_logdebug("input command run ok\n");
}

void LogicDebugClient::onScriptRunOk()
{
	nd_logdebug("script run ok\n");
}

bool LogicDebugClient::onConsoleInput(std::string &text)
{
	nd_logdebug("console input\n");
	return false;
}

ndxml *LogicDebugClient::getParserInfo()
{
	ShareProcessLists *processList = ShareProcessLists::get_Instant();
	processHeaderInfo * proceInfo = processList->getProcessInfo(m_aimProcessId);

	if (!proceInfo){
		return NULL;
	}

	nd_filemap_t fileMap;
	int ret = nd_mem_share_create(proceInfo->shareMemName,0, &fileMap);
	if (-1 == ret) {
		return NULL;
	}
	ndxml *xml = ndxml_from_text((char *)nd_mem_share_addr(&fileMap));

	nd_mem_share_close(&fileMap);
	return xml;
}


bool LogicDebugClient::getRunningProcess(Process_vct_t &processes)
{
	return ShareProcessLists::get_Instant()->getProcesses(processes);
}
//////////////////////////////////////////////////////////////////////////

LocalDebugger::LocalDebugger(LogicParserEngine *parser, LogicObjectBase *owner) :m_parser(parser)
{
	//m_client = NULL;
	//m_isAttached = false;
	m_bStep = false;
	m_bRunOut = false;
	m_getConsoleInput = false;
	m_encodeType = ND_ENCODE_TYPE;
	
	m_RunningSem = NULL;
	m_cliSem = NULL;
	m_cmdSem = NULL;
	m_thHost = 0;

	memset(&m_outPutMem, 0, sizeof(m_outPutMem));

	ndxml_initroot(&m_parserInfo);
}

LocalDebugger::~LocalDebugger()
{
	stopHost();

	if (m_RunningSem){
		postEndDebug();
	}
}

// debug function, return 0 success ,renturn -1 error , error== would_block , in step-debug
int LocalDebugger::runCmdline(int argc, const char *argv[], int encodeType )
{
	nd_assert(m_parser);

	ParserCmdLine *cmdLine = new ParserCmdLine(argc, argv);

	if (!preStartDebug(0)) {
		return -1;
	}

	ndthread_t thId = 0;
	ndth_handle h = nd_createthread(parseThreadRun, (void*)cmdLine, &thId, 0);
	if (h){
		return 0;
	}
	return -1;
}


int LocalDebugger::runHost()
{
	if (!preStartDebug(0, LogicEngineRoot::get_Instant()->CheckIsDebug())) {
		return -1;
	}
	if (!startCMDRecvTh()) {
		return -1;
	}

	return 0;
}

void LocalDebugger::stopHost()
{
	if (m_cmdSem){
		ShareProcessLists *processList = ShareProcessLists::get_Instant();
		processHeaderInfo * proceInfo = processList->getProcessInfo(nd_processid());
		if (proceInfo)	{
			nd_atomic_set(&proceInfo->srvHostCmd, E_DBG_INPUT_CMD_TERMINATED);
		}
		nd_sem_post(m_cmdSem);

		if (m_thHost){
			nd_waitthread(m_thHost);
			m_thHost = 0;
		}
		nd_sem_destroy(m_cmdSem);
		m_cmdSem = NULL;
	}
}

bool LocalDebugger::postEndDebug()
{
	ShareProcessLists *root = ShareProcessLists::get_Instant();
	root->delProcess(nd_processid());
	if (m_RunningSem) {
		nd_sem_destroy(m_RunningSem);
		m_RunningSem = 0;
	}

	if (m_cliSem) {
		nd_sem_destroy(m_cliSem);
		m_cliSem = 0;
	}

	if (nd_mem_share_addr(&m_outPutMem)) {
		nd_mem_share_close(&m_outPutMem);
	}
	return true;
}

int LocalDebugger::addBreakPoint(const char *funcName, const char *nodeName, bool isTemp)
{
	if (!isBreakPoint(funcName, nodeName))	{
		m_breakPoints.push_back(BreakPointInfo(funcName, nodeName, isTemp));
		nd_logdebug("add break points %s %s\n", funcName, nodeName);
	}
	return 0;
}
int LocalDebugger::delBreakPoint(const char *funcName, const char *nodeName)
{
	for (breakPoint_vct::iterator it = m_breakPoints.begin(); it != m_breakPoints.end(); it++) {
		if (0 == ndstricmp(it->functionName.c_str(), funcName) &&
			0 == ndstricmp(it->nodeName.c_str(), nodeName))	{
			m_breakPoints.erase(it);

			nd_logdebug("Remove break points %s %s\n", funcName, nodeName);
			break;
		}
	}
	return 0;
}


void LocalDebugger::clearBreakpoint()
{
	m_breakPoints.clear();
}

bool LocalDebugger::preStartDebug(NDUINT32 processId, bool withDebugInfo )
{
	processHeaderInfo * proceInfo = NULL;
	ShareProcessLists *pProcessRoot = ShareProcessLists::get_Instant();
	if (processId == 0)	{

		LogicEngineRoot *root = LogicEngineRoot::get_Instant();
		NDUINT32 myProcessId = nd_processid();
		proceInfo = pProcessRoot->createProcessInfo(myProcessId, nd_process_name(), root->getMainModuleName(), withDebugInfo);
	}
	else {
		proceInfo = pProcessRoot->getProcessInfo(processId);

	}
	if (!proceInfo)	{
		nd_logerror("create debug info for %d process error\n", processId);
		return false;
	}


	int ret = nd_mem_share_create(proceInfo->shareMemName, 1024 * 1024, &m_outPutMem);
	if (-1 == ret) {
		nd_logerror("blueprint vm open debug share memfile error %s\n", nd_last_error());
		return false;
	}

	m_RunningSem = nd_sem_open(proceInfo->semName);
	if (!m_RunningSem) {
		nd_logerror("blueprint vm open debug share memfile error %s\n", nd_last_error());
		return false;
	}

	m_cliSem = nd_sem_open(proceInfo->semClient);
	if (!m_cliSem) {
		nd_logerror("blueprint vm open debug share memfile error %s\n", nd_last_error());
		return false;
	}
	return true;
}


bool LocalDebugger::startCMDRecvTh()
{
	processHeaderInfo * proceInfo = NULL;
	ShareProcessLists *pProcessRoot = ShareProcessLists::get_Instant();
	proceInfo = pProcessRoot->getProcessInfo(nd_processid());
	
	if (!proceInfo)	{
		return false;
	}

	m_cmdSem = nd_sem_open(proceInfo->semCMDth);
	if (!m_cmdSem) {
		return false;
	}

	ndthread_t thId = 0;
	m_thHost = nd_createthread(cmdReceiverThread, (void*)this, &thId, 0);
	if (!m_thHost){
		return false;
	}

	return true;

}

bool LocalDebugger::requestConsoleInput(std::string &inText)
{
	if (-1 == ntfClient(E_DBG_OUTPUT_CMD_CONSOLE_INPUT, NULL, NULL)) {
		return false;
	}
	ndatomic_t recvCmd;
	int ret;
	NDUINT32 processId = nd_processid();
	ShareProcessLists *processList = ShareProcessLists::get_Instant();
	processHeaderInfo * proceInfo = processList->getProcessInfo(processId);

	if (!proceInfo || proceInfo->debuggerProcessId == 0) {
		return false;
	}

REWAIT:
	proceInfo->runStat = E_RUN_DBG_STAT_WAIT_CMD;
	ret = nd_sem_wait(m_RunningSem, INFINITE);
	proceInfo->runStat = E_RUN_DBG_STAT_RUNNING;
	if (0 != ret) {
		return false;
	}

	recvCmd = nd_atomic_read(&proceInfo->inputCmd);
	proceInfo->inputCmd = E_DBG_INPUT_CMD_NONE;

	if (recvCmd == E_DBG_INPUT_CMD_CONSOLE_ACK) {
		
		inText = proceInfo->cmdBuf;
		return true;
	}
	else {
		goto REWAIT;
	}
	
}

//run on parser
int LocalDebugger::onEnterStep(LogicParserEngine *parser,const char *funcName, const char *nodeName)
{
	m_parser = parser;
	if (isBreakPoint(funcName, nodeName, true) || m_bStep){
		return waitEvent(funcName, nodeName);
	}
	else if (m_bRunOut && m_parser->checkLastStepIsCall()) {
		m_bRunOut = false;
		return waitEvent(funcName, nodeName);
	}
	return 0;
}

//send script run success to debug-client
int LocalDebugger::ScriptRunOk(LogicParserEngine *parser)
{
	m_bStep = false;

	if (!m_cliSem){
		return 0;
	}
	//int ret = 0;
	NDUINT32 processId = nd_processid();
	ShareProcessLists *processList = ShareProcessLists::get_Instant();
	processHeaderInfo * proceInfo = processList->getProcessInfo(processId);

	if (!proceInfo || proceInfo->debuggerProcessId == 0){
		return 0;
	}

	//needn't to send when local-debug
	if (proceInfo->debuggerProcessId == proceInfo->processId){
		return 0;
	}

	std::string funcName, nodeName;

	ndxml_root &parserInfo = getParserInfo();

	void *outAddr = nd_mem_share_addr(&m_outPutMem);
	size_t outSize = (size_t)nd_mem_share_size(&m_outPutMem);
	ndxml_to_buf(&parserInfo, (char*)outAddr, outSize);

	ntfClient(E_DBG_OUTPUT_CMD_SCRIPT_RUN_OK, NULL, NULL);
	return 0;
}

bool LocalDebugger::isBreakPoint(const char *func, const char *node, bool bTrytoDel )
{
	if (m_breakPoints.size())	{
		for (breakPoint_vct::iterator it = m_breakPoints.begin(); it != m_breakPoints.end(); it++) {
			if (0 == ndstricmp(it->functionName.c_str(), func) &&
				0 == ndstricmp(it->nodeName.c_str(), node))	{
				if (bTrytoDel && it->tempBreak)	{
					m_breakPoints.erase(it);
				}
				return true;
			}
		}
	}
	return false;
}


int LocalDebugger::ntfClient(int cmd, const char *func, const char *nodeName)
{
	NDUINT32 processId = nd_processid();
	ShareProcessLists *processList = ShareProcessLists::get_Instant();
	processHeaderInfo * proceInfo = processList->getProcessInfo(processId);
	if (!proceInfo){
		return -1;
	}

	proceInfo->outputCmd = cmd;

	if (func && nodeName) {
		ndsnprintf(proceInfo->cmdBuf, sizeof(proceInfo->cmdBuf), "<node func=\"%s\" exenode=\"%s\" />", func, nodeName);
	}
	nd_sem_post(m_cliSem);
	return 0;
}

int LocalDebugger::waitEvent(const char *runningFunc, const char *runningNode)
{
	int ret = 0;
	NDUINT32 processId = nd_processid();
	ShareProcessLists *processList = ShareProcessLists::get_Instant();
	processHeaderInfo * proceInfo = processList->getProcessInfo(processId);

	if (!proceInfo || proceInfo->debuggerProcessId == 0){
		return 0;
	}

	std::string funcName, nodeName;

	ndxml_root &parserInfo = getParserInfo();

	void *outAddr = nd_mem_share_addr(&m_outPutMem);
	size_t outSize = (size_t)nd_mem_share_size(&m_outPutMem);
	ndxml_to_buf(&parserInfo,(char*)outAddr , outSize);

	if (-1 == ntfClient(E_DBG_OUTPUT_CMD_STEP, runningFunc, runningNode)) {
		nd_logerror("notify debugger client error\n");
		return 0;
	}
	ndatomic_t recvCmd = 0;
RE_WIAT:
	proceInfo->runStat = E_RUN_DBG_STAT_WAIT_CMD;
	//m_client->onEnterStep(runningFunc, runningNode);
	ret = nd_sem_wait(m_RunningSem, INFINITE);

	proceInfo->runStat = E_RUN_DBG_STAT_RUNNING;
	if (0!=ret)	{
		return -1;
	}
	
	proceInfo = processList->getProcessInfo(processId);
	//handler event 

	recvCmd = nd_atomic_read(&proceInfo->inputCmd);
	proceInfo->inputCmd = E_DBG_INPUT_CMD_NONE;
	//handler event 
	switch (recvCmd)
	{
	case E_DBG_INPUT_CMD_NONE:
		goto RE_WIAT;
		break;
	case E_DBG_INPUT_CMD_NEW_BREAKPOINT:
		if (getParamFromInput(funcName, nodeName)) {
			addBreakPoint(funcName.c_str(), nodeName.c_str());
		}

		ntfClient(E_DBG_OUTPUT_CMD_HANDLE_ACK, NULL,NULL );

		goto RE_WIAT;
		break;
	case E_DBG_INPUT_CMD_STEP_RUN:
		m_bStep = true;
		break;
	case E_DBG_INPUT_CMD_CONTINUE:
		m_bStep = false;
		break;
	case E_DBG_INPUT_CMD_RUN_TO_CURSOR:
		//m_parser->setStepMode(false);
		m_bStep = false;
		if (getParamFromInput(funcName, nodeName)) {
			addBreakPoint(funcName.c_str(), nodeName.c_str(),true);
		}
		break;
	case E_DBG_INPUT_CMD_RUN_OUT:
		m_bStep = false;
		m_bRunOut = true;
		break;
	case E_DBG_INPUT_CMD_DEL_BREAKPOINT:
		if (getParamFromInput(funcName, nodeName)) {
			delBreakPoint(funcName.c_str(), nodeName.c_str());
		}
		ntfClient(E_DBG_OUTPUT_CMD_HANDLE_ACK, NULL, NULL);
		goto RE_WIAT;

		break;
	case E_DBG_INPUT_CMD_TERMINATED:
		return -1;
		break;
	
	default:
		goto RE_WIAT;
		break;
	}

	return 0;
}


int LocalDebugger::waitClientCmd()
{
	int ret = 0;
	NDUINT32 processId = nd_processid();
	ShareProcessLists *processList = ShareProcessLists::get_Instant();

	processHeaderInfo * proceInfo = processList->getProcessInfo(processId);

	std::string funcName, nodeName;
	
	do 	{
		ret = nd_sem_wait(m_cmdSem, INFINITE);
		if (0 != ret)	{
			return -1;
		}
		//handler event 

		ndatomic_t recvCmd = nd_atomic_read(&proceInfo->srvHostCmd);
		proceInfo->srvHostCmd = E_DBG_INPUT_CMD_NONE;
		//handler event 
		switch (recvCmd)
		{
		case E_DBG_INPUT_CMD_NEW_BREAKPOINT:
			if (getParamFromInput(funcName, nodeName)) {
				addBreakPoint(funcName.c_str(), nodeName.c_str());
			}
			ntfClient(E_DBG_OUTPUT_CMD_HANDLE_ACK, NULL, NULL);
			break;
		case E_DBG_INPUT_CMD_DEL_BREAKPOINT:
			if (getParamFromInput(funcName, nodeName)) {
				delBreakPoint(funcName.c_str(), nodeName.c_str());
			}
			ntfClient(E_DBG_OUTPUT_CMD_HANDLE_ACK, NULL, NULL);	
			break;
		case  E_DBG_INPUT_CMD_TERMINATED:
			if (proceInfo->debuggerProcessId) {
				ntfClient(E_DBG_OUTPUT_CMD_TERMINATED, NULL, NULL);
			}
			return 0;

		case E_DBG_INPUT_CMD_ATTACHED:
			if (proceInfo->debuggerProcessId == 0)	{
				nd_logerror("attached error , debugger process id = 0\n");
			}
			break;
		case E_DBG_INPUT_CMD_DEATTACHED:
			nd_logdebug("deattached debug \n");
			clearBreakpoint();
			break;
		case E_DBG_INPUT_CMD_ADD_BREAKPOINT_BATCH:
			getInputBreakpoints();
			break;
		
		default:
			break;
		}

	} while (1);

	return 0;
}

ndxml_root & LocalDebugger::getParserInfo()
{
	makeParserInfo();
	return m_parserInfo;
}



bool LocalDebugger::OutPutInfo(ndxml *dataInfo)
{
	if (!nd_mem_share_addr(&m_outPutMem)) {
		return false;
	}
	size_t size = ndxml_to_buf(dataInfo, (char *)nd_mem_share_addr(&m_outPutMem), (int)nd_mem_share_size(&m_outPutMem));
	if (size == 0)	{
		return false;
	}
	return true;
}


bool LocalDebugger::getParamFromInput(std::string &funcName, std::string &nodeName)
{
	NDUINT32 processId = nd_processid();
	ShareProcessLists *processList = ShareProcessLists::get_Instant();
	processHeaderInfo * proceInfo =  processList->getProcessInfo(processId);

	ndxml *xml = ndxml_from_text(proceInfo->cmdBuf);
	if (xml){
		LogicEngineRoot *root = LogicEngineRoot::get_Instant();
		funcName = ndxml_getattr_val(xml, "func");
		nodeName = ndxml_getattr_val(xml, "exenode");
		ndxml_free(xml);
		if (proceInfo->cmdEncodeType != root->GetEncodeType() )	{
			LogicDataObj d1;
			d1.InitSet(funcName.c_str());
			d1.ConvertEncode(proceInfo->cmdEncodeType , root->GetEncodeType());
			funcName = d1.GetString();

			d1.InitSet(nodeName.c_str());
			d1.ConvertEncode(proceInfo->cmdEncodeType, root->GetEncodeType());
			nodeName = d1.GetString();
		}
		return true;
	}
	return false;	
}

bool LocalDebugger::addBreakpointBatch(ndxml *xml)
{
	if (!xml) {
		return false;
	}
	nd_assert(0 == ndstricmp(ndxml_getname(xml), "Breakpoints"));
	int count = ndxml_getsub_num(xml);
	for (int i = 0; i < count; i++) {
		ndxml *subNode = ndxml_getnodei(xml, i);
		nd_assert(subNode);

		const char *funcName = ndxml_getattr_val(subNode, "func");
		const char *nodeName = ndxml_getattr_val(subNode, "exenode");
		if (funcName && nodeName && *nodeName) {
			addBreakPoint(funcName, nodeName);
		}
	}
	return true;
}

bool LocalDebugger::getInputBreakpoints()
{
	NDUINT32 processId = nd_processid();
	ShareProcessLists *processList = ShareProcessLists::get_Instant();
	processHeaderInfo * proceInfo = processList->getProcessInfo(processId);

	ndxml *xml = ndxml_from_text(proceInfo->cmdBuf);
	if (xml  ){
		addBreakpointBatch(xml);
		ndxml_free(xml);
		return true;
	}
	return false;
}

bool LocalDebugger::makeParserInfo()
{
	ndxml_destroy( &m_parserInfo );
	runningStack *stack = m_parser->getCurStack();
	if (!stack)	{
		nd_logerror("current runnint stack is NULL\n");
		return false;
	}
	
	int len;
	char buf[1024];
	char *p = buf;
	
	len = ndsnprintf(p, sizeof(buf), "%s(", stack->cmd->cmdname);
	p += len;

	for (int  i = 1; i < stack->params.size(); i++)	{
		len = stack->params[i].Sprint(p, sizeof(buf) - (p - buf));
		p += len;
	}
	len = ndsnprintf(p, sizeof(buf) - (p - buf), ")");
	p += len;

	ndxml *xmlInfo = ndxml_addnode(&m_parserInfo, "local",NULL);
	if (!xmlInfo)	{
		return false;
	}

	ndxml_addnode(xmlInfo, "function", buf);

	ndxml_addnode(xmlInfo, "instruct_flag", m_parser->m_registorFlag? "true": "false");
	ndxml_addnode(xmlInfo, "control_flag", m_parser->m_registorCtrl ? "true" : "false");

	
	len = m_parser->m_registerVal.Sprint(buf, sizeof(buf));
	ndxml_addnode(xmlInfo, "$curValue", len>0 ? buf : "0");


	for (int i = 0; i < stack->local_vars.size(); i++)	{
		len = stack->local_vars[i].var.Sprint(buf, sizeof(buf));
		ndxml_addnode(xmlInfo, stack->local_vars[i].name.c_str(), len>0 ? buf : "0");
	}
	return true;
}
