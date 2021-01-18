/* file logicengine_api.h
 *
 * interface of game logic engine 
 *
 * create by duan 
 *
 * 2015-5-20
 */

#ifndef _LOGIC_ENGINE_API_H_
#define _LOGIC_ENGINE_API_H_

#include "nd_common/nd_common.h"
#include "nd_common/cJSON.h"
//#include "nd_net/nd_netlib.h"

class LogicEngineRoot;
class LogicParserEngine;
class LogicUserDefStruct;
//install c-api
//int init_sys_functions(LogicEngineRoot *root);

LOGIC_PARSER_API int common_export_c_api_descript(const char *outfile);
LOGIC_PARSER_API int common_export_error_list(const char *outfile);//export error to xml file 
LOGIC_PARSER_API int common_export_error_csv(const char *outfile);


LOGIC_PARSER_API bool apollo_printf(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result);
LOGIC_PARSER_API bool apollo_log(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result);
LOGIC_PARSER_API bool apollo_time_func(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result);
LOGIC_PARSER_API int apollo_logic_out_put(const char *text);
LOGIC_PARSER_API int apollo_input_line(LogicParserEngine*parser, LogicDataObj &result);

#ifdef ND_OUT_LOG_2CTRL

LOGIC_PARSER_API int _apollo_script_printf(const char *stm, ...);
#define apollo_script_printf _apollo_script_printf

#else 
#define apollo_script_printf(stm,...)
#endif


#endif
