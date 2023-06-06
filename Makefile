.PHONY: clean fclean re all check_sources check_headers

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
SRCS_SERVER		=	main.cpp Taskmaster.cpp Tintin_reporter.cpp Config.cpp Job.cpp 
SRCS_CLIENT		=	main.cpp
 
HEADERS			=	Taskmaster.hpp Tintin_reporter.hpp ntos.hpp Config.hpp Job.hpp tm_values.hpp


CPP_DBG_FLAGS		=	#-g3 -fsanitize=address
CPP_CMP_FLAGS		=	-Wextra -Wall -Werror


SRC_FILES		=	$(shell find $(SRC_DIR) | grep -E '$(shell echo $(SRCS_SERVER) | tr ' ' '|')')
HEADER_FILES	=	$(shell find $(INC_DIR) | grep -E '$(shell echo $(HEADERS) | tr ' ' '|')')
OBJS			=	$(addprefix $(BIN_DIR)/, $(SRC_FILES:.cpp=.o))
CPP_INC_FLAGS	+=	$(addprefix -I,$(shell echo $(HEADER_FILES) | tr ' ' '\n' | rev | cut -d'/' -f2- | rev | sort | uniq))
C_LFLAG			+=	$(addprefix -L,$(addprefix $(LIB_DIR), $(LIBRARIES)))

# include prefix definitions
include fancyPrefix.mk

#	include OS detection
include detectOs.mk

#	include libraries variables
include libraries.mk

#   Main rule
all: display_os $(LIB_DIR) $(YAML_LIB) $(ARGPARSE_LIB) check_sources check_headers $(NAME)
	@ echo "$(PREFIX_INFO) all done."

# include display_os
include displayOs.mk

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




#	./lib folder creation
$(LIB_DIR): 
	mkdir -p $(LIB_DIR)




# YAML library

$(YAML_LIB): 
	@ echo "$(PREFIX_INFO) Installation de la lib YAML..."
	@ wget -q https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-$(YAML_LIB_VERSION).tar.gz -O $(LIB_DIR)/yaml.gz
	@ cd $(LIB_DIR) \
		&& tar -xf yaml.gz \
		&& mv yaml-cpp-yaml-cpp-$(YAML_LIB_VERSION) yaml
	@ cd $(YAML_LIB_PATH) \
		&& mkdir -p build \
		&& cd build \
		&& cmake .. \
		&& make
# && make install
	@ rm -rf $(LIB_DIR)/yaml.gz
	@ echo "$(PREFIX_INFO) Installation de la lib YAML done !"

uninstall_yaml_library:
	@ echo "$(PREFIX_INFO) Desinstallation de la lib YAML..."
# Remove everything from make install 
# rm -rf /usr/local/lib/libyaml-cpp.a /usr/local/lib/libyaml-0.2.dylib /usr/local/lib/libyaml-cpp.a /usr/local/lib/libyaml.a /usr/local/lib/libyaml.dylib /usr/local/lib/libyaml.la /usr/local/include/yaml-cpp /usr/local/include/yaml.h /usr/local/share/cmake/yaml-cpp /usr/local/lib/cmake/GTest /usr/local/share/pkgconfig/yaml-cpp.pc /usr/local/include/gmock /usr/local/lib/libgmock.a /usr/local/lib/libgmock_main.a /usr/local/lib/pkgconfig/yaml-0.1.pc /usr/local/lib/pkgconfig/gmock.pc /usr/local/lib/pkgconfig/gmock_main.pc /usr/local/lib/libgtest.a /usr/local/lib/pkgconfig/gtest.pc /usr/local/lib/pkgconfig/gtest_main.pc /usr/local/include/gtest /usr/local/lib/libgtest_main.a
	@ rm -rf $(YAML_LIB_PATH)
	@ echo "$(PREFIX_INFO) Desinstallation de la lib YAML done !"






# OpenSSL library

$(OPENSSL_LIB_PATH): 
	@ echo "$(PREFIX_INFO) Installation de la lib OpenSSL..."
	@ mkdir -p $(OPENSSL_LIB_PATH)
	@ echo "$(PREFIX_WARN) Installation de la lib OpenSSL not implmented !"
#@echo "$(PREFIX_INFO) Installation de la lib OpenSSL done !"

uninstall_openssl_library:
	@ echo "$(PREFIX_INFO) Desinstallation de la lib OpenSSL..."
	@ echo "$(PREFIX_WARN) Desinstallation de la lib OpenSSL not implmented !"
# @ echo "$(PREFIX_INFO) Desinstallation de la lib OpenSSL done !"





# Install cpp_argparse.hpp
$(ARGPARSE_LIB):
	@ echo "$(PREFIX_INFO) Installation de la lib header cpp_argparse.hpp ..."
	@ git clone https://gist.github.com/Ludrak/eb3f0483b863bbda8eb4fa8cfe283aac $@

uninstall_argparse:
	@ rm -f ./lib/cpp_argparse.hpp
	@ echo "$(PREFIX_INFO) Desinstallation de la lib header cpp_argparse.hpp ..."






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
fclean: clean 
	@ echo "$(PREFIX_CLEAN) Cleaning $(NAME)"
	@ rm -f $(NAME)

fcleanlib: fclean uninstall_yaml_library uninstall_argparse
	@ echo "$(PREFIX_CLEAN) Cleaning $(LIB_DIR)/"
	@ rm -rf $(LIB_DIR)
	@ echo "Cleaned all libraries."

# remake rule
re: fclean all

# make re with lib
relib: fcleanlib all

