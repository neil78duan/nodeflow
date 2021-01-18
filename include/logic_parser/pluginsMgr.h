/* file pluginsMgr.h
 *
 * plugins manager, include dll-base-plugin and script-base-plugin
 *
 * create by duan
 *
 * 2018.12.7
 */

#ifndef _PLUGINS_MGR_H_
#define _PLUGINS_MGR_H_

#include "nd_common/nd_common.h"
#include "logic_parser/logicApi4c.h"
#include "logic_parser/logicEngineRoot.h"
#include <map>
#include <string>
#include <vector>

typedef int(*logic_plugin_init_func)();
typedef const char* (*logic_plugin_name)();
typedef void(*logic_plugin_run)();

// 
// struct menu_func_info
// {
// 	char name[64];
// 	logic_plugin_menu_func func;
// };
// struct menu_func_info_script
// {
// 	char name[64];
// 	char funcname[64];
// };

//typedef menu_func_info * (*get_plugin_menu) (int *);

class LOGIC_PARSER_CLASS PluginsMgr
{
public:
	PluginsMgr();
	~PluginsMgr();

	static PluginsMgr *get_Instant();
	static void destroy_Instant();

	int Create(const char *name);
	void Destroy();

	struct script_plugin_t
	{
		script_plugin_t(const char *file, const char* name, const char*mod) :pluginName(name), fileName(file), modulename(mod)
		{

		}
		script_plugin_t() {}
		std::string pluginName;
		std::string fileName;
		std::string modulename;
	};
	//load plugin , return the plugin's name
	const char* load(const char*fileName, const char *pLocaltion,  bool isSystem = false);  //return plugin display name

	bool unLoadPlugin(const char *pluginName);
	const script_plugin_t* getPluginInfo(const char *moduleName);
	void DestroyPlugins(bool isSystem = false);
	void UnloadAll();

	const char* getDesc(const char* fileName, char *inputBuf, size_t size, int encodeType=ND_ENCODE_UTF8);
	bool runPlugin(const char *pluginName);

	LogicEngineRoot& getRoot() { return  m_root; }

	static std::string pluginFileName(const char *pluginConfigName);
	static std::string pluginName(const char *inputFileName);

	typedef std::map<std::string, HINSTANCE> plugin_map;
	typedef std::vector<script_plugin_t> plugin_script_vct;
private:

	const char * loadPlugin(const char *pluginName, const char *pluginPath, bool isSystem = false);
	const char* loadScriptPlugin(const char *pluginFile, const char *pluginPath, bool isSystem = false);

	bool findPlugin(const char *fileName);

	plugin_map m_plugins;
	plugin_map m_sysPlugins;

	plugin_script_vct m_scriptPlugins;
	plugin_script_vct m_scriptSysPlugins;

	LogicEngineRoot m_root;

};

#endif
