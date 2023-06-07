
###############################################################################
# OS Detecting 
# 
# Supported os: OSX WIN32 LINUX UNKNOWN
# 
# Fill OS_DETECTED with the detected os.
# Fill COMP with the adequate compilator (Support gcc && clang)
# Append -D$(OS_DETECTED) to the CPP_CMP_FLAGS var
#

ifeq ($(OS),Windows_NT)
    OS_DETECTED		=	WIN32
	CPP_CMP_FLAGS	+=	-DWIN32
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        OS_DETECTED			=	LINUX
		CPP_CMP_FLAGS		+=	-D$(OS_DETECTED) -std=gnu11  
	    ifeq ($(LANG),C)
			COMP		=	gcc
		else ifeq ($(LANG),C++)
			COMP		=	g++
		endif
    else ifeq ($(UNAME_S),Darwin)
        OS_DETECTED 		=	OSX
		CPP_CMP_FLAGS		+=	-D$(OS_DETECTED) -std=c++11
		ifeq ($(LANG),C)
			COMP		=	clang
		else ifeq ($(LANG),C++)
			COMP		=	g++
		endif
	else
		OS_DETECTED	=	UNKNOWN
		COMP		=	gcc
    endif
endif


############################################################################################