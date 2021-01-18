#make file 

SRCDIR=.
OBJDIR = ./.objs

include ../../Rules.make


##############set flag


CFLAGS += -I$(TOPDIR)/include/logic_parser 
##########make 
##############################################################


ifeq ($(DEBUG),y)
	PROJS = $(LIBDIR)/lib$(NODEFLOW_PROJ_NAME)_s_d.a
	DLLPROJS = $(LIBDIR)/lib$(NODEFLOW_PROJ_NAME)_d.$(DLL_EXT_NAME)
else 
	PROJS = $(LIBDIR)/lib$(NODEFLOW_PROJ_NAME)_s.a
	DLLPROJS = $(LIBDIR)/lib$(NODEFLOW_PROJ_NAME).$(DLL_EXT_NAME)
endif

ifeq ($(BUILD_DLL),y)
   PROJS = $(DLLPROJS)
   LOCAL_CFLAG :=  $(CFLAGS) -fPIC 

else 
   LOCAL_CFLAG :=  $(CFLAGS) 
endif


all: $(PROJS)

$(PROJS): $(OBJS)
	$(AR) $(PROJS)  $(PathOBJS)  

dll: $(DLLPROJS)
 
$(DLLPROJS): $(OBJS)
	$(CPP_DLL) -fPIC -o $(DLLPROJS)  $(PathOBJS) $(LFLAGS) -l$(COMMON_DLL)

.c.o:
	$(CC) $(INCLUDE) $(LOCAL_CFLAG) -o $(OBJDIR)/$*.o $<

.cpp.o:
	$(CPP) $(INCLUDE) $(LOCAL_CFLAG) -o $(OBJDIR)/$*.o $<
	
clean-dll:
	rm -f $(DLLPROJS) $(PathOBJS)  $(PathOBJS:.o=.d)

clean:
	rm -f $(PROJS) $(PathOBJS)  $(PathOBJS:.o=.d)

