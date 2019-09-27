SUB_DIRS = libvidcapture app
#BUILD_DIRS = ../lib ../bin

all: $(SUB_DIRS)
FORCE:

#$(SUB_DIRS): FORCE | $(BUILD_DIRS)
$(SUB_DIRS): FORCE
	@echo Making $@
	$(MAKE) -C $@

#$(BUILD_DIRS):
#	@echo Making $@
#	mkdir $@

#clean: FORCE | $(BUILD_DIRS)
clean: FORCE
	@echo Making $@
	@for i in $(SUB_DIRS); do \
		$(MAKE) -C $$i $@ || exit 1;\
	done
