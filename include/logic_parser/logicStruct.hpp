//
//  logicStruct.hpp
//  logic_parser
//
//  Created by duanxiuyun on 16/5/9.
//  Copyright 2016 duanxiuyun. All rights reserved.
//

/*
 * implemention of struct for script 
 * this is json like data struct
 */
#ifndef logicStruct_hpp
#define logicStruct_hpp

#include "logic_parser/logicDataType.h"
#include <vector>

struct cJSON;
#define USER_DEF_VAR_NAME_SIZE 64

class LOGIC_PARSER_CLASS ParamNode
{
public:
	ParamNode():m_val()
	{
		m_name[0] = 0 ;
		
	}
	ParamNode(const char * name, const LogicDataObj &val)
	{
		Init(name, val) ;
	}

	ParamNode &operator =(const ParamNode &r)
	{
		Init(r.m_name, r.m_val) ;
		return  *this ;
	}	
	
	bool operator== (const char *name) const
	{
		if (name && *name) {
			return (0 == ndstrcmp(this->m_name, name));
		}
		return false;
	}
	
	LogicDataObj &getVal() {return m_val;}
	
	friend class LogicUserDefStruct ;
private:
	void Init(const char * name,const LogicDataObj &val)
	{
		m_name[0] = 0;
		m_val = val;
		if (name && *name) {
			ndstrncpy(m_name, name, sizeof(m_name));
		}
	}
	char m_name[USER_DEF_VAR_NAME_SIZE] ;
	LogicDataObj m_val ;
};

class LOGIC_PARSER_CLASS LogicUserDefStruct
{
public:
	LogicUserDefStruct(const LogicUserDefStruct &orgData) ;
	LogicUserDefStruct() ;
	~LogicUserDefStruct();

	int FromStream(void *data, size_t size, int byteOrder = 1);
	int ToStream(void *buf, size_t bufsize, int byteOrder = 1) const;
	void ToNode(LogicDataObj &val) const {
		val.InitSet(*this);
	}
	
	LogicUserDefStruct &operator =(const LogicUserDefStruct &r);
	bool operator == (const LogicUserDefStruct &r) const;
	
	bool push_back(const char *name,const LogicDataObj &val) ; //add member ,if the name is exist return false
	void set(const char *name, const LogicDataObj &val) ;// add member if name is exist replace it .
	LogicDataObj get(const char *name) const;
	bool pop_back() { m_members.pop_back(); return true; }
	
	
	int Print(logic_print, void *pf, bool bAsJson=false) const;
	int Sprint(char *buf, size_t size, bool bAsJson = false) const;

	size_t count() const { return m_members.size(); }
	LogicDataObj *ref(size_t index);
	LogicDataObj *ref(const char *name);
	const char *getName(int index);

	const char *_readFromBin(const char *pdata, size_t size, int byteOrder=1);
	bool InitFromSTDtext(const char *inputText);	//read from text which write in STD json format 
	bool InitFromFile(const char *file);	//read from json text file which write in STD json format 

	bool isArray() { return m_rootIsArray; }
protected:
	//void Destroy();
	LogicDataObj *fetch(const char *name) ;
	bool _cjsonToUserDefine(cJSON *root);
	bool _cjsonArrayToUserDefine(cJSON *node);

	typedef std::vector<ParamNode> param_vct_t ;

	bool m_rootIsArray;
	
	param_vct_t m_members ;
};

#endif /* logicStruct_hpp */
