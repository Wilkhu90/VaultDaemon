# VaultDaemon
A light-weight daemon written in Pure C lang for rotating HashiCorp Vault secrets like ssh keys, passwords etc. on remote nodes. The code expect you to use Dynamic Secrets in HashiCorp Vault.

## Build
```ruby
yum -y install libcurl libcurl-devel
gcc -lcurl VaultDaemon.c libjsmn.a -o VaultDaemon
```

## Deploy
```ruby
./VaultDaemon <VAULT TOKEN> <VAULT_HOST>/secret/ssh_key
```
## Verify
Just check the system logs based on the OS and you should be able to see an entry every 30 seconds regarding Vault daemon.
- Centos - /var/log/messages
- Ubuntu - /var/log/syslog
