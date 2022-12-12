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

static GMainLoop *loop = NULL;
GKeyFile *key_file;

static string get_doc_string(const gchar *group_name, const gchar *key)
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

static GKeyFile *load_doc_file()
    {
      
        string filepath = "/usr/share/wgcon/doc.ini";

        GError *error = nullptr;
        key_file = g_key_file_new();

        auto found = g_key_file_load_from_file(key_file, filepath.c_str(),
                                               GKeyFileFlags::G_KEY_FILE_NONE, &error);

        if (!found) {
            if (error) {
                g_warning("Load: documment could not be found. %s %s\n", error->message,
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
	g_main_loop_quit (loop);
}

bool is_active(NMClient *client, const char *device_name) {

	NMDevice *device;	
	
	device = nm_client_get_device_by_iface(client, device_name);
	if(!device) return FALSE;

	return nm_device_get_state (device) == NM_DEVICE_STATE_ACTIVATED;

}

static void get_wireguard_connections(NMClient *client,  int index)
{
	const GPtrArray *connections;
    NMConnection *con;
	const char *name;
	std::vector<std::string> actives;
	
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

				g_print("\e[0mWireGuard Disconnected->%s\n",name);
				
			return;
			}
		}
		

		if(index == 0) {

			string comment = get_doc_string(name,"comment");
			string location = get_doc_string(name,"location");
			string company = get_doc_string(name,"company");

			string str;
			str.append(comment);
			
			if(!location.empty()) {
				str.append(", ");
				str.append(location);
			}

			if(!company.empty()) {
				str.append(", ");
				str.append(company);
			}

			string active = is_active(client, name) == TRUE ? "*":"";

			g_print("%3d %2s [%12s] %s\n",
				idx, 
				active.c_str(),
				nm_connection_get_id(con), 
				str.c_str());

			if(is_active(client, name)) {
				if(std::find(actives.begin(), actives.end(), name) == actives.end())
						actives.push_back(string(name));
			}
		}
   	
	}
}

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




