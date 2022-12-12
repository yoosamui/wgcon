// cmake -Bbuild -H.
// cmake --build build
// sudo apt install libnm-dev libglib2.0-dev


#include "get_connection.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>

using namespace std;

typedef struct {
	GMainLoop *loop;
	NMActiveConnection *ac;

	int remaining;
} TestACInfo;

static GMainLoop *loop = NULL;

GKeyFile *key_file;

string get_doc_string(const gchar *group_name, const gchar *key)
{
        GError *error = nullptr;
        char *value = g_key_file_get_string(key_file, group_name, key, &error);
        if (error) {
            g_error_free(error);
            error = nullptr;

            return string{};
        }

		
        return value;
}
GKeyFile *load_doc_file()
    {
      
        string filepath = "/usr/share/mip-va-scan/doc.ini";

        GError *error = nullptr;
        key_file = g_key_file_new();

        auto found = g_key_file_load_from_file(key_file, filepath.c_str(),
                                               GKeyFileFlags::G_KEY_FILE_NONE, &error);

        if (!found) {
            if (error) {
                g_warning("Load: configuration could not be found. %s %s\n", error->message,
                          filepath.c_str());
                g_error_free(error);
                error = nullptr;
            }
			
            g_key_file_free(key_file);
            return nullptr;
        }

		

        return key_file;
    }


static void activate_failed_cb (GObject *object,
                    GAsyncResult *result,
                    gpointer user_data)
{
	/*
	NMClient *client = NM_CLIENT (object);
	NMActiveConnection *ac;
	GError *error = NULL;

	ac = nm_client_activate_connection_finish (client, result, &error);
	g_assert (ac == NULL);

	g_assert_error (error, NM_CLIENT_ERROR, NM_CLIENT_ERROR_OBJECT_CREATION_FAILED);
	g_clear_error (&error);
	*/
	
	g_main_loop_quit (loop);
}

bool is_active(NMClient *client, const char *device_name) {

	NMDevice *device;	
	
	device = nm_client_get_device_by_iface(client, device_name);
	if(!device) return FALSE;

	return nm_device_get_state (device) == NM_DEVICE_STATE_ACTIVATED;

}

void deactivate(NMClient *client, const char *device_name){

	NMDevice *device;
	NMActiveConnection *active_con;
	//const char *name;

	device = nm_client_get_device_by_iface(client, device_name);

	if(!device) return;

    active_con = nm_device_get_active_connection(device);
	//name = nm_connection_get_id(con);

	 g_print("Current active connection of device [%s]: %s \n", device_name, device_name);
	 nm_client_deactivate_connection_async (client,active_con,NULL,NULL,NULL);

	//
}


static void get_wireguard_connections(NMClient *client,  int index)
{
	const GPtrArray *connections;
    NMConnection *con;
	const char *name;
	std::vector<std::string> actives;
	TestACInfo info = { loop, NULL, 0 };

	connections = nm_client_get_connections(client );

	int idx = 0;
	for (int i = 0; i < connections->len; i++) {
		con = (NMConnection *)connections->pdata[i];

		if(strcmp("wireguard", nm_connection_get_connection_type(con))!=0)
		continue;

		name = nm_connection_get_id (con);

		if(std::find(actives.begin(), actives.end(), name) != actives.end()) continue;
		idx++;

		if(index > 0) {

			if( abs(index) == idx) {
				nm_client_activate_connection_async (client, con, NULL, NULL, NULL,activate_failed_cb ,NULL/*&info*/);
				g_main_loop_run(loop);
				
				g_print("WireGuard Connected->%s\n",name);

				return;	
			}

		}

		if(index < 0) {
			if(abs(index) == idx && is_active(client, name)) {
				auto device = nm_client_get_device_by_iface(client, name);
				auto active_con = nm_device_get_active_connection(device);
				nm_client_deactivate_connection_async (client,active_con,NULL,activate_failed_cb,NULL);
				g_main_loop_run(loop);

				g_print("WireGuard Disconnected->%s\n",name);
				
			return;
			}
		}
		

		if(index == 0) {

			string comment = get_doc_string(name,"comment");
			string location = get_doc_string(name,"location");
			string company = get_doc_string(name,"company");

			string str;
			str.append(comment);
			str.append(", ");
			str.append(location);
			str.append(", ");
			str.append(company);

			string active = is_active(client, name) == TRUE ? "*":"";

			g_print("\e[0m%3d \e[1;33m%2s \e[0m[%12s] \e[1;32m%s\e[0m\n",
				idx, 
				active.c_str(),
				nm_connection_get_id(con), 
				//nm_connection_get_connection_type(con),
				str.c_str());

			if(is_active(client, name)) {
				if(std::find(actives.begin(), actives.end(), name) == actives.end())
						actives.push_back(string(name));
			}
		}
   	
	}
}
/*
void get_connections(NMClient *client,  GMainLoop *loop)
{

	const GPtrArray *connections;
    NMConnection *con;
	NMSettingVpn *vpn;
	TestACInfo info = { loop, NULL, 0 };
	

	GAsyncResult *result;
	NMActiveConnection *ac;
	GError *error = NULL;



	connections = nm_client_get_connections(client );

	for (int i = 0; i < connections->len; i++) {
	
		con = (NMConnection *)connections->pdata[i];

		if( strcmp("wireguard", nm_connection_get_connection_type(con))!=0)
		continue;


		//vpn = nm_connection_get_setting_vpn (con);
		//if (vpn == NULL) continue;


		

//		g_print("network interface [%s]\n",nm_remote_connection_get_filename (con));



		g_print("Connections [%s] %s\n",nm_connection_get_id (con), nm_connection_get_connection_type (con) );
		deactivate(client, nm_connection_get_id (con));



//nm_client_deactivate_connection_async (client,(NMActiveConnection*)con,NULL,NULL,NULL);
                                       

		//nm_client_activate_connection_async (client,con, NULL, NULL, NULL,NULL ,&info);
	//	nm_connection_get_id ()

	 g_object_unref(con);
	 
	
	}

	g_main_loop_quit (loop);
}
*/

void get_devices(NMClient *client)
{

	NMActiveConnection *active_con;
    NMDevice *device;
    const GPtrArray *devices;
    const char *name, *con_id, *type;

    devices = nm_client_get_devices(client);

	 for (int i = 0; i < devices->len; i++) {

        device = (NMDevice *)devices->pdata[i];
        name = nm_device_get_iface(device);

		g_print("network interface [%s], device type is: %s\n", name, type);
    }
	 

}


int main(int argc, char *argv[])
{
    NMClient *client;
    GError *error = NULL;
    //GMainLoop *loop;
 	loop = g_main_loop_new(NULL, FALSE);

    // Connect to NetworkManager
    client = nm_client_new(NULL, &error);
    if (!client) {
        g_message("Error: Could not connect to NetworkManager: %s.", error->message);
        g_error_free(error);
        return 1;
    }

	int mode = 0;
	int idx = 0;

	if (argc == 2) {

		idx = atoi(argv[1]);
	}

	load_doc_file();
	get_wireguard_connections(client,  idx);


    // Clean up
    g_object_unref(client);

    return 0;
}




