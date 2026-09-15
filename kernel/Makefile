obj-m += nerine.o
nerine-y := src/intel_pmc_core.o

ccflags-y += -I$(src)/include -I$(src)/../include

ifneq ($(KERNELRELEASE),)
src/intel_pmc_core.o: $(src)/src/intel_pmc_core.c FORCE
	$(call if_changed_rule,cc_o_c)
endif

KERNEL_RELEASE ?= $(shell uname -r)
KERNEL_BUILD ?= /usr/lib/modules/$(KERNEL_RELEASE)/build
MODULE_BUILD ?= $(CURDIR)/../build/kernel
PYTHON ?= python3

all:
	$(MAKE) -C $(KERNEL_BUILD) M=$(CURDIR) MO=$(MODULE_BUILD) modules
	cd $(MODULE_BUILD) && $(PYTHON) $(KERNEL_BUILD)/scripts/clang-tools/gen_compile_commands.py -d $(MODULE_BUILD) -o compile_commands.json modules.order

clean:
	$(RM) -rf $(MODULE_BUILD)
