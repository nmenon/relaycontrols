/*
 * InterfacekitTest.cpp : Defines the entry point for the console application
 * Src: http://www.phidgets.com/downloads/libraries/libphidget_2.1.8.20110322.tar.gz
 * build:
 * gcc -lphidget21 -lpthread -ldl -lusb -lm -o output_file src_file
 * needs sudo permissions:
 * sudo chown root.root ~/bin/phidget-control
 * sudo chmod +s ~/bin/phidget-control
 */
#define APP_ERROR(ARGS...) {fprintf(stderr,ARGS);}

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <phidget21.h>

static int out_c = -1;
static int out_v = -1;
static int out_err = 0;
static int verbose = 0;

void display_generic_properties(CPhidgetHandle phid)
{
	int sernum, version;
	const char *deviceptr;
	CPhidget_getDeviceType(phid, &deviceptr);
	CPhidget_getSerialNumber(phid, &sernum);
	CPhidget_getDeviceVersion(phid, &version);

	printf("%s\n", deviceptr);
	printf("Version: %8d SerialNumber: %10d\n", version, sernum);
	return;
}

int IFK_DetachHandler(CPhidgetHandle IFK, void *userptr)
{
	APP_ERROR("DID YOU JUST REMOVE MY DEVICE??\n");
	return 0;
}

int IFK_ErrorHandler(CPhidgetHandle IFK, void *userptr, int ErrorCode,
		     const char *unknown)
{
	if(verbose)
		printf("Error handler ran %d!\n", ErrorCode);
	out_err = ErrorCode;
	return 0;
}

int IFK_OutputChangeHandler(CPhidgetInterfaceKitHandle IFK, void *userptr,
			    int Index, int Value)
{
	if(verbose)
		printf("Output change %d %d\n", Index, Value);
	out_c = Index;
	out_v = Value;
	return 0;
}

int control_phidget(int channel, int state, int serial_num)
{
	int err;
	CPhidgetInterfaceKitHandle IFK = 0;

	if(verbose)
		CPhidget_enableLogging(PHIDGET_LOG_VERBOSE, NULL);

	CPhidgetInterfaceKit_create(&IFK);

	CPhidgetInterfaceKit_set_OnOutputChange_Handler(IFK,
							IFK_OutputChangeHandler,
							NULL);
	CPhidget_set_OnError_Handler((CPhidgetHandle) IFK, IFK_ErrorHandler,
				     NULL);

	CPhidget_open((CPhidgetHandle) IFK, serial_num);

	/*wait 5 seconds for attach */
	if ((err =
	     CPhidget_waitForAttachment((CPhidgetHandle) IFK,
					5000)) != EPHIDGET_OK) {
		const char *errStr;
		CPhidget_getErrorDescription(err, &errStr);
		printf("Error waiting for attachment: (%d): %s\n", err, errStr);
		goto exit;
	}

	if (verbose)
		display_generic_properties((CPhidgetHandle) IFK);

	out_c = -1;
	out_v = -1;
	if (channel == 0xff) {
		int i;
		/* XXX: Should have grabbed numchannels - too lazy to check api */
		for (i = 0; i < 4; i++)
			CPhidgetInterfaceKit_setOutputState(IFK, i, state);
	} else {
		CPhidgetInterfaceKit_setOutputState(IFK, channel, state);
	}

exit:
	CPhidget_close((CPhidgetHandle) IFK);
	CPhidget_delete((CPhidgetHandle) IFK);

	if (out_err) {
		APP_ERROR("got error in op: %d\n", out_err);
		err = out_err;
	}

	if (channel != 0xff) {
		if ((out_c != channel) || (out_v != state)) {
			APP_ERROR("Did not actually change - result: channel=%d, value=%d\n",
					out_c, out_v);
			err = -1;
		}
	}

	return err;
}

void usage(char *a)
{
	printf("Usage: %s -c channel_num [-r time | [-y|-n]] [-s serial_num]\n"
		"Where:\n\t-c channel_num = channel number(use a for all)\n"
		"\t-v - verbose (debug)\n"
		"\t-s serial_num - Serial Number of the Phidget to attach to(default to -1(first available)\n"
		"\t-r time = reboot with time seconds after switch off\n"
		"\t-y -switch on\n"
		"\t-n - switch off\n", a);
}

int main(int argc, char *argv[])
{
	char *appname = argv[0];
	int channel = -1;
	int time = 0;
	int on = 0;
	int off = 0;
	int c;
	int serial_num = -1;

	while ((c = getopt(argc, argv,  "s:c:r:ynv" )) !=
			-1)
		switch (c) {
		case 'r':
			sscanf(optarg,"%d",&time);
			off = 1;
			on = 1;
			break;
		case 's':
			sscanf(optarg,"%d",&serial_num);
			break;
		case 'c':
			if (!strcmp(optarg, "a"))
				channel=0xff;
			else
				sscanf(optarg,"%d",&channel);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'y':
			on = 1;
			break;
		case 'n':
			off = 1;
			break;
		case '?':
			if ((optopt == 'r') || (optopt == 'c')) {
				APP_ERROR("Option -%c requires an argument.\n",
					  optopt)
			} else if (isprint(optopt)) {
				APP_ERROR("Unknown option `-%c'.\n", optopt)
			} else {
				APP_ERROR("Unknown option character `\\x%x'.\n",
					  optopt)
			}
			usage(appname);
			return 1;
		default:
			usage(appname);
			return -1;
		}
	if (channel == -1) {
		APP_ERROR("Need channel!\n");
		usage(appname);
		return -1;
	}
	if (!off && !on) {
		APP_ERROR("Need some state to go to for the channel\n");
		usage(appname);
		return -1;
	}

	if (off)
		control_phidget(channel, 0, serial_num);
	if (time)
		sleep(time);
	if (on)
		control_phidget(channel, 1, serial_num);

	return 0;
}
