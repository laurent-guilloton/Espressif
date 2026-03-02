# Guide d'Installation Proxmox avec Configuration Proxy

## Architecture Réseau
- **Serveur Proxmox**: 172.20.6.0/24
- **Proxy HTTP**: proxy-http.esisar.grenoble-inp.fr:3128
- **VMs**: Réseau privé NAT derrière Proxmox

## 1. Installation de Proxmox VE

### 1.1 Téléchargement et Préparation
```bash
# Télécharger l'image ISO Proxmox VE
wget https://enterprise.proxmox.com/iso/proxmox-ve_8.2-2.iso

# Créer une clé USB bootable (sur Linux)
dd if=proxmox-ve_8.2-2.iso of=/dev/sdX bs=4M status=progress
```

### 1.2 Installation
1. Démarrer sur l'ISO Proxmox
2. Choisir "Install Proxmox VE"
3. Configuration:
   - **Target Harddisk**: Sélectionner le disque principal
   - **Location and Timezone**: France/Paris
   - **Password**: Mot de passe root
   - **Email**: Email administrateur
   - **Management Network Configuration**:
     - **Management Interface**: eth0 (ou interface principale)
     - **Hostname (FQDN)**: proxmox.votre-domaine.local
     - **IP Address (CIDR)**: 172.20.6.X/24 (remplacer X par votre adresse)
     - **Gateway**: 172.20.6.1
     - **DNS Server**: 8.8.8.8 ou votre DNS interne
   - **OU** si vous préférez configurer via DHCP après installation:
     - Cocher "Configure network later"

## 2. Configuration Post-Installation

### 2.1 Configuration du Proxy Système
```bash
# Éditer les variables d'environnement proxy
nano /etc/environment

# Ajouter les lignes suivantes:
http_proxy=http://proxy-http.esisar.grenoble-inp.fr:3128
https_proxy=http://proxy-http.esisar.grenoble-inp.fr:3128
ftp_proxy=http://proxy-http.esisar.grenoble-inp.fr:3128
no_proxy=localhost,127.0.0.1,172.20.6.0/24
HTTP_PROXY=http://proxy-http.esisar.grenoble-inp.fr:3128
HTTPS_PROXY=http://proxy-http.esisar.grenoble-inp.fr:3128
FTP_PROXY=http://proxy-http.esisar.grenoble-inp.fr:3128
NO_PROXY=localhost,127.0.0.1,172.20.6.0/24
```

### 2.2 Configuration APT pour le Proxy
```bash
# Créer le fichier de configuration proxy pour APT
nano /etc/apt/apt.conf.d/95proxies

# Ajouter:
Acquire::http::Proxy "http://proxy-http.esisar.grenoble-inp.fr:3128";
Acquire::https::Proxy "http://proxy-http.esisar.grenoble-inp.fr:3128";
Acquire::ftp::Proxy "http://proxy-http.esisar.grenoble-inp.fr:3128";
```

### 2.3 Configuration du Dépôt No-Subscription
```bash
# Supprimer le dépôt enterprise (nécessite une clé)
rm /etc/apt/sources.list.d/pve-enterprise.list

# Ajouter le dépôt no-subscription
echo "deb http://download.proxmox.com/debian/pve bookworm pve-no-subscription" > /etc/apt/sources.list.d/pve-install-repo.list

# Ajouter la clé GPG
wget https://enterprise.proxmox.com/debian/proxmox-release-bookworm.gpg -O /etc/apt/trusted.gpg.d/proxmox-release-bookworm.gpg

# Mettre à jour les paquets
apt update
apt upgrade -y

# Redémarrer pour appliquer toutes les configurations
reboot
```

## 3. Configuration Réseau pour les VMs

### 3.1 Configuration du Bridge Linux
```bash
# Éditer la configuration réseau
nano /etc/network/interfaces

# Configuration recommandée avec DHCP pour l'interface principale:
auto lo
iface lo inet loopback

auto eno1  # Remplacer par votre interface physique
iface eno1 inet dhcp

auto vmbr0
iface vmbr0 inet manual
    bridge-ports eno1
    bridge-stp off
    bridge-fd 0
    bridge-vlan-aware yes
    bridge-vids 2-4094

# Réseau privé pour les VMs (NAT)
auto vmbr1
iface vmbr1 inet static
    address 192.168.100.1/24
    bridge-ports none
    bridge-stp off
    bridge-fd 0

# Appliquer la configuration
ifupdown2 -a
```

### 3.1.1 Alternative: Configuration avec IP statique sur vmbr0
```bash
# Si vous préférez que vmbr0 ait l'IP DHCP au lieu de l'interface physique:
auto lo
iface lo inet loopback

auto eno1
iface eno1 inet manual

auto vmbr0
iface vmbr0 inet dhcp
    bridge-ports eno1
    bridge-stp off
    bridge-fd 0

# Réseau privé pour les VMs (NAT)
auto vmbr1
iface vmbr1 inet static
    address 192.168.100.1/24
    bridge-ports none
    bridge-stp off
    bridge-fd 0
```

### 3.2 Configuration du NAT et Routage
```bash
# Activer le forwarding IP
echo 'net.ipv4.ip_forward=1' >> /etc/sysctl.conf
sysctl -p

# Configuration iptables pour le NAT
nano /etc/network/if-post-down.d/iptables

# Ajouter:
#!/bin/sh
iptables-restore < /etc/iptables.rules

# Créer les règles iptables (adapter l'interface selon votre configuration)
# Si eno1 a l'IP DHCP:
iptables -t nat -A POSTROUTING -s 192.168.100.0/24 -o eno1 -j MASQUERADE
iptables -A FORWARD -i vmbr1 -o eno1 -j ACCEPT
iptables -A FORWARD -i eno1 -o vmbr1 -m state --state RELATED,ESTABLISHED -j ACCEPT

# Si vmbr0 a l'IP DHCP (alternative):
# iptables -t nat -A POSTROUTING -s 192.168.100.0/24 -o vmbr0 -j MASQUERADE
# iptables -A FORWARD -i vmbr1 -o vmbr0 -j ACCEPT
# iptables -A FORWARD -i vmbr0 -o vmbr1 -m state --state RELATED,ESTABLISHED -j ACCEPT

# Sauvegarder les règles
iptables-save > /etc/iptables.rules
chmod +x /etc/network/if-post-down.d/iptables
```

### 3.3 Configuration DHCP pour les VMs
```bash
# Installer dnsmasq
apt install dnsmasq -y

# Configurer dnsmasq
nano /etc/dnsmasq.conf

# Ajouter:
interface=vmbr1
dhcp-range=192.168.100.100,192.168.100.200,12h
dhcp-option=option:router,192.168.100.1
dhcp-option=option:dns-server,8.8.8.8,8.8.4.4
server=8.8.8.8
server=8.8.4.4

# Démarrer dnsmasq
systemctl enable dnsmasq
systemctl start dnsmasq
```

## 4. Configuration du Proxy dans Proxmox

### 4.1 Configuration du Proxy pour les Templates
```bash
# Éditer la configuration Proxmox
nano /etc/proxmox/templates

# Ajouter la configuration proxy si nécessaire
```

### 4.2 Téléchargement des Templates
```bash
# Accéder à l'interface web Proxmox
https://172.20.6.X:8006

# Dans Datacenter -> storage -> local -> Templates
# Télécharger les templates Ubuntu, Debian, etc.
```

## 5. Création des VMs

### 5.1 Étapes de Création
1. **Créer une VM**:
   - Node: votre serveur Proxmox
   - VM ID: numéro unique (100, 101, etc.)
   - Name: nom de la VM
   - OS: utiliser un template ou ISO
   - System: BIOS/UEFI, Machine q35
   - Disks: taille du disque (20GB minimum)
   - CPU: nombre de cœurs
   - Memory: RAM (2GB minimum)
   - Network: Model VirtIO, Bridge vmbr1

2. **Configuration Réseau VM**:
   - Les VMs obtiendront une IP en 192.168.100.x via DHCP
   - Accès internet via NAT sur vmbr0

### 5.2 Configuration Proxy dans les VMs
```bash
# Dans chaque VM, configurer le proxy:
export http_proxy=http://proxy-http.esisar.grenoble-inp.fr:3128
export https_proxy=http://proxy-http.esisar.grenoble-inp.fr:3128
export ftp_proxy=http://proxy-http.esisar.grenoble-inp.fr:3128

# Ajouter au profil utilisateur
echo 'export http_proxy=http://proxy-http.esisar.grenoble-inp.fr:3128' >> ~/.bashrc
echo 'export https_proxy=http://proxy-http.esisar.grenoble-inp.fr:3128' >> ~/.bashrc
```

## 6. Accès aux Services

### 6.1 Accès Web Proxmox
- **URL**: https://[IP_DHCP_obtenue]:8006
- **Pour trouver votre IP**: `ip addr show eno1` ou `ip addr show vmbr0`
- **Utilisateur**: root@pam
- **Mot de passe**: mot de passe root configuré

### 6.2 Accès aux VMs
- **Console**: via l'interface web Proxmox
- **SSH**: depuis le serveur Proxmox vers 192.168.100.x
- **Port Forwarding** (optionnel):
```bash
# Exemple: forwarder le port 2222 vers le port 22 d'une VM
# Adapter l'interface selon votre configuration (eno1 ou vmbr0)
iptables -t nat -A PREROUTING -i eno1 -p tcp --dport 2222 -j DNAT --to 192.168.100.101:22
# Ou si vmbr0 a l'IP DHCP:
# iptables -t nat -A PREROUTING -i vmbr0 -p tcp --dport 2222 -j DNAT --to 192.168.100.101:22
```

## 7. Maintenance et Surveillance

### 7.1 Scripts de Maintenance
```bash
# Script de sauvegarde automatique
nano /usr/local/bin/backup_vms.sh

#!/bin/bash
vzdump --all --compress lzo --storage local --remove 1
chmod +x /usr/local/bin/backup_vms.sh

# Ajouter au crontab pour sauvegardes quotidiennes
echo "0 2 * * * /usr/local/bin/backup_vms.sh" | crontab -
```

### 7.2 Surveillance
```bash
# Installer des outils de surveillance
apt install htop iotop nethogs -y

# Vérifier l'état des VMs
qm list
```

## 8. Dépannage

### 8.1 Problèmes Courants
- **VM sans accès internet**: vérifier iptables et le forwarding
- **Proxy ne fonctionne pas**: vérifier la configuration dans /etc/environment
- **Bridge réseau**: vérifier la configuration dans /etc/network/interfaces

### 8.2 Commandes Utiles
```bash
# Vérifier les règles iptables
iptables -t nat -L -n -v

# Vérifier les bridges
brctl show

# Redémarrer le réseau
systemctl restart networking

# Vérifier le forwarding
cat /proc/sys/net/ipv4/ip_forward
```

## Notes Importantes
1. L'adresse IP 172.20.6.X sera obtenue automatiquement via DHCP
2. Pour connaître votre IP: `ip addr show eno1` ou `hostname -I`
3. Adapter les noms d'interfaces réseau (eno1, enpXsY, etc.)
4. Deux options de configuration: DHCP sur l'interface physique ou sur le bridge
5. Utiliser le dépôt no-subscription pour les mises à jour gratuites
6. Sauvegarder régulièrement la configuration et les VMs
7. Documenter les adresses IP attribuées aux VMs
8. Configurer un pare-feu si nécessaire

### Commandes utiles pour DHCP
```bash
# Vérifier l'IP obtenue
ip addr show eno1
# Ou
hostname -I

# Renouveler le bail DHCP
dhclient eno1

# Vérifier la configuration réseau
ip route show
```
