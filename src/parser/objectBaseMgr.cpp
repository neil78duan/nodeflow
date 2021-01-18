/* file objectBaseMgr.cpp
 *
 * game object base manager 
 *
 * create by duan 
 * 2015-5-13
 */
#include "logic_parser/objectBaseMgr.h"
#include "logic_parser/logicParser.h"
//#include "logic_parser/logicEngineRoot.h"

LogicObjectBase::LogicObjectBase() :m_count(0), m_errParam(0), 
m_opCheckShortRate(0), m_bOpCheckFoundObj(false), m_UserFuncHelper(0)
{

}
LogicObjectBase::~LogicObjectBase()
{
	if (m_UserFuncHelper) {
		delete m_UserFuncHelper;
		m_UserFuncHelper = 0;
	}
}

int LogicObjectBase::Create(const char*)
{
	return 0;
}

void LogicObjectBase::Destroy(int flag)
{
	if (m_UserFuncHelper) {
		delete m_UserFuncHelper;
		m_UserFuncHelper = 0;
	}
}

bool LogicObjectBase::opRead(const LogicDataObj& id, LogicDataObj &val)
{
	PARSE_TRACE("logic_engine_test: opRead(%d) \n", id.GetInt());
	val.InitSet(0);
	return true;
}

bool LogicObjectBase::opWrite(const LogicDataObj& id, const LogicDataObj &val)
{
	PARSE_TRACE("logic_engine_test: opWrite(%d) \n", id.GetInt());
	return true;
}


bool LogicObjectBase::opAdd(const LogicDataObj& id, const LogicDataObj &val)
{
	PARSE_TRACE("logic_engine_test: opAdd(%d) \n", id.GetInt());
	return true;
}


bool LogicObjectBase::opSub(const LogicDataObj& id, const  LogicDataObj &val)
{
	PARSE_TRACE("logic_engine_test: opSub(%d) \n", id.GetInt());
	return true;
}

bool LogicObjectBase::opCheck(const LogicDataObj& id, const  LogicDataObj &val)
{
	m_error = NDERR_NOT_SURPORT;
	return false;
}
//common operate 
bool LogicObjectBase::opOperate(const char *cmd, const LogicDataObj& id,  LogicDataObj &val)
{
	bool ret = false;
	if (m_UserFuncHelper) {
		ret = m_UserFuncHelper->callUserOperate(this, cmd, id, val);
	}
	if (ret == false) {
		nd_logerror("run operate error %s \n", cmd);
	}
	return ret;
}



bool LogicObjectBase::getOtherObject(const char*objName, LogicDataObj &val)
{
	if (m_UserFuncHelper) {
		if (m_UserFuncHelper->callGetOtherObj(this, objName, val)) {
			return true;
		}
	}
	
	nd_logerror("get object %s error \n", objName );
	m_error = NDERR_NOT_SURPORT;
	return false;
}


ILogicObject *LogicObjectBase::getObject(const char* destName)
{
	return getObjectMgr(destName);
}

LogicObjectBase *LogicObjectBase::getObjectMgr(const char* destName)
{
	//m_error = NDERR_NOT_SURPORT;
	return this;
}
int LogicObjectBase::Print(logic_print f, void *pf)
{
	return f(pf, "[LogicObjectBase]");
}

//affaie function
bool LogicObjectBase::BeginAffair()
{
	return m_count++ == 0;
}
bool LogicObjectBase::CommitAffair()
{
	return --m_count==0;
}
bool LogicObjectBase::RollbackAffair()
{
	return --m_count == 0;
}

bool LogicObjectBase::CheckInAffair()
{
	return m_count > 0;
}


NDUINT32 LogicObjectBase::getErrParam()
{
// 	NDUINT32 ret = m_errParam ;
// 	m_errParam = 0 ;
// 	return  ret ;
	return m_errParam;
}

int LogicObjectBase::getError()
{
	return m_error;
// 	NDUINT32 ret = m_error;
// 	m_error = 0;
// 	return  ret;

}


void LogicObjectBase::clearError()
{
	m_error = 0;
	m_errParam = 0;
}

bool LogicObjectBase::setOperateCmdFunction(logic_operate_func func)
{
	if (!m_UserFuncHelper) {
		m_UserFuncHelper = new LogicUserFuncMgrHelper;
	}
	m_UserFuncHelper->setOperateCmd(func);
	return true;
}
bool LogicObjectBase::setOtherObjectFunc(logic_getother_func func)
{
	if (!m_UserFuncHelper) {
		m_UserFuncHelper = new LogicUserFuncMgrHelper;
	}
	m_UserFuncHelper->setOtherObj(func);
	return true;
}

void LogicObjectBase::removeOperateCmdFunction(logic_operate_func func)
{
	if (m_UserFuncHelper) {
		m_UserFuncHelper->removeOperateCmd(func);
	}
}

void LogicObjectBase::removeOtherObjectFunc(logic_getother_func func)
{
	if (m_UserFuncHelper) {
		m_UserFuncHelper->removeOtherObj(func);
	}
}

/////////////////


LogicUserFuncMgrHelper::LogicUserFuncMgrHelper()
{

}
LogicUserFuncMgrHelper::~LogicUserFuncMgrHelper()
{

}

bool LogicUserFuncMgrHelper::setOperateCmd(logic_operate_func func)
{
	for (int i = 0; i < m_operate_funcs.size(); i++) {
		if (m_operate_funcs[i] == func) {
			return true;
		}
	}
	m_operate_funcs.push_back(func);
	return true;
}
bool LogicUserFuncMgrHelper::setOtherObj(logic_getother_func func)
{
	for (int i = 0; i < m_otherobj_funcs.size(); i++) {
		if (m_otherobj_funcs[i] == func) {
			return true;
		}
	}
	m_otherobj_funcs.push_back(func);
	return true;
}


void LogicUserFuncMgrHelper::removeOperateCmd(logic_operate_func func)
{
	for (int i = 0; i < m_operate_funcs.size(); i++) {
		if (m_operate_funcs[i] == func) {
			m_operate_funcs.erase(m_operate_funcs.begin()+i);
			return ;
		}
	}
}
void LogicUserFuncMgrHelper::removeOtherObj(logic_getother_func func)
{
	for (int i = 0; i < m_otherobj_funcs.size(); i++) {
		if (m_otherobj_funcs[i] == func) {
			m_otherobj_funcs.erase(m_otherobj_funcs.begin()+i);
			return ;
		}
	}
}

bool LogicUserFuncMgrHelper::callUserOperate(LogicObjectBase *pOwner, const char *cmd, const LogicDataObj& id, LogicDataObj &val)
{
	for (int i = 0; i < m_operate_funcs.size(); i++) {
		logic_operate_func func = m_operate_funcs[i];
		if (func(pOwner, cmd, id, val)) {
			return true;
		}
	}
	return false;
}
bool LogicUserFuncMgrHelper::callGetOtherObj(LogicObjectBase *pOwner, const char*objName, LogicDataObj &val)
{
	for (int i = 0; i < m_otherobj_funcs.size(); i++) {
		logic_getother_func func = m_otherobj_funcs[i];
		if (func(pOwner, objName, val)) {
			return true;
		}
	}
	return false;
}
