#ifndef NODE_LIST_H
#define NODE_LIST_H

#include <sndio.h>

struct desc_list
{
	sioctl_desc desc;
	int val;
	desc_list* next;
};

desc_list* desc_list_new( sioctl_desc, int val );
void desc_list_append( desc_list* list, sioctl_desc, int val );
desc_list* desc_list_find( desc_list* list, char* func );
void desc_list_destroy( desc_list* list );

struct node_list
{
	char node_name[SIOCTL_NAMEMAX];
	desc_list* functions;// functions: level, mute, ...
	node_list* next;
};

node_list* node_list_new( sioctl_desc, int val ); // create new item in list
void node_list_append( node_list* list, sioctl_desc desc, int val );// append new item to list with the given desc data
node_list* node_list_find( node_list* list, char* node_name);
void node_list_destroy( node_list* list );
node_list* node_list_last( node_list* list );
#endif
