/* file nf_main.cpp
 *
 * main function of node flow 
 *
 * create by duan 
 *
 * 2018.11.19
 */

#include <stdlib.h>
#include <stdio.h>

#include "nd_common/nd_common.h"
#include "logic_parser/logicApi4c.h"

extern const char *__nf_version_desc ;

int parse_cmdline(int argc, const char *argv[])
{
	if(argc <= 1) {
		ndfprintf(stdout, "usage : nodeflow script_file_name.nf [args ...]\n");
		return 1 ;
	}
	if (ndstrcmp(argv[1], "--help") == 0 || ndstrcmp(argv[1], "-h")==0) {
		ndfprintf(stdout, "usage : nodeflow script_file_name.nf [args ...]\n");
		return 1;
	}
	if (ndstrcmp(argv[1], "--version") == 0 || ndstrcmp(argv[1], "-v")==0) {
		ndfprintf(stdout, "%s \n", __nf_version_desc);
		return 1;
	}
	
	
	if (argc < 2) {
		ndfprintf(stderr, "usage : nodeflow script_file_name.nf [args ...]\n");
		exit(1);
	}
	if (!nd_existfile(argv[1])) {

		ndfprintf(stderr, "file %s not exist\n", argv[1]);
		exit(1);
	}
	return 0;
}

int main(int argc, const char *argv[])
{
	//parse command line
	if (parse_cmdline(argc, argv)) {
		exit(0);
	}

	size_t size = 0;
	void *pdata = nd_load_file(argv[1], &size);
	if (!pdata) {
		nd_logerror("can not open file %s\n", argv[1]);
		return -1;
	}

	int ret = logic_script_main_entry(argc-1, &argv[1], pdata, size);
	nd_unload_file(pdata);

	return ret;
}
