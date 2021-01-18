/* file logicApi.hpp
 *
 * api for cpp
 *
 * create by duan
 *
 * 2018-11-9
 *
 */

#ifndef _LOGIC_API_HPP_
#define _LOGIC_API_HPP_

#include "ndapplib/nd_iBaseObj.h"
#include "logic_parser/logicApi4c.h"
#include "logic_parser/logicDataType.h"

typedef std::vector<LogicDataObj> parse_arg_list_t;
typedef std::vector<std::string>function_vct;
class ILogicParser;

class  LOGIC_PARSER_CLASS ILogicObject : public NDIBaseObj
{
public:
	virtual bool opRead(const LogicDataObj& id, LogicDataObj &val) = 0;
	virtual bool opWrite(const LogicDataObj& id, const  LogicDataObj &val) = 0;
	virtual bool opAdd(const LogicDataObj& id, const  LogicDataObj &val) = 0;
	virtual bool opSub(const LogicDataObj& id, const  LogicDataObj &val) = 0;
	virtual bool opCheck(const LogicDataObj& id, const LogicDataObj &val) =0;
	//common operate @val  input-output
	virtual bool opOperate(const char *cmd, const LogicDataObj& id, LogicDataObj &val) = 0;

	//get object manager for opRead/write/...
	virtual ILogicObject *getObject(const char* destName) = 0;
	//get other object
	virtual bool getOtherObject(const char*objName, LogicDataObj &val) =0;
	virtual int Print(logic_print f, void *pf) = 0; //print object self info 

	//affair function
	virtual bool CheckInAffair()=0;
	virtual bool BeginAffair()=0;
	virtual bool CommitAffair()=0;
	virtual bool RollbackAffair()=0;

public:
	ILogicObject() {}
	virtual ~ILogicObject() {}
};

class  LOGIC_PARSER_CLASS  ILogicParser : public NDIBaseObj
{
public:
	virtual int runCmdline(int argc, const char *argv[], int encodeType = ND_ENCODE_TYPE) = 0;
	virtual bool runScript(int encodeType, const char *scriptName, LogicDataObj &result, int num, ...) = 0;
	virtual bool eventNtf(int event_id, int num, ...) = 0;
	
	virtual void Reset() = 0;
	virtual void setErrno(int errcode) = 0;
	virtual int getErrno() = 0;
	virtual void setSimulate(bool flag = false) = 0;
	virtual bool checkSimulate() = 0;
	virtual LogicDataObj &getValue() = 0;
	virtual ILogicObject *getOwner() = 0;
	virtual void setOwner(ILogicObject *owner) = 0;

protected:
	ILogicParser() {}
	virtual~ILogicParser() {}
};

typedef bool(*logicParser_func)(ILogicParser*parser, parse_arg_list_t &args, LogicDataObj &result);
typedef int(*logic_console_input_func)(ILogicParser*parser, LogicDataObj &result);

struct func_cpp_info
{
	func_cpp_info() :isAnsiC(0),func(0){}
	int isAnsiC;
	std::string comment;
	logicParser_func func;
};


class LOGIC_PARSER_CLASS ILogicRoot:public NDIBaseObj
{
public:
	
	static ILogicRoot *GetRoot();
	static void DestroyRoot();
	
	// set api function . call in c++ .
	static void installFunc(logicParser_func func, const char *name, const char *dispName,bool ansiC = false);
	static void setConsoleInput(logic_console_input_func func);
	static logic_console_input_func getConsoleInput( );
	
	virtual int LoadScript(const char *scriptFile, ILogicParser *initCallLoader)=0;
	virtual int LoadFromBuf(void *pAddr, size_t s, ILogicParser *initCallLoader)=0;
	virtual bool unloadModule(const char *moduleName)=0;
	
	virtual const function_vct *getEventFunc(int event_id)const=0;
	virtual const func_cpp_info* getCPPFunc(const char *funcName) const=0;
	
	
	virtual void installEvent(int event_id, const char *event_func)=0;
	
//	virtual logic_print setPrint(logic_print func, void *outfile)=0;
//
//	virtual  const char *getDftScriptModule() = 0;
//	virtual bool setDftScriptModule(const char *script)  = 0;
//
//	virtual void setOutPutEncode(int encode) = 0;
//	virtual int getOutPutEncode() = 0;
	
protected:
	
	ILogicRoot(){}
	virtual ~ILogicRoot(){}
};
#endif
