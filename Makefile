.PHONY: clean fclean re all os check_sources check_headers

#######################################################################
#
# Makefile for C projects
# (OS detect)
# Version: 3
#
#######################################################################

# Name of target executable
NAME		= taskmaster
LANG		= C++

# Locations 
SRC_DIR		= srcs
INC_DIR		= includes
BIN_DIR		= bin
LIB_DIR		= lib
LOG_DIR		= logs


# Sources & Headers 
# - fill only with name of the file
# - make will check for the file in SRC_DIR
# - use "-" if empty
SRCS_SERVER		=	main.cpp Tintin_reporter.cpp
SRCS_CLIENT		=	main.cpp
 
HEADERS			=	taskmaster.hpp Tintin_reporter.hpp ntos.hpp

LIBRARIES	=	argparse


# LIBFT_PATH		=	$(LIB_DIR)/libft
# ARGPARSE_PATH	=	$(LIB_DIR)/argparse

CPP_DBG_FLAGS		=	#-g3 -fsanitize=address
CPP_CMP_FLAGS		=	-Wextra -Wall -Werror

# CPP_INC_FLAGS		=	-I $(INC_DIR) -I$(LIB_DIR)/argparse 
# CPP_INC_FLAGS	=	-I $(INC_DIR) -I$(LIB_DIR)/argparse -I$(LIB_DIR)/libft/includes
# CPP_LNK_FLAGS		=	-L$(LIB_DIR)/argparse -largparse
# CPP_LNK_FLAGS	=	-L $(LIB_DIR)/libft -L$(LIB_DIR)/argparse -lft -largparse



SRC_FILES	=	$(shell find $(SRC_DIR) | grep -E '$(shell echo $(SRCS_SERVER) | tr ' ' '|')')
HEADER_FILES=	$(shell find $(INC_DIR) | grep -E '$(shell echo $(HEADERS) | tr ' ' '|')')
OBJS		=	$(addprefix $(BIN_DIR)/, $(SRC_FILES:.cpp=.o))
CPP_INC_FLAGS	+=	$(addprefix -I,$(shell echo $(HEADER_FILES) | tr ' ' '\n' | rev | cut -d'/' -f2- | rev | sort | uniq))
C_LFLAG		+=	$(addprefix -L,$(addprefix $(LIB_DIR), $(LIBRARIES)))

# include prefix definitions
include fancyPrefix.mk

#	include OS detection
include detectOs.mk

#   Main rule
all: gitinit yaml openssl display_os comp_lib check_sources check_headers $(NAME)
	@ echo "$(PREFIX_INFO) all done."

#	check_sources :
#	simple bash script to check duplicates sources files 
check_sources:
	@ duplicates=$$( echo $(SRC_FILES) | tr ' ' '\n' | rev | cut -d'/' -f1 | rev | sort | uniq -c | sed 's/^ *//g' | sed 's/ /./g' ) ; \
	error=0 ; \
	for source in $$duplicates ; do \
			if [ $$(echo $$source | cut -d '.' -f 1 | tr -d '\n' ) -gt 1 ] ; then \
				echo "$(PREFIX_DUPL)" Duplicates source files found for \"$$(echo $$source | cut -d '.' -f2- )\" in: ; \
				find $(SRC_DIR) | grep $$(echo $$source | cut -d '.' -f 2 | tr -d '\n' ); \
				error=1 ; \
			fi \
	done ; \
	if [ $$error -eq 1 ] ; then \
		echo "$(PREFIX_ERROR) Cannot manage duplicates files, aborting..." ; \
		exit 1 ; \
	fi

#	check_headers :
#	simple bash script to check duplicates header files 
check_headers:
	@ duplicates=$$( echo $(HEADER_FILES) | tr ' ' '\n' | rev | cut -d'/' -f1 | rev | sort | uniq -c | sed 's/^ *//g' | sed 's/ /./g' ) ; \
	error=0 ; \
	for source in $$duplicates ; do \
			if [ $$(echo $$source | cut -d '.' -f 1 | tr -d '\n' ) -gt 1 ] ; then \
				echo "$(PREFIX_DUPL)" Duplicates header files found for \"$$(echo $$source | cut -d '.' -f2- )\" in: ; \
				find $(INC_DIR) | grep $$(echo $$source | cut -d '.' -f 2 | tr -d '\n' ); \
				error=1 ; \
			fi \
	done ; \
	if [ $$error -eq 1 ] ; then \
		echo "$(PREFIX_ERROR) Cannot manage duplicates files, aborting..." ; \
		exit 1 ; \
	fi

#	LIBRARIES

# YAML library

YAML_LIB_VERSION	=	0.2.5
YAML_LIB_PATH 		=	lib/yaml/
CPP_LNK_FLAGS		+=	-lyaml -L $(YAML_LIB_PATH)

install_yaml_library:
	@echo "$(PREFIX_INFO) Installation de la lib YAML..."
	@mkdir -p $(YAML_LIB_PATH)
	@cd $(YAML_LIB_PATH) \
		&& wget https://pyyaml.org/download/libyaml/yaml-$(YAML_LIB_VERSION).tar.gz -O yaml.tar.gz $(VOIDED) \
		&& tar -xvf yaml.tar.gz $(VOIDED) \
		&& cd yaml-$(YAML_LIB_VERSION) \
		&& ./configure $(VOIDED) \
		&& make $(VOIDED) \
		&& make install $(VOIDED)
	@echo "$(PREFIX_INFO) Installation de la lib YAML done !"

uninstall_yaml_library:
	@echo "$(PREFIX_INFO) Desinstallation de la lib YAML..."
	@ cd $(YAML_LIB_PATH) \
		&& cd yaml-$(YAML_LIB_VERSION) \
		&& make uninstall $(VOIDED)
	@ rm -rf $(YAML_LIB_PATH)
	@ echo "$(PREFIX_INFO) Desinstallation de la lib YAML done !"

yaml: install_yaml_library

# OpenSSL library

# Openssl
OPENSSL_LIB_VERSION		=	1.1
ifeq ($(OS_DETECTED),LINUX)
	OPENSSL_LIB_PATH	=	/usr/
else ifeq ($(UNAME_S),Darwin)
	OPENSSL_LIB_PATH		=	$(shell brew --prefix openssl@$(OPENSSL_VERSION))
endif
# CPP_LNK_FLAGS			+=	-lssl -lcrypto -L $(OPENSSL_LIB_PATH)/lib/
# CPP_INC_IFLAGS			+=	-I $(OPENSSL_LIB_PATH)/include

install_openssl_library:
	@ echo "$(PREFIX_INFO) Installation de la lib OpenSSL..."
	@ mkdir -p $(OPENSSL_LIB_PATH)
	@ echo "$(PREFIX_WARN) Installation de la lib OpenSSL not implmented !"
#@echo "$(PREFIX_INFO) Installation de la lib OpenSSL done !"

uninstall_openssl_library:
	@ echo "$(PREFIX_INFO) Desinstallation de la lib OpenSSL..."
	@ echo "$(PREFIX_WARN) Desinstallation de la lib OpenSSL not implmented !"
# @ echo "$(PREFIX_INFO) Desinstallation de la lib OpenSSL done !"

openssl: install_openssl_library

comp_lib:
	@ echo "$(PREFIX_INFO) Compiling libraries..."
	@ for lib in $(LIBRARIES) ; do \
		echo "$(PREFIX_INFO) library $$lib done."; \
		make -C $(LIB_DIR)/$$lib ; \
	done

#	Clean of libraries
fclean_lib:
	@ echo "$(PREFIX_CLEAN) Cleaning libraries."
	@ for lib in $(LIBRARIES) ; do \
		echo "$(PREFIX_CLEAN) Cleaning library $$lib"; \
		make -C $(LIB_DIR)/$$lib fclean; \
	done

#	Bin directory
$(BIN_DIR):
	@ echo "$(PREFIX_WARN) No bin dir found. Creating one."
	@ mkdir -p $(BIN_DIR)

#	Linking rule
$(NAME): $(BIN_DIR) $(OBJS)
	 $(COMP) $(CPP_CMP_FLAGS) $(CPP_DBG_FLAGS) $(OBJS) -o $(NAME) $(CPP_LNK_FLAGS)
	@ echo "$(PREFIX_LINK) Linking done for: $(NAME)"


# Compilation rule 
$(BIN_DIR)/$(SRC_DIR)/%.o : $(SRC_DIR)/%.cpp $(HEADER_FILES)
	@ mkdir -p $(BIN_DIR)/$(shell dirname $<)
	@ $(COMP) $(CPP_CMP_FLAGS) $(CPP_DBG_FLAGS) $(CPP_INC_FLAGS) -c $< -o $@  
	@ echo "$(PREFIX_COMP) Compiled: $(shell basename $<)"

# clean rule
clean:
	@ echo "$(PREFIX_CLEAN) Cleaning $(BIN_DIR)/"
	@ rm -rf $(BIN_DIR)

# final clean rule
fclean: fclean_lib clean
	@ echo "$(PREFIX_CLEAN) Cleaning $(LIB_DIR)/"
	@ rm -rf $(LIB_DIR)
	@ echo "$(PREFIX_CLEAN) Cleaning $(NAME)"
	@ rm -f $(NAME)

# remake rule
re: fclean all

# git submodule initialisation

gitinit:
	@mkdir -p $(LIBFT_PATH)
	@find $(LIBFT_PATH) -maxdepth 0 -empty -type d -exec printf "$(PREFIX_WARN) The library directory is empty.\n$(PREFIX_WARN) Cloning library 'libft' into {}.\n" \; -exec git clone "https://github.com/NATHAN76543217/libft.git" {} \;
# @mkdir -p $(ARGPARSE_PATH)
# @find $(ARGPARSE_PATH) -maxdepth 0 -empty -type d -exec printf "$(PREFIX_WARN) The library directory is empty. Cloning library 'argparse' into {}" \; -exec git clone "https://github.com/NATHAN76543217/argparse.git" {} \;

