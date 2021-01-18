/* file logicEngineRooot.cpp
 *
 * root manager of logic engine 
 *
 * create by duan
 * 2015-5-12
 */

#include "logic_parser/logicEndian.h"
#include "logic_parser/logicStruct.hpp"
#include "logic_parser/logicEngineRoot.h"
#include "logic_parser/logic_compile.h"
#include "nd_common/nd_iconv.h"

#include "nd_net/byte_order.h"
#include "ndapplib/ndsingleton.h"

cpp_func_map* LogicEngineRoot:: m_c_funcs = NULL;
logic_console_input_func LogicEngineRoot:: m_console_input =NULL;

//int LogicEngineRoot::m_displayEncodeType= ND_ENCODE_TYPE;

LogicEngineRoot *LogicEngineRoot::get_Instant()
{
	return NDSingleton<LogicEngineRoot>::Get();
}

void LogicEngineRoot::destroy_Instant()
{
	NDSingleton<LogicEngineRoot>::Destroy();
}

LogicEngineRoot::LogicEngineRoot() :m_logicObjOwner(this), m_globalDebugger(&m_logicObjOwner.m_parser,NULL)
{
	m_isDebug = 0;
	m_screen_out_func = 0;
	m_print_file = 0;
	m_scriptEncodeType = E_SRC_CODE_ANSI;
	m_displayEncodeType = ND_ENCODE_TYPE;
	if (!m_c_funcs)	{
		m_c_funcs = new cpp_func_map;
	}
	
}

LogicEngineRoot::~LogicEngineRoot()
{
}

int LogicEngineRoot::Create(const char *name)
{

	return 0;
}
void LogicEngineRoot::Destroy(int )
{
	UnloadAllModules();
	if (m_c_funcs) {
		m_c_funcs->clear();
		delete m_c_funcs;
		m_c_funcs = NULL;
	}
	m_globalDebugger.stopHost();
}


bool LogicEngineRoot::UnloadAllModules()
{
	//unloadScript();
	script_module_map::iterator it;
	for (it = m_modules.begin(); it != m_modules.end(); ) {
		_unloadModule(it->first.c_str(), *(it->second));
		delete it->second;
		it->second = 0;
		m_modules.erase(it++);
	}

	m_dftScriptModule.clear();
	m_event_entry.clear();
	return false;

}

void LogicEngineRoot::update(ndtime_t tminterval)
{
	m_logicObjOwner.m_parser.update(tminterval);
}

const char *LogicEngineRoot::_parserScriptStreamHeader(const char *data, size_t &size )
{
	if(data[0] == '#' && data[1]=='!') {
		const char *p = ndstrchr(data+2, '\n') ;
		if(p) {
			++p ;
			size -= p - data;
			return p ;
		}
	}
	return data ;
}
bool LogicEngineRoot::unloadModule(const char *moduleName)
{
	script_module_map::iterator it;
	for (it = m_modules.begin(); it != m_modules.end(); ++it) {
		if (0==ndstricmp(moduleName,it->first.c_str())) {
			_unloadModule(it->first.c_str(), *(it->second));
			delete it->second;
			it->second = 0;
			m_modules.erase(it);
			if (0 == ndstricmp(moduleName, m_dftScriptModule.c_str()) ) {
				if (m_modules.size() > 0) {
					m_dftScriptModule = m_modules.begin()->first.c_str();
				}
			}
			return true;
		}
	}
	return false;
}

int LogicEngineRoot::LoadFromBuf(void *pAddr, size_t dataLength, ILogicParser *initCallLoader)
{
	return LoadFromBufEx(pAddr, dataLength, (LogicParserEngine*)initCallLoader, NULL);
}
int LogicEngineRoot::LoadFromBufEx(void *pAddr, size_t dataLength, LogicParserEngine *initCallLoader, const char *userDefName)
{
	//script come from file , must be utf8
	NDUINT8 byteOrder = -1, encodeType = -1;
	NDUINT16 moduleSize = 0;
	NDUINT32 size;
	NDUINT64 compileTm;
	char moudleName[128];
	
	//const char *pf = (const char *)pAddr;
	const char *pf =_parserScriptStreamHeader((const char*)pAddr, dataLength) ;
	pAddr = (void*)pf ;
	
	char *pEnd = ((char*)pAddr) + dataLength;

#define READ_FROM_BUFF(_outAddr,_readlen, _readTimes, _pf) \
	if( (_pf + _readlen* _readTimes) <= pEnd ) {		\
		memcpy(_outAddr, _pf, _readlen * _readTimes);	\
		_pf += (_readlen* _readTimes) ;					\
	}													\
	else {												\
		nd_logerror("read " #_outAddr  " error !\n");	\
		return -1;										\
	}
	
	READ_FROM_BUFF(&byteOrder, sizeof(byteOrder), 1, pf);
	
	READ_FROM_BUFF(&encodeType, sizeof(encodeType), 1, pf);
	m_scriptEncodeType = (int)encodeType;

	READ_FROM_BUFF(&m_isDebug, sizeof(m_isDebug), 1, pf);

	READ_FROM_BUFF(&compileTm, sizeof(compileTm), 1, pf);

	//read author name 
	READ_FROM_BUFF(&moduleSize, sizeof(moduleSize), 1, pf);
	moduleSize = lp_stream2host(moduleSize, byteOrder);
	if (moduleSize > 0) {
		READ_FROM_BUFF(moudleName, moduleSize, 1, pf);
		moudleName[moduleSize] = 0;
		m_author = moudleName;
	}
	moduleSize = 0;
	//end name 
	//m_compileTm = lp_stream2host(compileTm, byteOrder);


	READ_FROM_BUFF(&moduleSize, sizeof(moduleSize), 1, pf);
	moduleSize = lp_stream2host(moduleSize, byteOrder);

	if (moduleSize > sizeof(moudleName)) {
		nd_logerror("module name is too much, maybe file is demaged!\n");
		return -1;
	}
	else if (moduleSize > 0) {
		READ_FROM_BUFF(moudleName, moduleSize, 1, pf);
	}

	moudleName[moduleSize] = 0;
	m_moduleChangedTime[moudleName] = compileTm;

	if (m_mainModule.size() == 0) {
		m_mainModule = moudleName;
	}

	NDUINT8 isGlobal = 0;
	script_func_map *pscripts = new script_func_map;
	if (!pscripts) {
		return -1;
	}
	script_func_map &script = *pscripts;
	scriptCmdBuf *pInitEntry = NULL;

	std::string lastFuncName;
	while (pf  < ((const char*)pAddr + dataLength)) {
		READ_FROM_BUFF(&isGlobal, 1, 1, pf);

		READ_FROM_BUFF(&size, 1, sizeof(size), pf);
		size = lp_stream2host(size, byteOrder);

		nd_assert(size < dataLength);

		if ( size < dataLength) {
			scriptCmdBuf *pcmdbuf = (scriptCmdBuf*)malloc(sizeof(scriptCmdBuf) + size);
			if (pcmdbuf) {
				char *p = (char*)(pcmdbuf + 1);
				READ_FROM_BUFF(p, 1, size, pf);

				pcmdbuf->cmdname = p;
				size_t namesize = ndstrlen(p) + 1;
				pcmdbuf->buf = p + namesize;
				pcmdbuf->size = (int)(size - namesize);
				pcmdbuf->byteOrder = (int)byteOrder;
				pcmdbuf->encodeType = m_scriptEncodeType;

				lastFuncName = pcmdbuf->cmdname;

				pcmdbuf->funcType = (LogicFunctionType)isGlobal;

				if (isGlobal == NF_FUNC_GLOBAL || !moudleName[0]) {
					_pushGlobal(pcmdbuf);
				}
				else {					
					script[std::string(pcmdbuf->cmdname)] = pcmdbuf;
				}

				if (ndstrcmp(DEFAULT_LOAD_INITILIZER_FUNC, pcmdbuf->cmdname) == 0) {
					pInitEntry = pcmdbuf;
				}
			}
			else {
				nd_logerror("malloc error\n");
				return -1;
			}
		}
		else {
			nd_logerror("load script error \n");
			return -1;
		}
	}
	const char *myName = userDefName;
	if (!myName  ) {
		myName = moudleName;
	}

	if (m_dftScriptModule.empty()) {
		m_dftScriptModule = moudleName;
	}
	m_modules[myName] = pscripts;

	m_lastloadModule = myName;

	if (pInitEntry && initCallLoader) {
		LogicDataObj result;
		if (!initCallLoader->_runCmdBuf(moudleName, pInitEntry, 0)) {
			nd_logerror("run %s error %d\n", DEFAULT_LOAD_INITILIZER_FUNC, initCallLoader->getErrno());
			return -1;
		}
		initCallLoader->eventNtf(LOGIC_EVENT_MODULE_LOADED, 1, pscripts);
	}
	

	return 0;
}
int LogicEngineRoot::LoadScript(const char *scriptStream, ILogicParser *initCallLoader )
{
	size_t size = 0;
	void *pdata = nd_load_file(scriptStream, &size);
	if (!pdata) {
		nd_logerror("can not open file %s\n", scriptStream);
		return -1;
	}
	int ret = LoadFromBuf(pdata, size, initCallLoader);
	nd_unload_file(pdata);
	return ret;
		
}

const char* LogicEngineRoot::_convertFuncName(const char *text, int inputEncodeType)
{
	const char*ret;
	static __ndthread  char _s_funcNameBuf[128];
	if (inputEncodeType == m_scriptEncodeType || inputEncodeType == E_SRC_CODE_ANSI) {
		ret = text;
	}
	else {
		ret = _s_funcNameBuf;
		if (m_scriptEncodeType == E_SRC_CODE_UTF_8)	{
			nd_gbk_to_utf8(text, _s_funcNameBuf, (int)sizeof(_s_funcNameBuf));
		}
		else if (inputEncodeType == E_SRC_CODE_UTF_8){
			nd_utf8_to_gbk(text, _s_funcNameBuf, (int)sizeof(_s_funcNameBuf));
		}
		else {
			ret = text;
		}
		
	}
	return ret;
}
int LogicEngineRoot::Reload(const char *scriptStream, ILogicParser *loader)
{
	unloadScript();
	return LoadScript(scriptStream,loader);
}

int LogicEngineRoot::unloadScript()
{
	script_module_map::iterator it;
	for (it = m_modules.begin(); it != m_modules.end(); ++it){
		if (it->second)	{
			_unloadModule(it->first.c_str(),*(it->second));
			delete it->second;
			it->second = 0;
		}
	}
	m_modules.clear();
	m_dftScriptModule.clear();
	m_moduleChangedTime.clear();
	m_mainModule.clear();
	return 0;
}

int LogicEngineRoot::_unloadModule(const char *moduleName, script_func_map &module)
{
	script_func_map::iterator it;

	getGlobalParser().eventNtf(LOGIC_EVENT_MODULE_UNLOADED, 1, moduleName);

	for (it = module.begin(); it != module.end(); ++it){
		if (it->second)	{
			std::string funcName = moduleName;
			funcName += ".";
			funcName += it->first;
			removeFromEvents(funcName.c_str());
			free(it->second);
			it->second = 0;
		}
	}
	return 0;
}


bool LogicEngineRoot::addGlobalFunction(int byteOrder, char*name, void *data, size_t size)
{
	size_t namesize = ndstrlen(name) + 1;
	scriptCmdBuf *pcmdbuf = (scriptCmdBuf*)malloc(sizeof(scriptCmdBuf) + size + namesize);
	if (pcmdbuf) {

		pcmdbuf->cmdname = (char*)(pcmdbuf + 1);
		pcmdbuf->buf = (char*)pcmdbuf->cmdname + namesize;

		ndstrncpy((char*)pcmdbuf->cmdname, name, namesize);
		memcpy((char*)pcmdbuf->buf, data, size);
		pcmdbuf->size = (int)size;

		pcmdbuf->byteOrder = (int)byteOrder;

		_pushGlobal(pcmdbuf);
		return true;
	}
	return false;
}

bool LogicEngineRoot::_pushGlobal(scriptCmdBuf *pcmdbuf)
{
	script_module_map::iterator it = m_modules.find(GLOBAL_MODULE_NAME);
	if (it == m_modules.end()){
		script_func_map *pscripts = new script_func_map; 
		(*pscripts)[std::string(pcmdbuf->cmdname)] = pcmdbuf;
		m_modules[GLOBAL_MODULE_NAME] = pscripts;
	}
	else {
		(*(it->second))[std::string(pcmdbuf->cmdname)] = pcmdbuf;
	}
	return true;
}

bool LogicEngineRoot::setDftScriptModule(const char *script)
{
	script_module_map::iterator it = m_modules.find(script);
	if (it != m_modules.end()) {
		m_dftScriptModule = script;
		return true;
	}
	return false;
}


void LogicEngineRoot::setOutPutEncode(int encode) 
{ 
	m_displayEncodeType = encode; 
}

const function_vct *LogicEngineRoot::getEventFunc(int event_id)const
{
	event_table_map::const_iterator it = m_event_entry.find(event_id);
	if (it == m_event_entry.end()){
		return NULL;
	}
	return &it->second;
}

const func_cpp_info* LogicEngineRoot::getCPPFunc(const char *funcName) const
{
	std::string realName ;
	if (0 != ndstrncmp(funcName, "CPP.",4)) {
		realName = "CPP." ;
		realName += funcName ;
	}
	else {
		realName = funcName ;
	}
	
	//find in c-function-list
	cpp_func_map::const_iterator it =  m_c_funcs->find(realName);
	if (it!=m_c_funcs->end()) {
		
		return &(it->second);
	}
	
	return NULL;
}

void LogicEngineRoot::installFunc(logicParser_func func, const char *name, const char *comment, bool ansiC )
{
	if (!name || !name[0]){
		return;
	}
#if (ND_ENCODE_TYPE!=E_SRC_CODE_UTF_8)
	char func_namebuf[128];
	char func_descbuf[128];
	name = nd_gbk_to_utf8(name, func_namebuf, sizeof(func_namebuf));
	comment = nd_gbk_to_utf8(comment, func_descbuf, sizeof(func_descbuf));
#endif
	

	func_cpp_info node;
	node.comment = comment;
	node.func = func;
	if (ansiC) {
		node.isAnsiC = 1;
	}
	
	if (!m_c_funcs){
		m_c_funcs = new cpp_func_map;
	}
	if (ndstrncmp(name, "CPP.", 4)!=0) {
		std::string realName = "CPP." ;
		realName += name ;
		(*m_c_funcs)[realName] = node;
	}

	else {
		(*m_c_funcs)[std::string(name)] = node;
	}
}
void LogicEngineRoot::setConsoleInput(logic_console_input_func func)
{
	m_console_input = func;
}

logic_console_input_func LogicEngineRoot::getConsoleInput()
{
	return m_console_input;
}
void LogicEngineRoot::installEvent(int event_id, const char *event_func)
{
	if (!event_func || !*event_func) {
		return;
	}

	event_table_map::iterator it = m_event_entry.find(event_id);
	if (it != m_event_entry.end()){
		it->second.push_back(event_func);
	}
	else {
		function_vct funcVct;
		funcVct.push_back(event_func);
		m_event_entry[event_id] = funcVct;
	}
	
}


const scriptCmdBuf* LogicEngineRoot::_findScript(const char *funcName, const char *moduleName, const char**outModuleName) const
{
	script_module_map::const_iterator module_it = m_modules.find(moduleName);
	if (module_it == m_modules.end()){
		return NULL;
	}
	const script_func_map *pfuncMap = module_it->second;
	if (!pfuncMap){
		return NULL;
	}

	script_func_map::const_iterator itscript = pfuncMap->find(std::string(funcName));
	if (itscript != pfuncMap->end()) {
		if (outModuleName ) {
			*outModuleName = module_it->first.c_str();
		}
		return (itscript->second);
	}
	else {
		return  NULL;
	}
}
const scriptCmdBuf* LogicEngineRoot::getScript(const char *funcName, const char *moduleName, const char**outModuleName)const
{
	const char *p = ndstrchr(funcName, '.');
	if (p){
		const char *realname = ++p;
		char module[128];

		ndstr_nstr_end(funcName, module, '.', sizeof(module));
		return _findScript(realname, module, outModuleName);
	}
	else if (moduleName && *moduleName) {
		const scriptCmdBuf*pCmd = _findScript(funcName, moduleName, outModuleName);
		if (!pCmd){
			//found from global 
			*outModuleName = NULL;
			pCmd = _findScript(funcName, GLOBAL_MODULE_NAME, NULL);
		}
		return pCmd;
		
	}
	else {
		const scriptCmdBuf*pCmd = _findScript(funcName, GLOBAL_MODULE_NAME,NULL);
		if (pCmd){
			if (outModuleName) { *outModuleName = NULL; }
			return pCmd;
		}
		if (m_dftScriptModule.size()){
			return _findScript(funcName, m_dftScriptModule.c_str(), outModuleName);
		}
	}
	return NULL;

}
void LogicEngineRoot::removeFromEvents(const char *funcName)
{
	if (!funcName || !*funcName){
		return;
	}
	for (event_table_map::iterator it = m_event_entry.begin(); it!= m_event_entry.end(); it++){
		function_vct &funcs = it->second;
		for (size_t i = 0; i < funcs.size(); i++){
			if (funcs[i] == funcName) {
				funcs.erase( funcs.begin() + i);
				return;
			}
		}
	}
}

int LogicEngineRoot::dumbCPPfunc(const char *outXmlfile)
{
	ndxml_root xmlroot;
	ndxml_initroot(&xmlroot);
	ndxml_addattrib(&xmlroot, "version", "1.0");
	ndxml_addattrib(&xmlroot, "encoding", "utf8");
	ndxml *xml_funcs = ndxml_addnode(&xmlroot, LOGIC_FUNCTION_LIST_NAME, NULL);
	nd_assert(xml_funcs);
	if (!xml_funcs)	{
		return -1;
	}

	std::map<std::string, std::string> expFuncs;

	for (cpp_func_map::iterator it = m_c_funcs->begin(); it != m_c_funcs->end(); it++) {
		expFuncs[it->second.comment] = it->first;
	}

	for (std::map<std::string, std::string> ::iterator it = expFuncs.begin(); it != expFuncs.end(); it++) {
		ndxml *xmlsub = ndxml_addnode(xml_funcs, "node", it->second.c_str());
		if (xmlsub)	{
			std::string funcName = "[API] :: ";
			funcName += it->first;
			ndxml_addattrib(xmlsub, "desc", funcName.c_str());
		}
	}
	 
	int ret = ndxml_save(&xmlroot, outXmlfile);

	ndxml_destroy(&xmlroot);

	return ret;
}

int LogicEngineRoot::test()
{
	LogicParserEngine &tmpEngine = getGlobalParser();
	tmpEngine.setSimulate(true);

	script_module_map::const_iterator it;
	tmpEngine.eventNtf(1, 0);
	for (it = m_modules.begin(); it != m_modules.end(); ++it){
		const script_func_map &funcs = *(it->second);
		for (script_func_map::const_iterator it_script = funcs.begin(); it_script != funcs.end(); it_script++){
			bool ret = tmpEngine._runCmdBuf(it->first.c_str(), it_script->second, 0);
			if (!ret)	{
				if (tmpEngine.isInitiativeQuit())	{
					continue;
				}
				int runerror = tmpEngine.getErrno();
				if (runerror == LOGIC_ERR_AIM_OBJECT_NOT_FOUND || runerror == LOGIC_ERR_WOULD_BLOCK || runerror >= LOGIC_ERR_USER_DEFINE_ERROR) {
					continue;
				}
				nd_logerror("test script func %s error \n", it_script->second->cmdname);
				return -1;
			}
		}
	}

	for (int i = 3; i < 100; i++){
		tmpEngine.eventNtf(i, 0);
	}
	
	tmpEngine.eventNtf(2, 0);
	tmpEngine.setSimulate(false);
	return 0;
}

bool LogicEngineRoot::getModuleChangedTime(const char *moduleName, LogicDataObj &result)
{
	if (moduleName && *moduleName)	{

		module_changed_tm_map::iterator it = m_moduleChangedTime.find(moduleName); 
		if (it != m_moduleChangedTime.end()){
			result.InitSet(it->second);
			return true;
		}
		
	}
	else {

		module_changed_tm_map::iterator it = m_moduleChangedTime.find(m_dftScriptModule);
		if (it != m_moduleChangedTime.end()){
			result.InitSet(it->second);
			return true;
		}
	}
	time_t now = time(NULL);
	result.InitSet(now);
	return true;
}

logic_print LogicEngineRoot::setPrint(logic_print func, void *outfile)
{
	logic_print ret = m_screen_out_func;
	m_screen_out_func = func;
	m_print_file = outfile;
	return ret;
}

// 
// LogicDataObj *LogicEngineRoot::getVar(const char *name)
// {
// 	LogicData_vct::iterator it = m_global_vars.begin(); 
// 	for (; it != m_global_vars.end(); it++)	{
// 		if (0==ndstricmp(it->name.c_str(), name) )	{
// 			return &(it->var);
// 		}
// 	}
// 	return NULL;
// }


////////////////////////////////////////////////////
///test object manager
TestLogicObject::TestLogicObject():m_parser()
{
	m_parser.setSimulate(true);
	m_parser.setOwner(this);

}

TestLogicObject::TestLogicObject(LogicEngineRoot *root) :m_parser(this, root)
{
	m_parser.setSimulate(true);
}

TestLogicObject::~TestLogicObject()
{

}

bool TestLogicObject::loadDataType(const char *file)
{
	//if (-1 == loadUserDefFromMsgCfg(file, m_dataType)){
	//	return false;
	//}
	return true;
}

bool TestLogicObject::opRead(const LogicDataObj& id, LogicDataObj &val)
{
	PARSE_TRACE("logic_engine_test: opRead(%d) \n", id.GetInt());
	_setval(val);
	return true;
}

bool TestLogicObject::opWrite(const LogicDataObj& id, const LogicDataObj &val)
{
	PARSE_TRACE("logic_engine_test: opWrite(%d) \n", id.GetInt());
	//_setval(val);
	return true;
}


bool TestLogicObject::opAdd(const LogicDataObj& id, const LogicDataObj &val)
{
	PARSE_TRACE("logic_engine_test: opAdd(%d) \n", id.GetInt());
	//_setval(val);
	return true;
}


bool TestLogicObject::opSub(const LogicDataObj& id, const  LogicDataObj &val)
{
	PARSE_TRACE("logic_engine_test: opSub(%d) \n", id.GetInt());
	//_setval(val);
	return true;
}


bool TestLogicObject::opCheck(const LogicDataObj& id, const  LogicDataObj &val)
{
	if (val.GetDataType() == OT_STRING){
		const char *pText = val.GetText();
		if (pText && *pText){
			if (0 == ndstricmp(pText, "test-error"))	{
				m_error = NDERR_SUCCESS;
				return false;
			}
		}
	}
	//m_error = NDERR_NOT_SURPORT;
	return true;
}

bool TestLogicObject::opOperate(const char *cmd, const LogicDataObj& id, LogicDataObj &val)
{
	_myBase::opOperate(cmd, id, val);
	return true;
}


bool TestLogicObject::getOtherObject(const char*objName, LogicDataObj &val)
{
// 	if (callGetOtherObj(objName, val)) {
// 		return true;
// 	}
	if (0 == ndstricmp(objName, "LogFile")) {
		val.InitSet("ndlog.log");
		return true;
	}
	else if (0 == ndstricmp(objName, "LogPath")) {
		val.InitSet("../log");
		return true;
	}
	else if (0 == ndstricmp(objName, "WritablePath")) {
		val.InitSet("../log");
		return true;
	}

	else if (0 == ndstricmp(objName, "SelfName")) {
		val.InitSet("defaultTestObject");
		return true;
	}
	else if (0 == ndstricmp(objName, "self")) {
		val.InitSet((void*)this, OT_OBJ_BASE_OBJ);
		return true;
	}

	else if (0 == ndstricmp(objName, "machineInfo")) {
		char buf[256];
		val.InitSet(nd_common_machine_info(buf, sizeof(buf)));
		return true;
	}
	else if (0 == ndstricmp(objName, "settingFile")) {
		const char *pSettingFile = LogicCompiler::get_Instant()->getConfigFileName();
		val.InitSet(pSettingFile);
		return true;
	}
	else if (0 == ndstricmp(objName, "globalParser")) {
		LogicParserEngine &parser = LogicEngineRoot::get_Instant()->getGlobalParser();
		val.InitSet(&parser);
		return true;
	}
	else {
		return _myBase::getOtherObject(objName, val);
	}
	return false;
}
LogicObjectBase *TestLogicObject::getObjectMgr(const char* destName)
{
	PARSE_TRACE("logic_engine_test: getObjectMgr(%s) \n", destName);
	return this;
}


int TestLogicObject::Print(logic_print f, void *pf)
{
	PARSE_TRACE("this is object for test_engine\n");
	return 31;
}

bool TestLogicObject::BeginAffair()
{
	PARSE_TRACE("logic_engine_test: BeginAffair() \n");
	return true;
}

bool TestLogicObject::CommitAffair()
{
	PARSE_TRACE("logic_engine_test: CommitAffair \n");
	return true;
}

bool TestLogicObject::RollbackAffair()
{
	PARSE_TRACE("logic_engine_test: RollbackAffair() \n");
	return true;
}


void TestLogicObject::_setval(LogicDataObj &val)
{
	LogicDataObj data;
	data.InitSet("0");
	val = data;
}

