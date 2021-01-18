/* file logicengine_api.h
 *
 * interface of game logic engine 
 *
 * create by duan 
 *
 * 2015-5-20
 */

#include "nd_common/nd_common.h"
#include "nd_common/nd_iconv.h"
#include "nd_crypt/nd_crypt.h"
#include "logic_parser/pluginsMgr.h"
#include "logic_parser/logicParser.h"
#include "logic_parser/logic_compile.h"
#include "logic_parser/logic_function.h"
#include "logic_parser/logicStruct.hpp"
//#include "logic_parser/dbldata2netstream.h"
#include "logic_parser/logic_editor_helper.h"
//#include "logic_parser/dbl_mgr.h"
#include "nd_common/cJSON.h"


APOLLO_SCRIPT_API_DEF(_sys_username, "sys_用户名()")
{
	result.InitSet(nd_get_sys_username());
	return true;
}

APOLLO_SCRIPT_API_DEF(_sys_get_path, "sys_获取工作目录()")
{
	result.InitSet(nd_getcwd());
	return true;
}

APOLLO_SCRIPT_API_DEF(_sys_change_path, "sys_切换目录(newpath)")
{
	CHECK_ARGS_NUM(args, 2, parser);

	std::string newPath = args[1].GetString();
	if (newPath.size() > 0) {
		std::string oldPath = nd_getcwd();
		if (0 == nd_chdir(newPath.c_str())) {
			result.InitSet(oldPath.c_str());
			return true;
		}
		return false;
	}
	return true;
}

APOLLO_SCRIPT_API_DEF(_sys_mkfile, "sys_创建文件(filepath)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);

	std::string newPath = args[1].GetString();
	if (newPath.size() > 0) {
		if (0 == nd_mkfile(newPath.c_str())) {
			return true;
		}
		parser->setErrno(NDERR_OPENFILE);
		return false;
	}
	return true;
}

APOLLO_SCRIPT_API_DEF(_sys_copyfile, "sys_复制文件(filesrc,filedest)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);

	std::string pathSrc = args[1].GetString();
	std::string pathDest = args[2].GetString();

	if (pathSrc.size() > 0 && pathDest.size() > 0) {
		if (0 == nd_cpfile(pathSrc.c_str(),pathDest.c_str() )) {
			return true;
		}
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}
	return false;

}

APOLLO_SCRIPT_API_DEF(_sys_mkdir, "sys_创建目录(pathname)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);

	std::string newPath = args[1].GetString();
	if (newPath.size() > 0) {
		if (0 == nd_mkdir(newPath.c_str())) {
			return true;
		}
		return false;
	}
	return true;
}

APOLLO_SCRIPT_API_DEF(_sys_rmfile, "sys_删除文件(filepath)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);

	std::string newPath = args[1].GetString();
	if (newPath.size() > 0) {
		if (0 == nd_rmfile(newPath.c_str())) {
			return true;
		}
		return false;
	}
	return true;
}


APOLLO_SCRIPT_API_DEF(_sys_rmdir, "sys_删除目录(path)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);

	std::string newPath = args[1].GetString();
	if (newPath.size() > 0) {
		if (0 == nd_rmdir(newPath.c_str())) {
			return true;
		}
		return false;
	}
	return true;
}


APOLLO_SCRIPT_API_DEF(_sys_getenv, "sys_getenv(env_name)")
{
	CHECK_ARGS_NUM(args, 2, parser);
	result.InitSet(getenv(args[1].GetText()));
	return true;
}



APOLLO_SCRIPT_API_DEF(_sys_write_textfile, "sys_写入文本文件(fileName, var1,var2....)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);

	std::string filename = args[1].GetString();
	if (filename.size() == 0)	{
		parser->setErrno(NDERR_INVALID_INPUT);
		return false;
	}
	FILE *pf = fopen(filename.c_str(), "w");
	if (!pf)	{
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}
	
	for (size_t i = 2; i < args.size(); i++) {
		LogicDataObj &data = args[i];
		data.Print((logic_print)ndfprintf, (void *)pf);
	}
	ndfprintf(pf, "\n");

	fclose(pf);
	return true;
}

APOLLO_SCRIPT_API_DEF(_sys_appendto_textfile, "sys_追加文本文件(fileName, var1,var2....)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);

	std::string filename = args[1].GetString();
	if (filename.size() == 0) {
		parser->setErrno(NDERR_INVALID_INPUT);
		return false;
	}
	FILE *pf = fopen(filename.c_str(), "a");
	if (!pf) {
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}

	for (size_t i = 2; i < args.size(); i++) {
		LogicDataObj &data = args[i];
		data.Print((logic_print)ndfprintf, (void *)pf);
	}
	ndfprintf(pf, "\n");

	fclose(pf);
	return true;
}
//send message api apollo_write_file(filename,var1,var2...
//bool apollo_write_file(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_write_file, "sys_写入文件(filename,var1,var2...)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);

	//CHECK_DATA_TYPE(args[2], OT_STRING);
	const char *filename = args[1].GetText();
	if (!filename || !*filename) {
		parser->setErrno(NDERR_PARAM_INVALID);

		return false;
	}
	FILE *pf = fopen(filename, "w");
	if (!pf) {
		parser->setErrno(NDERR_OPENFILE);
		return false;
	}

	for (size_t i = 2; i < args.size(); i++) {
		if (args[i].GetDataType() == OT_BINARY_DATA) {
			fwrite(args[i].GetBinary(), 1, args[i].GetBinarySize(), pf);
		}
		else {
			args[i].Print((logic_print)ndfprintf, pf);
		}
	}
	fclose(pf);
	return true;
}
APOLLO_SCRIPT_API_DEF(apollo_appendto_file, "sys_追加到文件(filename,var1,var2...)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);

	//CHECK_DATA_TYPE(args[2], OT_STRING);
	const char *filename = args[1].GetText();
	if (!filename || !*filename) {
		parser->setErrno(NDERR_PARAM_INVALID);

		return false;
	}
	FILE *pf = fopen(filename, "a");
	if (!pf) {
		parser->setErrno(NDERR_OPENFILE);
		return false;
	}

	for (size_t i = 2; i < args.size(); i++) {
		if (args[i].GetDataType() == OT_BINARY_DATA) {
			fwrite(args[i].GetBinary(), 1, args[i].GetBinarySize(), pf);
		}
		else {
			args[i].Print((logic_print)ndfprintf, pf);
		}
	}
	fclose(pf);
	return true;
}

APOLLO_SCRIPT_API_DEF(_sys_open_file_stream, "sys_OpenStream(fileName)")
{
	CHECK_ARGS_NUM(args, 3, parser);

	std::string filename = args[1].GetString();
	if (filename.size() == 0)	{
		parser->setErrno(NDERR_INVALID_INPUT);
		return false;
	}

	FILE *pf = fopen(filename.c_str(), "wb");
	if (!pf)	{
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}

	result.InitSet((void*)pf, OT_FILE_STREAM);

	return true;

}

APOLLO_SCRIPT_API_DEF(_sys_close_file_stream, "sys_CloseStream(fileStream)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);
	
	if (args[1].GetDataType() == OT_FILE_STREAM)	{
		FILE *pf = (FILE *)args[1].GetObjectAddr();
		if (pf)	{
			fclose(pf);
		}
	}

	//args[2].InitSet((int)0);
	return true;
}

APOLLO_SCRIPT_API_DEF(_sys_get_time_zone, "sys_time_zone()")
{
	result.InitSet((int)nd_time_zone());
	return true;

}


APOLLO_SCRIPT_API_DEF(_sys_textread_file_stream, "sys_TextReadLine(fileStream)")
{
	CHECK_ARGS_NUM(args, 2, parser);
	if (args[1].GetDataType() != OT_FILE_STREAM){
		parser->setErrno(NDERR_BAD_GAME_OBJECT);
		return false;
	}
	FILE *pf = (FILE*)args[1].GetObjectAddr();
	if (!pf){
		parser->setErrno(NDERR_PARAM_INVALID);
		return false;
	}
	char bufline[4096];
	bufline[0] = 0;
	if (fgets(bufline, sizeof(bufline), pf)) {
		result.InitSet(bufline);
	}
	else if (feof(pf) )	{
		bufline[0] = 0; 
		result.InitSet(bufline);
	}
	return true;

}

APOLLO_SCRIPT_API_DEF(apollo_editor_version, "compiler_version()")
{
	result.InitSet(LogicEditorHelper::getEditorVersion());
	return true;
}

APOLLO_SCRIPT_API_DEF(apollo_editor_version_index, "compiler_version_index()")
{
	result.InitSet(LogicEditorHelper::getEditorVersionId());
	return true;
}


APOLLO_SCRIPT_API_DEF(apollo_convert_to_string, "输出到字符串(inputVar)")
{
	CHECK_ARGS_NUM(args, 2, parser);
	
	if (args[1].CheckValid()) {
		std::string outStr;
		if (args[1].toStdString(outStr) > 0) {
			result.InitSet(outStr.c_str());
		}
		else {
			result.InitSet((const char*)NULL);
		}
	}
	else {
		parser->setErrno(NDERR_PARAM_INVALID);
		return false;
	}
	return true;

}


//bool apollo_str_cmp(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_str_cmp, "str字符串比较(str1,str2)")
{
	CHECK_ARGS_NUM_ONLY(args, 3, parser);
	//CHECK_DATA_TYPE(args[1], OT_STRING, parser);
	//CHECK_DATA_TYPE(args[2], OT_STRING, parser);

	const char *p1 = args[1].GetText();
	const char *p2 = args[2].GetText();
	bool ret = false;

	if (p1 && p2){
		if (ndstricmp(p1, p2) == 0) {
			ret = true;
		}
	}
	result.InitSet(ret);
	return true;
}

//"字符串查找(str1,str2)"
//bool apollo_str_str(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_str_str, "str字符串查找(str1,str2)")
{
	CHECK_ARGS_NUM_ONLY(args, 3, parser);
	//CHECK_DATA_TYPE(args[1], OT_STRING, parser);
	CHECK_DATA_TYPE(args[2], OT_STRING, parser);
	result = LogicDataObj();

	if (!args[1].CheckValid() || args[1].GetDataType() != OT_STRING) {
		return true;
	}
	const char *p1 = args[1].GetText();
	const char *p2 = args[2].GetText();

	
	if (p1 && p2){
		const char *str1 = ndstristr(p1, p2);
		if (str1) {
			result.InitSet(str1);
		}		
	}
	return true;
}

//"字符串长度(str1)"
//bool apollo_str_len(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_str_len, "str字符串长度(str1)")
{
	CHECK_ARGS_NUM_ONLY(args, 2, parser);
	//CHECK_DATA_TYPE(args[1], OT_STRING, parser);

	result.InitSet(0);
	if (!args[1].CheckValid() || args[1].GetDataType() != OT_STRING) {
		return true;
	}

	const char *p1 = args[1].GetText();	
	if (p1 && *p1){
		result.InitSet((int)ndstrlen(p1));
	}
	return true;
}

//return (str1 + str2) ;
//bool apollo_str_add(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_str_add, "str字符串相加(str1, str2...)")
{
	CHECK_ARGS_NUM(args, 2, parser);
	std::string str1 = args[1].GetString();
		
	for (size_t i = 2; i < args.size(); i++) {
		str1 += args[i].GetString();
	}

	result.InitSet(str1.c_str());
	return true;

}

//str_insert(aim_str, insert_pos, inserted_text)
//bool apollo_str_insert(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_str_insert, "str字符串插入(aim_str, insert_pos, inserted_text)")
{
	CHECK_ARGS_NUM_ONLY(args, 4, parser);
	std::string str1 = args[1].GetText();
	size_t index = args[2].GetInt();
	const char *pReplace = args[3].GetText();
	if (str1.size() == 0 || !pReplace || !*pReplace){
		return false;
	}

	str1.insert(index, pReplace);
	result.InitSet(str1.c_str());
	return true;
}

//str_insert(src,replaced_str, new_str)
//bool apollo_str_replace(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_str_replace, "str字符串替换(aim_str, replaced, new_text)")
{

	CHECK_ARGS_NUM_ONLY(args, 4, parser);
	const char*pSrc = args[1].GetText();
	const char *compStr = args[2].GetText();
	const char *pReplace = args[3].GetText();

	if (!pSrc || !*pSrc || !compStr || !*compStr){
		return false;
	}
	size_t s =  ndstrlen(compStr);

	std::string val1;
	const char *p = pSrc;
	while (p && *p ) {
		char *start = (char *)ndstristr(p, compStr);
		if (start){
			char ch = *start;
			*start = 0;

			val1 += p;

			if (pReplace && *pReplace){
				val1 += pReplace;
			}

			*start = ch;

			p = start + s;
		}
		else{
			val1 += p;
			break;
		}

	}
	result.InitSet(val1.c_str());

	return true;

}


//str_insert(src_str,erased_str)
//bool apollo_str_erase(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_str_erase, "str字符串删除(aim_str, erased_str)")
{
	CHECK_ARGS_NUM_ONLY(args, 3, parser);
	const char*pSrc = args[1].GetText();
	const char *eraseStr = args[2].GetText();

	if (!pSrc || !*pSrc || !eraseStr || !*eraseStr){
		return false;
	}
	size_t s = ndstrlen(eraseStr);

	std::string val1;
	const char *p = pSrc;
	while (p && *p) {
		char *start = (char *)ndstristr(p, eraseStr);
		if (start){
			char ch = *start;
			*start = 0;

			val1 += p;


			*start = ch;

			p = start + s;
		}
		else{
			val1 += p;
			break;
		}

	}
	result.InitSet(val1.c_str());

	return true;

}

APOLLO_SCRIPT_API_DEF(apollo_md5, "str_get_md5(inTextOrBin)")
{
	CHECK_ARGS_NUM_ONLY(args, 2, parser);
	char md5buf[33];
	md5buf[0] = 0;
 	if (args[1].GetDataType() == OT_STRING) {
		const char *pSrc = args[1].GetText();
		if (pSrc&& *pSrc) {
			MD5CryptStr32(pSrc, md5buf);
		}
	}
	else if (args[1].GetDataType() == OT_BINARY_DATA && args[1].GetBinarySize() >0) {
		MD5Crypt32((char*)args[1].GetBinary(), (int)args[1].GetBinarySize(),(char*) md5buf);
	}
	if (md5buf[0] == 0) {
		parser->setErrno(NDERR_PARAM_INVALID);
		return false;
	}
	result.InitSet(md5buf);

	return true;

}
APOLLO_SCRIPT_API_DEF(apollo_base64_encode, "str_encode_base64(inTextOrBin)")
{
	CHECK_ARGS_NUM_ONLY(args, 2, parser);
	if (args[1].GetDataType() == OT_STRING) {
		const char *pSrc = args[1].GetText();
		if (pSrc&& *pSrc) {
			size_t s = strlen(pSrc);
			char *buff = (char*) malloc(s*3);
			nd_base64_encode(pSrc,(unsigned int)s, buff);
			result.InitSet(buff);
			free(buff);
		}
	}
	else if (args[1].GetDataType() == OT_BINARY_DATA && args[1].GetBinarySize() > 0) {
		size_t s = args[1].GetBinarySize();
		char *buff = (char*)malloc(s * 3);
		nd_base64_encode((char*)args[1].GetBinary(),(unsigned int)s, buff);
		result.InitSet(buff);
		free(buff);
	}
	else {
		parser->setErrno(NDERR_PARAM_INVALID);
		return false;
	}
	return true;

}



APOLLO_SCRIPT_API_DEF(apollo_file_md5, "file_get_md5(filepath)")
{
	CHECK_ARGS_NUM_ONLY(args, 2, parser);
	char md5buf[33];

	const char*pSrc = args[1].GetText();
	if (!pSrc || !*pSrc) {
		parser->setErrno(NDERR_PARAM_INVALID);
		return false;
	}

	if (!MD5file(pSrc, md5buf)) {

		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}
	result.InitSet(md5buf);
	return true;
}


APOLLO_SCRIPT_API_DEF(apollo_encode_convert, "str_编码类型转换(strInput, fromType, toType)")
{
	CHECK_ARGS_NUM_ONLY(args, 4, parser);
	
	int fromType = nd_get_encode_val(args[2].GetText());
	int toType = nd_get_encode_val(args[3].GetText());

	result = args[1];
	if (fromType != toType) {
		result.ConvertEncode(fromType, toType);
	}
	return true;

}

APOLLO_SCRIPT_API_DEF(apollo_encode_get_script, "sys_当前函数编码()")
{	
	const char *pName = nd_get_encode_name(parser->curEncodeType());
	if (pName) {
		result.InitSet(pName);
		return true;
	}
	return false;
}
APOLLO_SCRIPT_API_DEF(apollo_encode_get_host, "sys_获得主机编码()")
{
	const char *pName = nd_get_encode_name(ND_ENCODE_TYPE);
	if (pName) {
		result.InitSet(pName);
		return true;
	}
	return false;
}

APOLLO_SCRIPT_API_DEF(apollo_encode_get_ctrl_out, "sys_控制台输出编码()")
{
	const char *pName = nd_get_encode_name(parser->getRoot()->getOutPutEncode());
	if (pName) {
		result.InitSet(pName);
		return true;
	}
	return false;
}

APOLLO_SCRIPT_API_DEF(apollo_get_author_name, "nf_获得开发者名称()")
{
	LogicEngineRoot *root = parser->getRoot();
	if (!root) {
		result.InitSet("unknown");
	}
	result.InitSet(root->getAuthor());
	return true;
}

//bool apollo_load_script_file(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_load_script_file, "nf_加载脚本(script_name)")
{
	
	CHECK_ARGS_NUM(args, 2, parser);
	CHECK_DATA_TYPE(args[1], OT_STRING, parser);
	
	LogicEngineRoot *root = parser->getRoot();
	nd_assert(root);
	if (-1== root->LoadScript(args[1].GetText(), parser ) ) {
		nd_logerror("load %s script error \n", args[1].GetText());
		parser->setErrno(NDERR_OPENFILE);
		return  false ;
	}
	return  true ;
}
APOLLO_SCRIPT_API_DEF(apollo_change_def_module, "nf_改变默认模块(module_name)")
{

	CHECK_ARGS_NUM(args, 2, parser);
	CHECK_DATA_TYPE(args[1], OT_STRING, parser);

	LogicEngineRoot *root = parser->getRoot();
	nd_assert(root);

	if (! root->setDftScriptModule(args[1].GetText())) {
		nd_logerror("set default script to %s error \n", args[1].GetText());
		parser->setErrno(NDERR_INVALID_INPUT);
		return  false;
	}
	return  true;
}


//bool apollo_export_cpp_api(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_export_cpp_api, "nf_导出c函数(filename)->noRet")
{
	
	CHECK_ARGS_NUM(args, 2, parser);
	CHECK_DATA_TYPE(args[1], OT_STRING, parser);
	
	if(-1==common_export_c_api_descript(args[1].GetText()) ) {
		nd_logerror("export cpp api to %s error \n", args[1].GetText()) ;
		return false ;
	}
	
	return  true ;
}


int common_export_c_api_descript(const char *outfile)
{
	LogicEngineRoot *logicEngine = LogicEngineRoot::get_Instant();
	nd_assert(logicEngine);
	return logicEngine->dumbCPPfunc(outfile);
}


//#include "apollo_errors.h"

int common_export_error_list(const char *outfile)
{
	ndxml_root xmlroot;
	ndxml_initroot(&xmlroot);
	ndxml_addattrib(&xmlroot, "version", "1.0");
	ndxml_addattrib(&xmlroot, "encoding", "utf8");
	ndxml *xml_funcs = ndxml_addnode(&xmlroot, LOGIC_ERROR_LIST_NAME, NULL);
	nd_assert(xml_funcs);
	if (!xml_funcs)	{
		return -1;
	}

	for (int i = 0; i < NDERR_UNKNOWN; i++) {
		char buf[16];
		ndsnprintf(buf, 16, "%d", i);
		ndxml *xmlsub = ndxml_addnode(xml_funcs, "node", buf);
		if (xmlsub)	{
			const char *pDesc = nd_error_desc(i);
#ifdef _MSC_VER
			char desc_buf[256];
			pDesc = nd_gbk_to_utf8(pDesc, desc_buf, sizeof(desc_buf));
#endif
			ndxml_addattrib(xmlsub, "desc", pDesc);
		}
	}

	for (int i = NDERR_USERDEFINE +1; i < nd_error_get_user_number(); i++) {
		char buf[16];
		ndsnprintf(buf, 16, "%d", i);
		ndxml *xmlsub = ndxml_addnode(xml_funcs, "node", buf);
		if (xmlsub)	{
			const char *pDesc = nd_error_desc(i);
#ifdef _MSC_VER
			char desc_buf[256];
			pDesc = nd_gbk_to_utf8(pDesc, desc_buf, sizeof(desc_buf));
#endif
			ndxml_addattrib(xmlsub, "desc", pDesc);
		}
	}
	return ndxml_save(&xmlroot, outfile);
}

int common_export_error_csv(const char *outfile)
{
	FILE *pf = fopen(outfile, "w");
	if (!pf) {
		nd_logerror("can not open file %s\n", outfile);
		return -1;
	}
	fprintf(pf, "id,error\n");
	for (int i = 0; i < NDERR_UNKNOWN; i++) {

		const char *pDesc = nd_error_desc(i);
// #ifdef _MSC_VER
// 		char desc_buf[256];
// 		pDesc = nd_gbk_to_utf8(pDesc, desc_buf, sizeof(desc_buf));
// #endif
		fprintf(pf, "%d,%s\n", i, pDesc);

	}

	for (int i = NDERR_USERDEFINE + 1; i < nd_error_get_user_number(); i++) {
		const char *pDesc = nd_error_desc(i);
// #ifdef _MSC_VER
// 		char desc_buf[256];
// 		pDesc = nd_gbk_to_utf8(pDesc, desc_buf, sizeof(desc_buf));
// #endif
		fprintf(pf, "%d,%s\n", i, pDesc);


	}
	fflush(pf);
	fclose(pf);
	return 0;
}
//////////////////////////////////


//change server time 
//bool apollo_func_change_time(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_func_change_time,"sys修改时间(int:add_hours, int:add_minutes)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);

	int hours = args[1].GetInt();
	int minutes = args[2].GetInt();
	time_t pretm = app_inst_time(NULL);
	app_inst_set_hm(hours, minutes);

	time_t nowtm = app_inst_time(NULL);
	char buf1[64];
	char buf2[64];
	nd_logmsg("change time from %s to %s\n", 
		nd_get_datetimestr_ex(pretm, buf1, 64), nd_get_datetimestr_ex(nowtm, buf2, 64));
	return true;
}

//bool apollo_load_file_data(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_load_file_data, "nf_读取整个文件(filename)")
{
	CHECK_ARGS_NUM(args, 2, parser);

	size_t size = 0;
	const char *fileName = args[1].GetText();
	if (!fileName || !*fileName) {
		nd_logmsg("input file name error\n");
		parser->setErrno(NDERR_PARAM_INVALID);
		return false;
	}
	void *pdata = nd_load_file(fileName, &size);
	if (!pdata) {
		nd_logmsg("file %s not open\n", fileName);

		parser->setErrno(NDERR_OPENFILE);
		return false;
	}
	result.InitSet(pdata, size, OT_BINARY_DATA);
	nd_unload_file(pdata);
	return true;
}


APOLLO_SCRIPT_API_DEF(apollo_load_file_as_string, "nf_读取文件作为字符串(filename)")
{
	CHECK_ARGS_NUM(args, 2, parser);
	
	size_t size = 0;
	const char *fileName = args[1].GetText();
	if (!fileName || !*fileName) {
		nd_logmsg("input file name error\n");
		parser->setErrno(NDERR_PARAM_INVALID);
		return false;
	}
	void *pdata = nd_load_file(fileName, &size);
	if (!pdata) {
		nd_logmsg("file %s not open\n", fileName);

		parser->setErrno(NDERR_OPENFILE);
		return false;
	}
	result.InitSet((char*)pdata);
	//result.InitSet(pdata, size, OT_BINARY_DATA);
	nd_unload_file(pdata);

	return true;

}



//bool apollo_make_full_path(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_make_full_path, "nf_合成文件名(path,filename)")
{
	char buf[ND_FILE_PATH_SIZE];
	CHECK_ARGS_NUM(args, 3, parser);

	const char *path = args[1].GetText();
	const char *file = args[2].GetText();
	if (!path || !file) {
		parser->setErrno(NDERR_PARAM_INVALID);
		nd_logerror("input file or path is null\n");
		return false;
	}
	result.InitSet(nd_full_path(path, file, buf, sizeof(buf)));
	return true;
}

#include "nd_crypt/nd_cryptfile.h"
APOLLO_SCRIPT_API_DEF(apollo_encrypt_file, "nf_cryptFile(filename,outFileName, password)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);

	const char *inFileName = args[1].GetText();
	const char *outFileName = args[2].GetText();
	if (!inFileName || !outFileName)	{
		parser->setErrno(NDERR_PARAM_INVALID);
		nd_logerror("input file or path is null\n");
		return false;
	}

	size_t size = 0;
	void *pBuf = nd_load_file(inFileName, &size);
	if (!pBuf){
		nd_logerror("load file %s error\n", inFileName);
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}
	
	NDC_FILE *pf = ndc_fopen_w(outFileName, args[3].GetText()) ;
	if (!pf){

		nd_logerror("Open file %s error\n", outFileName);
		parser->setErrno(NDERR_IO);

		nd_unload_file(pBuf);
		return false;
	}
	ndc_fwrite(pBuf, size, 1, pf);
	ndc_fclose(pf);

	nd_unload_file(pBuf);
	return true;
}

APOLLO_SCRIPT_API_DEF(apollo_decrypt_file, "nf_deyptFile(filename,outFileName, password)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);

	const char *inFileName = args[1].GetText();
	const char *outFileName = args[2].GetText();
	if (!inFileName || !outFileName)	{
		parser->setErrno(NDERR_PARAM_INVALID);
		nd_logerror("input file or path is null\n");
		return false;
	}

	size_t size = 0;
	void *pBuf = ndc_load_file_ex(inFileName, &size, args[3].GetText());
	if (!pBuf){
		nd_logerror("load file %s error\n", inFileName);
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}

	FILE *outFile = fopen(outFileName, "wb");
	if (!outFile){
		nd_logerror("Open file %s error\n", outFileName);
		parser->setErrno(NDERR_IO);

		nd_unload_file(pBuf);
		//free(buf);
		return false;
	}
	fwrite(pBuf, size, 1, outFile);
	fclose(outFile);
	//free(buf);
	nd_unload_file(pBuf);

	return true;
}


APOLLO_SCRIPT_API_DEF(apollo_dump_function, "nf_dumpFunction(funcName,outFileName)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);

	const char *funcName = args[1].GetText();
	const char *outFileName = args[2].GetText();

	if (!funcName || !outFileName) {
		parser->setErrno(NDERR_PARAM_NUMBER_ZERO);
		return false;
	}

	const char *pModule = NULL;

	const scriptCmdBuf*script = parser->getRoot()->getScript(funcName, NULL, &pModule);
	if (!script){
		parser->setErrno(NDERR_FUNCTION_NOT_FOUND);
		return false;
	}

	FILE *pf = fopen(outFileName, "wb");
	if (!pf)	{
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}
	fwrite(script->buf, script->size, 1, pf);
	fclose(pf);
	return true;
}

APOLLO_SCRIPT_API_DEF(apollo_load_json_file, "nf_加载json文件(jsonFilePath)")
{
	CHECK_ARGS_NUM(args, 2, parser);
	const char *namePath = args[1].GetText();

	if (!namePath || !*namePath) {
		parser->setErrno(NDERR_PARAM_NUMBER_ZERO);
		return false;
	}
	
	LogicUserDefStruct jsonData;
	if (jsonData.InitFromFile(namePath)) {
		if (jsonData.isArray()) {
			LogicDataObj *pData = jsonData.ref((size_t)0);
			if (pData) {
				result = *pData;
				return true;
			}
		}
		else {
			result.InitSet(jsonData);
		}
		return true;
	}

	nd_logerror("convert json data from CJSON-2-USERDefinedata error\n");
	return false;
}

APOLLO_SCRIPT_API_DEF(apollo_text_to_json, "nf_文本转成JSON(json_text)")
{
	CHECK_ARGS_NUM(args, 2, parser);
	const char *pData = args[1].GetText();
	if (!pData || !*pData) {
		parser->setErrno(NDERR_PARAM_NUMBER_ZERO);
		return false;
	}
	LogicUserDefStruct jsonData;
	if (jsonData.InitFromSTDtext(pData)) {
		if (jsonData.isArray()) {
			LogicDataObj *pData = jsonData.ref((size_t)0);
			if (pData) {
				result = *pData;
				return true;
			}
		}
		else {
			result.InitSet(jsonData);
		}
		return true;
	}

	nd_logerror("convert json data from CJSON-2-USERDefinedata error\n");
	return false;
}


static bool _readXML2Json(ndxml *root, LogicUserDefStruct &jsonData)
{
	int count = ndxml_getsub_num(root);
	for (int i = 0; i < count; i++)	{
		ndxml *node = ndxml_getnodei(root, i);
		nd_assert(node);
		const char *v = ndxml_getval(node);
		if (v && *v) {
			jsonData.push_back(ndxml_getname(node), LogicDataObj(v));
		}
		else if (ndxml_getsub_num(node) > 0) {
			LogicUserDefStruct subData;
			bool ret = _readXML2Json(node, subData);
			if (ret && subData.count()) {
				jsonData.push_back(ndxml_getname(node), LogicDataObj(subData));
			}
		}			
	}
	return true;
}

APOLLO_SCRIPT_API_DEF(apollo_read_xml_node, "nf_读取xml节点(fileName, nodePath)")
{
	CHECK_ARGS_NUM(args, 3, parser);
	const char *fileName = args[1].GetText();
	const char *nodeName = args[2].GetText();

	if (!fileName || !nodeName) {
		parser->setErrno(NDERR_PARAM_NUMBER_ZERO);
		return false;
	}
	
	ndxml_root xmlFile;
	if (ndxml_load(fileName, &xmlFile) == -1) {
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}
	ndxml *node = ndxml_recursive_ref(&xmlFile,nodeName);
	if (!node) {
		parser->setErrno(NDERR_PARAM_NOT_EXIST);
		ndxml_destroy(&xmlFile);
		return false;
	}
		
	LogicUserDefStruct jsonData;
	_readXML2Json(node, jsonData);
	ndxml_destroy(&xmlFile);
	result.InitSet(jsonData);
	return true;
}
//////////////////////////////////////
// public cpp function 

static int _apollo_log(void *pf, const char *stm, ...)
{
	char buf[1024 * 4];
	char *p = buf;
	va_list arg;
	int done;

	va_start(arg, stm);
	done = ndvsnprintf(p, sizeof(buf), stm, arg);
	va_end(arg);

	nd_logtext(buf);
	return done;
}

static bool _apollo_out(LogicParserEngine*parser,parse_arg_list_t &args, logic_print print_func, void *pf)
{
	int displayType = parser->getRoot()->getOutPutEncode();
	
	for (int i = 0; i < args.size(); i++) {
		args[i].ConvertEncode(ND_ENCODE_TYPE, displayType);
		args[i].Print(print_func, pf);
		print_func(pf, " ");
	}
	print_func(pf, "\n");

	return true;
}

bool apollo_log(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
{
	char funcNamebuf[1024];
	snprintf(funcNamebuf, sizeof(funcNamebuf), "%s %s():",  nd_get_datetimestr(), parser->curFuncName());

	args.insert(args.begin(), LogicDataObj(funcNamebuf));

	return _apollo_out(parser,args, _apollo_log, NULL);
}


bool apollo_printf(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
{
//#ifdef ND_OUT_LOG_2CTRL
	LogicEngineRoot *root = parser->getRoot();
	nd_assert(root);
	logic_print print_func = root->m_screen_out_func;
	void *pf = (void*)root->m_print_file;
	if (!print_func)	{
#ifdef ND_OUT_LOG_2CTRL
		print_func = (logic_print)ndfprintf;
		pf = (void*)stdout;
#else 
		return true;
#endif 
	}
	return _apollo_out(parser,args, print_func, pf);
//#else
//	return true;
//#endif

}

#ifdef ND_OUT_LOG_2CTRL
int _apollo_script_printf(const char *stm, ...)
{
	char buf[1024 * 4];
	char *p = buf;
	va_list arg;
	int done;

	va_start(arg, stm);
	done = ndvsnprintf(p, sizeof(buf), stm, arg);
	va_end(arg);

	apollo_logic_out_put(buf);
	return done;
}
#endif

int apollo_logic_out_put(const char *text)
{
#ifdef ND_OUT_LOG_2CTRL
	LogicEngineRoot *root = LogicEngineRoot::get_Instant();
	nd_assert(root);
	logic_print print_func = root->m_screen_out_func;
	void *pf = (void*)root->m_print_file;
	if (!print_func)	{
		print_func = (logic_print)ndfprintf;
		pf = (void*)stdout;
	}
	return print_func(pf, "%s", text);
#else 
	return 0;
#endif
}


int apollo_input_line(LogicParserEngine*parser, LogicDataObj &result)
{
	logic_console_input_func console = LogicEngineRoot::getConsoleInput();
	if (console) {
		return console(parser, result);
	}
	else {
		char buf[4096];
		buf[0] = 0;
		char *p = fgets(buf, sizeof(buf), stdin);
		if (p) {
			size_t size = strlen(p);
			if (size > 0 && p[size - 1] == '\n') {
				--size;
				p[size] = 0;
			}
			result.InitSet((const char*)p);
			return (int)size;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////

enum eApolloOperateCmd
{
	E_APOLLO_TIME_OP_GET,
	E_APOLLO_TIME_OP_ADD,
	E_APOLLO_TIME_OP_SUB,
	E_APOLLO_TIME_OP_INTERVAL
};

enum eApolloTimeUnit
{
	E_APOLLO_TIME_UNIT_SECOND,
	E_APOLLO_TIME_UNIT_MINUTE,
	E_APOLLO_TIME_UNIT_HOUR,
	E_APOLLO_TIME_UNIT_DAY,
	E_APOLLO_TIME_UNIT_MONTH,
	E_APOLLO_TIME_UNIT_YEAR,
	E_APOLLO_TIME_UNIT_WEEKDAY,

};
//time function 
bool apollo_time_func(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
{

	CHECK_ARGS_NUM(args, 4, parser);

	CHECK_DATA_TYPE(args[1], OT_INT, parser);
	CHECK_DATA_TYPE(args[2], OT_INT, parser);
	//CHECK_DATA_TYPE(args[3], OT_TIME);
	int opType = args[1].GetInt();
	int opUnit = args[2].GetInt();
	if (E_APOLLO_TIME_OP_GET == opType) {
		int val = -1;
		switch (opUnit)
		{
		case E_APOLLO_TIME_UNIT_SECOND:
			val = args[3].GetTimeSecond();
			break;
		case E_APOLLO_TIME_UNIT_MINUTE:
			val = args[3].GetTimeMinute();
			break;
		case E_APOLLO_TIME_UNIT_HOUR:
			val = args[3].GetTimeHour();
			break;
		case E_APOLLO_TIME_UNIT_DAY:
			val = args[3].GetTimeDay();
			break;
		case E_APOLLO_TIME_UNIT_MONTH:
			val = args[3].GetTimeMonth();
			break;

		case E_APOLLO_TIME_UNIT_YEAR:
			val = args[3].GetTimeYear();
			break;
		case E_APOLLO_TIME_UNIT_WEEKDAY:
			val = args[3].GetTimeWeekDay();
			break;
		default:			
			return false;
		}
		result.InitSet(val);
	}
	else if (E_APOLLO_TIME_OP_ADD == opType || E_APOLLO_TIME_OP_SUB == opType) {
		if (args.size() < 4){
			nd_logerror("apollo_time_func error need 4 args on ADD/sub\n");
			return false;
		}
		CHECK_DATA_TYPE(args[4], OT_INT, parser);
		NDUINT64 deta = args[4].GetInt();
		time_t orgtime = args[3].GetTime();

		switch (opUnit)
		{
		case E_APOLLO_TIME_UNIT_SECOND:
			break;
		case E_APOLLO_TIME_UNIT_MINUTE:
			deta *= 60;
			break;
		case E_APOLLO_TIME_UNIT_HOUR:
			deta *= 3600;
			break;
		case E_APOLLO_TIME_UNIT_DAY:
			deta *= (3600 * 24);
			break;
		case E_APOLLO_TIME_UNIT_MONTH:
		{
			tm _tm1;
			if (localtime_r(&orgtime, &_tm1)) {
				int year = (int)(deta / 12);
				int month = deta % 12;

				if (E_APOLLO_TIME_OP_ADD==opType) {
					_tm1.tm_mon += month;
					_tm1.tm_year += year;
				}
				else {
					_tm1.tm_mon -= month;
					_tm1.tm_year -= year;
				}

				//roundup month
				if (_tm1.tm_mon >= 12)	{
					_tm1.tm_year += 1;
					_tm1.tm_mon -= 12;
				}
				else if (_tm1.tm_mon < 0)	{
					_tm1.tm_year -= 1;
					_tm1.tm_mon += 12;
				}

				//round up year
				if (_tm1.tm_year < 0){
					orgtime = 0;
				}
				else {
					orgtime = mktime(&_tm1);
				}
				result.InitSet(orgtime);
				return true;
			}

			return false;
		}
		break;

		case E_APOLLO_TIME_UNIT_YEAR:
		{
			tm _tm1;
			if (localtime_r(&orgtime, &_tm1)) {
				if (E_APOLLO_TIME_OP_ADD==opType) {
					_tm1.tm_year += (int)deta;
				}
				else {
					_tm1.tm_year -= (int)deta;
				}

				if (_tm1.tm_year < 0){
					orgtime = 0;
				}
				else {
					orgtime = mktime(&_tm1);
				}
				result.InitSet(orgtime);
				return true;
			}
			return false;
		}
		break;
		case E_APOLLO_TIME_UNIT_WEEKDAY:
			deta *= 7 * 3600 * 24;
			break;
		default:
			return false;
		}
		if (opType == E_APOLLO_TIME_OP_ADD)	{
			orgtime += deta;
		}
		else {
			orgtime += deta;
		}
		result.InitSet(orgtime);
	}
	else if (E_APOLLO_TIME_OP_INTERVAL) {
		CHECK_DATA_TYPE(args[4], OT_TIME, parser);
		time_t tim1 = args[3].GetTime(), tim2 = args[4].GetTime();
		if (tim1 < tim2){
			time_t _tmp = tim1;
			tim1 = tim2;
			tim2 = _tmp;
		}

		int val = 0;
		switch (opUnit)
		{
		case E_APOLLO_TIME_UNIT_SECOND:
			val = (int)(tim1 - tim2);
			break;
		case E_APOLLO_TIME_UNIT_MINUTE:
			val = (int)(tim1 / 60 - tim2 / 60);
			break;
		case E_APOLLO_TIME_UNIT_HOUR:
			val = (int)(tim1 / 3600 - tim2 / 3600);
			break;
		case E_APOLLO_TIME_UNIT_DAY:
			val = nd_time_day_interval(tim1, tim2);
			break;

		case E_APOLLO_TIME_UNIT_MONTH:
		{
			tm _tm1, _tm2;
			if (localtime_r(&tim1, &_tm1) && localtime_r(&tim2, &_tm2)) {
				NDUINT64 month1 = _tm1.tm_year * 12 + _tm1.tm_mon;
				NDUINT64 month2 = _tm2.tm_year * 12 + _tm2.tm_mon;
				val = (int)(month1 - month2);
			}
			else {
				return false;
			}
		}
		break;

		case E_APOLLO_TIME_UNIT_YEAR:
		{
			tm _tm1, _tm2;
			if (localtime_r(&tim1, &_tm1) && localtime_r(&tim2, &_tm2)) {
				//NDUINT64 month1 = _tm1.tm_year * 12 + _tm1.tm_mon;
				//NDUINT64 month2 = _tm2.tm_year * 12 + _tm2.tm_mon;
				val = _tm1.tm_year - _tm2.tm_year;
			}
			else {
				return false;
			}
		}
		break;

		case E_APOLLO_TIME_UNIT_WEEKDAY:
		{
			int timezone = nd_time_zone();
			tim1 += timezone * 3600;
			tim2 += timezone * 3600; // set localtime as gmtime

			tim1 += 4 * 3600 * 24;
			tim2 += 4 * 3600 * 24;  // 1970.1.1 is Thursday
			val = (int)(tim1 / (3600 * 24 * 7) - tim2 / (3600 * 24 * 7)); //get the week index of time1 and time2
		}

		break;
		default:
			return false;
		}
		result.InitSet(val);
	}
	else {
		return false;
	}
	return true;
}


static int compileProject(const char *projectMainFile)
{
	//begin compile
	ndxml_root xmlScript;
	ndxml_initroot(&xmlScript);
	if (-1 == ndxml_load_ex(projectMainFile, &xmlScript, "utf8")) {
		return -1;
	}
	
	ndxml *moduleInfo = ndxml_getnode(&xmlScript, "moduleInfo");
	if (!moduleInfo) {
		ndxml_destroy(&xmlScript);
		return -1;
	}
	
	ndxml *node = ndxml_getnode(moduleInfo, "out_file");
	if (!node) {
		ndxml_destroy(&xmlScript);
		return -1;
	}
	const char *ofile = ndxml_getval(node);
	if (!ofile) {
		ndxml_destroy(&xmlScript);
		return -1;
	}
	std::string outFileName = ofile;

	bool isDebug = false;
	ndxml_getnode(moduleInfo, "script_with_debug");
	if (node) {
		isDebug= ndxml_getval_int(node) ? true : false;
	}

	int encodeType = ND_ENCODE_UTF8 ;
	ndxml_getnode(moduleInfo, "script_out_encode");
	if (node) {
		encodeType = ndxml_getval_int(node);
	}

	ndxml_destroy(&xmlScript);

	LogicCompiler &lgcompile = *LogicCompiler::get_Instant();

	if (!lgcompile.compileXml(projectMainFile, outFileName.c_str(), encodeType, isDebug, ND_L_ENDIAN)) {

		const char *pFunc = lgcompile.m_cur_function.c_str();
		const char *pStep = lgcompile.m_cur_step.c_str();

		nd_logerror("compile error file %s, function %s, step %s , stepindex %d\n",
			lgcompile.m_cur_file.c_str(), pFunc, pStep, lgcompile.m_cur_node_index);

		return -1;
	}
	return 0;
}

APOLLO_SCRIPT_API_DEF(apollo_tool_compile_project, "nf_compile_project(project_path)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);

	const char *pFilePath = args[1].GetText();
	if (!pFilePath) {
		parser->setErrno(NDERR_INVALID_INPUT);
		return false;
	}

	char absFilePath[ND_FILE_PATH_SIZE];
	char absPath[ND_FILE_PATH_SIZE];
	if (!nd_absolute_filename(pFilePath, absFilePath, sizeof(absFilePath))) {
		parser->setErrno(NDERR_FILE_NOT_EXIST);
		return false;
	}
	absPath[0] = 0;
	nd_getpath(absFilePath, absPath, sizeof(absPath));

	const char *mainFile = nd_filename(absFilePath);
	if (!mainFile) {
		parser->setErrno(NDERR_INVALID_INPUT);
		return false;
	}

	std::string workingPath = nd_getcwd();
	nd_chdir(absPath);

	if (-1 == compileProject(mainFile)) {
		parser->setErrno(NDERR_BAD_FILE);

		nd_chdir(workingPath.c_str());
		return false;
		
	}
	nd_chdir(workingPath.c_str());
	return true;
}
APOLLO_SCRIPT_API_DEF(apollo_tool_compile_file, "nf_compile_file(file,outFile,isDebug)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);

	const char *pFilePath = args[1].GetText();
	if (!pFilePath) {
		parser->setErrno(NDERR_INVALID_INPUT);
		return false;
	}
	const char *outFile = args[2].GetText();
	if (!pFilePath) {
		parser->setErrno(NDERR_INVALID_INPUT);
		return false;
	}
	bool IsDebug = false;
	if (args.size() >= 4) {
		IsDebug = args[3].GetBool();
	}
	

	LogicCompiler &lgcompile = *LogicCompiler::get_Instant();

	if (!lgcompile.compileXml(pFilePath, outFile,ND_ENCODE_UTF8, IsDebug, ND_L_ENDIAN)) {

		const char *pFunc = lgcompile.m_cur_function.c_str();
		const char *pStep = lgcompile.m_cur_step.c_str();
		nd_logerror("compile error file %s, function %s, step %s , stepindex %d\n",
			lgcompile.m_cur_file.c_str(), pFunc, pStep, lgcompile.m_cur_node_index);

		parser->setErrno(NDERR_BAD_FILE);
		return false;
	}
	return true;
}

//install event handler
//bool apollo_func_install_event_handler(LogicParserEngine*parser, parse_arg_list_t &args, LogicDataObj &result)
APOLLO_SCRIPT_API_DEF(apollo_func_install_event_handler, "nf_安装全局事件处理器(str:function, int:event_id)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);
	CHECK_DATA_TYPE(args[1], OT_STRING, parser);
	CHECK_DATA_TYPE(args[2], OT_INT, parser);

	LogicEngineRoot *logicEngine = parser->getRoot();
	logicEngine->installEvent(args[2].GetInt(), args[1].GetText());
	return true;
}

APOLLO_SCRIPT_API_DEF(apollo_load_plugin, "nf_plugin_load(str:name, str:path)->noRet")
{
	CHECK_ARGS_NUM(args, 3, parser);
	CHECK_DATA_TYPE(args[1], OT_STRING, parser);
	CHECK_DATA_TYPE(args[2], OT_STRING, parser);


	PluginsMgr *plgMgr = PluginsMgr::get_Instant();
	return plgMgr->load(args[1].GetText(), args[2].GetText());

// 	LogicEngineRoot *logicEngine = parser->getRoot();
// 	return logicEngine->getPlugin().load(args[1].GetText(), args[2].GetText());
}

APOLLO_SCRIPT_API_DEF(apollo_unload_plugin, "nf_plugin_destroy(str:fileName)->noRet")
{
	CHECK_ARGS_NUM(args, 2, parser);
	CHECK_DATA_TYPE(args[1], OT_STRING, parser);


	PluginsMgr *plgMgr = PluginsMgr::get_Instant();
	return plgMgr->unLoadPlugin(args[1].GetText());

// 	LogicEngineRoot *logicEngine = parser->getRoot();
// 	return logicEngine->getPlugin().unLoadPlugin(args[1].GetText());
}

