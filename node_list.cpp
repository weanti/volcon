#include "node_list.h"

#include <stdlib.h>
#include <string.h>

desc_list* desc_list_new( sioctl_desc desc, int val )
{
	desc_list* list = (desc_list*)malloc( sizeof(desc_list) );
	list->desc = desc;
	strlcpy( list->desc.func, desc.func, SIOCTL_NAMEMAX );
	strlcpy( list->desc.group, desc.group, SIOCTL_NAMEMAX );
	strlcpy( list->desc.display, desc.display, SIOCTL_DISPLAYMAX );
	strlcpy( list->desc.node0.name, desc.node0.name, SIOCTL_NAMEMAX );
	strlcpy( list->desc.node1.name, desc.node1.name, SIOCTL_NAMEMAX );
	list->val = val;
	list->next = NULL;
	return list;
}

void desc_list_append( desc_list* list, sioctl_desc desc, int val )
{
	while( list->next )
	{
		list = list->next;
	}
	list->next = desc_list_new( desc, val );
}

desc_list* desc_list_find( desc_list* list, char* func )
{
	while( list )
	{
		if ( strcmp( list->desc.func, func ) == 0 )
		{
			return list;
		}
		list = list->next;
	}
	return NULL;
}

void desc_list_destroy( desc_list* list )
{
	desc_list* next = NULL;
	while( list )
	{
		next = list->next;
		free( list );
		list = next;
	}
}

node_list* node_list_new( sioctl_desc desc, int val )
{
	node_list* list = (node_list*)malloc( sizeof(node_list) );
	memset( list, 0, sizeof(node_list) );
	strlcpy( list->node_name, desc.node0.name, SIOCTL_NAMEMAX );
	list->functions = desc_list_new( desc, val );
	return list;
}

void node_list_append( node_list* list, sioctl_desc desc, int val )
{
	node_list* new_item = node_list_new( desc, val );
	list->next = new_item;
}

node_list* node_list_find( node_list* list, char* node_name)
{
	while( list )
	{
		if ( strcmp( list->node_name, node_name ) == 0 )
		{
			return list;
		}
		list = list->next;
	}
	return NULL;
}

void node_list_destroy( node_list* list )
{
	node_list* next = list->next;
	while( next )
	{
		next = list->next;
		list->next = NULL;
		desc_list_destroy( list->functions );
		free( list );
		list = next;
	}
}

node_list* node_list_last( node_list* list )
{
	while( list->next )
	{
		list = list->next;
	}
	return list;
}
