###############################################################################
# OS display 
# 
# Displays variables obtained by detectOs.mk
#

display_os:
ifeq ($(OS_DETECTED),WIN32)
	@echo "$(PREFIX_ERROR) This project cannot be compiled on windows"
	@exit 1
else ifeq ($(OS_DETECTED), UNKNOWN)
	@echo "$(PREFIX_ERROR) Unknown OS detected. Aborting."
	@exit 1
else
	@echo "$(PREFIX_INFO) OS detected: $(OS_DETECTED)"
endif