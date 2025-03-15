typedef int BOOL;
typedef unsigned char UCHAR;

#define VOID void

#define FALSE 0
#define TRUE 1

#define False 0
#define True 1

#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <stdbool.h>


void list_devices(void);
int GetDeviceCollections();

static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
snd_pcm_t *	playhandle = NULL;
snd_pcm_t *	rechandle = NULL;

char text[256] ;

typedef struct {
	int iotype ;
	char * name ;
	char * description ;
} device_info_t ;

device_info_t ** AllDeviceList = NULL ;
int AllDeviceCount = 0 ;

void free_dev_record( device_info_t * dr ) {
	if( dr != NULL ) {
		if( dr->name != NULL ) free( dr->name );
		if( dr->description != NULL ) free( dr->description );
		free( dr );
	}
}

void destroy_device_list() {
	if( AllDeviceList == NULL ) return ;
	while( AllDeviceCount > 0 ) {
		AllDeviceCount-- ;
		free_dev_record( AllDeviceList[AllDeviceCount] );
	}
	free( AllDeviceList );
	AllDeviceList = NULL;
}

device_info_t * new_device ( int id, char* iotype, char* name, char* description ) {
	int iotype_code = -1 ;
	device_info_t * dr = NULL ;
	if( strcmp( iotype, "Input") ) {
		iotype_code = 1 ;
	}
	else if( strcmp( iotype, "Output") ) {
		iotype_code = 0 ;
	}
	if( true || iotype_code >= 0 ) {
		sprintf( text, "Type %s (%d), Name = %s", iotype, iotype_code, name );
		printf( "%s\n", text );
		dr = malloc( sizeof(device_info_t) );
		dr->iotype = iotype_code ;
		dr->name = strdup(name);
		dr->description = strdup( description );
	}
	return dr ;
}

device_info_t ** add_new_device( device_info_t * d ) {
	if( d != NULL ) {
		AllDeviceCount++ ;
		AllDeviceList = realloc( AllDeviceList, sizeof( device_info_t ) * (AllDeviceCount+1) );
		AllDeviceList[ AllDeviceCount-1 ] = d ;
		AllDeviceList[ AllDeviceCount ] = NULL ;
	}
}

int main( int argc, char *argv[]) {

	printf("Debug: started the process\n");
	printf("Debug: number of devices before rescan = %d\n", AllDeviceCount );
	destroy_device_list();
	list_devices();
	destroy_device_list();
	printf("Debug: end of process\n");
	GetDeviceCollections();
}

void list_devices(void) {
	void **h, **n ;
	char *name, *descr, *io ;
	if (snd_device_name_hint(-1, "pcm", &h) < 0) {
		printf("No sound devices found\n");
		return;
	}
	n = h;
	int index =  0 ;
	const char * filter = stream == SND_PCM_STREAM_CAPTURE ? "Input" : "Output";
	while (*n != NULL) {
		name = snd_device_name_get_hint(*n, "NAME");
		descr = snd_device_name_get_hint(*n, "DESC");
		io = snd_device_name_get_hint(*n, "IOID");

		printf("*%2d: (%s) %s\n", index++, io, name);
		if (descr != NULL) {
			char * descr1 = descr ;
			printf(" -   ");
			while (*descr1) {
				if (*descr1 == '\n')
					printf("\n    ");
				else
					putchar(*descr1);
				descr1++;
			}
			putchar('\n');
		}
      	if (name != NULL)
	      		free(name);
		if (descr != NULL)
			free(descr);
		if (io != NULL)
			free(io);
		n++;
	}
	snd_device_name_free_hint(h);
}

int GetDeviceCollections() {
	snd_pcm_stream_t streamList[2] = { SND_PCM_STREAM_PLAYBACK, SND_PCM_STREAM_CAPTURE };
	snd_ctl_t *handle= NULL; // control handle
	snd_pcm_t *pcm= NULL;    // PCM device handle
	snd_ctl_card_info_t *info_ptr;
	snd_pcm_info_t *pcminfo_ptr;
	snd_pcm_hw_params_t *pars_ptr;
	snd_pcm_format_mask_t *fmask_ptr;

	char devId[ 64 ] = {'\0'}  ;
	char devName[ 80 ] = {'\0'} ;
	char NameString[256];

	// Get Device List from ALSA
	snd_ctl_card_info_alloca(&info_ptr);
	snd_pcm_info_alloca(&pcminfo_ptr);
	snd_pcm_hw_params_alloca(&pars_ptr);
	snd_pcm_format_mask_alloca(&fmask_ptr);

	char hwdev[80];
	unsigned int min, max, ratemin, ratemax;
	int card, err, dev, nsubd;

	card = -1 ;

	do {
		err = snd_card_next( &card ) ;
		if( err==0 && card >= 0 ) { // get next card in the list
			sprintf(hwdev, "hw:%d", card);
			printf("Device (card): %s\n", hwdev);
			err = snd_ctl_open(&handle, hwdev, 0);
			if(err<0) {
				printf("Error %d: Cannot get handle for '%s'\n", err, hwdev);
			}
			err = snd_ctl_card_info(handle, info_ptr);
			if(err<0) {
				printf( "Error %d: cannot get info pointer for '%s'\n", err, hwdev);
			}
			else if( err >= 0) {
				strncpy(devId, snd_ctl_card_info_get_id(info_ptr),63);
				strncpy(devName, snd_ctl_card_info_get_name(info_ptr),79);
				printf("   ID: %s\n   Name: %s\n",  devId, devName );
				// Devices list for the current card
				dev = -1 ;
				do {
					snd_ctl_pcm_next_device(handle, &dev);
					if( dev >= 0 ) {
						snd_pcm_info_set_device(pcminfo_ptr, dev); // get PCM info structure
						snd_pcm_stream_t stream ;
						/* Loop to browse through input and output subdevices
							output first, then input
						*/
						for( int i = 0; i < 2; i++ ) {
							stream = streamList[i] ;
							char * iotype = (stream == SND_PCM_STREAM_PLAYBACK ? "output" : "input") ;
							snd_pcm_info_set_stream(pcminfo_ptr, stream);
							snd_pcm_info_set_subdevice(pcminfo_ptr, 0);
							err= snd_ctl_pcm_info(handle, pcminfo_ptr);
							if( err < 0 ) { // device not found
								printf("   >   Device #%d not found for %s\n", dev, iotype );
								continue ; // go next loop round
							}
							
							printf("   > Now listing %s subdevices:\n", iotype) ;

							unsigned int subDevCount = snd_pcm_info_get_subdevices_count(pcminfo_ptr);
							unsigned int subDevAvailable = snd_pcm_info_get_subdevices_avail(pcminfo_ptr);
							printf("   > Device #%d has %d %s subdevices (%d available)\n", 
								dev, subDevCount, iotype, subDevAvailable);
							/* get name for device open */
							sprintf(hwdev, "hw:%d,%d", card, dev);
							err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);
							if( err == 0 ) {
								/* get all device parameters */
								err = snd_pcm_hw_params_any(pcm, pars_ptr);
								snd_pcm_hw_params_get_channels_min(pars_ptr, &min);
								snd_pcm_hw_params_get_channels_max(pars_ptr, &max);
								snd_pcm_hw_params_get_rate_min(pars_ptr, &ratemin, NULL);
								snd_pcm_hw_params_get_rate_max(pars_ptr, &ratemax, NULL);
								/* TODO: display all parameters */
								if( min == max ) {
									printf( "   - %u channel%s\n", min , (min == 1) ? "" : "s" );
								}
								else {
									printf("   - minimum %u channel%s, maximum %u channels\n", min, (min == 1) ? "" : "s" , max );
								}
								printf("   - sampling rate %u ... %u Hz\n", ratemin, ratemax);
								snd_pcm_close( pcm );
							}
							else {
								printf(" > Error %d: cannot open device %s\n", err, hwdev) ;
							}
						}
					}
				} while (dev >= 0) ;
			}
			snd_ctl_close( handle ); // close the handle when done
		}
	} while ( card >= 0 );
	return 0;
}