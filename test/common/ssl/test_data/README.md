# What are the identities, certificates and keys
There are 4 identities:
- **CA**: Certificate Authority for **No SAN**, **SAN With URI** and **SAN With
  DNS**. It has the self-signed certificate *ca_cert.pem*. *ca_key.pem* is its
  private key.
- **No SAN**: It has the certificate *no_san_cert.pem*, signed by the **CA**.
  The certificate does not have SAN field. *no_san_key.pem* is its private key.
- **SAN With URI**: It has the certificate *san_uri_cert.pem*, which is signed
  by the **CA** using the config *san_uri_cert.cfg*. The certificate has SAN
  field of URI type. *san_uri_key.pem* is its private key.
- **SAN With DNS**: It has the certificate *san_dns_cert.pem*, which is signed
  by the **CA** using the config *san_dns_cert.cfg*. The certificate has SAN
  field of DNS type. *san_dns_key.pem* is its private key.

# How to update certificates
**certs.sh** has the commands to generate all files except the private key
files. Running certs.sh directly will cause the certificate files to be
regenerated. So if you want to regenerate a particular file, please copy the
corresponding commands from certs.sh and execute them in command line.
