/* file main_tmpl.cpp
 *
 * main.cpp template file 
 *
 * create by duan 
 *
 * 2018.4.3
 */

#include "nd_common/nd_common.h"

#include "logic_parser/logic_compile.h"
#include "logic_parser/logic_debugger.h"
#include "logic_parser/logicEngineRoot.h"
#include "logic_parser/logicApi4c.h"

//static TestLogicObject *__apoOwner;

static int logic_instant_init(void *scriptData, size_t dataLength)
{
	if (-1== nd_common_init()) {
		return -1;
	}
	LogicEngineRoot *scriptRoot = LogicEngineRoot::get_Instant();
	nd_assert(scriptRoot);
	scriptRoot->setOutPutEncode(ND_ENCODE_TYPE);

	scriptRoot->setPrint((logic_print)ndfprintf, stdout);
	scriptRoot->getGlobalParser().setSimulate(false);

	if (0 != scriptRoot->LoadFromBuf(scriptData, dataLength,NULL)) {
		ndfprintf(stderr, "load script error \n");
		return -1;
	}
	return 0;
}

static int logic_instant_destroy()
{
	LogicEngineRoot *scriptRoot = LogicEngineRoot::get_Instant();
	scriptRoot->getGlobalParser().eventNtf(LOGIC_EVENT_SHUTDOWN, 0);
	LogicEngineRoot::destroy_Instant();
	return 0;
}

static int script_run(int arc, const char *argv[])
{
	const char **myargAddrs = new const char *[arc];
	myargAddrs[0] = "main";
	for (int i = 1; i < arc; i++) {
		myargAddrs[i] = argv[i];
	}

	LogicEngineRoot *scriptRoot = LogicEngineRoot::get_Instant();
	scriptRoot->getGlobalParser().eventNtf(LOGIC_EVENT_START, 0);

	int ret = scriptRoot->getGlobalParser().runCmdline(arc, myargAddrs, ND_ENCODE_TYPE);
	delete[] myargAddrs;

	if (ret &&  scriptRoot->getGlobalParser().getErrno() != NDERR_WOULD_BLOCK) {
		ndfprintf(stderr, "run function %s error : %d \n", argv[0], ret);
		return -1;
	}
	return 0;
}

int logic_script_main_entry(int argc, const char *argv[],void *data, size_t data_size)
{
	if (-1 == logic_instant_init(data, data_size)) {
		ndfprintf(stderr, "init script engine error\n");
		return -1;
	}

	if (-1 == script_run(argc, argv)) {
		ndfprintf(stderr, "init script engine error\n");
		logic_instant_destroy();
		return 1;
	}
	logic_instant_destroy();
	return 0;
}

