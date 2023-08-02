# Mellon-API service

## Setup

* Create directory `/etc/mellon-api/`.
* Copy `main.conf` from `misc/Config Directory` to `/etc/mellon-api/`.
* * Create user mellon-api using `adduser --system --group --no-create-home mellon-api`.
* Create database path and change `databasePath` in `main.conf` accordingly. Make the directory writeable for the user `mellon-api`.
* Create directory `/var/log/mellon-api/` and make it writeable for user `mellon-api`.
* Create server certificates, place them and the CA certificate in `/etc/mellon-api/` and change `main.conf` to point to these files. 

## Add administrative certificate to database

```
certtool --generate-privkey --key-type ecdsa --curve secp384r1 --outfile privkey.pem
certtool --generate-request --no-crq-extensions --load-privkey privkey.pem --outfile request.pem
mellonbot -c /etc/mellon/signer.crt -k /etc/mellon/signer.key -s 4 -l request.pem
# or
# mellon-cli -d /dev/ttyUSB0 signX509Csr 4 request.pem
```