#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <netpacket/packet.h>
#include <inttypes.h>
#include <string.h>

enum ERRORS{
    GET_IF_ADDR_ERROR = 32,
    GET_NAME_INFO_ERROR = 64,
};

enum ADDITIONAL{
    EXISTS = 15,
    NOT_EXISTS = 30,
};



typedef struct interface{
    char name[16];
    char ipv4_address[NI_MAXHOST];
    char ipv6_address[NI_MAXHOST];
    char MAC_address[NI_MAXHOST];
    uint32_t tx_packets;
    uint32_t rx_packets;
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    struct interface *next;
}interface;




int CheckInterface(interface *head,char *name);
interface* AddInterface(interface **head,char *name);

int main(int argc,char **argv) {

    if(argc > 2 || argc == 1){
        printf("Usage:\n");
        printf("gen_statistics <file-name>\n");
    }else {
        char *filename = argv[argc - 1];
        printf("Generating file:%s\n",filename);
        struct ifaddrs *interfaces_head;
        if (getifaddrs(&interfaces_head) == -1) {
            printf("Error at function %s line %d\n", __FUNCTION__, __LINE__);
            return GET_IF_ADDR_ERROR;
        }
        struct ifaddrs *current_interface = interfaces_head;
        int interfaces_value = 0;
        while (current_interface != NULL) {
            interfaces_value++;
            current_interface = current_interface->ifa_next;
        }
        current_interface = interfaces_head;
        interface *interfaces = NULL;
        printf("Interfaces value:%d\n",interfaces_value);
        int counter = 0;
        while (current_interface != NULL) {
            int family = current_interface->ifa_addr->sa_family;
            if(family == AF_INET){
                char host[NI_MAXHOST];
                memset(host,0,NI_MAXHOST);
                if(getnameinfo(current_interface->ifa_addr,sizeof(struct sockaddr_in),host,NI_MAXHOST,NULL,0,NI_NUMERICHOST) != 0){
                    printf("Error at function %s line %d",__FUNCTION__,__LINE__);
                    exit(GET_NAME_INFO_ERROR);
                }
                if(CheckInterface(interfaces,current_interface->ifa_name) == NOT_EXISTS){
                    interface *new_interface = AddInterface(&interfaces,current_interface->ifa_name);
                    strcpy(new_interface->name,current_interface->ifa_name);
                        strcpy(new_interface->ipv4_address, host);


                }
                else{
                    interface *current = interfaces;
                    while(strcmp(current->name,current_interface->ifa_name) != 0){
                        current = current->next;
                    }
                    strcpy(current->name,current_interface->ifa_name);

                        strcpy(current->ipv4_address, host);


                };
                //
                // strcpy(interfaces[interface_number].ipv4_address,host);
            }
            else if(family == AF_INET6){
                char host[NI_MAXHOST];
                memset(host,0,NI_MAXHOST);
                if(getnameinfo(current_interface->ifa_addr,sizeof(struct sockaddr_in6),host,NI_MAXHOST,NULL,0,NI_NUMERICHOST) != 0){
                    printf("Error at function %s line %d",__FUNCTION__,__LINE__);
                    exit(GET_NAME_INFO_ERROR);
                }
                if(CheckInterface(interfaces,current_interface->ifa_name) == NOT_EXISTS){
                    interface *new_interface = AddInterface(&interfaces,current_interface->ifa_name);
                    strcpy(new_interface->name,current_interface->ifa_name);
                    if(strlen(new_interface->ipv6_address) == 0) {
                        strcpy(new_interface->ipv6_address, host);
                    }
                }
                else{
                    interface *current = interfaces;
                    while(strcmp(current->name,current_interface->ifa_name)){
                        current = current->next;
                    }
                    strcpy(current->name,current_interface->ifa_name);
                    if(strlen(current->ipv6_address) == 0) {
                        strcpy(current->ipv6_address, host);

                    }
                };
                
            }
            else if(family == AF_PACKET){
                struct sockaddr_ll *s= (struct sockaddr_ll*)current_interface->ifa_addr;
                char mac[NI_MAXHOST];
                memset(mac,0,NI_MAXHOST);
                for(int i = 0;i < s->sll_halen;i++){
                    char node[3];
                    snprintf(node,3,"%02x",s->sll_addr[i]);
                    node[2] = '\0';
                    strncat(mac,node,2);
                    (i+1 != s->sll_halen) ? strncat(mac,":",1) : strncat(mac,"\0",1);
                }
                if(CheckInterface(interfaces,current_interface->ifa_name) == NOT_EXISTS){
                    interface *new_interface = AddInterface(&interfaces,current_interface->ifa_name);
                    if(current_interface->ifa_data != NULL) {
                        struct rtnl_link_stats *stats = current_interface->ifa_data;
                        strcpy(new_interface->MAC_address, mac);
                        strcpy(new_interface->name,current_interface->ifa_name);
                        new_interface->tx_bytes = stats->tx_bytes;
                        new_interface->tx_packets = stats->tx_packets;
                        new_interface->rx_bytes = stats->rx_bytes;
                        new_interface->rx_packets = stats->rx_packets;
                    }
                }
                else{
                    interface *current = interfaces;
                    while(current->name != current_interface->ifa_name){
                        current = current->next;
                    }
                    if(current_interface->ifa_data != NULL) {
                        struct rtnl_link_stats *stats = current_interface->ifa_data;
                        strcpy(current->MAC_address, mac);
                        strcpy(current->name,current_interface->ifa_name);
                        current->tx_bytes = stats->tx_bytes;
                        current->tx_packets = stats->tx_packets;
                        current->rx_bytes = stats->rx_bytes;
                        current->rx_packets = stats->rx_packets;
                    }
                };
            }
            current_interface = current_interface->ifa_next;
        };


        interface *print_current = interfaces;
        while(print_current != NULL){
            printf("Name:%s\nIPv4:%s\nIPv6:%s\nMAC address:%s\nrx packets = %10u rx bytes = %10u\ntx packets = %10u tx bytes = %10u\n\n\n",print_current->name,print_current->ipv4_address,print_current->ipv6_address,print_current->MAC_address,print_current->rx_packets,print_current->rx_bytes,print_current->tx_packets,print_current->tx_bytes);
            print_current = print_current->next;
        }
        freeifaddrs(interfaces_head);
        
    }
}




int CheckInterface(interface *head,char *name){
    interface *current = head;
    while(current != NULL){
        if(strcmp(current->name,name) == 0){
            return EXISTS;
        }
        current = current->next;
    }
    return NOT_EXISTS;
}


interface* AddInterface(interface **head,char *name){
    if(*head == NULL){
        *head = malloc(sizeof(interface));
        interface *tmp = *head;
        strcpy(tmp->name,name);
        tmp->next = NULL;
        return *head;
    }
    else{
        interface *current = *head;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = (interface*)malloc(sizeof(interface));
        strcpy(current->next->name,name);
        current->next->next = NULL;
    }
}

