/* file pluginsMgr.cpp
 *
 * plugins manager, include dll-base-plugin and script-base-plugin
 *
 * create by duan
 *
 * 2018.12.7
 */

#include "logic_parser/pluginsMgr.h"
#include "logic_parser/logicEngineRoot.h"



static const char *_getTextFromScript(const char *scripText, char *buf, size_t size, int encodeType)
{
	//LogicEngineRoot *root = LogicEngineRoot::get_Instant();
	//int funcEncodeType = root->getGlobalParser().curEncodeType();
	LogicDataObj dataTmp(scripText);

	buf[0] = 0;
    if (ND_ENCODE_TYPE != encodeType) {
        if (dataTmp.ConvertEncode(ND_ENCODE_TYPE, encodeType)){
            dataTmp.GetVal(buf, size);
        }
    }
    else {
        strncpy(buf, scripText, size);
    }
	
	return buf;
}

#define _GET_FUNCNAME(_src) _getTextFromScript(_src,inputBuf, size,encodeType) 

PluginsMgr::PluginsMgr():m_root()
{
}

PluginsMgr::~PluginsMgr()
{
}

PluginsMgr *PluginsMgr::get_Instant()
{
	return NDSingleton<PluginsMgr>::Get();
}

void PluginsMgr::destroy_Instant()
{
	NDSingleton<PluginsMgr>::Destroy();
}

int PluginsMgr::Create(const char *name)
{
	m_root.Create(name);

	return 0;
}
void PluginsMgr::Destroy()
{
	UnloadAll();
	m_root.Destroy();
}

std::string PluginsMgr::pluginFileName(const char *pluginConfigName)
{
	const char *extName = nd_file_ext_name(pluginConfigName);
	if (extName && 0 == ndstricmp(extName, "nf")) {
		return  std::string(pluginConfigName);
	}
	else {
		char filePath[256];
#ifdef __ND_WIN__
		ndsnprintf(filePath, sizeof(filePath), "%s.dll", pluginConfigName);

#elif defined(__ND_MAC__)
		ndsnprintf(filePath, sizeof(filePath), "lib%s.dylib", pluginName);
#else
		ndsnprintf(filePath, sizeof(filePath), "lib%s.so", pluginName);
#endif
		return std::string(filePath);
	}

}


std::string PluginsMgr::pluginName(const char *inputFileName)
{
	const char *extName = nd_file_ext_name(inputFileName);
	if (extName && 0 == ndstricmp(extName, "nf")) {
		return  std::string(inputFileName);
	}
	else {
		char filePath[256];
		nd_file_name_without_ext(inputFileName, filePath, sizeof(filePath));
#ifdef __ND_WIN__
		return std::string(filePath);

#else
		if (0 == strncmp(filePath, "lib", 3)) {
			return std::string(&filePath[3]);
		}
		else {
			return std::string(filePath);
		}
#endif
	}

}

bool PluginsMgr::findPlugin(const char *filename)
{
	plugin_map::iterator it = m_plugins.find(filename);
	if (it != m_plugins.end()) {
		return true;
	}

	it = m_sysPlugins.find(filename);
	if (it != m_sysPlugins.end()) {
		return true;
	}

	for (plugin_script_vct::iterator it = m_scriptPlugins.begin(); it != m_scriptPlugins.end(); ++it) {
		if (strcmp(it->fileName.c_str(), filename) == 0) {
			return true;
		}
	}

	for (plugin_script_vct::iterator it = m_scriptSysPlugins.begin(); it != m_scriptSysPlugins.end(); ++it) {
		if (strcmp(it->fileName.c_str(), filename) == 0) {
			return true;
		}
	}
	return false;
}



const char* PluginsMgr::load(const char*fileName, const char *pLocaltion, bool isSystem)
{
	const char *retname = NULL;
	char filePath[ND_FILE_PATH_SIZE];
	const char *extName = nd_file_ext_name(fileName);
	if (extName && 0 == ndstricmp(extName, "nf")) {
		nd_full_path(pLocaltion, fileName, filePath, sizeof(filePath));
		retname = loadScriptPlugin(fileName, filePath, isSystem);
	}
	else {
#ifdef __ND_WIN__
		ndsnprintf(filePath, sizeof(filePath), "%s/%s.dll", pLocaltion, fileName);

#elif defined(__ND_MAC__)
		ndsnprintf(filePath, sizeof(filePath), "%s/lib%s.dylib", pLocaltion, pluginName);
#else
		ndsnprintf(filePath, sizeof(filePath), "%s/lib%s.so", pLocaltion, pluginName);
#endif
		retname = loadPlugin(fileName, filePath, isSystem);
	}

	if (!retname) {
		nd_logerror("load %s plugin error\n", fileName);
	}
	return retname;
}

//nodeflow_plugin_desc

const char* PluginsMgr::getDesc(const char* fileName, char *inputBuf, size_t size, int encodeType )
{

	plugin_map::iterator it = m_plugins.find(fileName);
	if (it != m_plugins.end()) {
		logic_plugin_name namefunc = (logic_plugin_name)nd_dll_entry(it->second, "nodeflow_plugin_desc");
		if (namefunc) {
			return _GET_FUNCNAME(namefunc());
		}
		return NULL;
	}

	it = m_sysPlugins.find(fileName);
	if (it != m_sysPlugins.end()) {
		logic_plugin_name namefunc = (logic_plugin_name)nd_dll_entry(it->second, "nodeflow_plugin_desc");
		if (namefunc) {
			return _GET_FUNCNAME(namefunc());
		}
		return NULL;
	}

	LogicDataObj result;
	LogicEngineRoot *root = &m_root;

	for (plugin_script_vct::iterator it = m_scriptPlugins.begin(); it != m_scriptPlugins.end(); ++it) {
		if (strcmp(it->fileName.c_str(), fileName) == 0) {

			std::string initFunction = it->modulename.c_str();
			initFunction += ".plugin_desc";
			if (root->getGlobalParser().runScript(ND_ENCODE_ANSI, initFunction.c_str(), result, 0)) {

				return _GET_FUNCNAME(result.GetText());
				//return result.GetString();
			}

			return NULL;
		}
	}

	for (plugin_script_vct::iterator it = m_scriptSysPlugins.begin(); it != m_scriptSysPlugins.end(); ++it) {
		if (strcmp(it->fileName.c_str(), fileName) == 0) {

			std::string initFunction = it->modulename.c_str();
			initFunction += ".plugin_desc";
			if (root->getGlobalParser().runScript(ND_ENCODE_ANSI, initFunction.c_str(), result, 0)) {

				return _GET_FUNCNAME(result.GetText());
				//return result.GetString();
			}

			return NULL;
		}
	}

	return NULL;
}


bool PluginsMgr::runPlugin(const char *name)
{
	plugin_map::iterator it = m_plugins.find(name);
	if (it != m_plugins.end()) {
		logic_plugin_run namefunc = (logic_plugin_run)nd_dll_entry(it->second, "nodeflow_plugin_run");
		if (namefunc) {
			namefunc();
			return true;
		}
		return false;
	}

	it = m_sysPlugins.find(name);
	if (it != m_sysPlugins.end()) {
		logic_plugin_run namefunc = (logic_plugin_run)nd_dll_entry(it->second, "nodeflow_plugin_run");
		if (namefunc) {
			namefunc();
			return true;
		}
		return false;
	}

	LogicDataObj result;
	LogicEngineRoot *root = &m_root;

	for (plugin_script_vct::iterator it = m_scriptPlugins.begin(); it != m_scriptPlugins.end(); ++it) {
		if (strcmp(it->pluginName.c_str(), name) == 0) {

			std::string initFunction = it->modulename;
			initFunction += ".plugin_run";
			if (root->getGlobalParser().runScript(ND_ENCODE_ANSI, initFunction.c_str(), result, 0)) {
				return true;
			}
			return false;
		}
	}

	for (plugin_script_vct::iterator it = m_scriptSysPlugins.begin(); it != m_scriptSysPlugins.end(); ++it) {
		if (strcmp(it->pluginName.c_str(), name) == 0) {

			std::string initFunction = it->modulename;
			initFunction += ".plugin_run";
			if (root->getGlobalParser().runScript(ND_ENCODE_ANSI, initFunction.c_str(), result, 0)) {
				return true;
			}
			return false;
		}
	}
	return false;
}

const char* PluginsMgr::loadPlugin(const char *pluginName, const char *pluginPath, bool isSystem)
{
	if (!pluginName || !pluginPath) {
		nd_logerror("load dll:input name error\n");
		return NULL;
	}
	if (findPlugin(pluginName)) {
		nd_logerror("load dll:%s already loaded\n", pluginName);
		return NULL;
	}

	plugin_map &plugins = isSystem ? m_sysPlugins : m_plugins;

	HINSTANCE hinst = nd_dll_load(pluginPath);
	if (!hinst) {
		nd_logerror("load dll:%s\n", nd_last_error());
		return NULL;
	}
	
	logic_plugin_init_func initfunc = (logic_plugin_init_func)nd_dll_entry(hinst, "nodeflow_plugin_init");
	if (initfunc) {
		if (-1 == initfunc()) {
			nd_dll_unload(hinst);
			nd_logerror("called function:%s\n", "nodeflow_plugin_init");
			return NULL;
		}
	}
	std::pair< plugin_map::iterator, bool > ret =  plugins.insert(std::make_pair(pluginName, hinst));
	if (ret.second == true) {
		
		logic_plugin_name nameFunc = (logic_plugin_name)nd_dll_entry(hinst, "nodeflow_plugin_name");
		if (nameFunc) {
			return nameFunc();
		}
	}
	return NULL;
}

bool PluginsMgr::unLoadPlugin(const char *name)
{

	plugin_map::iterator it = m_plugins.find(name);
	if (it != m_plugins.end()) {
		logic_plugin_init_func initfunc = (logic_plugin_init_func)nd_dll_entry(it->second, "nodeflow_plugin_destroy");
		if (initfunc) {
			initfunc();
		}
		nd_dll_unload(it->second);
		m_plugins.erase(it);
		return true;
	}

	it = m_sysPlugins.find(name);
	if (it != m_sysPlugins.end()) {
		logic_plugin_init_func initfunc = (logic_plugin_init_func)nd_dll_entry(it->second, "nodeflow_plugin_destroy");
		if (initfunc) {
			initfunc();
		}
		nd_dll_unload(it->second);
		m_sysPlugins.erase(it);
		return true;
	}

	LogicDataObj result;
	LogicEngineRoot *root = &m_root;

	for (plugin_script_vct::iterator it = m_scriptPlugins.begin(); it != m_scriptPlugins.end(); ++it) {
		if (strcmp(it->fileName.c_str(), name) == 0) {

			std::string initFunction =it->modulename;
			initFunction += ".plugin_destroy";
			root->getGlobalParser().runScript(ND_ENCODE_ANSI, initFunction.c_str(), result, 0);
			root->unloadModule(name);
			m_scriptPlugins.erase(it);

			return true;
		}
	}

	for (plugin_script_vct::iterator it = m_scriptSysPlugins.begin(); it != m_scriptSysPlugins.end(); ++it) {
		if (strcmp(it->fileName.c_str(), name) == 0) {

			std::string initFunction = it->modulename;
			initFunction += ".plugin_destroy";
			root->getGlobalParser().runScript(ND_ENCODE_ANSI, initFunction.c_str(), result, 0);
			root->unloadModule(name);
			m_scriptSysPlugins.erase(it);
			return true;
		}
	}
	return false;

}


const PluginsMgr::script_plugin_t* PluginsMgr::getPluginInfo(const char *moduleName)
{

	for (plugin_script_vct::iterator it = m_scriptPlugins.begin(); it != m_scriptPlugins.end(); ++it) {
		if (strcmp(it->modulename.c_str(), moduleName) == 0) {
			return &(*it);
		}
	}

	for (plugin_script_vct::iterator it = m_scriptSysPlugins.begin(); it != m_scriptSysPlugins.end(); ++it) {
		if (strcmp(it->modulename.c_str(), moduleName) == 0) {
			return &(*it);
		}
	}
	return NULL;
}

void PluginsMgr::UnloadAll()
{
	DestroyPlugins(false);
	DestroyPlugins(true);
	m_root.UnloadAllModules();
}

void PluginsMgr::DestroyPlugins(bool isSystem)
{
	plugin_map &plugins = isSystem ? m_sysPlugins : m_plugins;

	for (plugin_map::iterator it = plugins.begin(); it != plugins.end(); ++it) {
		logic_plugin_init_func initfunc = (logic_plugin_init_func)nd_dll_entry(it->second, "nodeflow_plugin_destroy");
		if (initfunc) {
			initfunc();
		}
		nd_dll_unload(it->second);
	}
	plugins.clear();

	plugin_script_vct &scriptPlugin = isSystem? m_scriptSysPlugins: m_scriptPlugins;
	LogicEngineRoot *root = &m_root;

	LogicDataObj result;
	for (plugin_script_vct::iterator it = scriptPlugin.begin(); it != scriptPlugin.end(); it++) {
		std::string initFunction = it->modulename;
		initFunction += ".plugin_destroy";
		root->getGlobalParser().runScript(ND_ENCODE_ANSI, initFunction.c_str(), result, 0);
		root->unloadModule(it->modulename.c_str());
	}
	scriptPlugin.clear();
}


const char* PluginsMgr::loadScriptPlugin(const char *pluginName, const char *pluginPath, bool isSystem )
{
	if (findPlugin(pluginName)) {
		//already load this plugin
		return NULL;
	}

	const char *filePath = pluginPath;

	LogicEngineRoot *root = &m_root;

	if (-1 == root->LoadScript(filePath,&root->getGlobalParser())) {
		return NULL;
	}
	const char *moduleName =  root->getLastLoadModule();

	PluginsMgr::script_plugin_t pluginInfo;

	pluginInfo.fileName = pluginName;
	pluginInfo.modulename = moduleName;


	std::string runFunction = moduleName;
	runFunction += ".plugin_init";
	LogicDataObj result;
	if (!root->getGlobalParser().runScript(ND_ENCODE_ANSI, runFunction.c_str(), result, 0)) {
		root->unloadModule(moduleName);
		return NULL;
	}

	runFunction = moduleName;
	runFunction += ".plugin_name";
	if (root->getGlobalParser().runScript(ND_ENCODE_ANSI, runFunction.c_str(), result, 0)) {
		result.ConvertEncode(ND_ENCODE_TYPE, ND_ENCODE_UTF8);
		pluginInfo.pluginName = result.GetString();
	}
	else {
		pluginInfo.pluginName = moduleName;
	}

	plugin_script_vct &scriptPlugin = isSystem ? m_scriptSysPlugins : m_scriptPlugins;

	scriptPlugin.push_back(pluginInfo);
	return scriptPlugin.rbegin()->pluginName.c_str();
}
