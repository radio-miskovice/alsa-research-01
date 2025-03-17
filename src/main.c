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


#include "main.h"

const char * STREAM_OUTPUT = "Output" ;
const char * STREAM_INPUT = "Input" ;

char text[256] ;

int rows, cols, info, status ; 

CollectionItem ** InputCollection = NULL ;
CollectionItem ** OutputCollection = NULL ;

void InitializeCollections() {
	if(InputCollection == NULL) {
		InputCollection = realloc( InputCollection, sizeof(CollectionItem *) );
		InputCollection[ 0 ] = NULL ;
	}
	if(OutputCollection == NULL) {
		OutputCollection = realloc( OutputCollection, sizeof(CollectionItem *) );
		OutputCollection[ 0 ] = NULL ;
	}
}

void AppendCollectionItem ( CollectionItem *** collection, CollectionItem * item ) {
	int length = 0;
	// find count of collection items
	while( (*collection)[length++] != NULL ) ;
	/* replace NULL in the last element by the new item */
	(*collection)[length-1] = item ;  
	/* add space for one more item */
	*collection = realloc( *collection, (length+1) * sizeof( CollectionItem * ) ); 
	/* set new stop mark */
	(*collection)[length] = NULL ; 
}

void FreeItem ( CollectionItem * item ) {
	if( item != NULL ) {
		free( item->name );
		free( item->description );
	}
	free( item );
}

void FreeCollection( CollectionItem ** collection ) {
	CollectionItem ** c = collection ;
	while( *c != NULL ) {
		FreeItem(*c) ;
		c++ ;
	}
	free(collection);
}

void showItemContent( CollectionItem * item ) {
	printf("\nDevice: %s\n", item->name );
	printf("Description: %s\n", item->description );
	printf("Channels (min, max):\t%u\t%u\n", item->minChannels, item->maxChannels);
	printf("Rate (min, max):\t%u\t%u\n", item->minRate, item->maxRate);
}

void showCollection( CollectionItem ** collection ) {
	CollectionItem ** c = collection ;
	printf("Collection ptr %lx\n", c);
	int i = 0 ;
	while( *c != NULL ) {
		printf( "[%d] ", i++ );
		showItemContent( *c );
		c++ ;
	}
}

int main( int argc, char *argv[]) {
	int line ;
	InitializeCollections();
	GetDeviceCollections();
	puts("Debug: end of name hints scan");
	for( int i = 0; InputCollection[i] != NULL; i++ ) {
		showItemContent( InputCollection[i] );
	}
	// showCollection( InputCollection );
	for( int i = 0; OutputCollection[i] != NULL; i++ ) {
		showItemContent( OutputCollection[i] );
	}
	// showCollection( OutputCollection );
	FreeCollection( InputCollection);
	FreeCollection( OutputCollection);
}

void GetDeviceCollections(void) {
	void **h, **n ;
	char *name, *descr, *io ;
	int err ;
	snd_pcm_stream_t stream ;
	snd_pcm_t * pcm ;
	snd_pcm_hw_params_t * params_ptr ;
	unsigned int minChannels = 0, maxChannels = 0, minRate = 0, maxRate = 0 ;

	char firstline[256] ;
	char deviceLine[256] ;
	deviceLine[255] = '\0';

	if (snd_device_name_hint(-1, "pcm", &h) < 0) {
		printf("No sound devices found\n");
		return;
	}
	n = h;
	int index =  0 ;

	snd_pcm_hw_params_alloca(&params_ptr);

	while (*n != NULL) {
		name = snd_device_name_get_hint(*n, "NAME");
		descr = snd_device_name_get_hint(*n, "DESC");
		io = snd_device_name_get_hint(*n, "IOID");
		if( strchr( name, ':') != NULL && io != NULL 
				&& strncmp(name, "surround", strlen("surround")) != 0 ) {
			if( strcmp(io, STREAM_OUTPUT) == 0) {
				stream = SND_PCM_STREAM_PLAYBACK ;
			}
			else if( strcmp(io, STREAM_INPUT) == 0) {
				stream = SND_PCM_STREAM_CAPTURE ;
			}
			/* here the value of stream could hypothetically be undefined
			   under very special circumstances. Practically that should 
			   never happen. Can we assign -1 to an enum?
			*/
			printf(" %2d: %s\t (%s) ", index++, name, io);
			/** copy only the first line of the description **/

			char * eol ;
			eol = strchr(descr, '\n');
			if(eol != NULL ) *eol = '\0' ;
			strcpy( firstline, descr );
			if( eol != NULL ) *eol = '\n' ;
			/** first line copied **/

			/* Here we open the device by name */

			err = snd_pcm_open( &pcm, name, stream, 0 );
			if( err != 0 ) {
				printf("ERROR %d: could not open device %s by name for %s!\n", err, name, io);
			}
			else {
				printf("Test open OK.");

				err = snd_pcm_hw_params_any(pcm, params_ptr);
				
				printf( "\nGot params, err = %d", err );
				snd_pcm_hw_params_get_channels_min(params_ptr, &minChannels);
				snd_pcm_hw_params_get_channels_max(params_ptr, &maxChannels);
				snd_pcm_hw_params_get_rate_min(params_ptr, &minRate, NULL);
				snd_pcm_hw_params_get_rate_max(params_ptr, &maxRate, NULL);

				err = snd_pcm_close(pcm) ;
				// snd_pcm_hw_params_free( params_ptr );
				if(err != 0) {
					printf("\tbut error %d occurred when closing...\n", err);
				}
				else {
					/* everything is fine, device could be opened, we add it to the collection */
					printf(", add collection item");

					CollectionItem * item = malloc( sizeof( CollectionItem ));
					item->name = strdup( name );
					item->description = strdup( firstline );
					item->minChannels = minChannels ;
					item->maxChannels = maxChannels ;
					item->minRate = minRate ;
					item->maxRate = maxRate ;
					if( 0 == strcmp(io, STREAM_INPUT ))
						{
							AppendCollectionItem( &InputCollection, item );
						}
					else 
						{
							AppendCollectionItem( &OutputCollection, item );
						}
					printf( "\n\tDEVICE '%s' ; %s\n", item->name, item->description );
				}
			}
		}
		free(io);	
		free(descr);
		free(name);
		n++;
	}
	snd_device_name_free_hint(h);
}
