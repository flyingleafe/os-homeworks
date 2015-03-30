EXECUTABLES=$(shell echo */)

.PHONY: all
all:
	@$(foreach ex, $(EXECUTABLES), $(MAKE) -C $(ex);)

.PHONY: clean
clean:
	@$(foreach ex, $(EXECUTABLES), $(MAKE) -C $(ex) clean;)
