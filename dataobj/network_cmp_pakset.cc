#include "network_cmp_pakset.h"
#include "network_packet.h"
#include "network.h"
#include "translator.h"
#include "umgebung.h"
#include "../simgraph.h"
#include "../utils/cbuffer_t.h"


stringhashtable_iterator_tpl<checksum_t*> nwc_pakset_info_t::server_iterator(pakset_info_t::info);
SOCKET nwc_pakset_info_t::server_receiver = INVALID_SOCKET;

bool nwc_pakset_info_t::execute(karte_t *)
{
	// server side of the communication
	// client side in network_compare_pakset_with_server
	if (umgebung_t::server) {
		nwc_pakset_info_t *nwi = new nwc_pakset_info_t();
		bool send = false;
		bool ready = false;
		switch(flag) {
			case CL_INIT:       // client want pakset info
			{
				if (server_receiver!=INVALID_SOCKET  &&  network_get_client_id(server_receiver)>0) {
					// we are already talking to another client
					nwi->flag = SV_ERROR;
					nwi->send(packet->get_sender());
					break;
				}
				server_receiver = packet->get_sender();
				// restart iterator
				server_iterator = pakset_info_t::info;

				nwi->flag = SV_PAKSET;
				nwi->chk = pakset_info_t::get_checksum();
				nwi->name = strdup("pakset");
				dbg->warning("nwc_pakset_info_t::execute", "send info about %s",nwi->name);
				send = true;
				break;
			}
			case CL_WANT_NEXT: // client received one info packet, wants next
				if (server_iterator.next()) {
					nwi->flag = SV_DATA;
					nwi->chk  = server_iterator.get_current_value();
					nwi->name = strdup(server_iterator.get_current_key());
					dbg->warning("nwc_pakset_info_t::execute", "send info about %s",nwi->name);
				}
				else {
					nwi->flag = SV_LAST;
					ready = true;
				}
				send = true;

				break;
			case CL_QUIT:      // client ends this negotiation
				server_receiver = INVALID_SOCKET;
				break;
			default: ;
		}
		if (send) {
			if(network_get_client_id(server_receiver)>0) {
				nwi->send(server_receiver);
			}
			else {
				// client disappeared
				server_receiver = INVALID_SOCKET;
			}
			if (nwi->name) delete [] name;
		}
		delete nwi;
		if (ready) {
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
	if (has_info) {
		if (packet->is_loading()) {
			chk = new checksum_t();
		}
		chk->rdwr(packet);
	}
}

// declaration of stuff from network.cc needed here.
const char *network_open_address( const char *cp, long timeout_ms );
extern SOCKET my_client_socket;

void network_compare_pakset_with_server(const char* cp, std::string &msg)
{
	msg.clear();
	// open from network
	SOCKET old_my_client_socket = my_client_socket;
	const char *err = network_open_address( cp, 50000 );
	if(  err==NULL  ) {
		network_add_client(my_client_socket); // for network_check_activity
		// client side of comparison
		// server part in nwc_pakset_info_t::execute
		// start
		nwc_pakset_info_t *nwi = new nwc_pakset_info_t();
		nwi->flag = nwc_pakset_info_t::CL_INIT;
		nwi->send(my_client_socket);
		// copy our info to addon
		// ie treat all our pak's as if they were not present on the server
		stringhashtable_tpl<checksum_t*> addons;
		{
			stringhashtable_iterator_tpl<checksum_t*> iterator(pakset_info_t::get_info());
			while(iterator.next()) {
				addons.put(iterator.get_current_key(), iterator.get_current_value());
			}
		}
		//
		stringhashtable_tpl<checksum_t*> missing, different;
		// show progress bar
		uint32 num_paks = addons.get_count()+1;
		uint32 progress = 0;
		if(is_display_init()  &&  num_paks>0) {
			display_set_progress_text(translator::translate("Comparing pak's ..."));
			display_progress(progress, num_paks);
		}
		// communication loop
#define MAX_WRONG_PAKS 10
		uint16 wrong_paks=0;
		bool ready = false;
		do {
			nwi = NULL;
			// wait for nwc_pakset_info_t, ignore other commands
			for(uint8 i=0; i<5; i++) {
				network_command_t* nwc = network_check_activity( NULL, 10000 );
				if (nwc  &&  nwc->get_id() == NWC_PAKSETINFO) {
					nwi = (nwc_pakset_info_t*)nwc;
					break;
				}
			}

			if (nwi == NULL) {
				dbg->warning("network_compare_pakset_with_server", "server did not answer");
				nwi = new nwc_pakset_info_t();
				nwi->flag = nwc_pakset_info_t::CL_QUIT;
				nwi->send(my_client_socket);
				delete nwi;
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
					nwi = new nwc_pakset_info_t();
					nwi->flag = nwc_pakset_info_t::CL_WANT_NEXT;
					nwi->send(my_client_socket);
					delete nwi;
					break;
				}
				case nwc_pakset_info_t::SV_DATA:
				{
					checksum_t* chk = addons.remove(nwi->name);
					if (chk) {
						if((*chk)==(*(nwi->chk))) {
							// found identical besch's
						}
						else {
							different.put(nwi->name, nwi->chk);
							wrong_paks++;
						}
						progress++;
					}
					else {
						missing.put(nwi->name, nwi->chk);
						wrong_paks++;
					}
					nwi = new nwc_pakset_info_t();
					if (wrong_paks<=MAX_WRONG_PAKS) {
						// request new data
						nwi->flag = nwc_pakset_info_t::CL_WANT_NEXT;
					}
					else {
						nwi->flag = nwc_pakset_info_t::CL_QUIT;
					}
					nwi->send(my_client_socket);
					delete nwi;
					break;
				}
				case nwc_pakset_info_t::SV_LAST:
				case nwc_pakset_info_t::SV_ERROR:
				default:
					ready = true;
			}
			// update progress bar
			if(is_display_init()  &&  num_paks>0) {
				display_progress(progress, num_paks);
			}
		} while (!ready  &&  wrong_paks<=MAX_WRONG_PAKS);
		// now report the result
		msg.append("<title>");
		msg.append(translator::translate("Pakset differences"));
		msg.append("</title>\n");
		if (wrong_paks<=MAX_WRONG_PAKS  &&  !addons.empty()) {
			stringhashtable_iterator_tpl<checksum_t*> iterator(addons);
			msg.append("<h1>");
			msg.append(translator::translate("Pak(s) not on server:"));
			msg.append("</h1><br>\n");
			while(iterator.next()) {
				dbg->warning("network_compare_pakset_with_server", "PAK NOT ON SERVER: %s", iterator.get_current_key());
				msg.append(translator::translate(iterator.get_current_key()));
				msg.append("<br>\n");
			}
			msg.append("<br>\n");
		}
		if (!different.empty()) {
			stringhashtable_iterator_tpl<checksum_t*> iterator(different);
			msg.append("<h1>");
			msg.append(translator::translate("Pak(s) different:"));
			msg.append("</h1><br>\n");
			while(iterator.next()) {
				dbg->warning("network_compare_pakset_with_server", "PAK DIFFERENT: %s", iterator.get_current_key());
				msg.append(translator::translate(iterator.get_current_key()));
				msg.append("<br>\n");
			}
			msg.append("<br>\n");
		}
		if (!missing.empty()) {
			stringhashtable_iterator_tpl<checksum_t*> iterator(missing);
			msg.append("<h1>");
			msg.append(translator::translate("Pak(s) missing on client:"));
			msg.append("</h1><br>\n");
			while(iterator.next()) {
				dbg->warning("network_compare_pakset_with_server", "PAK MISSING: %s", iterator.get_current_key());
				msg.append(translator::translate(iterator.get_current_key()));
				msg.append("<br>\n");
			}
		}
		if (wrong_paks>MAX_WRONG_PAKS) {
			msg.append("<br>\n");
			msg.append("<br>\n");
			cbuffer_t buf(1024);
			buf.printf(translator::translate("Only first %d differing paks reported. There are probably more."), wrong_paks);
			msg.append((const char*)buf);
			msg.append("<br>\n");
		}
		network_remove_client(my_client_socket);
	}
	my_client_socket = old_my_client_socket;
	if(err) {
		dbg->warning("network_connect", err);
	}
}
