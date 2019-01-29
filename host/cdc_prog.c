/*
 * cdc_prog.c - send fpga bitstream via USB CDC -> SPI bridge
 * 01-25-19 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define VERSION "0.1"

static void help(void) __attribute__ ((noreturn));

static void help(void)
{
	fprintf(stderr,
	    "Usage: cdc_prog [-p port][-v][-V] BITSTREAM\n"
		"  BITSTREAM is a file containing the FPGA bitstream to download\n"
		"  -p specify the CDC port (eg. /dev/ttyACM1, etc.)\n"
		"  -v enables verbose progress messages\n"
		"  -V prints the tool version\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int flags = 0, verbose = 0, sz = 0, rsz, wsz, buffs;
    char *port = "/dev/ttyACM1";
	int cdc_file;		/* CDC device */
    FILE *infile;
    char buffer[64];
    struct termios termios_p;
    
	/* handle (optional) flags first */
	while (1+flags < argc && argv[1+flags][0] == '-')
	{
		switch (argv[1+flags][1])
		{
		case 'p':
			if (2+flags < argc)
                port = argv[flags+2];;
			flags++;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'V':
			fprintf(stderr, "cdc_prog version %s\n", VERSION);
			exit(0);
		default:
			fprintf(stderr, "Error: Unsupported option "
				"\"%s\"!\n", argv[1+flags]);
			help();
		}
		flags++;
	}
    
    /* open output port */
    cdc_file = open(port, O_RDWR);
    if(cdc_file<0)
	{
		printf("Couldn't open cdc device %s\n", port);
        exit(1);
	}
	else
	{
        /* port opened so set up termios for raw binary */
        printf("opened cdc device %s\n", port);
        
        /* get TTY attribs */
        tcgetattr(cdc_file, &termios_p);
        
        /* input modes */
        termios_p.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IUCLC | INLCR| IXANY );

        /* output modes - clear giving: no post processing such as NL to CR+NL */
        termios_p.c_oflag &= ~(OPOST|OLCUC|ONLCR|OCRNL|ONLRET|OFDEL);

        /* control modes - set 8 bit chars */
        termios_p.c_cflag |= ( CS8 ) ;

        /* local modes - clear giving: echoing off, canonical off (no erase with 
        backspace, ^U,...), no extended functions, no signal chars (^Z,^C) */
        termios_p.c_lflag &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);

        termios_p.c_cflag |= CRTSCTS;	// using flow control via CTS/RTS
        
        /* set attribs */
        tcsetattr(cdc_file, 0, &termios_p);		
    }
    
    /* get bitstream file */
    if (2+flags > argc)
    {
        /* missing argument */
        fprintf(stderr, "Error: Missing bitstream file\n");
        close(cdc_file);
        help();
    }
    else
    {
        /* open file */
        if(!(infile = fopen(argv[flags + 1], "rb")))
        {
            fprintf(stderr, "Error: unable to open bitstream file %s for read\n", argv[flags + 1]);
            close(cdc_file);
            exit(1);
        }
        
        /* get length */
        fseek(infile, 0L, SEEK_END);
        sz = ftell(infile);
        fseek(infile, 0L, SEEK_SET);
        fprintf(stderr, "Opened bitstream file %s with size %d\n", argv[flags + 1], sz);
    }
    
    /* send the header */
    buffer[0] = 's';
    buffer[1] = (sz>>24)&0xff;
    buffer[2] = (sz>>16)&0xff;
    buffer[3] = (sz>>8)&0xff;
    buffer[4] = sz&0xff;
    write(cdc_file, buffer, 5);
    
    /* send the body 64 bytes at a time */
    wsz = 0;
    buffs = 0;
    while((rsz = fread(buffer, 1, 64, infile)) > 0)
    {
        write(cdc_file, buffer, rsz);
        wsz += rsz;
        buffs++;
    }
    fprintf(stderr, "Sent %d bytes, %d buffers\n", wsz, buffs);
	
	/* get result */
	read(cdc_file, buffer, 1);
	fprintf(stderr, "Result: %d\n", buffer[0]);
    
    fclose(infile);
    close(cdc_file);
    return 0;
}