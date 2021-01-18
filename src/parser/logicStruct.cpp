//
//  logicStruct.cpp
//  logic_parser
//
//  Created by duanxiuyun on 16/5/9.
//  Copyright 2016 duanxiuyun. All rights reserved.
//

#include "logic_parser/logicStruct.hpp"
#include "logic_parser/logicEndian.h"
#include "nd_common/cJSON.h"

static int _read_string(const char *pCmdStream, char *outbuf, size_t size, int byteOrder)
{
	NDUINT16 size16 = 0;
	lp_stream_t p = (lp_stream_t) pCmdStream;
	//size16 = *((*(NDUINT16**)&p)++);
	p = lp_read_stream(p, size16, byteOrder);
	size -= 2;

	if (size16 >= size){
		return -1;
	}
	if (size16 > 0) {
		memcpy(outbuf, p, size16);
	}
	outbuf[size16] = 0;
	p += size16;
	return (int)(p - pCmdStream);
}

LogicUserDefStruct::LogicUserDefStruct(const LogicUserDefStruct &orgData) 
{
	m_rootIsArray = orgData.m_rootIsArray;
	m_members = orgData.m_members;
}
LogicUserDefStruct::LogicUserDefStruct() : m_rootIsArray(false)
{
	
}


LogicUserDefStruct::~LogicUserDefStruct()
{

}
const char *LogicUserDefStruct::_readFromBin(const char *p, size_t size, int byteOrder )
{
	char name[256];
	const char *pend = p + size;
	while (p < pend) {
		name[0] = 0;
		int len = _read_string(p, name, sizeof(name), byteOrder);
		if (len == -1) {
			return NULL;
		}
		p += len;
		size -= len;

		LogicDataObj val;
		len = val.ReadStream(p, size, byteOrder);
		if (-1 == len) {
			return NULL;
		}
		p += len;
		size -= len;
		push_back(name, val);
	}
	return p;
}

int LogicUserDefStruct::FromStream(void *data, size_t size, int byteOrder )
{
	const char *p = (const char*)data;
	//char name[256];
	NDUINT16 data_len = 0;
	p = lp_read_stream((lp_stream_t)p, data_len, byteOrder);
	if (data_len > size - 2)
		return	 -1;

	p = _readFromBin(p, data_len, byteOrder);
	if (!p) {
		return -1;
	}
	/*
	const char *pend = p + data_len;
	while (p < pend){
		name[0] = 0;
		int len = _read_string(p, name, sizeof(name),byteOrder);
		if (len == -1){
			return -1;
		}
		p += len;
		size -= len;

		LogicDataObj val;
		len = val.ReadStream(p,size, byteOrder);
		if (-1 == len){
			return -1;
		}
		p += len;
		size -= len;
		push_back(name,  val);
		
	}*/
	return (int)(p - (const char*)data);
}

int LogicUserDefStruct::ToStream(void *buf, size_t bufsize, int byteOrder ) const
{
	lp_stream_t p = lp_write_stream((char*)buf, (NDUINT16)0, byteOrder);
	bufsize -= 2;

	param_vct_t::const_iterator it = m_members.begin();
	for (; it != m_members.end(); it++){
		int len = 0;
		if (it->m_name[0]) {
			len = (int)ndstrlen(it->m_name);
			if (len >= bufsize) {
				return -1;
			}
		}
		
		p = lp_write_stream(p, (NDUINT16)len, byteOrder);
		bufsize -= 2;

		if (len > 0) {
			ndstrncpy(p, it->m_name, bufsize);
			bufsize -= len;
			p += len;
		}
		len = it->m_val.WriteStream(p,bufsize, byteOrder);
		if (-1 == len){
			return -1;
		}
		p += len;
		bufsize -= len;

	}
	NDUINT16 size = (NDUINT16)(p - (char*)buf) -2;
	lp_write_stream((lp_stream_t)buf, size , byteOrder);
	return size + 2 ;
}


LogicUserDefStruct & LogicUserDefStruct::operator =(const LogicUserDefStruct &r)
{
	m_rootIsArray = r.m_rootIsArray;
	m_members = r.m_members ;
	return *this ;
}
bool LogicUserDefStruct::operator == (const LogicUserDefStruct &r) const
{
	return  false ;
}


bool LogicUserDefStruct::push_back(const char *name, const LogicDataObj &val)
{
	if (fetch(name)) {
		return  false ;
	}
	m_members.push_back(ParamNode(name, val)) ;
	return true ;
}

void LogicUserDefStruct::set(const char *name, const LogicDataObj &val)
{
	LogicDataObj* pdata = fetch(name) ;
	if (pdata) {
		*pdata = val ;
	}
	else {
		m_members.push_back(ParamNode(name, val)) ;
	}
}
LogicDataObj LogicUserDefStruct::get(const char *name)const
{
	LogicDataObj* pdata = ((LogicUserDefStruct*)this)->fetch(name);
	if (!pdata) {
		return LogicDataObj() ;
	}
	return  *pdata ;
}

int LogicUserDefStruct::Print(logic_print outFunc, void *pf, bool bAsJson ) const
{
	int ret = 0 ;
	param_vct_t::const_iterator it ;

	ret += outFunc(pf, "{");
	size_t i = 0;
	for (it = m_members.begin(); it != m_members.end(); ++it) {
		++i;
		if (it->m_name[0]) {
			if (bAsJson) {
				ret += outFunc(pf, " \"%s\":", it->m_name);
			}
			else {
				ret += outFunc(pf, " %s:", it->m_name);
			}
		}

		LogicDataObj *dataVal = (LogicDataObj *)&(it->m_val);
		bool isJson = dataVal->setOutAsJson(bAsJson);
		
		ret += dataVal->Print(outFunc, pf);
		if (i < m_members.size()){
			ret += outFunc(pf, ", ");
		}
		dataVal->setOutAsJson(isJson);
	}

	ret += outFunc(pf, "}\n");
	
	return ret ;
}

int LogicUserDefStruct::Sprint(char *buf, size_t size, bool bAsJson ) const
{
	int ret = 0 ;
	param_vct_t::const_iterator it ;
	for (it=m_members.begin(); it != m_members.end(); ++it) {
		
		LogicDataObj *dataVal = (LogicDataObj *)&(it->m_val);
		bool isJson = dataVal->setOutAsJson(bAsJson);

		ret += dataVal->Sprint(buf, size) ;

		dataVal->setOutAsJson(isJson);
	}
	return ret ;
}


LogicDataObj *LogicUserDefStruct::ref(size_t index)
{
	if (index < m_members.size()) {
		return &(m_members[index].m_val);
	}
	return NULL;
}

LogicDataObj *LogicUserDefStruct::ref(const char *name)
{
	return fetch(name);
}


const char *LogicUserDefStruct::getName(int index)
{
	if (index < m_members.size()){
		return m_members[index].m_name;
	}
	return NULL;
}

LogicDataObj* LogicUserDefStruct::fetch(const char *name)
{
	if (!name || !*name) {
		return NULL;
	}
	param_vct_t::iterator it ;
	for (it=m_members.begin(); it != m_members.end(); ++it) {
		if ( (*it) == name ) {
			return  &(it->getVal() );
		}
	}
	return  NULL;
}

// json

bool LogicUserDefStruct::InitFromSTDtext(const char *inputText)
{
	cJSON *root = cJSON_Parse(inputText);
	if (!root) {
		nd_logerror("parse json from input text error :%s\n", cJSON_GetErrorPtr());
		return false;
	}
	//bool ret = _cjsonToUserDefine(root);
	bool ret = false;
	if (root->type == cJSON_Array) {
		ret = _cjsonArrayToUserDefine(root);
		m_rootIsArray = true;
	}
	else {
		m_rootIsArray = false;
		ret = _cjsonToUserDefine(root);
	}

	cJSON_Delete(root);
	return ret;
}

bool LogicUserDefStruct::InitFromFile(const char *filePath)
{
	size_t size;
	char *pData = (char*)nd_load_file(filePath, &size);
	if (!pData) {
		nd_logerror("can not load %s\n", filePath);
		return false;
	}

	cJSON *root = cJSON_Parse(pData);
	if (!root) {
		nd_unload_file(pData);
		nd_logerror("parse json data error:%s\n", cJSON_GetErrorPtr());
		return false;
	}
	nd_unload_file(pData);
	bool ret = false;
	if (root->type == cJSON_Array) {
		ret = _cjsonArrayToUserDefine(root);
		m_rootIsArray = true;
	}
	else {
		m_rootIsArray = false;
		ret = _cjsonToUserDefine(root);
	}
	//bool ret = _cjsonToUserDefine(root);

	cJSON_Delete(root);
	return ret;
}

bool LogicUserDefStruct::_cjsonArrayToUserDefine(cJSON *node)
{
	LogicDataObj arrayDblNode;
	int counts = cJSON_GetArraySize(node);

	arrayDblNode.InitReservedArray(counts, OT_USER_DEFINED);
	for (size_t i = 0; i < counts; i++) {
		LogicUserDefStruct subUserData;
		cJSON *subNode = cJSON_GetArrayItem(node, (int)i);
		if (subNode) {
			if (subUserData._cjsonToUserDefine(subNode)) {
				arrayDblNode.SetArray(LogicDataObj(subUserData), (int)i);
			}
		}
	}
	push_back(node->string, arrayDblNode);
	return true;

}
bool LogicUserDefStruct::_cjsonToUserDefine(cJSON *root)
{
	int numbers = cJSON_GetArraySize(root);
	for (size_t i = 0; i < numbers; i++) {
		cJSON *node = cJSON_GetArrayItem(root, (int)i);
		if (!node) {
			continue;
		}
		switch (node->type)
		{
		case cJSON_Number:
			push_back(node->string, LogicDataObj((float)node->valuedouble));
			break;
		case cJSON_String:
			push_back(node->string, LogicDataObj(node->valuestring));
			break;
		case cJSON_Array:
		{
			_cjsonArrayToUserDefine(node);
// 			LogicDataObj arrayDblNode;
// 			int counts = cJSON_GetArraySize(node);
// 
// 			arrayDblNode.InitReservedArray(counts, OT_USER_DEFINED);
// 			for (size_t i = 0; i < counts; i++) {
// 				LogicUserDefStruct subUserData;
// 				cJSON *subNode = cJSON_GetArrayItem(node, (int)i);
// 				if (subNode) {
// 					if (subUserData._cjsonToUserDefine(subNode)) {
// 						arrayDblNode.SetArray(LogicDataObj(subUserData), (int)i);
// 					}
// 				}
// 			}
// 			push_back(node->string, arrayDblNode);
			break;
		}
		case cJSON_Object:
		{
			LogicUserDefStruct jsonData;

			//nd_assert(node->string && node->string[0]);
			if (jsonData._cjsonToUserDefine(node)) {
				push_back(node->string, LogicDataObj(jsonData));
				break;
			}
			return false;
		}
		default:
			return false;
		}
	}
	return true;
}