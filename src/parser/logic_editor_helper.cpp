/* file logic_editor_helper.cpp
 *
 * editor common function
 *
 * create by duan 
 *
 * 2016-1-14
 */

#include "logic_parser/logic_editor_helper.h"

namespace LogicEditorHelper
{
	static const char *g_cur_edit_version = "1.2.0";
	static int g_edit_version_id = 10200;
	int CXMLAlias::Create(ndxml *xmlAlias)
	{
		ndxml *node;
		if (m_created)
			return 0;

		int num = ndxml_getsub_num(xmlAlias);
		if (num <= 0)
			return -1;

		m_pBuf = new sAlias[num];

		for (int i = 0; i < num; i++) {
			node = ndxml_refsubi(xmlAlias, i);
			if (node) {
				m_pBuf[m_num].str_var = ndxml_getname(node);
				m_pBuf[m_num].str_alias = ndxml_getval(node);
				m_num++;
			}
		}
		m_created = 1;
		return m_num;
	}

	const char* CXMLAlias::GetAlia(const char *valname)
	{
		for (int i = 0; i < m_num; i++)
		{
			if (0 == ndstricmp(valname, m_pBuf[i].str_var.c_str()))
				return m_pBuf[i].str_alias.c_str();
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////


	const char *_szDataKindsDesc[] =		//data type name 
	{
		"none",		
		"enum",		
		"bool",		
		"numeral",	
		"string",	
		"password",	
		"hide",		
		"in_file",	
		"out_file",	
		"dir",		
		"reference",		
		"user_define",		
		"key_value",
		"multi_text"		//multi-line text
	};
	
	const char *_szReserved[] = {		//reserved words
		"kinds",				
		"enum_value",			
		"name",
		"createtime",
		"desc",
		"expand",				
		"rw_stat",				
		"param",				
		"delete",				
		"create_template",
		"reference_type",
		"data_type_define",
		"expand_list",
		"restrict",				
		"comp_cond",			
		"comp_value",
		"user_param",			
		"auto_index",
		"expand_stat",
		"enable_drag",
		"need_confirm",
		"key",
		"value",
		"referenced_only",
		"replace_val",
		"create_label",
		"var_name",
		"connect_in_seq",
		"breakPointAnchor",
		"onlyForDebug",
		"breakPoint",
		"displayChildren",
		"blueprintCtrl",
		"blueprintTips",
		"defaultNext",
		"realCmd"
	};


	const char *_GetDataTypeName(eDataType dataType)
	{
		return _szDataKindsDesc[dataType];
	}
	const char *_GetReverdWord(eReservedType rType)
	{
		return _szReserved[rType];
	}

	int CheckReserved(const char *param)
	{
		int num = sizeof(_szReserved) / sizeof(char*);
		for (int i = 0; i < num; i++) {
			if (0 == ndstricmp(param, _szReserved[i]))
				return i;
		}
		return -1;
	}
	bool CheckCanDelete(ndxml *xml)
	{
		char *kinds = (char*)ndxml_getattr_val(xml, _szReserved[ERT_DELETE]);
		if (kinds) {
			return (0 != ndstricmp(kinds, "no"));
		}
		return true;
	}
	
	int CheckDataType(const char *param)
	{
		int num = sizeof(_szDataKindsDesc) / sizeof(char*);
		for (int i = 0; i < num; i++) {
			if (0 == ndstricmp(param, _szDataKindsDesc[i]))
				return i;
		}
		return -1;
	}

	ndxml *getDefinedType(ndxml_root *root, const char *defTypeName)
	{
		ndxml *typeXml = ndxml_getnode(root, _szReserved[ERT_DATA_DEFINE]);
		if (typeXml){
			return ndxml_refsub(typeXml, defTypeName);
		}
		return NULL;
	}

	int GetXmlValType(ndxml *xml, ndxml_root *root)
	{
		char *kinds = (char*)ndxml_getattr_val(xml, _szReserved[ERT_KIND]);
		if (kinds) {
			int ret = CheckDataType(kinds);
			if (EDT_REF_DEFINED == ret){
				char *ref_typename = (char*)ndxml_getattr_val(xml, _szReserved[ERT_REF_TYPE]);
				if (!ref_typename)	{
					return EDT_NONE;
				}
				ndxml *defined_type = getDefinedType(root, ref_typename);
				if (defined_type){
					return GetXmlValType(defined_type, root);
				}
			}
			return ret;
		}
		return EDT_NONE;
	}
	//////////////////////

	const char *_getXmlParamVal(ndxml *xml, ndxml_root *root, eReservedType paramName)
	{
		char *pEnumList = (char*)ndxml_getattr_val(xml, _szReserved[paramName]);
		if (pEnumList) {
			return pEnumList;
		}
		else if(root) {
			char *ref_typename = (char*)ndxml_getattr_val(xml, _szReserved[ERT_REF_TYPE]);
			if (ref_typename) {
				ndxml *defined_type = getDefinedType(root, ref_typename);
				if (defined_type)	{
					return  ndxml_getattr_val(defined_type, _szReserved[paramName]);
				}
			}
		}
		return NULL;
	}
	
	const char* GetXmlParam(ndxml *xml)
	{
		return (char*)ndxml_getattr_val(xml, _szReserved[ERT_PARAM]);
	}

	const char *_GetNodeComment(ndxml *xml)
	{
		ndxml *pSub = ndxml_getnode(xml, "comment");
		if (pSub) {
			return ndxml_getval(pSub);
		}
		else {
			const char *p = ndxml_getname(xml);
			const char* ret = ndstrchr(p, '_');
			if (ret){
				return ++ret;
			}
			return p;
		}
	}

	const char *_GetXmlName(ndxml *xml, CXMLAlias *alias)
	{
		//alias
		const char *name = ndxml_getattr_val(xml, _szReserved[ERT_NAME]);
		if (!name) {
			name = ndxml_getname(xml);
			if (alias)	{
				const char *alia_name = alias->GetAlia(name);
				if (alia_name)	{
					return alia_name;
				}
			}
		}
		return name;
	}

	const char *_GetXmlDesc(ndxml *xml)
	{
		const char *name = ndxml_getattr_val(xml, _szReserved[ERT_DESC]);
		if (!name) {
			name =  "NULL";
		}
		return name;
	}
	//
	bool CheckHide(ndxml *xml)
	{
		char *kinds = (char*)ndxml_getattr_val(xml, _szReserved[ERT_KIND]);
		if (kinds) {
			if (EDT_HIDE == CheckDataType(kinds))
				return true;
		}
		return false;
	}

	//
	bool CheckExpand(ndxml *xml)
	{
		char *kinds = (char*)ndxml_getattr_val(xml, _szReserved[ERT_EXPAND]);
		if (kinds) {
			if (0 == ndstricmp(kinds, "yes"))
				return true;
		}
		return false;
	}

	bool CheckDisplayChildren(ndxml *xml)
	{
		char *kinds = (char*)ndxml_getattr_val(xml, _szReserved[ERT_DISPLAY_CHILDREN]);
		if (kinds) {
			if (0 == ndstricmp(kinds, "no"))
				return false;
		}
		return true;
	}

	bool checkAddNewChild(ndxml *xml)
	{
		char*template_name = (char*)ndxml_getattr_val(xml, _szReserved[ERT_TEMPLATE]);
		if (!template_name || !*template_name) {
			return false;
		}
		return true;
	}

	int get_string_array(const char *pValue, text_vect_t &texts)
	{
		if (!pValue)
			return 0;
		char subtext[128];
		do 	{
			subtext[0] = 0;
			pValue = (char*)ndstr_nstr_end(pValue, subtext, ',', sizeof(subtext));
			if (pValue && *pValue == ',') {
				++pValue;
			}
			if (subtext[0]){
				texts.push_back(std::string(subtext));
			}
		} while (pValue && *pValue);
		return (int) texts.size();
	}

	//
	bool CheckReadOnly(ndxml *xml)
	{
		char *kinds = (char*)ndxml_getattr_val(xml, _szReserved[ERT_RW]);
		if (kinds) {
			if (0 == ndstricmp(kinds, "read"))
				return true;
		}
		//XML parent ;
		//if(0==xml.Parent(parent) )
		//	return CheckReadOnly(parent) ;
		return false;
	}
	ndxml *GetCreateTemplate(ndxml *xml, ndxml_root *root)
	{
		char*template_name = (char*)ndxml_getattr_val(xml, _szReserved[ERT_TEMPLATE]);
		if (!template_name) {
			return NULL;
		}
		ndxml *template_root = ndxml_getnode(root, _szReserved[ERT_TEMPLATE]);
		if (!template_root)
			return NULL;

		ndxml *create_template = ndxml_refsub(template_root, template_name);
		
		while (create_template) {
			const char *pRef = ndxml_getattr_val(create_template, "ref_from");
			if (pRef && (0 == ndstricmp(pRef, "yes") || 0 == ndstricmp(pRef, "true")))	{
				create_template = LogicEditorHelper::_getRefNode(create_template, ndxml_getval(create_template));
			}
			else {
				break;
			}
		}

		return create_template;
	}
	/////////////////////////////////////////////////////////////////////////////

	bool getRealVarName(const char *pInput, std::string &realname)
	{
		char tmpbuf[128];
		if (!pInput) {
			return false;
		}
		realname = getRealValFromStr(pInput, tmpbuf, sizeof(tmpbuf));
		return  !realname.empty();
 	}

	bool getDisplayVarName(const char *pInput, std::string &displayname)
	{
		char tmpbuf[128];
		if (!pInput) {
			return false;
		}
		displayname = getDisplayNameFromStr(pInput, tmpbuf, sizeof(tmpbuf));
		return  !displayname.empty();
	}

	bool buildValName(const char*realName, const char*display,  std::string &fullText)
	{
		char tmpbuf[1024];
		fullText = buildDisplaNameValStr(realName, display,tmpbuf, sizeof(tmpbuf));
		return  !fullText.empty();
	}

	const char *getRealValFromStr(const char *pInput, char *outpub, size_t bufsize)
	{
		if (!pInput || !*pInput) {
			return "none";
		}
		const char *p = ndstrstr(pInput, "_realval=");
		if (p){
			p += 9;
			outpub[0] = 0;
			ndstr_nstr_end((char*)p, outpub, '&', (int)bufsize);
			if (outpub[0])	{
				return outpub;
			}
		}
		return pInput;
	}
	const char *getDisplayNameFromStr(const char *pInput, char *outpub, size_t bufsize)
	{
		if (!pInput || !*pInput) {
			return "none";
		}
		const char *p = ndstrstr(pInput, "_dispname=");
		if (p && *p){
			p += 10;
			outpub[0] = 0;
			ndstr_nstr_end((char*)p, outpub, '&', (int)bufsize);
			if (outpub[0])	{
				return outpub;
			}
		}
		return pInput;
	}

	bool tryToReplaceDispName(ndxml *xml)
	{

		const char *pReplacePath = ndxml_getattr_val(xml, "replace_val");
		if (!pReplacePath) {
			return true;
		}
		const char *newValue = ndxml_getval(xml);
		if (!newValue) {
			return false;
		}
		char tmpbuf[128];
		newValue = getDisplayNameFromStr(newValue, tmpbuf, sizeof(tmpbuf));

		ndxml *repXml = LogicEditorHelper::_getRefNode(xml, pReplacePath);
		if (!repXml) {
			return true;
		}
		const char *attrName = LogicEditorHelper::_getRefNodeAttrName(pReplacePath);

		char buf[1024];
		const char *val = NULL;
		size_t size = sizeof(buf);

		if (attrName) {
			val = ndxml_getattr_val(repXml, attrName);
		}
		else {
			val = ndxml_getval(repXml);
		}

		//replace data
		const char *p = ndstr_nstr_ansi(val, buf, '$', sizeof(buf));
		size -= p - val;
		++p;

		ndstrncat(buf, "$", size);
		size--;

		ndstrncat(buf, newValue, size);
		size -= strlen(newValue) + 1;
		ndstrncat(buf, "$", size);

		p = ndstrchr(p, '$');

		if (p && *p) {
			++p;
			ndstrncat(buf, p, size);
		}

		if (attrName) {
			ndxml_setattrval(repXml, attrName, buf);
		}
		else {
			ndxml_setval(repXml, buf);
		}
		return true;
	}
	const char *buildDisplaNameValStr(const char *val, const char*dispName, char *outpub, size_t bufsize)
	{
		ndsnprintf(outpub, bufsize, "_realval=%s&_dispname=%s", val, dispName);
		return outpub;
	}


	ndxml *_getRefNode(ndxml*node, const char *xmlPath)
	{
		return ndxml_recursive_ref(node, xmlPath);		
	}


	bool _CheckIsChild(ndxml *childXml, const char *parentName)
	{
		ndxml *parent = ndxml_get_parent(childXml);

		while (parent)	{
			const char *name =ndxml_getname(parent);
			if (0==ndstricmp(name,parentName) )	{
				return true;
			}
			parent = ndxml_get_parent(parent);
		}
		return false;
	}

	const char *_getRefNodeAttrName(/*ndxml *node,*/ const char *xmlPath)
	{
		return nd_file_ext_name(xmlPath);		
	}


	time_t getScriptChangedTime(ndxml_root *xmlFile)
	{
		//char buf[64];
		ndxml *xmlModule = ndxml_getnode(xmlFile, "moduleInfo");
		if (xmlModule)	{
			ndxml *valxml = ndxml_getnode(xmlModule, "LastMod");
			if (valxml) {
				//ndsnprintf(buf, sizeof(buf), "%lld", )
				const char *pVal = ndxml_getval(valxml);
				if (pVal){
					return (ndtime_t) atoll(pVal);
				}
			}
		}
		return time(NULL);
		
	}

	const char *getEditorVersion()
	{
		return g_cur_edit_version;
	}

	int getEditorVersionId()
	{
		return g_edit_version_id;
	}

	void setScriptChangedTime(ndxml_root *xmlFile, time_t changedTime)
	{
		char buf[64];
		ndxml *xmlModule = ndxml_getnode(xmlFile, "moduleInfo");
		if (xmlModule)	{

			ndxml *valxml = ndxml_getnode(xmlModule, "EditorVersion");
			if (valxml) {
				ndxml_setval(valxml, g_cur_edit_version);
			}
			else {
				valxml = ndxml_addnode(xmlModule, "EditorVersion", g_cur_edit_version);
				if (valxml) {
					ndxml_addattrib(valxml, "kinds", "hide");
				}
			}

			if (0 != changedTime) {
				ndsnprintf(buf, sizeof(buf), "%lld", (NDUINT64)changedTime);
				valxml = ndxml_getnode(xmlModule, "LastMod");
				if (valxml) {
					ndxml_setval(valxml, buf);
				}
				else {
					ndxml_addnode(xmlModule, "LastMod", buf);
				}
			}

		}
	}

	bool getBoolValue(const char *value)
	{
		if (!value || !*value)	{
			return false;
		}
		if (ndstricmp(value, "no") == 0 || ndstricmp(value, "false") == 0 || 
			ndstricmp(value, "disable") == 0 || ndstricmp(value, "failed") == 0 ||
			ndstricmp(value, "error") == 0 || ndstricmp(value, "0") == 0  )
		{
			return false;
		}
		return true;
	}


	int getClosureName( ndxml *closure,  ndxml*funcNode, char *output, size_t bufsize)
	{
		const char *pFuncName = ndxml_getattr_val(funcNode, "name");
		if (!pFuncName || !*pFuncName) {
			return -1;
		}
		ndxml *xmlName = ndxml_getnode(closure, "closure_name");
		if (!xmlName) {
			return -1;
		}
		const char *closureName = ndxml_getval(xmlName);
		if (!closureName || !*closureName) {
			return -1;
		}
		return ndsnprintf(output, bufsize, "%s@%s", pFuncName, closureName);
	}


	const char *getModuleName(ndxml_root *scriptXml)
	{
		ndxml *xmlModule = ndxml_getnode(scriptXml, "moduleInfo");
		if (!xmlModule) {
			return NULL;
		}
		ndxml *node = ndxml_getnode(xmlModule, "moduleName");
		if (node) {
			return ndxml_getval(node);
		}
		return NULL;
	}

	bool setModuleName(ndxml_root *scriptXml, const char *moduleName)
	{
		ndxml *xmlModule = ndxml_getnode(scriptXml, "moduleInfo");
		if (!xmlModule) {
			return false;
		}
		ndxml *node = ndxml_getnode(xmlModule, "moduleName");
		if (node) {
			ndxml_setval(node, moduleName);
		}
		else {
			ndxml *xml = ndxml_addnode(xmlModule, "moduleName", moduleName);
			if (!xml)	{
				return false;
			}
			ndxml_addattrib(xml, "kinds", "hide");
		}
		return true;
	}

	//
#define VALUE_HELPER_MARKER (char)0x5c
	int textToEdit(const char *savedText, std::string &dispText)
	{
		const char*inputText =savedText;
		if (!inputText || !*inputText) {
			return 0;
		}
		size_t old_size = dispText.size();
		while (*inputText) {
			int tmpA = 0;
			char *pOut = (char*)&tmpA;
			int len = ndstr_read_utf8char((char**)&inputText, &pOut);
			pOut = (char*)&tmpA;

			if (len == 1) {
				
				char ch = *pOut;
				if (ch == VALUE_HELPER_MARKER) {
					char ch2 = *inputText++;
					if (ch2 == 0) {
						break;
					}
					else if (ch2 == 'n') {
						dispText += (char)0x0a;
					}
					else if (ch2 == 't') {
						dispText += (char)0x09;
					}
					else if (ch2 == VALUE_HELPER_MARKER) {
						dispText += VALUE_HELPER_MARKER;
					}
				}
				else {
					dispText += pOut;
				}
			}
			else {
				dispText += pOut;
			}
		}
		return (int)(dispText.size() - old_size);
	}
	int textToSave(const char *srcFromEditor, std::string &saveText)
	{
		const char *inputText = srcFromEditor;
		if (!inputText || !*inputText) {
			return 0;
		}
		size_t old_size = saveText.size();
		while (*inputText) {
			int tmpA = 0;
			char *pOut = (char*)&tmpA;
			int len = ndstr_read_utf8char((char**)&inputText, &pOut);

			pOut = (char*)&tmpA;

			if (len == 1) {
				char ch = *pOut;
				if (ch == 0x0a) {
					saveText += VALUE_HELPER_MARKER;
					saveText += 'n';
				}
				else if (ch == 0x09) {
					saveText += VALUE_HELPER_MARKER;
					saveText += 't';
				}
				else if (ch == 0x5c) {
					saveText += VALUE_HELPER_MARKER;
					saveText += VALUE_HELPER_MARKER;
				}
				else if (ch >= 0x20) {
					saveText += pOut;
				}
			}
			else {
				saveText += pOut;
			}
		}
		return (int)(saveText.size() - old_size);
	}

}