**Tarea 1: Servidor y Cliente**

## Instrucciones Server
```bash
cd ./Tarea\ 1/
make
sudo cp ./build/server /usr/local/bin/server
sudo cp ./Service/ImagServer.service /etc/systemd/system/ImagServer.service
sudo cp ./Service/ImagServer.conf /etc/ImagServer.conf
sudo touch /var/log/imageserver.log
sudo systemctl start ImagServer.service
sudo systemctl status ImagServer.service
```

## Instrucciones Server
```bash
sudo docker build -t image_client .
sudo docker run --rm -it \
  --add-host=host.docker.internal:host-gateway \
  -v "$(pwd)/imgs:/app/imgs" \
  --entrypoint /bin/sh \
  image_client
./client
```

## Otras instrucciones utiles

```bash
sudo systemctl stop ImagServer.service
sudo systemctl restart ImagServer.service
sudo systemctl daemon-reload
```


## Direcciones
- Unit file systemd
```bash
/etc/systemd/system/ImagServer.service
```

- Archivo de configuracion 
```bash
/etc/ImagServer.conf
```

- Log del Servidor
```bash
/var/log/imageserver.log
```

- Ejecutable
```bash
/usr/local/bin/server
```

