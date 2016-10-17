# This version of the makefile seems to correctly 
# include header file dependencies, and will re-build
# .o files if any of the included .h files change
OBJS := europi.o europi_framebuffer_utils.o splash.o europi_func1.o europi_func2.o

# link
europi: $(OBJS)
	gcc $(OBJS) -Wall -Wno-trigraphs -o europi -lpigpio -lrt -lpthread 

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info;
# more complicated dependency computation, so all prereqs listed
# will also become command-less, prereq-less targets
#   sed:    strip the target (everything before colon)
#   sed:    remove any continuation backslashes
#   fmt -1: list words one per line
#   sed:    strip leading spaces
#   sed:    add trailing colons
%.o: %.c
	gcc -c $(CFLAGS) $*.c -o $*.o
	gcc -MM $(CFLAGS) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

# remove compilation products
clean:
	rm -f europi *.o *.d
	