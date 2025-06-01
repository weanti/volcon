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
bool right_mouse_button;
// ========== sndio handling ===
void ondesc( void* arg, sioctl_desc* desc, int val);
void onval( void* arg, unsigned int addr, unsigned int val );
// ============= ui ===========
void add_controls( Fl_Group* group, node_list* list );
void slider_cb( Fl_Widget* w, long arg );
void checkbox_cb( Fl_Widget* w, long arg );
// find a widget that contols the device with the given address
Fl_Widget* find_widget( Fl_Group* group, unsigned addr );
int event_dispatch( int event, Fl_Window* w );

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
	Fl_Widget* w = find_widget( group, addr );
	Fl_Button* b = dynamic_cast<Fl_Button*>(w);
	if ( b && ( val != b->value() ) )
	{
		b->value( val );
		return;
	}
	Fl_Valuator* v = dynamic_cast<Fl_Valuator*>(w);
	if ( v && ( val != v->value() ) )
	{
		v->value( val );
		return;
	}
}

void add_controls( Fl_Group* group, node_list* list )
{
	group->begin();
	int slider_count = 0;
	int group_start = 0;
	Fl_Check_Button* mute_checkbox = NULL;
	while ( list )
	{
		desc_list* func = list->functions;
		Fl_Box* group_label = new Fl_Box( FL_FLAT_BOX, 0, group_start*20, group->w()/2, 20, list->node_name );
		group_label->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT );
		while ( func )
		{
			switch( func->desc.type )
			{
				case SIOCTL_NUM:
					{
						slider_count++;
						Fl_Hor_Value_Slider* slider = new Fl_Hor_Value_Slider( 0, (group_start+slider_count)*20, group->w()-20, 20 );
						slider->bounds( 0, func->desc.maxval );
						slider->value( func->val );
						slider->callback( slider_cb );
						slider->argument( func->desc.addr );
					}
					break;
				case SIOCTL_SW:
					// Create only 1 mute switch for a group.
					// Reason:
					// The system exposes a mute switch for all controls, but it seems that seem to work for all controls in a group example: 2 controls are listed for output (left and right), 2 mute switches are listed, but either switch mutes left AND right.
					if ( strstr( func->desc.func, "mute" ) && mute_checkbox == NULL )
					{
						mute_checkbox = new Fl_Check_Button( group->w()/2, group_start*20, 60, 20, func->desc.func );
						mute_checkbox->callback( checkbox_cb );
						mute_checkbox->argument( func->desc.addr );
						mute_checkbox->value( func->val );
					}
					break;
			}
			func = func->next;
		}
		group_start += slider_count+1;
		slider_count = 0;
		list = list->next;
		mute_checkbox = NULL;
	}
	group->end();
}

Fl_Widget * find_widget( Fl_Group* group, unsigned int addr )
{
	for ( int i = 0; i < group->children(); i++ )
	{
		Fl_Widget * w = dynamic_cast<Fl_Widget*>(group->child( i ));
		if ( w && w->argument() == addr )
		{
			return w;
		}
	}
	return NULL;
}

int event_dispatch( int event, Fl_Window* w )
{
	switch( event )
	{
		case FL_MOVE:
			right_mouse_button = Fl::event_button() == FL_RIGHT_MOUSE;
			break;
		default:
			right_mouse_button = false;

	}
	return Fl::handle_( event, w );
}

void slider_cb( Fl_Widget* w, long arg )
{
	Fl_Hor_Value_Slider* slider = dynamic_cast<Fl_Hor_Value_Slider*>(w);
	unsigned int addr = (unsigned int)arg;
	const bool independent_moving = right_mouse_button;
	if ( independent_moving )
	{
		sioctl_setval( hdl, addr, slider->value() );
	}
	else
	{
		// find the group of the changed control
		node_list* n = controls;
		for ( ; n != NULL; n = n->next )
		{
			desc_list* f = n->functions;
			for ( ; f != NULL; f = f->next )
			{
				if ( f->desc.addr == addr )
					break;
			}
			if ( f )
				break;
		}
		if ( n )
		{
			for ( desc_list* f = n->functions; f != NULL; f = f->next )
			{
				if ( f->desc.type == SIOCTL_NUM )
				{
					sioctl_setval( hdl, f->desc.addr, slider->value() );
				}
			}
		}
	}
}

void checkbox_cb( Fl_Widget* w, long arg )
{
	Fl_Check_Button* checkbox = dynamic_cast<Fl_Check_Button*>(w);
	unsigned int addr = (unsigned int)arg;
	sioctl_setval( hdl, addr, checkbox->value() );
}

int main( int argc, char** argv )
{
	right_mouse_button = false;
	hdl = sioctl_open( SIO_DEVANY, SIOCTL_READ | SIOCTL_WRITE, 1 );
	if ( ! hdl )
	{
		fprintf(stderr, "failed to open sound device\n");
		exit(1);
	}
	Fl_Window wnd(320, 240, "Volume control");
	Fl_Scroll hscroll(0, 0, 320, 240);
	hscroll.type(Fl_Scroll::VERTICAL);
	wnd.end();
	wnd.show(argc, argv);
	Fl::event_dispatch( event_dispatch );
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
		Fl::wait(1.0);
		if ( ! wnd.shown() )
			break;
	}
	free( pfds );
	sioctl_close(hdl);
	node_list_destroy(controls);
	return 0;
}
