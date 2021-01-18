/* file logicEngineRooot.h
 *
 * root manager of logic engine 
 *
 * create by duan
 * 2015-5-12
 */

#ifndef _LOGIC_ENGINE_ROOT_H_ 
#define _LOGIC_ENGINE_ROOT_H_

#include "logic_parser/logicParser.h"
#include "logic_parser/logic_function.h"
#include "logic_parser/logic_debugger.h"
#include "logic_parser/logicStruct.hpp"
//#include "logic_parser/pluginsMgr.h"

#define  DEFAULT_LOAD_INITILIZER_FUNC "_module_init_entry" 
#define GLOBAL_MODULE_NAME "_global"
#define LOGIC_FUNCTION_LIST_NAME "func_list"
#define LOGIC_EVENT_LIST_NAME "event_list"
#define LOGIC_ERROR_LIST_NAME "error_list"

//typedef bool(*logicParser_func)(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result);
//typedef int(*logic_console_input_func)(LogicParserEngine*parser, LogicDataObj &result);

class LogicEngineRoot;
//LogicObjectBase for editor and tool
class LOGIC_PARSER_CLASS TestLogicObject : public LogicObjectBase
{
	typedef LogicObjectBase _myBase;
public:
	TestLogicObject();
	TestLogicObject(LogicEngineRoot *root);
	virtual~TestLogicObject();

	virtual bool loadDataType(const char *file);
	bool opRead(const LogicDataObj& id, LogicDataObj &val);
	bool opWrite(const LogicDataObj& id, const LogicDataObj &val);
	bool opAdd(const LogicDataObj& id, const LogicDataObj &val);
	bool opSub(const LogicDataObj& id, const  LogicDataObj &val);
	bool opCheck(const LogicDataObj& id, const  LogicDataObj &val);
	//common operate 
	bool opOperate(const char *cmd, const LogicDataObj& id, LogicDataObj &val);

	bool getOtherObject(const char*objName, LogicDataObj &val);
	LogicObjectBase *getObjectMgr(const char* destName);
	int Print(logic_print f, void *pf); //print object self info 

	bool BeginAffair();
	bool CommitAffair();
	bool RollbackAffair();

public:
	LogicParserEngine m_parser;
private:

	void _setval(LogicDataObj &val);
};

typedef std::map<std::string, func_cpp_info> cpp_func_map;

class ParserCmdLine
{
public:
	ParserCmdLine() :argc(0)
	{
	}
	ParserCmdLine(int in_argc, const char *in_argv[]) 
	{
		argc = 0;
		for (int i = 0; i < in_argc; i++)	{
			pushParam(in_argv[i]);
		}
	}

	~ParserCmdLine()
	{
		for (int i = 0; i < argc; i++)	{
			free(argv[i]);
		}
	}

	bool pushParam(const char *param)
	{
		if (argc < 10)	{
			size_t s = ndstrlen(param) + 1;

			argv[argc] = (char*) malloc(s);
			ndstrncpy(argv[argc], param, s);
			argc++;
			return true;
		}
		return false;
	}

public:
	int argc;
	char *argv[10];
};

class LOGIC_PARSER_CLASS LogicEngineRoot:public ILogicRoot
{
public:
	LogicEngineRoot();
	~LogicEngineRoot();

	static LogicEngineRoot *get_Instant();
	static void destroy_Instant();
	
	int LoadScript(const char *scriptFile, ILogicParser *initCallLoader);
	int LoadFromBuf(void *pAddr, size_t s, ILogicParser *initCallLoader);
	bool unloadModule(const char *moduleName);
	bool UnloadAllModules();
	const char *getLastLoadModule() {	return m_lastloadModule.c_str();}
	int Reload(const char *scriptFile, ILogicParser *initCallLoader);

	int Create(const char *name);
	void Destroy(int flag =0);
	void update(ndtime_t tminterval);
	
	const scriptCmdBuf* getScript(const char *funcName,const char *inModuleName, const char**outModuleName) const;
	const function_vct *getEventFunc(int event_id)const;
	const func_cpp_info* getCPPFunc(const char *funcName) const;

	// set api function . call in c++ . 
	static void installFunc(logicParser_func func, const char *name, const char *dispName,bool ansiC = false);
	static void setConsoleInput(logic_console_input_func func);
	static logic_console_input_func getConsoleInput( );
	
	void installEvent(int event_id, const char *event_func);
	void removeFromEvents(const char *funcName);

	//export c++ function to xml
	int dumbCPPfunc(const char *outXmlfile);// out put cpp function for editor using.
	int test();
	
	const char* _convertFuncName(const char *text, int inputEncodeType);
	
	bool getModuleChangedTime(const char *moduleName, LogicDataObj &result);
	const char *getAuthor(){		return m_author.c_str();	}
	int GetEncodeType() {return m_scriptEncodeType;	} //get current module script encode-type
	bool CheckIsDebug() {return m_isDebug ? true : false;	}
	logic_print setPrint(logic_print func, void *outfile);
	
	
	int getErrno(){ return m_logicObjOwner.m_parser.getErrno(); }

	LogicParserEngine &getGlobalParser() {return m_logicObjOwner.m_parser;	}
	LocalDebugger &getGlobalDebugger() { return  m_globalDebugger; }
	TestLogicObject &getDefParserOwner() { return m_logicObjOwner; }

	const char *getDftScriptModule() const {return m_dftScriptModule.c_str();}
	bool setDftScriptModule(const char *script) ;

	void setOutPutEncode(int encode);
	int getOutPutEncode() { return m_displayEncodeType; }


	const char *getMainModuleName() { return m_mainModule.c_str(); }		
	bool addGlobalFunction(int byteOrder, char*name, void *data, size_t size);
	//PluginsMgr & getPlugin() { return m_plugin; }
public :
	logic_print m_screen_out_func;
	void *m_print_file;
	
private:

	typedef std::map<std::string, scriptCmdBuf*> script_func_map;	
	typedef std::map<std::string, script_func_map*>script_module_map;
	typedef std::map<std::string, time_t>module_changed_tm_map;
	//typedef std::map<int, std::string> event_table_map;
	typedef std::map<int, function_vct> event_table_map;
	

	int LoadFromBufEx(void *pAddr, size_t s, LogicParserEngine *initCallLoader, const char *userDefName);
	int unloadScript();
	int _unloadModule(const char *moduleName,script_func_map &module);
	bool _pushGlobal(scriptCmdBuf *pcmdbuf);
	const char *_parserScriptStreamHeader(const char *data, size_t &size ) ;

	const scriptCmdBuf* _findScript(const char *funcName, const char *moduleName, const char**outModuleName) const;
	

	int m_displayEncodeType;
	int m_scriptEncodeType;

	NDUINT8 m_isDebug;

	script_module_map m_modules;
	static cpp_func_map *m_c_funcs;
	event_table_map m_event_entry;
	module_changed_tm_map m_moduleChangedTime;

	static logic_console_input_func m_console_input;

	TestLogicObject m_logicObjOwner;
	LocalDebugger m_globalDebugger;
	std::string m_dftScriptModule ; //default run script module if undefine module
	std::string m_mainModule;		//project name for display
	std::string m_author;
	std::string m_lastloadModule;

	//PluginsMgr m_plugin;

};




#define CHECK_ARGS_NUM_ONLY(_args, _num, _parser) \
	if (_args.size() < _num){		\
		if(_parser)_parser->setErrno(NDERR_FEW_PARAMS); \
		nd_logerror("%s error need %d args \n",__FUNCTION__, _num);	\
		return false;				\
						}

#ifndef ND_DEBUG
#define CHECK_ARGS_NUM(_args, _num, _parser) \
	if (_args.size() < _num){		\
		if(_parser)_parser->setErrno(NDERR_FEW_PARAMS); \
		nd_logerror("%s error need %d args \n",__FUNCTION__, _num);	\
		return false;				\
				}
#else 
#define CHECK_ARGS_NUM(_args, _num, _parser) \
	if (_args.size() < _num){		\
		if(_parser)_parser->setErrno(NDERR_FEW_PARAMS); \
		nd_logerror("%s error need %d args \n",__FUNCTION__, _num);	\
		return false;				\
	}								\
	else {							\
		for (int _j=1; _j<_num; _j++) {	\
			if(!_args[_j].CheckValid()) {	\
				if(_parser)_parser->setErrno(NDERR_PARAM_INVALID); \
				nd_logerror("%s error arg[%d] is invalid\n",__FUNCTION__, _j);	\
				return false ;			\
			}							\
		}								\
	}

#endif

#define CHECK_DATA_TYPE(_data, _type, _parser) \
	if (_data.GetDataType() != _type) {	\
		if (_parser)_parser->setErrno(NDERR_PARAM_TYPE_NOT_MATCH); \
		nd_logerror(#_data " type error need " #_type " \n");	\
		return false;					\
			}									\
	if (_type >= OT_OBJECT_VOID && NULL==_data.GetObjectAddr()) {	\
		if(_parser)_parser->setErrno(NDERR_PARAM_INVALID); \
		nd_logerror(#_data " is NULL \n");	\
		return false;						\
			}



struct logicFuncInstallHelper
{
	logicFuncInstallHelper(logicParser_func expFunction, const char *name, const char *comment,bool ansiC=false)
	{
		LogicEngineRoot::installFunc(expFunction, name, comment, ansiC);
	}

};

#ifdef WITHOUT_LOGIC_PARSER

#define APOLLO_SCRIPT_API_DEF(_FUNC_NAME, _COMMENT) \
bool _FUNC_NAME(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)


#define APOLLO_SCRIPT_API_DEF_GLOBAL(_FUNC_NAME, _COMMENT) \
bool _FUNC_NAME(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)

#else 

#define APOLLO_SCRIPT_API_DEF(_FUNC_NAME, _COMMENT) \
static bool _FUNC_NAME(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result) ;		\
static logicFuncInstallHelper __s_installer_helper##_FUNC_NAME((logicParser_func)_FUNC_NAME,#_FUNC_NAME, _COMMENT) ;	\
bool _FUNC_NAME(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)


#define APOLLO_SCRIPT_API_DEF_GLOBAL(_FUNC_NAME, _COMMENT) \
bool _FUNC_NAME(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result) ;		\
static logicFuncInstallHelper __s_installer_helper##_FUNC_NAME((logicParser_func)_FUNC_NAME,#_FUNC_NAME, _COMMENT) ;	\
bool _FUNC_NAME(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)

#endif 

#endif 
