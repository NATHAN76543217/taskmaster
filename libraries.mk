
#lib yaml
YAML_LIB_PATH 		= $(LIB_DIR)/yaml
YAML_LIB			= $(YAML_LIB_PATH)/build/libyaml-cpp.a
YAML_LIB_VERSION	=	0.7.0
CPP_INC_FLAGS		+=	-I $(YAML_LIB_PATH)/include
CPP_LNK_FLAGS		+=	-L $(YAML_LIB_PATH)/build -lyaml-cpp 


#lib Openssl
OPENSSL_LIB_VERSION		=	1.1
ifeq ($(OS_DETECTED),LINUX)
	OPENSSL_LIB_PATH	=	/usr/
else ifeq ($(shell uname),Darwin)
	OPENSSL_LIB_PATH		=	$(shell brew --prefix openssl@$(OPENSSL_LIB_VERSION))
endif
# CPP_LNK_FLAGS			+=	-lssl -lcrypto -L $(OPENSSL_LIB_PATH)/lib/
# CPP_INC_IFLAGS			+=	-I $(OPENSSL_LIB_PATH)/include


#lib cpp_argparse
ARGPARSE_LIB 		= $(LIB_DIR)/argparse
CPP_INC_FLAGS		+=	-I $(ARGPARSE_LIB)
