###**Cliente - Servidor UDP**

###**CLIENTE**

Como no es orientado a conexion, el cliente hace menos pasos

Tiene que hacer toda la creacion y binding del socket pero la conexion nno la tiene

Aca no iteramos para ver que addreinfo usamos. Agarramos la primera directamente

addrCriteria.ai_socketype = SOCK_DGRAM --> datagramas, osea sockets

En main

Recibimos la ip del servidor, el string a mandar, numero de puerto o servicio

Si no me pasaron el puerto, pasa string echo directamente.

Luego, creamos el socket --> Puede fallar --> puede no haber mas fd o pase alguno de los valores invalidos

**sendto**

--> necesita el socket, el string, la longitud del string, la dirección del servidor y la longitud de la dirección
--> puede fallar: puedo mandar un tamaño que sea mas grande que el permitidos
udp le dice a capa ip --> mandamelo a tal ip
puede fallar porq no matchea la ip que le paso con la tabla de ruteos --> me devuelve error.

Puede bloquearse por que se llenan los buffers del servidor

##recvfrom (funcion para leer udp)

recive sockfd, buffer de donde leer, longitud a leer, ip de donde viene y su tamaño

**es bloqueante**

devuelve numero de bytes leidos

Hay que autenticar la respuesta

Para ello usamos sockAddrsEqual

Es como el equals de POO pero para sockaddr

Con validar el fromaddres NO BASTA, puede haber ip spoofing o me pueden mandar un paquete armado con ipRAW

---

###**SERVIDOR**

AI_PASSIVE --> definimos que sea un socket pasivo. a la espera de info

por cada datagrama tengo que quedarme con la info del que me envio para poder responder

recvmsg --> mas completa que recvfrom

me permite leer los bytes de a partes. No se pierden los demas. Podemos leerlos en la otra lectura.

sendto --> igual al del cliente

respondemos al ip que vino del cliente

###**hacemos ciclo infinito para obtener cada datagrama**
