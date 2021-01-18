
topdir=(shell pwd)
VPATH = .

# include ./Rules.make

SUBDIRS =  src

remote_port = 22
remote_host = $(MYAWS)

all: debug


debug:
	for n in $(SUBDIRS); do $(MAKE) -C $$n debug || exit 1; done

release:
	for n in $(SUBDIRS); do $(MAKE) -C $$n release || exit 1; done


dll : dll-debug

dll-debug:
	cd src/parser;  make dll DEBUG="y" PROFILE="y" BUILD_DLL="y" || exit 1 ;

dll-release:
	cd src/parser;  make dll DEBUG="n" PROFILE="n" BUILD_DLL="y" || exit 1 ;

clean-dll:
	cd src/parser; make clean-dll

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean DEBUG="n" PROFILE="n"; done
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean DEBUG="y" PROFILE="y" || exit 1; done

update:
	git commit -m "auto commit " -a; git pull

commit:
	make update ; git push


config:
	chmod u+x ./config.sh ; ./config.sh

http_debug:
	chmod u+x ./installer/gen_publis_ver.sh
	./installer/gen_publis_ver.sh ./src/nfmain/nf_version.cpp ./installer/version_id.txt < ./installer/version_id.txt
	cp ./src/nfmain/nf_version.cpp ./src/httpServer/
	cd ../ndsdk ; make clean; make 
	make clean; make 
	
http_pack:
	chmod u+x ./installer/gen_publis_ver.sh
	./installer/gen_publis_ver.sh ./src/nfmain/nf_version.cpp ./installer/version_id.txt < ./installer/version_id.txt
	cp ./src/nfmain/nf_version.cpp ./src/httpServer/
	cd ../ndsdk ; make clean; make release
	make clean; make release
	echo "make pack ....."
	chmod u+x ./installer/make_pack.sh;
	./installer/make_pack.sh ./bin nodeflow_centos < ./installer/version_id.txt

push_node :
	[ -f ./bin/nfhttp ] && scp -P $(remote_port)  ./bin/nfhttp $(remote_host):/usr/local/bin; exit 0
	[ -f ./bin/nodeflow ] && scp -P $(remote_port)  ./bin/nodeflow $(remote_host):/usr/local/bin ; exit 0
	[ -f ./bin/nfhttp_d ] && scp -P $(remote_port)  ./bin/nfhttp_d $(remote_host):/usr/local/bin/nfhttp ; exit 0
	[ -f ./bin/nodeflow_d ] && scp -P $(remote_port)  ./bin/nodeflow_d $(remote_host):/usr/local/bin/nodeflow; exit 0

#	scp -P1101  ../ndsdk/bin/linux_x86_64/monitor $(RABBIT_HOST):/usr/local/bin

