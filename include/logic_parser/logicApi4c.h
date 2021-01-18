/* file logicApi4c.h
 *
 * define logic parse api for ansi-c
 *
 * create by duan 
 *
 * 2018.11.2
 */

#ifndef _LOGIC_API4C_H_
#define _LOGIC_API4C_H_


#include "nd_common/nd_common.h"
#include "nd_common/nd_export_def.h"

#if defined(ND_COMPILE_AS_DLL) && (defined(_WINDOWS) || defined(WIN32) || defined(WIN64))
#if  defined(LOGIC_PARSE_EXPORTS) 
# define LOGIC_PARSER_API 				CPPAPI  DLL_EXPORT
# define LOGIC_PARSER_CLASS 			DLL_EXPORT
#else
# define LOGIC_PARSER_API 				CPPAPI  DLL_IMPORT
# define LOGIC_PARSER_CLASS				DLL_IMPORT
#endif
#else 
# define LOGIC_PARSER_API 				CPPAPI
# define LOGIC_PARSER_CLASS				
#endif 

#define LOGIC_TYPE_INT  0x0 
#define LOGIC_TYPE_FLOAT  0x1
#define LOGIC_TYPE_STRING  0x2
#define LOGIC_TYPE_INT8  0x3
#define LOGIC_TYPE_INT16  0x4
#define LOGIC_TYPE_INT64  0x5
#define LOGIC_TYPE_BOOL  0x6
#define LOGIC_TYPE_ARRAY  0x7
#define LOGIC_TYPE_USER_DEFINED  0xc

#define OFFSET_MASK 0xabababababababab
typedef void *logicDataPtr;		//data type 
typedef void *logicRootHandle;	// root of context
typedef void *logicParserHandle;	// virtual machine 
typedef void *logicJsonDataPtr;

typedef int logicResult;		//0 is success,
#define LOGIC_BOOL int
//parser function
LOGIC_PARSER_API logicParserHandle logicGetGlobalParser();
LOGIC_PARSER_API logicParserHandle logicCreateParser();
LOGIC_PARSER_API void logicDestroyParser(logicParserHandle);

//system entry 
LOGIC_PARSER_API int logicInitMachine();
LOGIC_PARSER_API int logicGetLastError(logicParserHandle);
LOGIC_PARSER_API int logicLoadScript(const char *scriptFile);
LOGIC_PARSER_API LOGIC_BOOL logicRunFunction(logicParserHandle parser, int argc, const char *argv[]);

//data function 
LOGIC_PARSER_API int logicDataGetType(logicDataPtr pData);

LOGIC_PARSER_API NDUINT32 logicDataGetInt(logicDataPtr pData);
LOGIC_PARSER_API float logicDataGetFloat(logicDataPtr pData);
LOGIC_PARSER_API NDUINT64 logicDataGetLong(logicDataPtr pData);

LOGIC_PARSER_API const char *logicDataGetOrgText(logicDataPtr pData);		//only used when type == OT_STRING
LOGIC_PARSER_API char *logicDataConvertToText(logicDataPtr pData) ; //textAddr output memory addr , need destroy by logicDataDestroyText
LOGIC_PARSER_API void logicDataDestroyText(char *textAddr);
LOGIC_PARSER_API logicDataPtr logicDataGetJson(logicDataPtr pData,const char *memberName);


LOGIC_PARSER_API void *logicDataGetObjectAddr(logicDataPtr pData) ;
LOGIC_PARSER_API void *logicDataGetBinary(logicDataPtr pData) ;
LOGIC_PARSER_API size_t logicDataGetBinarySize(logicDataPtr pData) ;
LOGIC_PARSER_API nd_handle logicDataGetNDHandle(logicDataPtr pData);

LOGIC_PARSER_API NDUINT32 logicDataGetarrayInt(logicDataPtr pData,int index) ;
LOGIC_PARSER_API float logicDataGetarrayFloat(logicDataPtr pData, int index) ;
LOGIC_PARSER_API NDUINT64 logicDataGetarrayInt64(logicDataPtr pData, int index) ;
LOGIC_PARSER_API const char *logicDataGetarrayOrgText(logicDataPtr pData, int index) ;
LOGIC_PARSER_API char* logicDataConvertArrayToText(logicDataPtr pData, int index) ;		//like logicDataConvertToText
LOGIC_PARSER_API logicJsonDataPtr GetarrayUser(logicDataPtr pData, int index) ;

LOGIC_PARSER_API logicResult logicDataSetInt(logicDataPtr pData, NDUINT32 val);
LOGIC_PARSER_API logicResult logicDataSetFloat(logicDataPtr pData, float val);
LOGIC_PARSER_API logicResult logicDataSetLong(logicDataPtr pData, NDUINT64 val);
LOGIC_PARSER_API logicResult logicDataSetText(logicDataPtr pData, const char* val);
LOGIC_PARSER_API logicResult logicDataFromText(logicDataPtr pData, const char* inputText, int inputType);

typedef LOGIC_BOOL (*logicParseC_entry)(logicParserHandle parser, int argc , logicDataPtr argv[], logicDataPtr retVal);


LOGIC_PARSER_API logicResult logicRootInstallFunc(logicParseC_entry entry,const char*scriptApiName, const char*comment);
LOGIC_PARSER_API LOGIC_BOOL logic_c_hello_world(logicParserHandle parser, int argc, logicDataPtr argv[], logicDataPtr retVal); //for test

LOGIC_PARSER_API LOGIC_BOOL logic_script_main_entry(int argc, const char *argv[], void *data, size_t data_size);

LOGIC_PARSER_API LOGIC_BOOL logicLoadPlugin(const char *pluginName, const char *pluginPath);

// DEFINE event id 

#ifdef _APOLLO_DEFINE_EVENT_VAL 
#undef _APOLLO_DEFINE_EVENT_VAL
#endif 
#define _APOLLO_DEFINE_EVENT_VAL(enum_name, _id, _desc) LOGIC_##enum_name = _id ,
enum eLogicEventId {
#include "logic_parser/_logicEventId.h"
	ELOGIC_EVENT_NUMBER
};

#undef _APOLLO_DEFINE_EVENT_VAL

#endif
