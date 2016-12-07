#include "ts/ink_defs.h"
#include "ts/ink_platform.h"
#include <string.h>
#include <ts/ts.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <map>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PLUGIN_NAME              "ws_hosts"
#define DEFAULT_SERVER_PORT      80
#define DEFAULT_HOSTS_FILE       "/etc/hosts"

#define skip_space(p)   while(isspace(*p) && *p != '\0') p++
#define to_space(p)     while(!isspace(*p) && *p != '\0') p++

class WSHosts {
public:
    bool GetAddr(struct sockaddr_in *server_addr, const char *host);
    void Clear();
    bool LoadConfig(const char *filename);
    bool ReloadConfig(const char *filename);

    WSHosts() {};
    ~WSHosts() { Clear(); };

private:
    std::map<std::string, struct sockaddr_in> hosts_map;
    std::string hosts_file;
};

void WSHosts::Clear()
{
    hosts_file = "";
    hosts_map.clear();
}

bool WSHosts::GetAddr(struct sockaddr_in *server_addr, const char *host)
{
    std::map<std::string, struct sockaddr_in>::iterator it;

    if (hosts_map.empty()) {
        return false;
    }

    it = hosts_map.find(host);
    if (it == hosts_map.end()) {
        return false;
    }

    memcpy(server_addr, &it->second, sizeof(*server_addr));

    return true;
}

bool WSHosts::ReloadConfig(const char *filename)
{
    Clear();

    return LoadConfig(filename);
}

bool WSHosts::LoadConfig(const char *filename)
{
    char    path[1024];
    FILE    *file = NULL;
    char    line[LINE_MAX];
    int     ln = 0;
    char    *p = NULL;
    char    *ip = NULL;
    char    *host = NULL;
    struct sockaddr_in server_addr;

    if (*filename == '/') {
        strcpy(path, filename);
    } else {
        snprintf(path, 1024, "%s/%s", TSConfigDirGet(), filename);
    }

    hosts_file = filename;

    TSDebug(PLUGIN_NAME, "config path is %s", path);

    file = fopen(path, "r");
    if (file == NULL) {
        TSDebug(PLUGIN_NAME, "Could not open %s for reading, %s.", path, strerror(errno));
        return false;
    }

    while (fgets(line, LINE_MAX, file) != NULL) {
        ln++;
        p = line;
        ip = NULL;
        host = NULL;
        //192.168.1.1 test1.com   test2.com
        skip_space(p);

        if (*p == '\0' || *p == '#') {
            TSDebug(PLUGIN_NAME, "skiped line %d.", ln);
            continue;
        }

        ip = p;
        to_space(p);
        if (*p == '\0') {
            continue;
        }
        *p++ = '\0';

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port   = htons(DEFAULT_SERVER_PORT);
        if (!inet_pton(AF_INET, ip, (void *)&server_addr.sin_addr.s_addr)) {
            TSError("[ws_hosts] invalid addr(%s) in line %d.", ip, ln);
            continue;
        }

        while(true) {
            skip_space(p);
            if (*p == '\0') {
                goto next;
            }

            host = p;
            to_space(p);
            if (*p != '\0') {
                *p++ = '\0';
            }

            hosts_map[host] = server_addr;
            TSDebug(PLUGIN_NAME, "add record [ip : %s, host: %s].", ip, host);
        }
next:
        continue;
    }

    return true;
}

static int
ws_hosts_main_handle(TSCont cont, TSEvent event, void *edata)
{
    TSHttpTxn txnp = (TSHttpTxn)edata;
    struct sockaddr_in server_addr;
    WSHosts *hosts = NULL;
    char host[128] = {0};
    char temp[INET_ADDRSTRLEN] = {0};

    TSDebug(PLUGIN_NAME, "start to find host.");

    hosts = (WSHosts*) TSContDataGet(cont);
    switch (event) {
    case TS_EVENT_HTTP_POST_REMAP:
        if (TSAfterRemapHostGet(txnp, host, 128) == TS_ERROR) {
            TSDebug(PLUGIN_NAME, "get host failed.");
            break;
        }

        TSDebug(PLUGIN_NAME, "host is %s.", host);

        memset(&server_addr, 0, sizeof(server_addr));
        if (!hosts->GetAddr(&server_addr, host)) {
            TSDebug(PLUGIN_NAME, "host not found.");
            break;
        }

        if (TSHttpTxnServerAddrSet(txnp, (struct sockaddr *)&server_addr) == TS_ERROR) {
            TSError("[%s] TSHttpTxnServerAddrSet %s failed", PLUGIN_NAME, host);
            break;
        }

        TSDebug(PLUGIN_NAME, "ip is %s.", inet_ntop(AF_INET, &server_addr.sin_addr.s_addr, temp, INET_ADDRSTRLEN));

        break;
    default:
        break;
    }

    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);

    return 0;
}

void
TSPluginInit(int argc, const char *argv[])
{
    TSCont  cont;
    TSPluginRegistrationInfo info;
    const char      *hosts_file = DEFAULT_HOSTS_FILE;
    WSHosts    *hosts = NULL;

    TSDebug(PLUGIN_NAME, "Starting plugin init.");

    if (argc == 2) {
        hosts_file = argv[1];
    } else if (argc > 2) {
        TSError("[ws_hosts] there are too many args, must be 1 or 0 arg.");
    }

    hosts = new WSHosts();
    if (!hosts->LoadConfig(hosts_file)) {
        TSError("[ws_hosts] load config failed.");
        return;
    }

    info.plugin_name   = (char *)PLUGIN_NAME;
    info.vendor_name   = (char *)"sinobbd";
    info.support_email = (char *)"sinobbd@sinobbd.com";

    if (TSPluginRegister(&info) != TS_SUCCESS) {
        TSError("[ws_hosts] Plugin registration failed.");
        return;
    } else {
        TSDebug(PLUGIN_NAME, "Plugin registration succeeded.");
    }

    cont = TSContCreate(ws_hosts_main_handle, NULL);
    TSContDataSet(cont, (void *)hosts);
    TSHttpHookAdd(TS_HTTP_POST_REMAP_HOOK, cont);

    TSDebug(PLUGIN_NAME, "Plugin Init Complete.");
}
