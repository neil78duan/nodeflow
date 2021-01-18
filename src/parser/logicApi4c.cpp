/* file logicApi4c.cpp
 *
 * define logic parse api for ansi-c
 *
 * create by duan 
 *
 * 2018.11.2
 */

#include "logic_parser/logicDataType.h"
#include "logic_parser/logicEngineRoot.h"
#include "logic_parser/pluginsMgr.h"
#include "logic_parser/logicApi4c.h"
#include "logic_parser/logicStruct.hpp"

#include <string>


logicParserHandle logicGetGlobalParser()
{
	return (logicParserHandle) &(LogicEngineRoot::get_Instant()->getGlobalParser());
}

logicParserHandle logicCreateParser()
{
	return new LogicParserEngine();
}

void logicDestroyParser(logicParserHandle parser)
{
	LogicParserEngine *p = (LogicParserEngine*)parser ;
	delete p;
}


int logicInitMachine()
{
	if (-1 == nd_common_init()) {
		return -1;
	}
	LogicEngineRoot *scriptRoot = LogicEngineRoot::get_Instant();
	nd_assert(scriptRoot);
	scriptRoot->setOutPutEncode(ND_ENCODE_TYPE);

	scriptRoot->setPrint((logic_print)ndfprintf, stdout);
	return 0;
}

int logicGetLastError(logicParserHandle handle)
{
	LogicParserEngine *parser = (LogicParserEngine*)handle;
	if (!parser) {
		return -1;
	}
	return parser->getErrno();
}
int logicLoadScript(const char *scriptFile)
{
	LogicEngineRoot *scriptRoot = LogicEngineRoot::get_Instant();
	return scriptRoot->LoadScript(scriptFile,NULL);
}

LOGIC_BOOL logicRunFunction(logicParserHandle parser, int argc, const char *argv[])
{
	LogicEngineRoot *scriptRoot = LogicEngineRoot::get_Instant();
	int ret = scriptRoot->getGlobalParser().runCmdline(argc, argv, ND_ENCODE_TYPE);
	if (ret == -1) {
		return false;
	}
	return true;
}


int logicDataGetType(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return (int)dataObj->GetDataType();
}


NDUINT32 logicDataGetInt(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetInt();
}
float logicDataGetFloat(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetFloat();
}
NDUINT64 logicDataGetLong(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetInt64();
}

const char *logicDataGetOrgText(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetText();
}
char *logicDataConvertToText(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	std::string str1 = dataObj->GetString();
	if (str1.size() == 0) {
		return NULL;
	}
	char *p = (char*) malloc(str1.size() + 1);
	ndstrncpy(p, str1.c_str(), str1.size());
	return p;
}

void logicDataDestroyText( char *textAddr)
{
	free(textAddr);
}

logicDataPtr logicDataGetJson(logicDataPtr pData, const char *memberName)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;

	LogicUserDefStruct *pUser = (LogicUserDefStruct *) dataObj->getUserDef();
	if (!pUser) {
		return NULL;
	}

	return(logicDataPtr) pUser->ref(memberName);
}

void *logicDataGetObjectAddr(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetObjectAddr();
}

void *logicDataGetBinary(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetBinary();
}
size_t logicDataGetBinarySize(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetBinarySize();
}

nd_handle logicDataGetNDHandle(logicDataPtr pData)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetNDHandle();
}

NDUINT32 logicDataGetarrayInt(logicDataPtr pData, int index)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetarrayInt(index);
}
float logicDataGetarrayFloat(logicDataPtr pData, int index)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetarrayFloat(index);
}
NDUINT64 logicDataGetarrayInt64(logicDataPtr pData, int index)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetarrayInt64(index);
}
const char *logicDataGetarrayOrgText(logicDataPtr pData, int index)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return dataObj->GetarrayText(index);
}
char* logicDataConvertArrayToText(logicDataPtr pData, int index)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	std::string str1 = dataObj->GetarrayString(index);
	if (str1.size() == 0) {
		return NULL;
	}
	char *p = (char*)malloc(str1.size() + 1);
	ndstrncpy(p, str1.c_str(), str1.size());
	return p;
}

logicJsonDataPtr GetarrayUser(logicDataPtr pData, int index)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	return (logicJsonDataPtr*) dataObj->GetarrayUser(index);
}

logicResult logicDataSetInt(logicDataPtr pData, NDUINT32 val)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	dataObj->InitSet((int)val);
	return 0;
}
logicResult logicDataSetFloat(logicDataPtr pData, float val)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	dataObj->InitSet((float)val);
	return 0;
}
logicResult logicDataSetLong(logicDataPtr pData, NDUINT64 val)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	dataObj->InitSet(val);
	return 0;
}
logicResult logicDataSetText(logicDataPtr pData, const char* val)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	dataObj->InitSet(val);
	return 0;
}


logicResult logicDataFromText(logicDataPtr pData, const char* inputText, int inputType)
{
	nd_assert(pData);
	LogicDataObj *dataObj = (LogicDataObj*)pData;
	if (!dataObj->InitTypeFromTxt(inputText, (DBL_ELEMENT_TYPE)inputType)) {
		return -1;
	}
	return 0;
}

logicResult logicRootInstallFunc(logicParseC_entry entry, const char*scriptApiName, const char*comment)
{
	LogicEngineRoot::installFunc((logicParser_func)entry, scriptApiName,comment,true);
	return 0;
}

LOGIC_BOOL logic_c_hello_world(logicParserHandle parser, int argc, logicDataPtr argv[], logicDataPtr retVal)
{
	logicDataSetText( retVal,"hello world");
	return 1;
}

LOGIC_BOOL logicLoadPlugin(const char *pluginName, const char *pluginPath)
{
	PluginsMgr *plgMgr = PluginsMgr::get_Instant();
	return plgMgr->load(pluginName, pluginPath) ? 1 : 0;

}

LOGIC_BOOL logicDestroyPlugin(const char *pluginName)
{

	PluginsMgr *plgMgr = PluginsMgr::get_Instant();
	return plgMgr->unLoadPlugin(pluginName) ? 1 : 0;
}

////

ILogicRoot *ILogicRoot::GetRoot()
{
	return (ILogicRoot*)LogicEngineRoot::get_Instant() ;
}
void ILogicRoot::DestroyRoot()
{
	LogicEngineRoot::destroy_Instant();
}

void ILogicRoot::installFunc(logicParser_func func, const char *name, const char *dispName,bool ansiC )
{
	LogicEngineRoot::get_Instant()->installFunc( func,name,dispName,ansiC );
}
void ILogicRoot::setConsoleInput(logic_console_input_func func)
{
	LogicEngineRoot::get_Instant()->setConsoleInput( func);
}
logic_console_input_func ILogicRoot::getConsoleInput( )
{
	return LogicEngineRoot::get_Instant()->getConsoleInput();
}
