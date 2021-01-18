/* file objectBaseMgr.h
 *
 * game object base manager 
 *
 * create by duan 
 * 2015-5-13
 */

 #ifndef _OBJECT_BASE_MGR_H_
 #define _OBJECT_BASE_MGR_H_

#include "logic_parser/logicDataType.h"
#include "logic_parser/logicApi.hpp"

typedef NDUINT32 operator_index_t;


#define CHECK_ID_INVALID(_id) \
	if (!id.CheckValid())	{	\
		m_error = NDERR_SCRIPT_INSTRUCT; \
		return false;			\
	}

class LogicObjectBase;
class LogicUserFuncMgrHelper;
typedef bool(*logic_operate_func)(LogicObjectBase *pObj, const char* cmd, const LogicDataObj& id, LogicDataObj &val);
typedef bool(*logic_getother_func)(LogicObjectBase *pObj, const char* objName, LogicDataObj &val);

//logicParser Engine run the command Read/Write/Add/Sub/Clear/Check only throw objects LogicObjectBase
//so you need inherit LogicObjectBase if your object need be operated by script.
class LOGIC_PARSER_CLASS LogicObjectBase : public ILogicObject
{
public:
	LogicObjectBase();
	virtual ~LogicObjectBase();
	virtual		int Create(const char*name);
	virtual 	void Destroy(int flag);

public:
	virtual bool opRead(const LogicDataObj& id, LogicDataObj &val) ; //copy data

	virtual bool opWrite(const LogicDataObj& id, const  LogicDataObj &val);

	virtual bool opAdd(const LogicDataObj& id, const  LogicDataObj &val);

	virtual bool opSub(const LogicDataObj& id, const  LogicDataObj &val) ;
	
	virtual bool opCheck(const LogicDataObj& id, const LogicDataObj &val) ;

	//common operate @val  input-output
	virtual bool opOperate(const char *cmd, const LogicDataObj& id,  LogicDataObj &val); 

	//get object manager for opRead/write/...
	virtual LogicObjectBase *getObjectMgr(const char* destName);

	//get other object
	virtual bool getOtherObject(const char*objName, LogicDataObj &val) ;

	virtual int Print(logic_print f, void *pf); //print object self info 

	//affair function
	virtual bool CheckInAffair();
	virtual bool BeginAffair();
	virtual bool CommitAffair();
	virtual bool RollbackAffair();

	// throught this function you can make the system-defined LogicObjectBase run you owner function when called opOperate() ;
	bool setOperateCmdFunction( logic_operate_func func);
	void removeOperateCmdFunction(logic_operate_func func);

	bool setOtherObjectFunc(logic_getother_func func);
	void removeOtherObjectFunc(logic_getother_func func);
	
	NDUINT32 getErrParam() ;
	int getError();
	void clearError();

	int getCheckShortRate() { return m_opCheckShortRate; }
	bool checkObjectIsExist() { return m_bOpCheckFoundObj; }
	void clearCheckShortRate() { m_bOpCheckFoundObj= false; m_opCheckShortRate = 0; }
protected:
	ILogicObject *getObject(const char* destName);

	int m_count;

	int m_error;
	NDUINT32 m_errParam ;
	NDUINT16 m_opCheckShortRate;
	bool m_bOpCheckFoundObj;
private:
	LogicUserFuncMgrHelper * m_UserFuncHelper;
};

class  LOGIC_PARSER_CLASS  LogicObjAffairHelper
{
public:
	LogicObjAffairHelper(LogicObjectBase *ta) :m_affair(ta)
	{
		m_affair->BeginAffair();
	}
	virtual ~LogicObjAffairHelper()
	{
		if (m_affair){
			m_affair->CommitAffair();
			m_affair = NULL;
		}
	}
	void Rollback()
	{
		m_affair->RollbackAffair();
		m_affair = NULL;
	}
private:
	LogicObjectBase *m_affair;

};
class LogicUserFuncMgrHelper
{
public :
	LogicUserFuncMgrHelper();
	~LogicUserFuncMgrHelper();

	bool setOperateCmd(logic_operate_func func);
	bool setOtherObj(logic_getother_func func);

	void removeOperateCmd(logic_operate_func func);
	void removeOtherObj(logic_getother_func func);

	bool callUserOperate(LogicObjectBase *pOwner,const char *cmd, const LogicDataObj& id, LogicDataObj &val);
	bool callGetOtherObj(LogicObjectBase *pOwner,const char*objName, LogicDataObj &val);

private:
	std::vector<logic_operate_func> m_operate_funcs;
	std::vector<logic_getother_func> m_otherobj_funcs;
};

#endif 
 