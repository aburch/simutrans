/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "network_cmp_pakset.h"
#include "network_packet.h"
#include "network.h"
#include "network_socket_list.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer.h"
#include "../simloadingscreen.h"

#include <stdlib.h>

stringhashtable_tpl<checksum_t*>::iterator nwc_pakset_info_t::server_iterator;
SOCKET nwc_pakset_info_t::server_receiver = INVALID_SOCKET;


nwc_pakset_info_t::~nwc_pakset_info_t()
{
	delete chk;
	free(name);
}


bool nwc_pakset_info_t::execute(karte_t *)
{
	// server side of the communication
	// client side in network_compare_pakset_with_server
	if(  env_t::server  ) {
		nwc_pakset_info_t nwi;
		bool send = false;
		bool ready = false;
		switch(flag) {

			case CL_INIT:       // client want pakset info
			{
				if (server_receiver!=INVALID_SOCKET  &&  socket_list_t::has_client(server_receiver)) {
					// we are already talking to another client
					nwi.flag = SV_ERROR;
					nwi.send(packet->get_sender());
					// ignore result of send, we don't want to talk to that client either
					break;
				}
				server_receiver = packet->get_sender();
				// restart iterator
				server_iterator = pakset_info_t::info.begin();

				nwi.flag = SV_PAKSET;
				nwi.chk = new checksum_t(*pakset_info_t::get_checksum());
				nwi.name = strdup("pakset");
				DBG_MESSAGE("nwc_pakset_info_t::execute", "send info about %s",nwi.name);
				send = true;
				break;
			}

			case CL_WANT_NEXT: // client received one info packet, wants next
				if (server_iterator != pakset_info_t::info.end()) {
					nwi.flag = SV_DATA;
					nwi.chk  = new checksum_t(*server_iterator->value);
					nwi.name = strdup(server_iterator->key);
					DBG_MESSAGE("nwc_pakset_info_t::execute", "send info about %s",nwi.name);
					++server_iterator;
				}
				else {
					nwi.flag = SV_LAST;
					ready = true;
				}
				send = true;
				break;

			case CL_QUIT:      // client ends this negotiation
				server_receiver = INVALID_SOCKET;
				break;
			default: ;
		}
		if(  send  ) {
			if(socket_list_t::has_client(server_receiver)) {
				// send, if unsuccessful stop comparing
				if (!nwi.send(server_receiver)) {
					ready = true;
				}
			}
			else {
				// client disappeared
				server_receiver = INVALID_SOCKET;
			}
		}
		if(  ready  ) {
			// all information sent
			server_receiver = INVALID_SOCKET;
		}
	}
	return true;
}


void nwc_pakset_info_t::rdwr()
{
	network_command_t::rdwr();

	packet->rdwr_byte(flag);
	packet->rdwr_str(name);
	bool has_info = (chk!=NULL  &&  chk->is_valid())  ||  packet->is_loading();
	packet->rdwr_bool(has_info);
	if(  has_info  ) {
		if(  packet->is_loading()  ) {
			chk = new checksum_t();
		}
		chk->rdwr(packet);
	}
}


static bool str_cmp(const char *a, const char *b)
{
	return strcmp(a,b) < 0;
}


void network_compare_pakset_with_server(const char* cp, std::string &msg)
{
	// open from network
	const char *err = NULL;
	SOCKET const my_client_socket = network_open_address(cp, err);
	if(  err==NULL  ) {
		socket_list_t::add_client(my_client_socket); // for network_check_activity
		// client side of comparison
		// server part in nwc_pakset_info_t::execute
		{
			// start
			nwc_pakset_info_t nwi(nwc_pakset_info_t::CL_INIT);
			if (!nwi.send(my_client_socket)) {
				dbg->warning("network_compare_pakset_with_server", "send of NWC_PAKSETINFO failed");
				socket_list_t::remove_client(my_client_socket);
				return;
			}
		}
		// copy our info to addon
		// ie treat all our paks as if they were not present on the server
		stringhashtable_tpl<checksum_t*> addons;
		{
			for(auto const& i : pakset_info_t::get_info()) {
				addons.put(i.key, i.value);
			}
		}
		// we do a sorted verctor of names ...
		vector_tpl<const char *> missing, different;

		// show progress bar
		const uint32 num_paks = addons.get_count()+1;
		uint32 progress = 0;
#define MAX_WRONG_PAKS 10
		uint16 wrong_paks = 0;

		if(num_paks>0) {
			loadingscreen_t ls(translator::translate("Comparing pak files ..."), num_paks );
			// communication loop
			bool ready = false;
			do {
				nwc_pakset_info_t *nwi = NULL;
				// wait for nwc_pakset_info_t, ignore other commands
				for(uint8 i=0; i<5; i++) {
					network_command_t* nwc = network_check_activity( NULL, 10000 );
					if (nwc  &&  nwc->get_id() == NWC_PAKSETINFO) {
						nwi = (nwc_pakset_info_t*)nwc;
						break;
					}
					delete nwc;
				}

				if (nwi == NULL) {
					dbg->warning("network_compare_pakset_with_server", "server did not answer");
					nwc_pakset_info_t nwi_quit(nwc_pakset_info_t::CL_QUIT);
					if (!nwi_quit.send(my_client_socket)) {
						err = "send of NWC_PAKSETINFO failed";
					}
					break;
				}
				switch(nwi->flag) {
					case nwc_pakset_info_t::SV_PAKSET:
					{
						if(pakset_info_t::get_pakset_checksum()==(*(nwi->chk))) {
							// found identical paksets
						}
						else {
							wrong_paks++;
						}
						progress++;
						// request new data
						nwc_pakset_info_t nwi_data(nwc_pakset_info_t::CL_WANT_NEXT);
						if(!nwi_data.send(my_client_socket)) {
							err = "send of NWC_PAKSETINFO failed";
							ready = true;
						}
						break;
					}

					case nwc_pakset_info_t::SV_DATA:
					{
						checksum_t* chk = addons.remove(nwi->name);
						if(chk) {
							if((*chk)==(*(nwi->chk))) {
								// found identical desc's
							}
							else {
								different.insert_ordered( nwi->name, str_cmp );
								nwi->clear();
								wrong_paks++;
							}
							progress++;
						}
						else {
							missing.insert_ordered( nwi->name, str_cmp );
							nwi->clear();
							wrong_paks++;
						}
						nwc_pakset_info_t nwi_next;
						if (wrong_paks<=MAX_WRONG_PAKS) {
							// request new data
							nwi_next.flag = nwc_pakset_info_t::CL_WANT_NEXT;
						}
						else {
							nwi_next.flag = nwc_pakset_info_t::CL_QUIT;
						}
						if(!nwi_next.send(my_client_socket)) {
							err = "send of NWC_PAKSETINFO failed";
							ready = true;
						}
						break;
					}

					case nwc_pakset_info_t::SV_LAST:
					case nwc_pakset_info_t::SV_ERROR:
					default:
						ready = true;
				}

				// update progress bar
				ls.set_progress(progress);
				delete nwi;

			} while (!ready  &&  wrong_paks<=MAX_WRONG_PAKS);
		}

		// now report the result
		msg.append("<title>");
		msg.append(translator::translate("Pakset differences"));
		msg.append("</title>\n");
		if(wrong_paks<=MAX_WRONG_PAKS  &&  !addons.empty()) {
			msg.append("<h1>");
			msg.append(translator::translate("Pak(s) not on server:"));
			msg.append("</h1><br>\n");
			for(auto const& i : addons) {
				dbg->warning("network_compare_pakset_with_server", "PAK NOT ON SERVER: %s", i.key);
				msg.append(translator::translate(i.key+3));
				msg.append("<br>\n");
			}
			msg.append("<br>\n");
		}
		if (!different.empty()) {
			msg.append("<h1>");
			msg.append(translator::translate("Pak(s) different:"));
			msg.append("</h1><br>\n");
			for(const char * const& i : different) {
				dbg->warning("network_compare_pakset_with_server", "PAK DIFFERENT: %s", i);
				msg.append(translator::translate(i+3)); // the first three letters are the type ...
				msg.append("<br>\n");
			}
			msg.append("<br>\n");
		}
		if (!missing.empty()) {
			msg.append("<h1>");
			msg.append(translator::translate("Pak(s) missing on client:"));
			msg.append("</h1><br>\n");
			for(const char * const& i : missing) {
				dbg->warning("network_compare_pakset_with_server", "PAK MISSING: %s", i);
				msg.append(translator::translate(i+3)); // the first three letters are the type ...
				msg.append("<br>\n");
			}
		}
		if (wrong_paks>MAX_WRONG_PAKS) {
			msg.append("<br>\n");
			msg.append("<br>\n");
			cbuffer_t buf;
			buf.printf(translator::translate("Only first %d differing paks reported. There are probably more."), wrong_paks);
			msg.append((const char*)buf);
			msg.append("<br>\n");
		}
		socket_list_t::remove_client(my_client_socket);
	}
	if(err) {
		dbg->warning("network_compare_pakset_with_server", err);
	}
}
