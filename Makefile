BINARIES:= hub-ctrl phidget-control

all: $(BINARIES)

clean:
	rm $(BINARIES)

hub-ctrl: hub-ctrl.c
	$(CC) -lusb -o hub-ctrl hub-ctrl.c

phidget-control: phidget-control.c
	$(CC) -lphidget21 -lpthread -ldl -lusb -lm -o phidget-control phidget-control.c
