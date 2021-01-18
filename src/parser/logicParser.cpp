//
//  logicParser.cpp
//  gameHall
//
//  Created by duanxiuyun on 15-3-24.
//  Copyright (c) 2015 duanxiuyun. All rights reserved.
//

#include <math.h>
#include <string>

#include "logic_parser/logicEndian.h"
#include "logic_parser/logicParser.h"
//#include "logic_parser/dbl_mgr.h"
#include "logic_parser/logic_compile.h"
#include "logic_parser/logicEngineRoot.h"
#include "logic_parser/logicStruct.hpp"
#include "ndstl/nd_utility.h"

// such as arr[varUser.arr[1]]  ,read varUser.arr[1]
static const char *ndstr_fetch_array_index(const char *input, char *output, size_t size)
{
	int markNum = 1;
	const char *p = (char*)ndstrchr(input, _ARRAR_BEGIN_MARK);
	if (p && *p == _ARRAR_BEGIN_MARK){
		++p;
	}
	else {
		return NULL;
	}

	while (*p && size > 0) {

		if (*p == _ARRAR_BEGIN_MARK) {
			++markNum;
		}
		else if (*p == _ARRAR_END_MARK) {
			--markNum;
			if (markNum == 0)	{
				break;
			}
		}
		*output++ = *p++;
		--size;
	}
	*output++ = 0;
	if (*p == _ARRAR_END_MARK)	{
		++p;
	}
	return p;
}



//float LogicObjectBase::opCalc(void *func, int size)
//{
//	return 0 ;
//}
//////////////////////////////
//
//LogicParserEngine::script_func_map LogicParserEngine::s_func_table;

LogicParserEngine::LogicParserEngine() :m_owner(NULL)
{
	Create();
	m_registerVal.EnableReference(false);
	m_root = LogicEngineRoot::get_Instant();
}

LogicParserEngine::LogicParserEngine(LogicObjectBase *owner,  LogicEngineRoot *root ) : m_owner(owner)
{
	Create();
	m_registerVal.EnableReference(false);
	if (!root) {
		m_root = LogicEngineRoot::get_Instant();
	}
	else {
		m_root = root;
	}
}
LogicParserEngine::~LogicParserEngine()
{
	Destroy(1);
	m_root = LogicEngineRoot::get_Instant();
}


int LogicParserEngine::Create(const char *)
{
	m_registorFlag = false;
	m_registorCtrl = true;
	m_OnErrorExit = false;
	m_simulate = false;
	m_bStepMode = false;
	m_bLastIsCalled = false;

	m_bIniativeQuit = false;
	m_runInTimer = false;
	m_sys_errno = 0;
	m_registerCount = 0;
	m_cmdByteOrder = 0;

	m_registerVal = LogicDataObj();

	m_curStack = NULL;
	m_loopIndex = LogicDataObj();

	m_dbg_cur_node_index = 0;
	m_dbg_node[0] = 0;
	m_dbg_fileInfo[0] = 0;
	for (int i = 0; i < LOGIC_ERR_NUMBER; i++) {
		m_skipErrors[i] = 0;
	}

	return 0;
}
void LogicParserEngine::Destroy(int )
{
	m_events.clear();
	m_needHandleEvents.clear();
	m_hanlers.clear();
	m_timer.clear();
	m_delTimerQueue.clear();
	m_runningStacks.clear();


	UserDefData_map_t::iterator it;
	for (it = m_useDefType.begin(); it != m_useDefType.end(); it++) {
		if (it->second) {
			delete (it->second);
			it->second = 0;
		}
	}
	m_useDefType.clear();

	m_global_vars.clear();

}

int LogicParserEngine::runCmdline(int argc, const char *argv[], int encodeType )
{
	parse_arg_list_t params;
	for (int i = 0; i < argc; i++)	{
		params.push_back(LogicDataObj(argv[i]));
	}
	m_bIniativeQuit = false;
	LogicDataObj result;

	if (runScript(argv[0], params, result, encodeType)) {
		return 0;
	}
	return getErrno();
}

bool LogicParserEngine::runScript(int encodeType ,const char *scriptName,LogicDataObj &result, int param_num, ...)
{
	LogicDataObj fname;
	parse_arg_list_t params;
	const char *argName = scriptName;
	
	fname.InitSet(argName);
	params.push_back(fname);
	
	va_list arg;
	int num = param_num;
	
	va_start(arg, param_num);
	while (num-- > 0) {
		LogicDataObj data1;
		data1.InitSet(va_arg(arg, const char*));
		params.push_back(data1);
	}
	va_end(arg);
	
	return runScript(scriptName, params, result, encodeType);
}
bool LogicParserEngine::runScript(const char *scriptName, parse_arg_list_t &args, LogicDataObj &result, int encodeType)
{
	
	const char *pFuncname = m_root->_convertFuncName(scriptName, encodeType);
	const char *moduleName = NULL;
	const scriptCmdBuf *pcmdBuf = m_root->getScript(pFuncname, NULL, &moduleName);
	if (pcmdBuf) {

		setErrno(LOGIC_ERR_SUCCESS) ;
		if (true == _runCmdBuf(moduleName,pcmdBuf, args)) {
			result = getValue() ;
			return  true ;
		}
		//else if (m_sys_errno != NDERR_WOULD_BLOCK) {
		//	nd_logerror("run %s script error %d: %s\n", scriptName, getErrno(), nd_error_desc(getErrno()));
		//}
		
	}
	else {
		setErrno(LOGIC_ERR_FUNCTION_NOT_FOUND);
		nd_logerror("%s() script engine not found %s script-file\n", curFuncName(), scriptName);
	}
	return false;
}

bool LogicParserEngine::eventNtf(int event_id, int num, ...)
{
	parse_arg_list_t params;
	va_list arg;
	va_start(arg, num);
	while (num-- > 0) {
		LogicDataObj data1;
		data1.InitSet(va_arg(arg, const char*));
		params.push_back(data1);
	}
	va_end(arg);
	
	
	return eventNtf(event_id, params);
}

bool LogicParserEngine::eventNtf(int event_id, parse_arg_list_t &args)
{
	LogicDataObj result;
	onEvent(event_id, args);
	
	const function_vct *funcsOfEvent = m_root->getEventFunc(event_id);
	if (!funcsOfEvent){
		//nd_logwarn("event %d function not registered \n", event_id);
		setErrno(LOGIC_ERR_FUNCTION_NOT_FOUND);
		return false;
	}

	parse_arg_list_t params;
	params.push_back(LogicDataObj());
	
	parse_arg_list_t::iterator it = args.begin();
	for (; it != args.end(); it++) {
		params.push_back(*it);
	}

	for (size_t i = 0; i < funcsOfEvent->size(); i++){
		const char *script_name = (*funcsOfEvent)[i].c_str();
		if (*script_name) {
			params[0].InitSet(script_name);
			runScript(script_name, params, result);
		}
	}
	return true;
}

LogicObjectBase *LogicParserEngine::getLogicObjectMgr(const char *objname)
{
	if (m_owner){
		return m_owner->getObjectMgr(objname);
	}
	return NULL;
}

bool LogicParserEngine::_runCmdBuf(const char *moduleName ,const scriptCmdBuf *buf, parse_arg_list_t &params)
{
	int ret = -1;
	runningStack stack;

	Reset();
	stack.params = params;	
	stack.cur_point = buf->buf;
	stack.cmd = buf;
	stack.funcType = buf->funcType;

	if (moduleName && *moduleName){
		stack.curModuleName = moduleName;
	}
	else {
		stack.curModuleName.clear();
	}
	//runningStack *orgStack = m_curStack;

	m_bIniativeQuit = false;

	ret = callScript(&stack);

	int funcError = m_sys_errno;
	handleScriptGenEvent();
	m_sys_errno = funcError;

	return ret == 0;
}
bool LogicParserEngine::_runCmdBuf(const char *moduleName,const scriptCmdBuf *buf, int param_num, ...)
{
	LogicDataObj fname;
	parse_arg_list_t params;

	fname.InitSet(buf->cmdname);
	params.push_back(fname);

	va_list arg;
	int num = param_num ;
	
	va_start (arg, param_num);
	while (num-- > 0 ) {
		LogicDataObj data1;
		data1.InitSet(va_arg(arg, const char*));
		params.push_back(data1);
	}
	va_end (arg);

	m_bIniativeQuit = false;
	return _runCmdBuf(moduleName, buf, params);
}


const char *LogicParserEngine::getCurMoudle()
{
	if (m_curStack && m_curStack->curModuleName.size() ) {
		return  m_curStack->curModuleName.c_str() ;
	}
	return  NULL;
}

bool LogicParserEngine::getFuncHeader(const scriptCmdBuf *cmd, char *buf, size_t size)
{
	const char *p = cmd->buf;
	NDUINT32 opCmd = *((*(NDUINT32**)&p)++);
	if (opCmd != E_OP_HEADER_INFO){
		return false;
	}
	NDUINT16 len = *((*(NDUINT16**)&p)++); //string type
	len = *((*(NDUINT16**)&p)++); //string len
	if (len >= size){
		len = (NDUINT16) (size -1);
	}
	memcpy(buf, p, len);
	return true;
}
bool LogicParserEngine::runFunction(const char *moduleName ,const scriptCmdBuf *buf, parse_arg_list_t &args, LogicDataObj &result)
{
	//bool ret = false;
	runningStack stack;

	stack.params = args;
	stack.cur_point = buf->buf;
	stack.cmd = ( scriptCmdBuf *) buf;
	stack.curModuleName = moduleName;
	stack.funcType = buf->funcType;

	if (-1 == callScript(&stack)) {
		return false;
	}
	return true;

}


int LogicParserEngine::_reEntryScriptFunction(runningStack *stack)
{
	int ret = _baseCallScript(stack);
	if (m_sys_errno != NDERR_WOULD_BLOCK) {
		if (stack->affairHelper) {
			if (ret != 0 || !m_registorFlag) {
				stack->affairHelper->Rollback();
			}
			delete stack->affairHelper;
			stack->affairHelper = NULL;
		}
	}
	return ret;
}

int LogicParserEngine::callScript(runningStack *stack)
{
	if (m_curStack) {
		m_curStack->curRegisterCount = m_registerCount; 
		m_curStack->curRegisterCtrl = m_registorCtrl;
	}
	stack->curRegisterCount = m_registerCount;
	stack->curRegisterCtrl = m_registorCtrl;
	CallingStack_list::reverse_iterator it = m_runningStacks.rbegin();
	if (it != m_runningStacks.rend()) {
		stack->parentStack = &*it;
	}
	else {
		stack->parentStack = NULL;
	}

	m_runningStacks.push_back(*stack);
	it = m_runningStacks.rbegin(); 
	nd_assert(it != m_runningStacks.rend());

	int ret = _baseCallScript(&(*it));
	
	/*if (m_simulate == false)	{
		if (m_curStack && m_curStack->affairHelper)	{
			if (ret != 0 || !m_registorFlag) {
				m_curStack->affairHelper->Rollback();
			}
			delete m_curStack->affairHelper;
			m_curStack->affairHelper = NULL;
		}
	}
	else*/ if (m_sys_errno != NDERR_WOULD_BLOCK) {
		if (m_curStack && m_curStack->affairHelper)	{
			if (ret != 0 || !m_registorFlag) {
				m_curStack->affairHelper->Rollback();
			}
			delete m_curStack->affairHelper;
			m_curStack->affairHelper = NULL;
		}
	}

	//notify debugger run script success
	if (m_runningStacks.size()==1){
		onScriptRunCompleted();
	}

	m_runningStacks.pop_back();
	
	if (m_runningStacks.size() > 0)	{
		m_curStack = &(*(m_runningStacks.rbegin()));
	}
	else {
		m_curStack = NULL;
	}

	m_registerCount = stack->curRegisterCount ;
	m_registorCtrl = stack->curRegisterCtrl ;


	
	return ret;
}

//int LogicParserEngine::_runCmd(runningStack *stack)
int LogicParserEngine::_baseCallScript(runningStack *stack)
{
	//NDUINT16 dataTypeId;
	int readlen = 0;
	char *p = (char *) stack->cur_point ;
	NDUINT32 opCmd , opAim = 0 ;
	bool inException = false;
	bool scriptIsDebug = false;

	NDUINT32 cur_step_size = -1;
	const char *exception_addr = stack->exception_addr;
	
	operator_value_t val ;
	operator_index_t index ;
	NDUINT32 num ;
	NDUINT32 errorBeforeException = 0;
	
	LogicObjectBase* objAim = NULL ;
	LogicDataObj tmpInputVal, tmpIndexVal;

	m_curStack = stack; 
	m_cmdByteOrder = stack->cmd->byteOrder;
	m_sys_errno = 0;
	m_curFuncEncodeType = stack->cmd->encodeType;

	m_registerCount = stack->curRegisterCount;
	m_registorCtrl = stack->curRegisterCtrl;

	for (size_t i = 0; i < LOGIC_ERR_NUMBER; i++){
		if (m_curStack->skipErrors[i] != 0) {
			m_skipErrors[i] = m_curStack->skipErrors[i];
		}
	}

#define CHECK_INSTRUCTION_OVER_FLOW() \
	if (p < stack->cmd->buf || p >(stack->cmd->buf + stack->cmd->size)) {	\
		m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;	break;		\
			}

#define SHORT_JUMP(_p, _offset) \
	do { \
	if (_offset >= 0) (_p) += (_offset);	\
		else (_p) -= (size_t)(-(_offset));		\
	} while (0)

#define GET_VAR_FROM_STREAM(_stack, _pstream, _val) \
	do { \
		int _len = _getValue(_stack, _pstream, _val);	\
		if (_len <= 0){								\
			m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;		\
			break;										\
		}				\
		_pstream += _len;	\
	}while(0) ;	\
	if(m_sys_errno==LOGIC_ERR_INPUT_INSTRUCT)break 

#define GET_TEXT_FROM_STREAM(_buf, _size, _pstream) \
	do {	\
		int _len = _read_string(_pstream, _buf,_size);	\
		if (_len >0 ){	\
			_pstream += _len; \
			CHECK_INSTRUCTION_OVER_FLOW();	\
		}				\
		else {	m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;}	\
	} while (0) ;		\
	if(m_sys_errno==LOGIC_ERR_INPUT_INSTRUCT)break 


#define BEGIN_GET_FUNC_ARGS(_stack, _pstream) \
	_pstream = lp_read_stream(_pstream, num,m_cmdByteOrder);\
	if (num > 0) {							\
		LogicDataObj tmval1;					\
		parse_arg_list_t args;				\
		for (int i = 0; i < num; ++i) {		\
			readlen = _getValue(_stack, _pstream, tmval1);	\
			if (readlen <= 0){ m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;break;}		\
			_pstream += readlen;			\
			args.push_back(tmval1);			\
			CHECK_INSTRUCTION_OVER_FLOW();	\
		}									\
		if (m_sys_errno != LOGIC_ERR_INPUT_INSTRUCT) {	\

#define END_GET_FUNC_ARGS() \
				}\
		}

#define OBJ_OP_FUNC(_opfunc) do {\
		LogicDataObj objName1;	\
		GET_VAR_FROM_STREAM(stack, p, objName1);		\
		if(!objName1.GetText()) {m_sys_errno = LOGIC_ERR_PARSE_STRING; break;}\
		GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);		\
		GET_VAR_FROM_STREAM(stack, p, tmpInputVal);		\
		objAim = getLogicObjectMgr(objName1.GetText());				\
		if (objAim) {									\
			m_registorFlag = true ;						\
			if(tmpIndexVal.CheckValid()) {				\
				m_registorFlag = objAim->_opfunc(tmpIndexVal, tmpInputVal);	\
				if(!m_registorFlag) {						\
					m_sys_errno = objAim->getError();		\
					std::string str1, str2 ;				\
					tmpIndexVal.toStdString(str1) ;			\
					tmpInputVal.toStdString(str2) ;			\
					nd_logdebug(#_opfunc"(%s,%s) run on %s error code =%d\n",str1.c_str(),str2.c_str(), objName1.GetText(), m_sys_errno ) ; \
				}		\
			}			\
		}				\
		else {			\
			m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;	\
		}					\
	}while(0)


	while (p < (stack->cmd->buf + stack->cmd->size) ) {
		char name[128];
		name[0] = 0;

		//isdebug_step = false;
		ResetStep() ;
		//opCmd = *((*(NDUINT32**)&p)++) ;
		p = lp_read_stream(p, opCmd, m_cmdByteOrder);
		
		switch (opCmd) {
			case E_OP_EXIT:
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				index = tmpIndexVal.GetInt();
				m_registorFlag= (index==0) ? true: false ;
				p = (char*) (stack->cmd->buf + stack->cmd->size);
				m_sys_errno = tmpIndexVal.GetInt();
				
				if (!inException){
					m_bIniativeQuit = true;
				}
				break ;
			case E_OP_IDLE:
				m_registorFlag = true;
				break;
			case E_OP_EXCEPTION_BLOCK:
			{
				NDUINT16 len;
				p = lp_read_stream(p, len, m_cmdByteOrder);
				if (0 == exception_addr) {
					exception_addr = p;
					stack->exception_addr = exception_addr ;
				}
				p += len;
				m_registorFlag = true;
			}
				break;
			case E_OP_THROW:
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				index = tmpIndexVal.GetInt();
				if (inException){
					//already in exception
					break;
				}
				//jump to exception 
				inException = true;
				errorBeforeException = index;
				m_sys_errno = index;
				m_registorFlag = true;
				m_bIniativeQuit = true;

				if (exception_addr) {
					p = (char*)exception_addr;
					stack->cur_point = p;
				}
				else {
					nd_logdebug("call throw exception(%s) in %s bu without exception-handler\n",nd_error_desc(m_sys_errno), stack->cmd->cmdname);
					return -1;
				}
				
				continue;

			case  E_OP_CHECK_IS_SIMULATE:
				m_registorCtrl = m_simulate;
				m_registorFlag = true;
				break;

			case  E_OP_SET_ERROR:
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				index = tmpIndexVal.GetInt();
				m_registorFlag = (index == 0);
				m_sys_errno = index;
				break;
			case E_OP_GET_LAST_ERROR:
				if (inException) {
					m_registerVal.InitSet((int)errorBeforeException);
				}
				else {
					m_registerVal.InitSet(m_sys_errno);
				}
				m_registorFlag = true;
				break;

			case E_OP_CHECK_IN_USER_ERROR:
				m_registorCtrl = false;
				if (m_sys_errno)	{
					m_registorCtrl = !_checkIsSystemError(m_sys_errno);
				}
				m_registorFlag = true;
				break;
			case E_OP_CLEAR_ERROR_CODE:
				m_sys_errno = 0;
				m_registorFlag = true;
				break;
			case E_OP_GET_RUNNING_PARSER:
				m_registerVal.InitSet(this);
				m_registorFlag = true;
				break;

			case E_OP_DEBUG_INFO:
				//isdebug_step = true;
				scriptIsDebug = true;
				p = lp_read_stream(p, m_dbg_cur_node_index, m_cmdByteOrder);
				GET_TEXT_FROM_STREAM(m_dbg_node, sizeof(m_dbg_node),p);
				
				p = lp_read_stream(p, cur_step_size, m_cmdByteOrder);
				//step_start = p;
				CHECK_INSTRUCTION_OVER_FLOW();
				//apollo_script_printf("logic_script %s: %s %d\n", stack->cmd->cmdname, m_dbg_node, m_dbg_cur_node_index);
				m_registorFlag = true;
				
				if (-1 == _enterDebugMode()) {
					m_registorFlag = false;
				}
				m_bLastIsCalled = false;
				break;

			case E_OP_FILE_INFO:
				GET_TEXT_FROM_STREAM(m_dbg_fileInfo, sizeof(m_dbg_fileInfo), p);
				CHECK_INSTRUCTION_OVER_FLOW();
				m_registorFlag = true;
				//apollo_script_printf("logic_script %s:read file info :\n", stack->cmd->cmdname, m_dbg_fileInfo);
				break; 
			case E_OP_HEADER_INFO:
			{
				NDUINT16 a;
				p = lp_read_stream(p, a, m_cmdByteOrder);
				opAim = a;
				p = lp_read_stream(p, a, m_cmdByteOrder);
				readlen = a;

				p += readlen;
				CHECK_INSTRUCTION_OVER_FLOW();
				m_registorFlag = true;
				break;
			}
			case E_OP_ON_ERROR_CONTINUE:
				p = lp_read_stream(p, (NDUINT32&)val, m_cmdByteOrder);
				m_curStack->error_continue = val ? true : false;
				m_registorFlag = true;
				break;
			case E_OP_COMMIT_AFFAIR:
				_commitAffair();
				m_registorFlag = true;
				break;
			
			case E_OP_ROLLBACK_AFFAIR:
				_rollbacAffair();
				m_registorFlag = true;
				break;
			case E_OP_BEGIN_AFFAIR:
				m_registorFlag = _beginHelper();
				break;

			case E_OP_SKIP_ERROR:
				//p = lp_read_stream(p, opAim, m_cmdByteOrder);
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				_pushSKipError(tmpIndexVal.GetInt());
				m_registorFlag = true;
				break;

			case E_OP_READ:			//	读 eOperatorDest + operator_index_t
			
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);

				objAim = getLogicObjectMgr(tmpIndexVal.GetText());
				if (objAim) {
					m_registorFlag = objAim->opRead(tmpInputVal, m_registerVal);
					if (!m_registorFlag) {
						m_sys_errno = objAim->getError();
					}
				}
				else {
					m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
				}
				break ;
			case E_OP_WRITE:			//	写 eOperatorDest + operator_index_t + operator_value_t				
				OBJ_OP_FUNC(opWrite);
				break ;

			case E_OP_ADD :			//	增加 eOperatorDest + operator_index_t + operator_value_t + int
				OBJ_OP_FUNC(opAdd);
				break ;
			case E_OP_SUB :			//	减少 eOperatorDest + operator_index_t + operator_value_t + int
				OBJ_OP_FUNC(opSub);
				break ;			
			case E_OP_TEST:
				OBJ_OP_FUNC(opCheck);
				break;

			case E_OP_OPERATE:
			{
				LogicDataObj cmdData ,objName;
				
				GET_VAR_FROM_STREAM(stack, p, objName);
				GET_VAR_FROM_STREAM(stack, p, cmdData);
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				
				if(!objName.GetText() || !cmdData.GetText()) {
					m_sys_errno = LOGIC_ERR_PARSE_STRING;
					break;
				}
				objAim = getLogicObjectMgr(objName.GetText());

				if (objAim) {
					m_registorFlag = objAim->opOperate(cmdData.GetText(), tmpIndexVal, tmpInputVal);
					if (!m_registorFlag) {
						m_sys_errno = objAim->getError();
					}
					else {
						m_registerVal = tmpInputVal;
					}
				}
				else {
					m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
				}

				break;
			}
				
			case  E_OP_GET_OTHER_OBJECT:
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				if (m_owner){
					m_registorFlag = m_owner->getOtherObject(tmpIndexVal.GetText(),m_registerVal);
					if (!m_registorFlag){
						m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
					}
				}
 				else {
 					m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
 				}
 				break;
				
			case E_OP_EVENT_REMOVE:
			case E_OP_EVENT_INSTALL:
			{
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);

				if (tmpIndexVal.GetInt() && tmpInputVal.GetText())	{
					if (opCmd == E_OP_EVENT_REMOVE)	{
						_removeHandler(tmpIndexVal.GetInt(), tmpInputVal.GetText());
					}
					else {
						_installHandler(tmpIndexVal.GetInt(), tmpInputVal.GetText());
					}
				}
				m_registorFlag = true;
			}
				break;

			case E_OP_SET_COUNT_REG:

				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registerCount = tmpInputVal.GetInt();
				m_registorFlag = true;
				break;
			
			case E_OP_SET_LOOP_INDEX:

				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registorFlag =true;
				m_curStack->loopIndex = tmpInputVal.GetInt();
				break;

			case E_OP_PUSH_LOOP_INDEX:
				m_curStack->loopIndexStack.push_back(m_curStack->loopIndex);
				m_curStack->loopCountStack.push_back(m_registerCount);
				m_registorFlag = true;
				break;

			case E_OP_POP_LOOP_INDEX:
				if (m_curStack->loopIndexStack.size() > 0){
					m_curStack->loopIndex = m_curStack->loopIndexStack[m_curStack->loopIndexStack.size() - 1];
					m_curStack->loopIndexStack.pop_back();
					m_registorFlag = true;
				}
				else {
					nd_logerror("%s() loop index stack over flow\n", curFuncName());
					m_registorFlag = false;
					break;
				}

				if (m_curStack->loopCountStack.size() > 0){
					m_registerCount = m_curStack->loopCountStack[m_curStack->loopCountStack.size() - 1];
					m_curStack->loopCountStack.pop_back();
					m_registorFlag = true;
				}
				else {
					nd_logerror("%s() loop count register stack over flow\n", curFuncName());
					m_registorFlag = false;
				}
				break;

// 			case E_OP_GET_ARRAY_SIZE:
// 
// 				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
// 				m_registorFlag = true;
// 				m_registerVal.InitSet(tmpInputVal.GetArraySize());
// 				break;

			case E_OP_GET_ARR_TYPE:
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registorFlag = true;
				if (tmpInputVal.GetDataType() == OT_ARRAY)	{
					m_registerVal.InitSet(LogicDataObj::TypeToName(tmpInputVal.GetArrayType()));
				}
				else {
					m_registerVal.InitSet(LogicDataObj::TypeToName(OT_AUTO));
				}
				break;

			case E_OP_SUB_ELEMENTS_NUM:
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registorFlag = true;
				if (tmpInputVal.GetDataType() == OT_USER_DEFINED) {
					const LogicUserDefStruct *pData = tmpInputVal.getUserDef();
					if (pData) {
						m_registerVal.InitSet((int)pData->count());
					}
					else {
						m_registerVal.InitSet((int)1);
					}
				}
				else {
					m_registerVal.InitSet(tmpInputVal.GetArraySize());
				}
				break;
			case E_OP_GET_SUB_ELEMENT:
			{
				LogicDataObj arrayObj;
				GET_VAR_FROM_STREAM(stack, p, arrayObj);
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				index = tmpIndexVal.GetInt();
				if (arrayObj.GetDataType() == OT_USER_DEFINED) {
					LogicUserDefStruct *pData = (LogicUserDefStruct *)arrayObj.getUserDef();
					if (pData &&pData->ref(index)) {
						m_registerVal = *pData->ref(index);
					}
					else {
						m_registerVal = LogicDataObj();
					}
					
				}
				else {
					if (index >= arrayObj.GetArraySize()) {
						m_sys_errno = NDERR_LIMITED;
						m_registorFlag = false;
						break;
					}
					m_registerVal = arrayObj.GetArray(index);
				}
				m_registorFlag = true;
				break;
			}
			case E_OP_GET_SUB_JSON :
			{
				LogicDataObj arrayObj;
				GET_VAR_FROM_STREAM(stack, p, arrayObj);
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				index = tmpIndexVal.GetInt();
				if (arrayObj.GetDataType() == OT_USER_DEFINED) {
					LogicUserDefStruct mydata;
					LogicUserDefStruct *pData = (LogicUserDefStruct *)arrayObj.getUserDef();
					if (pData &&pData->ref(index)) {

						mydata.push_back("name", LogicDataObj(pData->getName(index)));
						mydata.push_back("value", LogicDataObj(*pData->ref(index)));

					}
					m_registerVal.InitSet(mydata);
					m_registorFlag = true;
				}
				else {
					m_sys_errno = NDERR_PARAM_TYPE_NOT_MATCH;
					m_registorFlag = false;
				}
				break;
			}
			case E_OP_MAKE_VAR:		//  make local variant	 name + LogicDataObj-stream
				readlen = _makeVar(stack, p);
				if (readlen > 0){
					p += readlen;
					m_registorFlag = true;
				}
				else {
					m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
				}
				break ;
			case E_OP_GLOBAL_VAR:		//  make local variant	 name + LogicDataObj-stream
				readlen = _makeVar(stack, p,false);
				if (readlen > 0){
					p += readlen;
					m_registorFlag = true;
				}
				else {
					m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
				}
				break;

			case E_OP_MAKE_CONST_VAR:		//  make local variant	 name + LogicDataObj-stream
				readlen = _makeVar(stack, p,true,true);
				if (readlen > 0) {
					p += readlen;
					m_registorFlag = true;
				}
				else {
					m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
				}
				break;
			case E_OP_MAKE_CONST_GLOBAL_VAR:		//  make local variant	 name + LogicDataObj-stream
				readlen = _makeVar(stack, p, false,true);
				if (readlen > 0) {
					p += readlen;
					m_registorFlag = true;
				}
				else {
					m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
				}
				break;

			case E_OP_MATH_OPERATE:
				p = lp_read_stream(p, opAim, m_cmdByteOrder);
				
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registorFlag = _mathOperate((eMathOperate)opAim,tmpIndexVal,tmpInputVal) ;
				break ;
			case E_OP_BITS_OPERATE: // & | ^ ~ << >>
				p = lp_read_stream(p, opAim, m_cmdByteOrder);
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registorFlag = _bitOperate((eBitOperate)opAim, tmpIndexVal, tmpInputVal);

				break;
			case E_OP_ASSIGNIN:				
				GET_TEXT_FROM_STREAM(name, sizeof(name), p);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registorFlag = _varAssignin(stack,name, tmpInputVal );
				break ;
			case E_OP_SET_VAR_VALUE:
			{
				LogicDataObj leftValue;
				GET_VAR_FROM_STREAM(stack, p, leftValue);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				leftValue.Assignin(tmpInputVal);
				m_registorFlag = true;

				break;
			}
			case E_OP_TEXT_2_DATATYPE:
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				if (tmpInputVal.GetDataType() != OT_STRING) {
					m_sys_errno = NDERR_PARAM_TYPE_NOT_MATCH;
					break;
				}
				m_registorFlag = m_registerVal.InitTypeFromTxt(tmpInputVal.GetText(), (DBL_ELEMENT_TYPE)tmpIndexVal.GetInt());
				if (!m_registorFlag) {
					m_sys_errno = LOGIC_ERR_PARSE_STRING;
				}
				break;
			case E_OP_DATATYPE_TRANSFER:
				name[0] = 0;
				GET_TEXT_FROM_STREAM(name, sizeof(name), p);
				p = lp_read_stream(p, opAim, m_cmdByteOrder);
				if (name[0]) {
					LogicDataObj*pData = _getLocalVar(stack, name);
					if (pData) {
						m_registorFlag = pData->TransferType((DBL_ELEMENT_TYPE)opAim);
						if (m_registorFlag && pData != &(m_registerVal)) {
							m_registerVal = *pData;
						}

					}
				}
				if (!m_registorFlag)	{
					nd_logerror("%s() datatype transfer error : read type or name from stream error\n", curFuncName());
					m_sys_errno = LOGIC_ERR_VARIANT_NOT_EXIST;
				}
				break;
			case E_OP_MAKE_USER_DATA:
				BEGIN_GET_FUNC_ARGS(stack, p);
				m_registorFlag =  _buildUserDefData(stack,args);
				END_GET_FUNC_ARGS();
				
				break;
			case E_OP_CREATE_TYPE:
				BEGIN_GET_FUNC_ARGS(stack, p);
				m_registorFlag = _CreateUserDefType(stack, args);
				END_GET_FUNC_ARGS();
				break;

			case E_OP_TYPENAME:
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registerVal.InitSet(tmpInputVal.getTypeName());
				m_registorFlag = true;
				break;

			case E_OP_COMP:	 		//	比较 eOperatorDest + eParserCompare  + operator_value_t	
				p = lp_read_stream(p, opAim, m_cmdByteOrder);

				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);

				m_registorCtrl = _opCmp(tmpIndexVal, tmpInputVal, (eParserCompare)opAim);
				m_registorFlag = true;

				break ;
			case E_OP_TEST_LAST_VAL:
				if (m_registerVal.CheckValid())	{
					m_registorCtrl = m_registerVal.TestValueIsValid();
				}
				else {
					m_registorCtrl = false;
				}
				m_registorFlag = true;
				break;

			case E_OP_TEST_VARIANT:
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				if (tmpInputVal.CheckValid()) {
					m_registorCtrl = tmpInputVal.TestValueIsValid();
				}
				else {
					m_registorCtrl = false;
				}
				m_registorFlag = true;
				break;
			
			case E_OP_TEST_VAR_INIT:
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registorCtrl = tmpInputVal.CheckValid();
				m_registorFlag = true;
				break;

			case E_OP_TEST_BOOL:
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				if (tmpInputVal.CheckValid()) {
					m_registorCtrl = tmpInputVal.GetBool();
				}
				else {
					m_registorCtrl = false;
				}
				m_registorFlag = true;

				break;
			case E_OP_CALC:	 		//	公式计算 NDUINT32 + BUF
			{
				float result = 0;
				GET_VAR_FROM_STREAM(stack, p, tmpIndexVal);
				m_registorFlag = (0 == _runFormula(tmpIndexVal.GetText(), &result));
				if (m_registorFlag) {
					m_registerVal.InitSet(result);
				}
				else {
					m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;
				}
			}
				break ;							
			
			case E_OP_GET_TIME:
			{
				time_t now = app_inst_time(NULL);
				m_registerVal.InitSet(now);
				m_registorFlag = true;
			}
				break;
			case E_OP_GET_LAST_CHANGED_TM:
			{
				m_registorFlag = m_root->getModuleChangedTime(NULL, m_registerVal);
				if (!m_registorFlag) {
					m_sys_errno = LOGIC_ERR_AIM_OBJECT_NOT_FOUND;
				}
			}
				break;
			
			case E_OP_ADD_TIMER:
				BEGIN_GET_FUNC_ARGS(stack, p);
				m_registorFlag = _addTimer(args);
				END_GET_FUNC_ARGS();
				break;

			case E_OP_DEL_TIMER:
				p = lp_read_stream(p, opAim, m_cmdByteOrder);
				GET_VAR_FROM_STREAM(stack, p, tmpInputVal);
				m_registorFlag = _delTimer(opAim != 0, tmpInputVal.GetText());
				break;

			case E_OP_SEND_EVENT:
				BEGIN_GET_FUNC_ARGS(stack, p);
				int eventID = args[0].GetInt();
				args.erase(args.begin());
				m_registorFlag = SendEvent(eventID, args);
				END_GET_FUNC_ARGS();
				break;
			case E_OP_WAIT_EVENT:			//  等待事件 int + operator_value_t[3]
				BEGIN_GET_FUNC_ARGS(stack, p);
				if (!inException){
					stack->cur_point = p;
					m_sys_errno = LOGIC_ERR_WOULD_BLOCK;
					int eventID = args[0].GetInt();
					args.erase(args.begin());
					m_registorFlag = waitEvent(stack, eventID, args);
					return -1;
				}
				END_GET_FUNC_ARGS();
				break;
			case E_OP_OUT_PUT:
				BEGIN_GET_FUNC_ARGS(stack, p);
				apollo_printf(this, args, tmpInputVal);
				m_registorFlag = true;
				END_GET_FUNC_ARGS();
				break;
			case E_OP_LOG:
				BEGIN_GET_FUNC_ARGS(stack, p);
				apollo_log(this, args, tmpInputVal);
				m_registorFlag = true;
				END_GET_FUNC_ARGS();
				break;

			case E_OP_CONSOLE_INPUT:
				apollo_input_line(this, m_registerVal);
				m_registorFlag = true;
				break;
			case E_OP_TIME_CMD:
				BEGIN_GET_FUNC_ARGS(stack, p);
				args.insert(args.begin(), LogicDataObj("time_func"));
				m_registorFlag = apollo_time_func(this, args, m_registerVal);
				if (!m_registorFlag)	{
					m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;
				}
				END_GET_FUNC_ARGS();
				break;

			case E_OP_CALL_FUNC:			//  调用函数 string(function_name) + string1 + string2	
				BEGIN_GET_FUNC_ARGS(stack, p);
				stack->cur_point = p;
				m_registorFlag = _callFunction(args);
				END_GET_FUNC_ARGS();
				break;
// 			case E_OP_REMOTE_CALL:
// 				BEGIN_GET_FUNC_ARGS(stack, p);
// 				stack->cur_point = p;
// 
// 				tmpInputVal = args[0];
// 				args.erase(args.begin());
// 				m_registorFlag = _callFunction(args,tmpInputVal.GetText());
// 				END_GET_FUNC_ARGS();
// 				break;

			case E_OP_SHORT_JUMP:
			{
				int offset;
				p = lp_read_stream(p, offset, m_cmdByteOrder);
				
				SHORT_JUMP(p, offset);
				CHECK_INSTRUCTION_OVER_FLOW();
				m_registorFlag = true;
			}
				break ;
			case  E_OP_LONG_JUMP:
			{
				NDUINT32 offset;
				p = lp_read_stream(p, offset, m_cmdByteOrder);

				p = (char*)stack->cmd->buf + offset;

				CHECK_INSTRUCTION_OVER_FLOW();
				m_registorFlag = true;
				break;
			}
			case E_OP_TEST_SUCCESS_SHORT_JUMP:
			{
				int offset;
				p = lp_read_stream(p, offset, m_cmdByteOrder);

				if (m_registorCtrl == true)	{
					SHORT_JUMP(p, offset);
					CHECK_INSTRUCTION_OVER_FLOW();
				}
				m_registorFlag = true;

			}
			break;
			case E_OP_TEST_FALSE_SHORT_JUMP:
			{
				int offset;
				p = lp_read_stream(p, offset, m_cmdByteOrder);

				if (m_registorCtrl==false)	{
					SHORT_JUMP(p, offset);
					CHECK_INSTRUCTION_OVER_FLOW();
				}
				m_registorFlag = true;

			}
			break;
			case E_OP_TEST_FALSE_LONG_JUMP:
			{
				NDUINT32 offset;
				p = lp_read_stream(p, offset, m_cmdByteOrder);
				if (m_registorCtrl==false)	{
					p = (char*)stack->cmd->buf + offset;

					CHECK_INSTRUCTION_OVER_FLOW();
				}
				m_registorFlag = true;

			}
			break;

			case E_OP_TEST_COUNT_JUMP:
			{
				int offset;
				p = lp_read_stream(p, offset, m_cmdByteOrder);

				if (m_registerCount-- > 1)	{//this block only jump ahead. so count end at 1
					SHORT_JUMP(p, offset);
				}
				CHECK_INSTRUCTION_OVER_FLOW();
				m_registorFlag = true;
				++m_curStack->loopIndex;
			}
				break;

			case E_OP_TEST_COUNT:
				m_registorFlag = true;
				m_registorCtrl = m_registerCount > 0;
				break;

			case E_OP_CHECK_DEBUG:
				m_registorCtrl = scriptIsDebug;
				m_registorFlag = true;
				break;
			case E_OP_CHECK_HOST_DEBUG:
#ifdef ND_DEBUG
				m_registorCtrl = true;
#else 
				m_registorCtrl = false;
#endif
				m_registorFlag = true;
				break; 
			case E_OP_PROCESS_EXIT:
				exit(1);
				break;
			case E_OP_PROCESS_ABORT:
				abort();
				break;

			default:
				m_registorFlag = false;
				m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;
				stack->cur_point = p ;
				break ;
		}
		stack->cur_point = p ;
				
		if (m_registorFlag==false ) {
			if (m_sys_errno == NDERR_WOULD_BLOCK)	{
				return -1;
			}
			else if (m_simulate && _checkIsSkip(m_sys_errno)) {
				continue;
			}
			
			else if (!_checkIsSystemError(m_sys_errno) && m_curStack->error_continue) {
				continue;
			}
			else if (inException == true){
				//on error when exception-handle
				m_sys_errno = errorBeforeException;
				nd_logdebug("%s error : %s  run in Exception handler \n", stack->cmd->cmdname, nd_error_desc(m_sys_errno));
				return -1;
			} 
			else if (exception_addr)	{
				//jump to exception 
				p = (char*)exception_addr;
				inException = true;
				errorBeforeException = m_sys_errno;
				m_registorFlag = true;

				nd_logdebug("%s catch exception : %s \n", stack->cmd->cmdname, nd_error_desc(m_sys_errno));
			}
			else {
				nd_logerror("%s() %s script error : %s \n", curFuncName(), stack->cmd->cmdname, nd_error_desc(m_sys_errno));
				return -1;
			}
			
		}
	}
	
	//exit
	if (inException == true){
		m_sys_errno = errorBeforeException;
		return -1;
	}
	return 0;
	
}

int LogicParserEngine::_makeVar(runningStack *runstack, char *pCmdStream, bool isLocal, bool isConst )//make variant from instruction 
{
	char*p = pCmdStream;
	char name[128];
	int len = _read_string(p, name, sizeof(name));
	if (len <= 0){

		m_sys_errno = LOGIC_ERR_PARSE_STRING;
		return -1;
	}
	p += len;

	logcal_variant node;	

	len = _getValue(runstack, p, node.var);
	if (len == -1){
		m_sys_errno = LOGIC_ERR_READ_STREAM;
		return -1;
	}
	
	p += len;

	node.name = name;
	LogicData_vct *pVarMgr = NULL;
	if (isLocal){
		pVarMgr = &runstack->local_vars;
		for (LogicData_vct::iterator it = pVarMgr->begin(); it != pVarMgr->end(); it++)	{
			if (ndstricmp((char*)it->name.c_str(), (char*)name) == 0) {
				it->var = node.var;
				return (int)(p - pCmdStream);
			}
		}
	}
	else {
		if (_getGlobalVar(name))	{
			return (int)(p - pCmdStream);
		}
		pVarMgr = &m_global_vars;
	}
	

	if (isConst) {
		node.var.setConstValue();
	}
	pVarMgr->push_back(node);
	return (int)(p - pCmdStream);
}


LogicDataObj *LogicParserEngine::_getGlobalVar(const char *name)
{
	LogicData_vct::iterator it = m_global_vars.begin();
	for (; it != m_global_vars.end(); it++) {
		if (0 == ndstricmp(it->name.c_str(), name)) {
			return &(it->var);
		}
	}
	return NULL;
}



bool LogicParserEngine::_getArg(runningStack *stack, int index, LogicDataObj &outValue)
{
	if (index < stack->params.size()){
		if (!outValue.Reference(stack->params[index])){
			outValue = stack->params[index];
		}
		return true;
	}

	else if (m_simulate)	{
		LogicDataObj data;
		data.InitSet("0");
		outValue = data;
		return true;
	}

	return false;
}

bool LogicParserEngine::getVarValue(const char *varname, LogicDataObj &outValue)
{
	return _getVarValue(m_curStack, varname, outValue);
}
bool LogicParserEngine::_getVarValue(runningStack *stack, const char *varname, LogicDataObj &outValue)
{
	if (ndstr_is_naturalnumber(varname)){
		outValue.InitSet(atoi(varname));
		return true;
	}
	else if (ndstr_is_numerals(varname)){
		outValue.InitSet((float)atof(varname));
		return true;
	}
	return _getVarEx(stack, varname, outValue);
}

LogicDataObj* LogicParserEngine::_getLocalVar(runningStack *stack, const char *varname)
{
	if (0 == ndstricmp(varname, "$CurValue") || 0 == ndstricmp(varname, "$LastValue") 
		|| 0 == ndstricmp(varname, "$?")
		|| 0 == ndstricmp(varname, "$value") ){
		return &m_registerVal ;
	}
	else if (0 == ndstricmp(varname, "$index")) {
		m_loopIndex.InitSet(stack->loopIndex);
		return &m_loopIndex;
	}
	else if (*varname == '$' && (varname[1] >= '0' && varname[1] <= '9') ) {
		const char *p = varname + 1;
		int index = atoi(p);
		if (index>0 && index <stack->params.size())	{
			return &stack->params[index];
		}
		return NULL;
	}
	for (LogicData_vct::iterator it = stack->local_vars.begin(); it != stack->local_vars.end(); it++)	{
		if (ndstricmp((char*)it->name.c_str(), (char*)varname) == 0) {
			return &it->var;
		}
	}

	if (stack->funcType == NF_FUNC_CLOSURE && stack->parentStack) {
		return _getLocalVar(stack->parentStack, varname);
	}
	return NULL;
}


bool LogicParserEngine::_getVarEx(runningStack *stack, const char *inputVarName, LogicDataObj &outVar, const LogicDataObj *appendValue )
{
	char name[128];
	char subName[128];
	const char *p = inputVarName;
	
	p = ndstr_parse_word_n(p, name,sizeof(name));

	LogicDataObj* pdata = _getLocalVar(stack, name);
	if (!pdata)	{

		pdata = _getGlobalVar(name);
		if (!pdata)	{
			nd_logerror("%s() var %s not exist\n", curFuncName(), name);
			return false;
		}

	}
	//LogicDataObj value = *pdata;
	LogicDataObj subVar;
	
	while (p && *p){		
		if (*p == _ARRAR_BEGIN_MARK) {
			p = ndstr_fetch_array_index(p, subName, sizeof(subName));
			if (!p)	{
				nd_logerror("%s() get %s var error\n", curFuncName(), inputVarName);
				return false;
			}
			LogicDataObj subIndex;
			if (_getVarValue(stack, subName, subIndex)) {
				int index = subIndex.GetInt();
				
				if (-1 == index) {
					
					if (!appendValue) {
						return false;
					}

					if (!pdata->pushArray( *appendValue )) {
						return false;
					}
					index = pdata->GetArraySize() - 1;

				}
				
				if (!subVar.ReferenceArray(*pdata, index)) {
					return false;
				}
			}
			else {
				return false;
			}
			pdata = &subVar;
		}
		else if (*p == '.') {
			++p;
			p = ndstr_parse_word_n(p, subName,sizeof(subName));

			LogicUserDefStruct *pUserDef =(LogicUserDefStruct *) pdata->getUserDef();
			if (!pUserDef) {
				return false;
			}
			pdata = pUserDef->ref(subName);
			if (!pdata) {
				
				if (p && ndstrchr(p, '.')) {
					nd_logerror("%s() can not found member %s\n", curFuncName(), subName);
					return false;
				}
				else {
					if (!appendValue) {
						return false;
					}

					pUserDef->set(subName, *appendValue);
					pdata = pUserDef->ref(subName);
					if (!pdata) {
						return false;
					}
				}
			}

		}
		else {
			break;
		}		
	}


	if (!outVar.Reference(*pdata)) {
		outVar = *pdata;
	}
	return true;
}


bool LogicParserEngine::_chdir(const char *curDir)
{
	if (m_curStack->workingPath.empty()) {
		m_curStack->workingPath = nd_getcwd();
	}
	return 0==nd_chdir(curDir);
}

bool LogicParserEngine::_rmfile(const char *filename)
{
	if (nd_existfile(filename)) {
		return 0 == nd_rmfile(filename);
	}
	return 0 == nd_rmdir(filename);
}

bool LogicParserEngine::_mkdir(const char *curDir)
{
	return 0 == nd_mkdir(curDir);
}


int LogicParserEngine::_getValue(runningStack *stack, char *pCmdStream, LogicDataObj &outValue)
{
	NDUINT16 type;
	char*p = lp_read_stream(pCmdStream, type, m_cmdByteOrder);

	if (OT_VARIABLE == (DBL_ELEMENT_TYPE)type){
		//get data from local vars 
		char name[1024];
		int len = _read_string(p, name, sizeof(name));
		if (len == -1){
			m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;
			nd_logerror("%s() read var name error \n", curFuncName());
			return -1;
		}
		p += len;
		if (!_getVarEx(stack, name, outValue))	{
			nd_logerror("%s() var %s not found\n", curFuncName(), name);
			m_sys_errno = LOGIC_ERR_VARIANT_NOT_EXIST;
		}
		
		return (int)(p - pCmdStream);
		
	}
	else if (OT_PARAM == (DBL_ELEMENT_TYPE)type) {
		//int index = *((*(int**)&p)++);
		NDUINT32 index;
		p = lp_read_stream(p, index, m_cmdByteOrder);

		if (_getArg(stack, index, outValue) ) {
			return (int)(p - pCmdStream);
		}
		else {
			m_sys_errno = LOGIC_ERR_PARAM_NOT_EXIST;
			nd_logerror("%s() get param(%d) error max_index=%lu\n", curFuncName(), index, stack->params.size());
			return -1;
		}
	}
	else if (OT_LAST_RET == (DBL_ELEMENT_TYPE)type) {
		outValue = m_registerVal;
		return (int)(p - pCmdStream);
	}
	else if (OT_USER_DEFINED_ARRAY == (DBL_ELEMENT_TYPE)type) {
		char name[1024];
		int len = _read_string(p, name, sizeof(name));
		if (len == -1){
			m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;
			nd_logerror("%s() read var name error \n", curFuncName());
			return -1;
		}
		p += len;
		if (!_getValueFromArray(stack,name, outValue) )	{
			m_sys_errno = LOGIC_ERR_VARIANT_NOT_EXIST;
			nd_logerror("%s() make array %s error\n", curFuncName(), name);
			return -1;
		}
		return (int)(p - pCmdStream);
	}
	else if (OT_USER_DEFINED == (DBL_ELEMENT_TYPE)type) {
		char name[1024];
		int len = _read_string(p, name, sizeof(name));
		if (len == -1){
			m_sys_errno = LOGIC_ERR_PARSE_STRING;
			nd_logerror("%s() read var name error \n", curFuncName());
			return -1;
		}
		if (-1==_getValueFromUserDef(name, outValue)) {
			nd_logerror("%s() read user define var %s error \n", curFuncName(), name);
			m_sys_errno = LOGIC_ERR_VARIANT_NOT_EXIST;
			//return -1;
		}
		p += len;
		return (int)(p - pCmdStream);
	}
	else {
		size_t size = stack->cmd->buf + stack->cmd->size - pCmdStream;
		int ret = outValue.ReadStream(pCmdStream,size, m_cmdByteOrder);
		if (ret >= 0) {
			outValue.ConvertEncode(m_curFuncEncodeType, ND_ENCODE_TYPE);
		}
		return ret;
	}

	//m_sys_errno = (int)LOGIC_ERR_INPUT_INSTRUCT;
	//return -1;
}


// get user define from text , format : userDataType(arg,....) ;
int LogicParserEngine::_getValueFromUserDef(const char *inputName, LogicDataObj &outValue)
{
	char name[128];
	const char *p = ndstr_nstr_end(inputName, name, '(', sizeof(name));
	
	ndstr_to_little(name);
	UserDefData_map_t::iterator it = m_useDefType.find(name);
	if (it==m_useDefType.end()) {
		
		return -1;
	}
#define _FETCH_ARG_TXT(_inText, _outTxt, _startCh,_endCh) \
	if(*_inText == _startCh) { \
		++_inText ;				\
		_inText = ndstr_nstr_end(_inText, _outTxt,_endCh, sizeof(_outTxt)); \
		if(_inText && *_inText==_endCh) { \
			++_inText ;	\
		}				\
	}
	
	//int ret = 0;
	LogicUserDefStruct myType = *(it->second);
	if (p && *p=='(' ) 	{
		// read init function 
		++p;
		char *pend =(char*) ndstrchr(p, ')');
		if (!pend || *pend != ')'){
			return -1;
		}
		*pend = 0;

		int index = 0 ;

		do {
			name[0] = 0;
			_FETCH_ARG_TXT(p, name, '[', ']')
			else _FETCH_ARG_TXT(p, name, '(', ')')
			else _FETCH_ARG_TXT(p, name, '\'', '\'')
			else _FETCH_ARG_TXT(p, name, '\"', '\"')
			else {
				p = ndstr_nstr_end(p, name, ',', sizeof(name));
			}
			if (name[0] )	{
				LogicDataObj *pMember = myType.ref(index);
				if (pMember){
					pMember->InitFromTxt(name);
				}
				else {
					break;
				}
			}
			++index;
			if (p && *p ==','){
				++p;
			}
		} while (p && *p);

		*pend = ')';		
	}

	outValue.InitSet(myType);

	return 0;
}

bool LogicParserEngine::_getValueFromArray(runningStack *stack, const char *inputText, LogicDataObj &outValue)
{
	char name[128];
	const char *p = ndstr_parse_word_n(inputText, name,sizeof(name));
	const LogicUserDefStruct*pUserType = NULL;

	DBL_ELEMENT_TYPE arrayType = LogicDataObj::getTypeFromName(name);
	if (arrayType == OT_USER_DEFINED){
		ndstr_to_little(name);
		pUserType = getUserDataType(name);
		if (!pUserType)	{
			nd_logerror("%s() can not found user define type %s\n", curFuncName(), name);
			return false;
		}
	}


	int index = 1;
	p = ndstr_first_valid(p);
	if (*p == _ARRAR_BEGIN_MARK) {
		p = ndstr_fetch_array_index(p, name, sizeof(name));
		if (!p)	{
			return false;
		}
		LogicDataObj subIndex;
		index = -1;
		if (_getVarValue(stack, name, subIndex)) {
			index = subIndex.GetInt();
		}
		if (-1 == index)	{
			nd_logerror("%s() get array size error %s\n", curFuncName(), name);
			return false;
		}
	}

	outValue.InitReservedArray(index, arrayType);
	if (pUserType){
		for (int i = 0; i < index; i++){
			outValue.SetArray(*pUserType, i);
		}
	}

	return true;
}

int LogicParserEngine::_read_string(char *pCmdStream, char *outbuf, size_t size)
{
	NDUINT16 size16 = 0;
	char *p = pCmdStream ;
	//size16 = *((*(NDUINT16**)&p)++); 
	p = lp_read_stream(p, size16, m_cmdByteOrder);
	if (size16 >= size){
		//input data is too long
		m_sys_errno = LOGIC_ERR_PARSE_STRING;
		return -1;
	}
	memcpy(outbuf, p, size16);
	outbuf[size16] = 0;
	//ndstrncpy(outbuf, p, size);
	//nd_assert(size16 == (NDUINT16)ndstrlen(p));
	p += size16;
	//nd_assert(*p == 0);
	//++p;

	return (int)(p - pCmdStream);

}


//外表事件驱动
void LogicParserEngine::onEvent(int event, parse_arg_list_t &params)
{
	//continue function that waiting event
	if (m_events.size()  > 0){
		event_list_t runningList;

		for (event_list_t::iterator it = m_events.begin(); it != m_events.end();) {
			if (checkEventOk(event, params, it->eventid, it->event_params)) {
				runningList.push_back(*it);
				it = m_events.erase(it);
			}
			else {
				++it;
			}
		}

		for (event_list_t::iterator it = runningList.begin(); it != runningList.end(); ++it) {
			_continueEvent(&(*it));
		}
	}

	//call local event handler
	if (m_hanlers.size() == 0){
		return;
	}
	
	for (event_handler_list_t::iterator it = m_hanlers.begin(); it != m_hanlers.end();++it) {
		if (it->event_id !=event){
			continue;
		}
		parse_arg_list_t mylist = params;
		mylist.insert(mylist.begin(), LogicDataObj(it->name.c_str()));
		LogicDataObj result;
		runScript(it->name.c_str(), mylist,result);
		//_callFunction(mylist);
	}
}

bool LogicParserEngine::SendEvent(int event_id, parse_arg_list_t &args)
{
	SendEventNode node;
	node.event_id = event_id;
	node.args = args;
		 
	m_needHandleEvents.push_back(node);
	return true;
}

void LogicParserEngine::update(ndtime_t interval)
{
	if (m_timer.size()==0){
		return;
	}
	timer_list_t::iterator it;
	m_runInTimer = true;

	for (it = m_timer.begin(); it != m_timer.end(); ){
		logicParserTimer *ptimer = &*it;
		ptimer->left_val -= interval;
		if (ptimer->left_val <= 0)	{
			//parse_arg_list_t args;
			//args.push_back(LogicDataObj(ptimer->params[0].GetText()));
			ptimer->left_val = ptimer->interval;
			_callFunction(ptimer->params);
			if (ptimer->type){
				it = m_timer.erase(it);
				continue; 
			}
		}
		++it;		
	}
	m_runInTimer = false;
	if (m_delTimerQueue.size() >0) {
		for (size_t i = 0; i < m_delTimerQueue.size(); i++)	{
			_delTimer(m_delTimerQueue[i].isGlobal, m_delTimerQueue[i].name.c_str());
		}
		m_delTimerQueue.clear();
	}
		

	parse_arg_list_t args;
	onEvent(0, args);		//event 0 is update event
}

bool LogicParserEngine::waitEvent(runningStack *stack, int event, parse_arg_list_t &params)
{
	StackWaitEventNode node ;
	
	m_curStack->curRegisterCtrl = m_registorCtrl;
	m_curStack->curRegisterCount = m_registerCount;

	node.eventid = event ;	
	node.event_params = params;	
	node.preRegisterVal = m_registerVal;

	node.stacks = m_runningStacks;
	m_events.push_back(node) ;
	setErrno(LOGIC_ERR_WOULD_BLOCK);

	event_list_t::reverse_iterator  it = m_events.rbegin();
	nd_assert(it != m_events.rend());

	//m_curStack = &(it->stack);

	return true ;
}

bool LogicParserEngine::_installHandler(int ev, const char *funcName)
{
	event_handler_list_t::iterator it;
	for (it = m_hanlers.begin(); it != m_hanlers.end(); ++it){
		if (it->event_id == ev && 0 == ndstricmp(funcName, it->name.c_str()))	{
			return true;
		}
	}

	m_hanlers.push_back(eventHandler(ev, funcName));
	return true;
}

bool LogicParserEngine::_removeHandler(int ev, const char *funcname)
{
	event_handler_list_t::iterator it;
	for (it =m_hanlers.begin(); it!=m_hanlers.end(); ++it){
		if (it->event_id == ev && 0==ndstricmp(funcname, it->name.c_str()))	{
			m_hanlers.erase(it);
			break;
		}
	}
	return true;
}


bool LogicParserEngine::_CreateUserDefType(runningStack *runstack, parse_arg_list_t &args)
{
	LogicUserDefStruct *userDefData = new LogicUserDefStruct();
	for (int i = 1; i < args.size(); ++i) {
		const char *name = args[i].GetText();
		++i;
		if (name) {
			userDefData->push_back(name, args[i]);
		}
	}

	const char *pText = args[0].GetText();
	char name[128];
	ndstrncpy(name, pText, sizeof(name));
	ndstr_to_little(name);

	m_useDefType[name] = userDefData;
	return true;

}
template<class T>
bool bitOperateExe(eBitOperate op, T lv, T rv, LogicDataObj &result)
{
	T val;
	switch (op)
	{
	case E_BIT_AND:
		val = lv & rv;
		break;
	case E_BIT_OR:
		val = lv | rv;
		break;
	case E_BIT_XOR:
		val = lv ^ rv;
		break;
	case E_BIT_NOT:
		val = ~lv ;
		break;
	case E_BIT_LEFT_MOVE:
		val = lv << rv;
		break;
	case E_BIT_RIGHT_MOVE:
		val = lv >> rv;
		break;
	default:
		return false;
		break;
	}

	result.InitSet(val);

	return true;
}

bool LogicParserEngine::_bitOperate(eBitOperate op, const LogicDataObj &var1, const LogicDataObj &var2)
{

	DBL_ELEMENT_TYPE  dtype = var1.GetDataType();
	NDUINT64 leftVal = var1.GetInt64();
	NDUINT64 rightVal = var2.GetInt64();
	if (dtype == OT_INT ) { 
		return  bitOperateExe(op, (int)leftVal, (int)rightVal, m_registerVal);
	}

	else if (dtype == OT_INT8) {
		return  bitOperateExe(op, (NDUINT8)leftVal, (NDUINT8)rightVal, m_registerVal);

	}
	else if (dtype == OT_INT16) {
		return  bitOperateExe(op, (NDUINT16)leftVal, (NDUINT16)rightVal, m_registerVal);

	}
	else if (dtype == OT_INT64) {
		return  bitOperateExe(op, (NDUINT64)leftVal, (NDUINT64)rightVal, m_registerVal);

	}
	else {
		LogicDataObj srcVal = var1;
		if (srcVal.BitOperateBin(op, (NDUINT8)var2.GetInt())){
			m_registerVal = srcVal;
			return true;
		}
		return false;
	}
	
}

bool LogicParserEngine::_mathOperate(eMathOperate op,const LogicDataObj &var1, const LogicDataObj &var2)
{
	if (var1.GetDataType() == OT_FLOAT || var1.GetDataType() == OT_INT ||
		var1.GetDataType() == OT_INT8 || var1.GetDataType() == OT_INT16 || var1.GetDataType() == OT_INT64) {

		double f1 = var1.GetFloat();
		double f2 = var2.GetFloat();
		double val = 0;

		switch (op) {
		case E_MATH_ADD:
			val = f1 + f2;
			break;
		case E_MATH_SUB:
			val = f1 - f2;
			break;
		case E_MATH_MUL:
			val = f1 * f2;
			break;
		case E_MATH_DIV:
			if (f2 == 0)	{
				nd_logerror("%s() div param is zero\n", curFuncName());
				m_sys_errno = NDERR_SCRIPT_INSTRUCT;
				return false;
			}
			val = f1 / f2;
			break;
		case E_MATH_SQRT:
		{
			val = sqrt(f1);
			break;
		}
		case E_MATH_MAX:
		{
			val = NDMAX(f1, f2);
			break;
		}
		case E_MATH_MIN:
		{
			val = NDMIN(f1, f2);
			break;
		}
		case E_MATH_RAND:
		{
			val = (float)logic_rand(var1.GetInt(), var2.GetInt());
			break;

		}
		default:
			m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;
			return false;
		}
		m_registerVal.InitSet((float)val);
	}
	else {
		
		switch (op) {
		case E_MATH_ADD:
			m_registerVal = var1 + var2;
			break;
		case E_MATH_SUB:
			m_registerVal = var1 - var2;
			break;
		case E_MATH_MUL:
			m_registerVal = var1 * var2;
			break;
		case E_MATH_DIV:
			m_registerVal = var1 / var2;
			break;
		default:
			m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;
			return false;
		}
	}

	return  true ;
}


bool LogicParserEngine::_varAssignin(runningStack *stack,const char *name, LogicDataObj &inputVal)
{
	LogicDataObj leftValue;
	if (_getVarEx(stack, name, leftValue, &inputVal)) {
		leftValue.Assignin(inputVal);
		return true;
	}
	return false;

#if 0
	char varname[128];
	char subName[128];
	const char *p = name;

	p = ndstr_parse_word_n(p, varname,sizeof(varname));

	LogicDataObj* pdata = _getLocalVar(stack, varname);
	if (!pdata)	{
		LogicEngineRoot *root = LogicEngineRoot::get_Instant();
		pdata = root->getVar(varname);
		if (!pdata)	{
			nd_logerror("var %s not exist\n", name);
			return false;
		}

	}
	//LogicDataObj value = *pdata;
	LogicUserDefStruct *pUserStruct =NULL;

	if (p && (*p == '.'  || *p == _ARRAR_BEGIN_MARK) ) {
		if (pdata->GetDataType() == OT_USER_DEFINED) {
			pUserStruct = (LogicUserDefStruct *)pdata->getUserDef();
			pdata = NULL;
		}
		while (p && *p) {
			if (*p == _ARRAR_BEGIN_MARK) {
				if (!pdata) {
					nd_logerror(" %s is not array\n", name);
					return false;
				}
				p = ndstr_fetch_array_index(p, subName, sizeof(subName));
				if (!p) {
					nd_logerror("get %s var error\n", name);
					return false;
				}
				LogicDataObj subIndex;
				if (_getVarValue(stack, subName, subIndex)) {
					int index = subIndex.GetInt();
					
					if (-1 == index) {
						pdata->pushArray(inputVal);
					}
					else {
						pdata->SetArray(inputVal, index);
					}
					return true;

// 					if (OT_USER_DEFINED == pdata->GetArrayType()) {
// 						pUserStruct = (LogicUserDefStruct *)pdata->GetarrayUser(index);
// 						pdata = NULL;
// 					}
// 					else {
// 						
// 
// 					}
				}
				else {
					return false;
				}
			}
			else if (*p == '.') {
				if (!pUserStruct) {
					nd_logerror(" %s is not json data\n", name);
					return false;
				}
				++p;
				p = ndstr_parse_word_n(p, subName,sizeof(subName));
				pdata = pUserStruct->ref(subName);
				if (!pdata) {
					if (p && ndstrchr(p, '.')) {
						nd_logerror("can not found member %s\n", subName);
						return false;
					}
					else {
						pUserStruct->set(subName, inputVal);
						return true;
					}
				}

				if (pdata->GetDataType() == OT_USER_DEFINED) {
					pUserStruct = (LogicUserDefStruct *)pdata->getUserDef();
					pdata = NULL;
				}
			}
			else {
				break;
			}
		}
	}

	if (pdata ){
		//*pdata = inputVal;
		pdata->Assignin(inputVal);
	}
	else if (pUserStruct && inputVal.GetDataType() == OT_USER_DEFINED && inputVal.getUserDef()) {
		*pUserStruct = *inputVal.getUserDef();
	}
	else {
		return false;
	}
	return true;
#endif
}

bool LogicParserEngine::_buildUserDefData(runningStack *runstack,parse_arg_list_t &args)
{	
	logcal_variant node;
	node.name = args[0].GetText() ;
	
	LogicUserDefStruct userDefData ;
	for (int i=1; i<args.size(); ++i) {
		const char *name = args[i].GetText() ;
		++i ;
		if (name) {
			userDefData.push_back(name, args[i]) ;
		}
	}
	
	node.var.InitSet(userDefData) ;
	
	runstack->local_vars.push_back(node);
	//m_registerVal = node.var ; //this is common create var do not change current value
	return true ;
	
}

bool LogicParserEngine::_addTimer(parse_arg_list_t &args)
{
	if (args.size() <5)	{
		nd_logerror("%s() timer error need 4 arguments\n", curFuncName());
		m_sys_errno = LOGIC_ERR_INPUT_INSTRUCT;
		return false;
	}
	parse_arg_list_t params;
	for (size_t i = 4; i < args.size(); i++) {
		params.push_back(args[i]);
	}
	if (!args[0].GetBool() ) {

		return _addTimer(args[1].GetInt(), (int)(args[2].GetFloat() * 1000), args[3].GetText(), params);
	}
	else {
		return m_root->getGlobalParser()._addTimer(args[1].GetInt(), (int)(args[2].GetFloat() * 1000), args[3].GetText(), params);
	}


}

bool LogicParserEngine::_addTimer(int type, int interval, const char *timername, parse_arg_list_t &params)
{
	logicParserTimer lptime;
	lptime.type = type;
	lptime.interval = interval;
	lptime.left_val = interval;
	lptime.name = timername;
	//lptime.functionName = funcName;
	lptime.params = params;

	//lptime.params.insert(lptime.params.begin(), LogicDataObj(funcName));

	m_timer.push_back(lptime);
	return true;
}
bool LogicParserEngine::_delTimer(bool isGlobal , const char *timername)
{
	if (m_runInTimer)	{
		m_delTimerQueue.push_back(timerTypeName(isGlobal, timername));
		return true;
	}
	else {
		if (isGlobal){
			return m_root->getGlobalParser()._delTimer(false, timername);
		}
		timer_list_t::iterator it;
		for (it = m_timer.begin(); it != m_timer.end(); ++it){
			if (0 == ndstricmp(timername, it->name.c_str()))	{
				m_timer.erase(it);
				return true;
			}
		}
		return true;
	}
}



bool LogicParserEngine::handleScriptGenEvent()
{
	if (m_needHandleEvents.size()==0){
		return true;
	}
	send_event_list_t tmpList = m_needHandleEvents;

	m_needHandleEvents.clear();

	send_event_list_t::iterator it = tmpList.begin();
	for (; it != tmpList.end(); ++it){
		eventNtf(it->event_id, it->args);
	}
	return true;
}


void LogicParserEngine::_pushSKipError(int err)
{
	if (err == NDERR_WOULD_BLOCK) {
		return;
	}
	for (size_t i = 0; i < LOGIC_ERR_NUMBER; i++){
		if (m_curStack->skipErrors[i] == 0 ) {
			m_curStack->skipErrors[i] = err;
			break;
		}
	}


	for (int i = 0; i < LOGIC_ERR_NUMBER; i++)	{
		if (m_skipErrors[i] == 0) {
			m_skipErrors[i] = err;
			break;
		}
	}
}

bool LogicParserEngine::_checkIsSkip(int err)
{
	for (size_t i = 0; i < LOGIC_ERR_NUMBER; i++){
		if (m_skipErrors[i]==0)	{
			break;
		}
		if (m_skipErrors[i] == err) {
			return true;
		}
	}
	return false;
}

bool LogicParserEngine::_callFunction(parse_arg_list_t &func_args,const char *moduleName)
{
	bool ret = _call(func_args, m_registerVal,moduleName) ;
	if (m_simulate && !ret )	{
		if (_checkIsSkip( getErrno())) {
			LogicDataObj val1;
			char buf[1024];
			ndsnprintf(buf, sizeof(buf), "function %s return val", func_args[0].GetText());
			//apollo_logic_out_put("call function :");
			//apollo_printf(this, func_args, val1);
			//apollo_logic_out_put("success \n");
			m_registerVal.InitSet((const char*)buf);
			//setErrno(NDERR_SUCCESS);
			ret = true;
		}	
	}
	return ret;
}

bool LogicParserEngine::_call(parse_arg_list_t &args, LogicDataObj &result, const char *moduleName)
{
	if (args.size() < 1){
		setErrno(LOGIC_ERR_PARAM_NUMBER_ZERO);
		nd_logerror("%s() function-call args-list is too few\n", curFuncName());
		return false;
	}
	DBL_ELEMENT_TYPE type = args[0].GetDataType();
	if (type != OT_STRING){
		setErrno(LOGIC_ERR_FUNCTION_NAME);
		nd_logerror("%s() function name type must be string\n", curFuncName());
		return false;
	}
	const char *funcName = args[0].GetText();
	if (!funcName || !funcName[0]){
		setErrno(LOGIC_ERR_FUNCTION_NAME);
		nd_logerror("%s() function name is NULL\n", curFuncName());
		return false;
	}
	
	const func_cpp_info*  cppfunc = m_root->getCPPFunc(funcName) ;
	if (cppfunc) {
		bool ret = false;
		if (!cppfunc->isAnsiC) {
			ret = cppfunc->func(this, args, result);
		}
		else {
			ret = _callAnsiC((void*)cppfunc->func, args, result);
		}
		if (m_simulate && !ret)	{
			setErrno(LOGIC_ERR_FUNCTION_NOT_FOUND);
		}
		return ret;
	}
	const char *pModule = moduleName ? moduleName: getCurMoudle();
	const scriptCmdBuf* pScript = m_root->getScript(funcName, pModule ,&pModule) ;
	if (pScript) {
		bool ret = runFunction(pModule, pScript, args, result);
		m_bLastIsCalled = true;
		return ret;
	}
	setErrno(LOGIC_ERR_FUNCTION_NOT_FOUND);
	if (ndstricmp_n(funcName,"CPP.",4)){
		nd_logerror("%s() script FUNCTION \"%s()\" NOT FOUND!!\n", curFuncName(), funcName);
	}
	
	return false;
}

bool LogicParserEngine::_callAnsiC(void *func, parse_arg_list_t &args, LogicDataObj &result)
{
	typedef bool(*script_c_api)(LogicParserEngine*parser, int argc, LogicDataObj* argv[], LogicDataObj*retVal);

	script_c_api cFunc =(script_c_api) func;

	LogicDataObj *argv[32];
	int count = NDMIN((int)args.size(), 32);
	for (int i = 0; i < count; i++) {
		argv[i] = &(args[i]);
	}

	return cFunc(this, count, argv, &result);
}

bool LogicParserEngine::opCalc(void *func, int size, float *result)
{
	*result = 0 ;
	return false ;
}

//only compare number data 
bool LogicParserEngine::_opCmp(LogicDataObj& compValData, LogicDataObj& invalData, eParserCompare op)
{
	if (!compValData.CheckValid() || !invalData.CheckValid()) {
		return false;
	}
	if (compValData.GetDataType() == OT_STRING &&  invalData.GetDataType()== OT_STRING){
		const char *p1 = compValData.GetText();
		const char *p2 = invalData.GetText();
		if (!p1 || !p2)	{
			return false;
		}
		int ret = ndstricmp(p1, p2);
		switch (op) {
		case ECMP_EQ:  			// =
			return ret == 0;
		case ECMP_LT:  			// <
			return ret < 0;
		case ECMP_BT:  			// >
			return ret > 0;
		case ECMP_ELT: 			// <=
			return !(ret > 0);
		case ECMP_EBT: 			// >=
			return !(ret < 0);
		case ECMP_NEQ:
			return !(ret == 0);
		default:
			break;
		}
	}
	else {

		float compVal = compValData.GetFloat();
		float inval = invalData.GetFloat();

		switch (op) {
		case ECMP_EQ:  			// =
			return compVal == inval;
			//return regval == cmpval;
		case ECMP_LT:  			// <
			return  compVal < inval;
		case ECMP_BT:  			// >
			if (compVal == inval)	{
				return false;
			}
			return !(compVal < inval);
		case ECMP_ELT: 			// <=
			if (compVal == inval)	{
				return true;
			}
			return  compVal < inval;
		case ECMP_EBT: 			// >=
			if (compVal == inval)	{
				return true;
			}
			return  !(compVal < inval);
			
		case ECMP_NEQ:
			return compVal != inval;
			
		default:
			break;
		}
	}

	m_sys_errno = LOGIC_ERR_PARSE_STRING;

	return false ;
}

void LogicParserEngine::Reset()
{
	m_registorFlag =false ;				//比较结果
	m_registorCtrl = true;
	m_sys_errno = LOGIC_ERR_SUCCESS;
	m_registerCount = 0;
	
}

void LogicParserEngine::setErrno(int errcode)
{
	m_sys_errno = errcode;
}


void LogicParserEngine::setSimulate(bool flag)
{
	m_simulate = flag;
// 	if (m_simulate)	{
// 		setOwner(owner);
// 	}
}

void LogicParserEngine::setOwner(ILogicObject *owner)
{
	m_owner = (LogicObjectBase*)owner;
}

const char *LogicParserEngine::curFuncName()
{
	if (m_curStack && m_curStack->cmd){
		return 	m_curStack->cmd->cmdname;
	}
	return NULL;
}

void LogicParserEngine::ResetStep()
{
	m_registorFlag = false ;				//比较结果
}


bool LogicParserEngine::checkEventOk(int event, parse_arg_list_t &params, int waited_id, parse_arg_list_t &waited_params)
{
	if (event != waited_id)	{
		return false;
	}
	if (waited_params.size() > params.size()){
		return false;
	}
	for (size_t i = 0; i < waited_params.size(); i++){
		if (waited_params[i] != params[i])	{
			return false;
		}
	}
	return true;
}

int LogicParserEngine::_continueEvent(StackWaitEventNode *eventNode)
{
	m_OnErrorExit = eventNode->preOnErrorExit;
	m_registerVal = eventNode->preRegisterVal;

	bool runSuccessed = true; 

	m_runningStacks = eventNode->stacks;

	for (CallingStack_list::reverse_iterator it = eventNode->stacks.rbegin(); it != eventNode->stacks.rend(); ) {
		CallingStack_list::reverse_iterator runit = it++;

		if (it != eventNode->stacks.rend())	{
			runit->parentStack = &*it;
		}
		else {
			runit->parentStack = NULL;
		}
		int ret = _reEntryScriptFunction(&(*runit));
		if (ret != 0){
			runSuccessed = false;
			break;
		}
		m_runningStacks.pop_back();
	}

	return 0;
}

//static __ndthread vm_cpu *__cur_vm = NULL;
//find var address
static int place_name_runtime(const char *input, char *buf, int size, void *user_data)
{
	vm_cpu *__cur_vm = (vm_cpu *) user_data;
	if (!__cur_vm){
		return -1;
	}
	LogicParserEngine *le = (LogicParserEngine*)vm_get_param(__cur_vm);
	if (!le){
		return -1;
	}
	runningStack *pstack = le->getCurStack();
	if (!pstack){
		return -1;
	}
	int index = 0;
	LogicData_vct::iterator it = pstack->local_vars.begin();
	for (; it != pstack->local_vars.end(); it++) {
		if (ndstricmp(input, it->name.c_str())==0) {
			ndsnprintf(buf, size, "[%d]",index);
			return 0;
		}
		++index;
	}
	
	nd_logerror("%s() 公式解析错误:不能找到变量[%s]\n", le->curFuncName(), input);
	return -1;
}

vm_value* logic_formula_get_mmaddr(vm_adddress addr, struct vm_cpu *vm)
{
	LogicParserEngine *le = (LogicParserEngine*)vm_get_param(vm);
	if (!le){
		return NULL;
	}
	runningStack *pstack = le->getCurStack();
	if (!pstack){
		return NULL;
	}
	if (addr > pstack->local_vars.size() || addr >= vm->mem_size) {
		nd_logerror("%s() too much vars addr=%d\n", le->curFuncName(), addr);
		return NULL;
	}
	vm->memory[addr] = pstack->local_vars[addr].var.GetFloat();
	return &vm->memory[addr];
}

int LogicParserEngine::_runFormula(const char *text, float *val)
{
	//init vm
	int ret = -1;
	if (!text || !text[0]){
		return 0;
	}

	vm_value mem[VM_STACK_SIZE];
	char cmd_buf[1024];
	vm_machine_init(&m_vmFormula, mem, VM_STACK_SIZE);
#if 0
	vm_set_echo_ins(&m_vmFormula, 1);
	vm_set_echo_result(&m_vmFormula, 1);
	vm_set_outfunc(nd_output);
	vm_set_errfunc(nd_output);
#else 
	vm_set_echo_ins(&m_vmFormula, 0);
	vm_set_echo_result(&m_vmFormula, 0);
	vm_set_outfunc(NULL);
	vm_set_errfunc(NULL);
#endif

	vm_set_param(&m_vmFormula, (void*)this);
	vm_set_mmfunc(&m_vmFormula, logic_formula_get_mmaddr, VM_STACK_SIZE);
	//__cur_vm = &m_vmFormula;

	//parse 
	size_t size = vm_parse_expression((char*)text, cmd_buf, sizeof(cmd_buf), place_name_runtime,&m_vmFormula);
	if (size > 0) {
		if (0 == vm_run_cmd(&m_vmFormula, cmd_buf, size)) {
			*val = vm_return_val(&m_vmFormula);
			ret = 0;
		}
		else {
			nd_logerror("%s() run formula %s error\n", curFuncName(), text);
		}
	}
	else {
		nd_logerror("%s() parse formula error %s\n", curFuncName(), text);
	}
	
	//__cur_vm = NULL;
	return ret;
}

bool LogicParserEngine::_beginHelper()
{
	 
	if (!m_curStack->affairHelper){
		m_curStack->affairHelper = new LogicObjAffairHelper(m_owner);
	}
	return true;
}

void LogicParserEngine::_commitAffair()
{
	if (m_curStack->affairHelper)	{
		delete m_curStack->affairHelper;
		m_curStack->affairHelper = 0;
	}
}

void LogicParserEngine::_rollbacAffair()
{
	if (m_curStack->affairHelper)	{
		m_curStack->affairHelper->Rollback();
		delete m_curStack->affairHelper;
		m_curStack->affairHelper = 0;
	}
}


const LogicUserDefStruct* LogicParserEngine::getUserDataType(const char *name) const
{
	//UserDefData_map_t &m_useDefType = LogicEngineRoot::get_Instant()->getUserDefType();
	UserDefData_map_t::const_iterator it = m_useDefType.find(name);
	if (it == m_useDefType.end()) {
		return NULL;
	}
	return it->second;
}


int LogicParserEngine::_enterDebugMode()
{
	LocalDebugger &debugger = m_root->getGlobalDebugger();	
	return debugger.onEnterStep(this,m_curStack->cmd->cmdname, m_dbg_node);
}


int LogicParserEngine::onScriptRunCompleted()
{
	LocalDebugger &debugger = m_root->getGlobalDebugger();

	return debugger.ScriptRunOk(this);
}



int logic_rand(NDUINT32 val1, NDUINT32 val2)
{
	int val;
	int range_min = (int)val1;
	int range_max = (int)val2;

	if (range_max < range_min) {
		int tmp = range_min;
		range_min = range_max;
		range_max = tmp;
	}
	else if (range_max == range_min)
		return (int)range_max;

	val = (int)((float)rand() / (float)RAND_MAX  * (range_max - range_min) + range_min);

	return val;
}


