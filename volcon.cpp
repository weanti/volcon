#include "node_list.h"

#include <errno.h>
#include <math.h>
#include <poll.h>
#include <sndio.h>
#include <stdio.h>
#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Window.H>

node_list* controls;
sioctl_hdl* hdl;
// ========== sndio handling ===
void ondesc( void* arg, sioctl_desc* desc, int val);
void onval( void* arg, unsigned int addr, unsigned int val );
// ============= ui ===========
void add_controls( Fl_Group* group, node_list* list );
void slider_cb( Fl_Widget* w, long arg );
void checkbox_cb( Fl_Widget* w, long arg );
// find a widget that contols the device with the given address
Fl_Valuator* find_widget( Fl_Group* group, unsigned addr );

void ondesc( void* arg, struct sioctl_desc* desc, int val)
{
	if ( ! desc )
	{
		add_controls( reinterpret_cast<Fl_Group*>(arg), controls );
		return;
	}
	switch( desc->type )
	{
		case SIOCTL_NUM:
		case SIOCTL_SW:
			break;
		default:
			return;
	}
	node_list* node_pos = node_list_find( controls, desc->node0.name ); 
	if ( node_pos != NULL )
	{
		desc_list* func_pos = desc_list_find( node_pos->functions, desc->func );
		if ( func_pos != NULL && func_pos->desc.addr == desc->addr )
			return;
		desc_list_append( node_pos->functions, *desc, val );
		return;
	}
	if ( controls == NULL )
	{
		controls = node_list_new( *desc, val );
	}
	else
	{
		node_list_append( node_list_last(controls), *desc, val );
	}
}

void onval( void* arg, unsigned int addr, unsigned int val )
{
	Fl_Group* group = reinterpret_cast<Fl_Group*>( arg );
	Fl_Valuator* v = find_widget( group, addr );
	if ( v )
	{
		v->value( val );
	}
}

void add_controls( Fl_Group* group, node_list* list )
{
	group->begin();
	int slider_count = 0;
	int switch_count = 0;
	while ( list )
	{
		desc_list* func = list->functions;
		Fl_Box* group_label = new Fl_Box( FL_FLAT_BOX, 60, slider_count*60, group->w(), 20, list->node_name );
		group_label->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT );
		while ( func )
		{
			switch( func->desc.type )
			{
				case SIOCTL_NUM:
					{
						Fl_Hor_Value_Slider* slider = new Fl_Hor_Value_Slider( 60, slider_count*60+20, group->w()-80, 20 );
						slider->bounds( 0, func->desc.maxval );
						slider->value( func->val );
						slider->callback( slider_cb );
						slider->argument( func->desc.addr );
						slider_count++;
					}
					break;
				case SIOCTL_SW:
					Fl_Check_Button* checkbox = new Fl_Check_Button( 0, switch_count*60+20, 60, 20, func->desc.func );
					checkbox->callback( checkbox_cb );
					checkbox->argument( func->desc.addr );
					checkbox->value( func->val );
					switch_count++;
					break;
			}
			func = func->next;
		}
		list = list->next;
		switch_count = slider_count = switch_count > slider_count ? switch_count : slider_count;
	}
	group->end();
}

Fl_Valuator* find_widget( Fl_Group* group, unsigned int addr )
{
	for ( int i = 0; i < group->children(); i++ )
	{
		Fl_Valuator * v = dynamic_cast<Fl_Valuator*>(group->child( i ));
		if ( v && v->argument() == addr )
		{
			return v;
		}
	}
	return NULL;
}

void slider_cb( Fl_Widget* w, long arg )
{
	Fl_Hor_Value_Slider* slider = dynamic_cast<Fl_Hor_Value_Slider*>(w);
	unsigned int addr = (unsigned int)arg;
	sioctl_setval( hdl, addr, slider->value() );
}

void checkbox_cb( Fl_Widget* w, long arg )
{
	Fl_Check_Button* checkbox = dynamic_cast<Fl_Check_Button*>(w);
	unsigned int addr = (unsigned int)arg;
	sioctl_setval( hdl, addr, checkbox->value() );
}

int main( int argc, char** argv )
{
	hdl = sioctl_open( SIO_DEVANY, SIOCTL_READ | SIOCTL_WRITE, 1 );
	if ( ! hdl )
	{
		fprintf(stderr, "failed to open sound device\n");
		exit(1);
	}
	Fl_Window wnd(320, 240);
	Fl_Scroll hscroll(0, 0, 320, 240);
	hscroll.type(Fl_Scroll::VERTICAL);
	wnd.end();
	wnd.show(argc, argv);
	controls = NULL;
	sioctl_ondesc(hdl, ondesc, &hscroll);
	sioctl_onval(hdl, onval, &hscroll);
	pollfd* pfds = (pollfd*)malloc(sizeof(pollfd)*sioctl_nfds(hdl));
	if (pfds == NULL)
	{
		exit(1);
	}
	int nfds = 0;
	int revents = 0;
	for ( ;; )
	{
		nfds = sioctl_pollfd(hdl, pfds, POLLIN);
		if ( nfds == 0 )
			break;
		while( poll(pfds, nfds, 0) < 0 )
		{
			if ( errno != EINTR )
			{
				exit(1);
			}
		}
		revents = sioctl_revents(hdl, pfds);
		if ( revents & POLLHUP )
		{
			break;
		}
		Fl::check();
		if ( ! wnd.shown() )
			break;
	}
	free( pfds );
	sioctl_close(hdl);
	node_list_destroy(controls);
	return 0;
}
