# Cliente FTP Concurrente

Proyecto de cliente FTP desarrollado en C. Implementa el protocolo FTP básico y concurrencia con procesos.

## Compilación
> **Nota Importante:** Para compilar correctamente, asegúrese de que los archivos auxiliares (`connectsock.c`, `connectTCP.c`, `errexit.c`) se encuentren en la misma carpeta que el código fuente.

```bash
make
```

## Uso
```bash
./ZunigaL-clienteFTP <IP_SERVIDOR>
```

## Comandos Soportados
* **get / put**: Transferencia de archivos (Concurrente).
* **ls / pwd / cd / mkdir / del**: Gestión de archivos y directorios.
* **quit**: Salir.
