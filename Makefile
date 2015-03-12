EXECUTABLES=cat revwords

all: 
	@$(foreach ex, $(EXECUTABLES), $(MAKE) -C $(ex);)

clean:
	@$(foreach ex, $(EXECUTABLES), $(MAKE) -C $(ex) clean;)

