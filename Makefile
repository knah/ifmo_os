SUBDIRS = lib cat filter revwords delwords bufcat
.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	make -C $@

clean:
	for sd in $(SUBDIRS); do \
		make -C $$sd clean; \
	done