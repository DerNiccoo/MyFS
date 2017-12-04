# directory with header files
HEADERDIR = includes

# directory with source files
SRCDIR = src

# temp directory with object files
OBJDIR = obj

# c compiler flags
CFLAGS = -g -Wall -I$(HEADERDIR) -DFUSE_USE_VERSION=26 `pkg-config fuse --cflags`

# c++ compiler flags
CPPFLAGS = -std=gnu++11 -g -Wall -I$(HEADERDIR) -DFUSE_USE_VERSION=26 `pkg-config fuse --cflags`

# linker flags
LINKFLAGS = -g -Wall
#LINKFLAGS = -Wall -L/usr/local/lib -losxfuse

# libraries
LIBS = `pkg-config fuse --libs`

# all targets in project TODO: add new targets here (and add objects and link target)
TARGETS = mount.myfs mkfs.myfs

# object files for target mkfs.myfs TODO: add new object files here
MKFS_MYFS_OBJS = $(OBJDIR)/blockdevice.o \
	$(OBJDIR)/MyFSMgr.o \
	$(OBJDIR)/Logger.o \
	$(OBJDIR)/mkfs.myfs.o

# object files for target mount.myfs TODO: add new object files here
MOUNT_MYFS_OBJS = $(OBJDIR)/blockdevice.o \
	$(OBJDIR)/MyFSMgr.o \
	$(OBJDIR)/Logger.o \
	$(OBJDIR)/myfs.o \
	$(OBJDIR)/wrap.o \
	$(OBJDIR)/mount.myfs.o

# build all targets
all: $(TARGETS)

# link target mkfs.myfs
mkfs.myfs: obj $(MKFS_MYFS_OBJS)
	g++ $(LINKFLAGS) -o $@ $(MKFS_MYFS_OBJS) $(LIBS)

# link target mount.myfs
mount.myfs: obj $(MOUNT_MYFS_OBJS)
	g++ $(LINKFLAGS) -o $@ $(MOUNT_MYFS_OBJS) $(LIBS)

# clean by removing object dir
clean:
	rm -rf $(OBJDIR)

# make object directory
$(OBJDIR):
	mkdir -p $(OBJDIR)
	
# compile c++ files with header dependency
$(OBJDIR)/%.o :  $(SRCDIR)/%.cpp $(HEADERDIR)/%.h
	g++ -c $(CPPFLAGS) -o $@  $<

# compile c++ files without header dependency
$(OBJDIR)/%.o :  $(SRCDIR)/%.cpp
	g++ -c $(CPPFLAGS) -o $@  $<
	
# compile c file with header dependency	
$(OBJDIR)/%.o :  $(SRCDIR)/%.c $(HEADERDIR)/%.h
	gcc -c $(CFLAGS) -o $@  $<

# compile c file with header dependency	
$(OBJDIR)/%.o :  $(SRCDIR)/%.c
	gcc -c $(CFLAGS) -o $@  $<
