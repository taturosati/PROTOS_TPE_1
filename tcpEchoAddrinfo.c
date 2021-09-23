#include "tcpEchoAddrinfo.h"

#define US_ASCII(x) ((x < 0 || x > 127) ? (0) : (1))

typedef struct t_buffer {
	char* buffer;
	size_t len;     // longitud del buffer
	size_t from;    // desde donde falta escribir
} t_buffer;

int main(int argc, char* argv[])
{
	int opt = TRUE;
	int master_socket;  // IPv4 e IPv6 (si estan habilitados)
	int new_socket, client_socket[MAX_SOCKETS], max_clients = MAX_SOCKETS, activity, i, sd;
	long valread;
	int max_sd;
	struct sockaddr_in address = { 0 };

	int port_used = PORT;

	if (argc > 1) {
		int received_port = atoi(argv[1]);
		log(DEBUG, "%s\n", argv[1]);

		if (received_port > 0 && received_port > MIN_PORT) {
			port_used = received_port;
		}

	}

	socklen_t addrlen = sizeof(address);
	struct sockaddr_storage clntAddr; // Client address
	socklen_t clntAddrLen = sizeof(clntAddr);

	char buffer[BUFFSIZE + 1];  //data buffer of 1K

	fd_set readfds; //set of socket descriptors

	t_buffer bufferWrite[MAX_SOCKETS]; //buffer de escritura asociado a cada socket, para no bloquear por escritura
	memset(bufferWrite, 0, sizeof bufferWrite);

	fd_set writefds;

	memset(client_socket, 0, sizeof(client_socket));

	master_socket = setupTCPServerSocket(port_used);

	int udpSock = udpSocket(PORT);
	if (udpSock < 0) {
		log(FATAL, "UDP socket failed");
	}
	else {
		log(DEBUG, "Waiting for UDP IPv4 on socket %d\n", udpSock);
	}

	// Limpiamos el conjunto de escritura
	FD_ZERO(&writefds);
	while (TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add masters sockets to set
		FD_SET(master_socket, &readfds);

		FD_SET(udpSock, &readfds);

		max_sd = udpSock;

		// add child sockets to set
		for (i = 0; i < max_clients; i++)
		{
			// socket descriptor
			sd = client_socket[i];

			// if valid socket descriptor then add to read list
			if (sd > 0){
				FD_SET(sd, &readfds);
				max_sd = max(sd, max_sd);
			}
		}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select(max_sd + 1, &readfds, &writefds, NULL, NULL);

		log(DEBUG, "select has something...");

		if ((activity < 0) && (errno != EINTR))
		{
			log(ERROR, "select error, errno=%d", errno);
			continue;
		}

		// Servicio UDP
		if (FD_ISSET(udpSock, &readfds)) {
			handleAddrInfo(udpSock);
		}

		//If something happened on the TCP master socket , then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = acceptTCPConnection(master_socket)) < 0)
			{
				log(ERROR, "Accept error on master socket %d", master_socket);
				continue;
			}

			// add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
			{
				// if position is empty
				if (client_socket[i] == 0)
				{
					client_socket[i] = new_socket;
					log(DEBUG, "Adding to list of sockets as %d\n", i);
					break;
				}
			}
		}

		for (i = 0; i < max_clients; i++) {
			sd = client_socket[i];

			if (FD_ISSET(sd, &writefds)) {
				handleWrite(sd, bufferWrite + i, &writefds);
			}
		}

		//else its some IO operation on some other socket :)
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];

			if (FD_ISSET(sd, &readfds))
			{
				//Check if it was for closing , and also read the incoming message
				if ((valread = read(sd, buffer, BUFFSIZE)) <= 0)
				{


					//Somebody disconnected , get his details and print

					// if (getpeername(sd, (struct sockaddr *) &address, &addrlen) < 0) {
					// 	perror("");
					// 	log(ERROR, "Get peer name failed");
					// } 

					// log(INFO, "Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

					//Close the socket and mark as 0 in list for reuse
					close(sd);
					client_socket[i] = 0;

					FD_CLR(sd, &writefds);
					// Limpiamos el buffer asociado, para que no lo "herede" otra sesión
					clear(bufferWrite + i);
				}

				// ECHO Hola como va ? ¥r

				//ECH
				else {
					//FLAGS
					// flag de ya recibi comando (ECHO, DATE, TIME) --> ya los parsie perfecto
					// flag si llego ¥r
					// flag de si ya pase los 100 caracteres
					// contador de cuanto leiste

					typedef struct sendBuffer {
						char buffer[95];
						
					}

					// int read_command = 0;
					// char command[10] = { 0 };
					// char toSend[95] = { 0 };
					// for (int j = 0; j < 100 && US_ASCII(buffer[j]); j++) {
					// 	if (read_command == 0 && buffer[j] == ' ') {
					// 		if (strcmp("ECHO", command) == 0) {
					// 			read_command = 1;
					// 		}
					// 		else if (strcmp("GET", command) == 0) {
					// 			read_command = 2;
					// 		}
					// 		continue;
					// 	}

					// 	if (read_command == 0 && j < 10) {
					// 		command[j] = buffer[j];
					// 	}

					// 	if (buffer[j] == '\r') {
					// 		if (buffer[j + 1] == '\n') {
					// 			//handleWrite(sd, toSend, client_socket[i]);
					// 			break;
					// 		}
					// 	}

					// 	if (read_command != 0) {
					// 		toSend[i] = buffer[i];
					// 	}
					// }
					log(DEBUG, "Received %zu bytes from socket %d\n", valread, sd);
					// activamos el socket para escritura y almacenamos en el buffer de salida
					FD_SET(sd, &writefds);

					// Tal vez ya habia datos en el buffer
					// TODO: validar realloc != NULL
					bufferWrite[i].buffer = realloc(bufferWrite[i].buffer, bufferWrite[i].len + valread);
					memcpy(bufferWrite[i].buffer + bufferWrite[i].len, buffer, valread);
					bufferWrite[i].len += valread;
				}
			}

		}
	}

	return 0;
}

void clear(t_buffer_ptr buffer) {
	free(buffer->buffer);
	buffer->buffer = NULL;
	buffer->from = buffer->len = 0;
}

// Hay algo para escribir?
// Si está listo para escribir, escribimos. El problema es que a pesar de tener buffer para poder
// escribir, tal vez no sea suficiente. Por ejemplo podría tener 100 bytes libres en el buffer de
// salida, pero le pido que mande 1000 bytes.Por lo que tenemos que hacer un send no bloqueante,
// verificando la cantidad de bytes que pudo consumir TCP.
void handleWrite(int socket, t_buffer_ptr buffer, fd_set* writefds) {
	size_t bytesToSend = buffer->len - buffer->from;
	if (bytesToSend > 0) {  // Puede estar listo para enviar, pero no tenemos nada para enviar
		log(INFO, "Trying to send %zu bytes to socket %d\n", bytesToSend, socket);
		size_t bytesSent = send(socket, buffer->buffer + buffer->from, bytesToSend, MSG_DONTWAIT);
		log(INFO, "Sent %zu bytes\n", bytesSent);

		if (bytesSent < 0) {
			// Esto no deberia pasar ya que el socket estaba listo para escritura
			// TODO: manejar el error
			log(FATAL, "Error sending to socket %d", socket);
		}
		else {
			size_t bytesLeft = bytesSent - bytesToSend;

			// Si se pudieron mandar todos los bytes limpiamos el buffer y sacamos el fd para el select
			if (bytesLeft == 0) {
				clear(buffer);
				FD_CLR(socket, writefds);
			}
			else {
				buffer->from += bytesSent;
			}
		}
	}
}

int udpSocket(int port) {

	int sock;
	struct sockaddr_in serverAddr;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log(ERROR, "UDP socket creation failed, errno: %d %s", errno, strerror(errno));
		return sock;
	}
	log(DEBUG, "UDP socket %d created", sock);
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET; // IPv4cle
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(sock, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		log(ERROR, "UDP bind failed, errno: %d %s", errno, strerror(errno));
		close(sock);
		return -1;
	}
	log(DEBUG, "UDP socket bind OK ");

	return sock;
}


void handleAddrInfo(int socket) {
	// En el datagrama viene el nombre a resolver
	// Se le devuelve la informacion asociada

	char buffer[BUFFSIZE];
	unsigned int len, n;

	struct sockaddr_in clntAddr;

	// Es bloqueante, deberian invocar a esta funcion solo si hay algo disponible en el socket    
	n = recvfrom(socket, buffer, BUFFSIZE, 0, (struct sockaddr*)&clntAddr, &len);
	if (buffer[n - 1] == '\n') // Por si lo estan probando con netcat, en modo interactivo
		n--;
	buffer[n] = '\0';
	log(DEBUG, "UDP received:%s", buffer);
	// TODO: parsear lo recibido para obtener nombre, puerto, etc. Asumimos viene solo el nombre

	// Especificamos solo SOCK_STREAM para que no duplique las respuestas
	struct addrinfo addrCriteria;                   // Criteria for address match
	memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;             // Any address family
	addrCriteria.ai_socktype = SOCK_STREAM;         // Only stream sockets
	addrCriteria.ai_protocol = IPPROTO_TCP;         // Only TCP protocol


	// Armamos el datagrama con las direcciones de respuesta, separadas por \r\n
	// TODO: hacer una concatenacion segura
	// TODO: modificar la funcion printAddressInfo usada en sockets bloqueantes para que sirva
	//       tanto si se quiere obtener solo la direccion o la direccion mas el puerto
	char bufferOut[BUFFSIZE];
	bufferOut[0] = '\0';

	struct addrinfo* addrList;
	int rtnVal = getaddrinfo(buffer, NULL, &addrCriteria, &addrList);
	if (rtnVal != 0) {
		log(ERROR, "getaddrinfo() failed: %d: %s", rtnVal, gai_strerror(rtnVal));
		strcat(strcpy(bufferOut, "Can't resolve "), buffer);
	}
	else {
		for (struct addrinfo* addr = addrList; addr != NULL; addr = addr->ai_next) {
			struct sockaddr* address = addr->ai_addr;
			char addrBuffer[INET6_ADDRSTRLEN];

			void* numericAddress = NULL;
			switch (address->sa_family) {
			case AF_INET:
				numericAddress = &((struct sockaddr_in*)address)->sin_addr;
				break;
			case AF_INET6:
				numericAddress = &((struct sockaddr_in6*)address)->sin6_addr;
				break;
			}
			if (numericAddress == NULL) {
				strcat(bufferOut, "[Unknown Type]");
			}
			else {
				// Convert binary to printable address
				if (inet_ntop(address->sa_family, numericAddress, addrBuffer, sizeof(addrBuffer)) == NULL)
					strcat(bufferOut, "[invalid address]");
				else {
					strcat(bufferOut, addrBuffer);
				}
			}
			strcat(bufferOut, "\r\n");
		}
		freeaddrinfo(addrList);
	}

	// Enviamos respuesta (el sendto no bloquea)
	sendto(socket, bufferOut, strlen(bufferOut), 0, (const struct sockaddr*)&clntAddr, len);

	log(DEBUG, "UDP sent:%s", bufferOut);

}

