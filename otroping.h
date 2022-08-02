#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
 
//Definimos el tamaño del bloque -o packet- a enviar
 int ttl_val=64;
#define PING_PKT_S 64
// Puerto automático por defecto
#define PORT_NO 0
 
//Definimos la velocidad de envío mlseg
#define PING_SLEEP_RATE 10
 
// Da el retraso de tiempo de espera para recibir paquetes, en segundos
#define RECV_TIMEOUT 1
 
// Definimos constante para bucle de ping repetitivo
int pingloop=1;
 
 
// Definimos la estructura
/*Como utilizaremos socket raw (SOCK_RAW):
 Estos sockets nos permiten el acceso a los protocolos de comunicaciones,
 con la posibilidad de hacer uso o no de protocolos de capa 3
 (nivel de red) y/o 4 (nivel de transporte),
 y por lo tanto dándonos el acceso a los protocolos
 directamente y a la información que reciben en ellos.
 El uso de sockets de este tipo nos va a permitir
 la implementación de nuevos protocolos, y por que no decirlo,
 la modificación de los ya existentes. Lo que implica
 el poder cambiar parámetros, incluso en diseño, del paquete
 a enviar*/
 
 
struct ping_pkt
{
    struct icmphdr hdr;
    char msg[PING_PKT_S-sizeof(struct icmphdr)];
};
 
/*
Cuando se calcula la suma de verificación,
el campo de suma de verificación primero debe borrarse a 0.
Cuando se transmite el paquete de datos,
se calcula la suma de verificación y se inserta en este campo.
Cuando se recibe el paquete de datos,
la suma de verificación se calcula nuevamente y
se verifica contra el campo de suma de verificación.
Si las dos sumas de verificación no coinciden,
se ha producido un error.*/
 
 
unsigned short checksum(void *b, int len)
{    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;
 //Comprobacion de la suma.
    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}
 
 
// Funcion controlador de interrupciones
 
void manejador(int modelo)
 
 
{
    pingloop=0;//Bucle
}
 
/*
Estructuras necesarias para realizar una busqueda
DNS. Si la entrada por argumento de main no es por IP,
si no que se realiza por nombre de dominio.
Cada dispositivo conectado a Internet tiene una
dirección IP única que otros equipos pueden usar
para encontrarlo. Los servidores DNS suprimen
la necesidad de que los humanos memoricen direcciones
IP tales como 192.168.1.1 (en IPv4) o nuevas.
Con la equivalencia de un nombre de dominio
Por ejemplo www.minombre.com.
*/
 
 
char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con)
{
    printf("\n RESOLUCION  DNS..\n");
    struct hostent *host_entity;
    char *ip=(char*)malloc(NI_MAXHOST*sizeof(char));
    int i;
 
    if ((host_entity = gethostbyname(addr_host)) == NULL)
    {
        // No encontramos IP asociada a host.
        return NULL;
    }
 
    //Llenando la estructura de direcciones.
    strcpy(ip, inet_ntoa(*(struct in_addr *) host_entity->h_addr));
 
    (*addr_con).sin_family = host_entity->h_addrtype;
    (*addr_con).sin_port = htons (PORT_NO);
    /*La función htons convierte un
    u_short de host a orden de bytes de red TCP/IP, o (PORT_NO)
    (que es big-endian)*/
 
 
    (*addr_con).sin_addr.s_addr  = *(long*)host_entity->h_addr;
 
    return ip;
 
}
 
/*Determinar la dirección de dominio o el nombre del hostname
a partir de una dirección IP concreta, de forma inversa.
DNS -inversa-proporciona no solo la resolución del nombre
o dirección IP, sino también podría facilitarnos alguna
información adicional. De esta forma,
además del nombre de host deseado, con un Lookup
también se podría obtener una asignación geográfica de la IP,
e información sobre el proveedor de servicios de Internet responsable.
Una herramienta habitual usada por consola de LINUX es el comando
nslookup. -$ nslookup-
*/
char* reverse_dns_lookup(char *ip_addr)
{
    struct sockaddr_in temp_addr;
    socklen_t len;
    char buf[NI_MAXHOST], *ret_buf;
 
    temp_addr.sin_family = AF_INET;
    temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
    len = sizeof(struct sockaddr_in);
 
    if (getnameinfo((struct sockaddr *) &temp_addr, len, buf,
                    sizeof(buf), NULL, 0, NI_NAMEREQD))
    {
        printf("No se resuelve el reverse lookup de hostname\n");
        return NULL;
    }
    ret_buf = (char*)malloc((strlen(buf) +1)*sizeof(char) );
    strcpy(ret_buf, buf);
    return ret_buf;
}
 
/*
El código permite probar numerosos parámetros del paquete ping.
Modificando los #define de cabecera.
Las opciones de la línea de comandos de la utilidad
ping y su salida varían entre las numerosas implementaciones.
Las opciones pueden incluir el tamaño de la carga útil,
la cantidad de pruebas, los límites para la cantidad de saltos
de red (TTL) que atraviesan los sistemas de comunicación,
el intervalo entre las solicitudes, y el tiempo
de espera para una respuesta. 
*/
void Enviando_Ping(int ping_sockfd, struct sockaddr_in *ping_addr,
                char *ping_dom, char *ping_ip, char *rev_host)
{
     int msg_count=0, i, addr_len, flag=1,msg_received_count=0;
 
    struct ping_pkt pckt;
    struct sockaddr_in r_addr;
    struct timespec time_start, time_end, tfs, tfe;
    long double rtt_msec=0, total_msec=0;
    struct timeval tv_out;
    tv_out.tv_sec = RECV_TIMEOUT;
    tv_out.tv_usec = 0;
 
    clock_gettime(CLOCK_MONOTONIC, &tfs);
   /*
La clock_gettime función () lee el reloj dado y escribe su valor absoluto.
El reloj puede ser un valor devuelto por
por la constante CLOCK_MONOTONIC, direccion &tfs
El reloj "monótono".
Su valor absoluto no tiene sentido.
La marca del tiempo  comienza en un punto
positivo indefinido y avanza continuamente.
De "monótono", es decir, de tal manera que nunca
disminuye o nunca aumenta.
   */
//************************************************************
     /* En el programa establecimos
      las opciones de socket en ip de TTL al valor a 64,
      con el fin de hacer pruebas se puede cambiar este valor
      int ttl_val=64.
      **************
     Llamamos tiempo  de vida (TTL) para hacer referencia a
     la cantidad de tiempo o "saltos" que se han
     establecido, y que un paquete debe existir dentro
     de una red antes de ser descartado por un enrutador de red.
     El TTL también se utiliza en otros contextos,
     como el almacenamiento en caché de CDN y
     el almacenamiento en caché de DNS.CDN permite acelerar la carga
     de las páginas, mejorar los tiempos de respuesta
     y la experiencia de usuario, proteger los datos,
      mejorar el posicionamiento de los sitios web
      y reducir el consumo de ancho de banda en cada uno de los países.
*/
    if (setsockopt(ping_sockfd, SOL_IP, IP_TTL,
               &ttl_val, sizeof(ttl_val)) != 0)
    {
        printf("La configuración de las opciones de socket en TTL ha fallado!\n");
 
    }
 
    else
    {
        printf("CORRECTO TTL..\n");
    }
 
    /*
     Con las siguiente función -setsockopt-
     configuramos el tiempo de espera de la recepción
    */
 
 
    setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&tv_out, sizeof tv_out);
 
    /*Enviamos el paquete icmp en un bucle infinito.
    Podríamos condicionar con un for o con un while, y la opción
    break el número de paquetes a enviar. En principio hemos establecido
    el bucle infinito. */
 
    while(pingloop)
    {
        /* Esta bandera,flag=1, o etiqueta, identificará si el
        paquete fue enviado o no */
 
        flag=1;
 
        //con sizeof añadimos un campo de relleno con el fin de ajustar el tamaño de este.
        bzero(&pckt, sizeof(pckt));
 
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid();
 
        for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
            pckt.msg[i] = i+'0';
 
        pckt.msg[i] = 0;
        pckt.hdr.un.echo.sequence = msg_count++;
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
 
 
        usleep(PING_SLEEP_RATE);
 
        /*Enviando el paquete*/
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        if ( sendto(ping_sockfd, &pckt, sizeof(pckt), 0,
           (struct sockaddr*) ping_addr,
            sizeof(*ping_addr)) <= 0)
        {
            printf("Fallo de paquete enviado\n");
            flag=0;
        }
 
        //Comprobamos que el paquete ha sido recibido
        addr_len=sizeof(r_addr);
 
        if ( recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0,
             (struct sockaddr*)&r_addr, &addr_len) <= 0
              && msg_count>1)
        {
            printf("Atención el paquete ha fallado!\n");
        }
 
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &time_end);
 
            double timeElapsed = ((double)(time_end.tv_nsec -time_start.tv_nsec))/1000000.0;
            rtt_msec = (time_end.tv_sec- time_start.tv_sec) * 1000.0 + timeElapsed;
 
            // Si el paquete no fue enviado
            if(flag)
            {
                if(!(pckt.hdr.type ==69 && pckt.hdr.code==0))
                {
                    printf("Error..Paquetes recibidos ICMP   Tipo %d codigo  %d\n",pckt.hdr.type, pckt.hdr.code);
                }
                else
                {
                    printf("%d bytes from %s (h: %s)(%s) msg_seq=%d ttl=%d rtt = %Lf ms.\n",PING_PKT_S, ping_dom, rev_host,ping_ip, msg_count,ttl_val, rtt_msec);
 
                    msg_received_count++;
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tfe);
    double timeElapsed = ((double)(tfe.tv_nsec -tfs.tv_nsec))/1000.0;
 
    total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+ timeElapsed;
 
    printf("Estadísticas de ping a----> %s\n", ping_ip);
    printf("Paquetes enviados=%d, Paquetes recibidos=%d, Paquetes perdidos=%f. Tiempo de envío= %Lf ms.\n",msg_count, msg_received_count,((msg_count - msg_received_count)/msg_count) * 100.0, total_msec);
}