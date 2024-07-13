# Define the compiler and flags
CC = gcc
CFLAGS = -Wall -O2

# Define the libraries to link against
LIBS = -lmicrohttpd -lpthread -ldl

# Define the target executable
TARGET = 2048server

# Define the source files
SRCS = c2048.c

# Define the object files
OBJS = $(SRCS:.c=.o)

# Installation directories
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib/2048server
SYSTEMD_DIR = /etc/systemd/system

# The default target
all: $(TARGET)

# Rule to link the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Rule to compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to install the executable and set up the service
install: all

	systemctl stop $(TARGET)
	# Create necessary directories
	mkdir -p $(BINDIR)
	mkdir -p $(LIBDIR)
	
	# Copy the executable and necessary files
	cp $(TARGET) $(BINDIR)/$(TARGET)
	cp -r lib $(LIBDIR)
	cp -r data $(LIBDIR)
	cp cert.pem $(LIBDIR)
	cp privkey.pem $(LIBDIR)
	
	# Create the systemd service file
	echo "[Unit]" > $(SYSTEMD_DIR)/$(TARGET).service
	echo "Description=2048 AI Server" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "After=network.target" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "[Service]" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "ExecStart=$(BINDIR)/$(TARGET)" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "WorkingDirectory=$(LIBDIR)" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "Restart=always" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "User=$(USER)" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "Environment=LD_LIBRARY_PATH=$(LIBDIR)" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "StandardOutput=journal" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "StandardError=journal" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "[Install]" >> $(SYSTEMD_DIR)/$(TARGET).service
	echo "WantedBy=multi-user.target" >> $(SYSTEMD_DIR)/$(TARGET).service

start: all	
	# Reload systemd configuration, enable, and start the service
	systemctl daemon-reload
	systemctl enable $(TARGET)
	systemctl start $(TARGET)

# Rule to clean the build directory
clean:
	rm -f $(OBJS) $(TARGET)
