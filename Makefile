BUILD_DIRS = lib bin

all: FORCE | $(BUILD_DIRS)
	@echo Making $@
	$(MAKE) -C src

$(BUILD_DIRS):
	@echo Making $@
	mkdir $@

clean: FORCE | $(BUILD_DIRS)
	@echo Making $@
	$(MAKE) -C src $@ || exit 1;\

FORCE:

