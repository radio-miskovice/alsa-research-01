typedef struct {
	char * name ; // name to be matched against
	char * description ;
	unsigned int minChannels ;
	unsigned int maxChannels ;
	unsigned int minRate ;
	unsigned int maxRate ;
} CollectionItem ;

void GetDeviceCollections();