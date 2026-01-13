# 🚀 RADS Constellation - Production Deployment Guide

**v0.0.4 "Constellation" - Complete Production Setup**

---

## 📋 Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Configuration](#configuration)
4. [Cluster Setup](#cluster-setup)
5. [Security Hardening](#security-hardening)
6. [Monitoring Setup](#monitoring-setup)
7. [High Availability](#high-availability)
8. [Backup & Recovery](#backup--recovery)
9. [Troubleshooting](#troubleshooting)
10. [Best Practices](#best-practices)

---

## 🎯 Prerequisites

### System Requirements

**Minimum (Development):**
- CPU: 2 cores
- RAM: 4GB
- Disk: 20GB
- OS: Linux kernel 4.x+

**Recommended (Production):**
- CPU: 8+ cores
- RAM: 16GB+
- Disk: 100GB+ SSD
- OS: Linux kernel 5.x+
- Network: 1Gbps+

### Software Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    libssl-dev \
    libpthread-stubs0-dev \
    git

# RHEL/CentOS
sudo yum install -y \
    gcc \
    make \
    openssl-devel \
    git
```

---

## 📦 Installation

### Option 1: From Source (Recommended)

```bash
# Clone repository
git clone https://github.com/rads-lang/rads.git
cd rads/src/constellation

# Build
make clean && make

# Install
sudo make install
# Installs to /usr/local/bin/constellation

# Verify installation
constellation --version
# Output: RADS Constellation v0.0.4
```

### Option 2: Binary Release

```bash
# Download latest release
wget https://github.com/rads-lang/rads/releases/download/v0.0.4/constellation-linux-amd64.tar.gz

# Extract
tar -xzf constellation-linux-amd64.tar.gz

# Install
sudo mv constellation /usr/local/bin/
sudo chmod +x /usr/local/bin/constellation

# Verify
constellation --version
```

### Option 3: Package Manager

```bash
# Ubuntu/Debian
sudo add-apt-repository ppa:rads-lang/stable
sudo apt-get update
sudo apt-get install rads-constellation

# RHEL/CentOS
sudo yum install rads-constellation
```

---

## ⚙️ Configuration

### Directory Structure

```bash
# Create constellation directories
sudo mkdir -p /etc/constellation
sudo mkdir -p /var/lib/constellation
sudo mkdir -p /var/log/constellation
sudo mkdir -p /etc/constellation/certs

# Set permissions
sudo chown -R constellation:constellation /var/lib/constellation
sudo chown -R constellation:constellation /var/log/constellation
```

### Main Configuration File

Create `/etc/constellation/config.yaml`:

```yaml
# Constellation Configuration
cluster:
  name: "production"
  node_name: "node-1"
  bind_address: "0.0.0.0:7946"
  advertise_address: "10.0.1.10:7946"

controller:
  api_address: "0.0.0.0:8080"
  max_instances: 1000
  enable_auth: true
  auth_token_file: "/etc/constellation/tokens"

security:
  enable_tls: true
  tls_cert: "/etc/constellation/certs/server.crt"
  tls_key: "/etc/constellation/certs/server.key"
  tls_ca: "/etc/constellation/certs/ca.crt"

resources:
  default_cpu_limit: 1.0
  default_memory_limit: "512MB"
  enforce_quotas: true

monitoring:
  enable_metrics: true
  metrics_port: 9090
  enable_logging: true
  log_level: "info"
  log_file: "/var/log/constellation/constellation.log"

storage:
  data_dir: "/var/lib/constellation"
  enable_replication: true
  replication_factor: 3
```

### Environment Variables

Create `/etc/constellation/environment`:

```bash
# Constellation Environment
CONSTELLATION_CONFIG=/etc/constellation/config.yaml
CONSTELLATION_LOG_LEVEL=info
CONSTELLATION_DATA_DIR=/var/lib/constellation
```

---

## 🌐 Cluster Setup

### Single-Node Setup (Development)

```bash
# Start controller
constellation start \
  --config=/etc/constellation/config.yaml \
  --node-name=dev-node

# Verify
constellation status
```

### Multi-Node Cluster (Production)

**Node 1 (Bootstrap):**

```bash
# Start first node
constellation start \
  --config=/etc/constellation/config.yaml \
  --node-name=node-1 \
  --bootstrap

# Get cluster token
constellation cluster token
# Output: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

**Node 2-N (Join):**

```bash
# Join cluster
constellation start \
  --config=/etc/constellation/config.yaml \
  --node-name=node-2 \
  --join=10.0.1.10:7946 \
  --token=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...

# Verify membership
constellation cluster members
```

### Systemd Service

Create `/etc/systemd/system/constellation.service`:

```ini
[Unit]
Description=RADS Constellation Controller
After=network.target
Wants=network-online.target

[Service]
Type=simple
User=constellation
Group=constellation
EnvironmentFile=/etc/constellation/environment
ExecStart=/usr/local/bin/constellation start --config=/etc/constellation/config.yaml
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=5s
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable constellation
sudo systemctl start constellation
sudo systemctl status constellation
```

---

## 🔒 Security Hardening

### TLS Certificate Setup

```bash
# Generate CA
openssl genrsa -out ca.key 4096
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt \
  -subj "/CN=Constellation CA"

# Generate server certificate
openssl genrsa -out server.key 4096
openssl req -new -key server.key -out server.csr \
  -subj "/CN=constellation.local"
openssl x509 -req -days 365 -in server.csr \
  -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt

# Install certificates
sudo cp ca.crt server.crt server.key /etc/constellation/certs/
sudo chmod 600 /etc/constellation/certs/server.key
```

### Authentication Setup

```bash
# Generate auth token
constellation auth generate-token --role=admin > /etc/constellation/tokens

# Create API key for applications
constellation auth create-key --name=app1 --permissions=deploy,scale
```

### Firewall Configuration

```bash
# Allow cluster communication
sudo ufw allow 7946/tcp  # Gossip
sudo ufw allow 7946/udp  # Gossip
sudo ufw allow 8080/tcp  # API
sudo ufw allow 9090/tcp  # Metrics

# Enable firewall
sudo ufw enable
```

---

## 📊 Monitoring Setup

### Prometheus Integration

Create `/etc/prometheus/constellation.yml`:

```yaml
scrape_configs:
  - job_name: 'constellation'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: '/metrics'
```

### Grafana Dashboard

```bash
# Import Constellation dashboard
# Dashboard ID: 12345 (from grafana.com)
```

### Log Aggregation

```bash
# Configure log shipping to centralized logging
# Example: Elasticsearch, Loki, etc.
```

---

## 🔄 High Availability

### Multi-Master Setup

```yaml
# config.yaml
ha:
  enable: true
  raft_peers:
    - "10.0.1.10:7947"
    - "10.0.1.11:7947"
    - "10.0.1.12:7947"
  election_timeout: "10s"
  heartbeat_interval: "1s"
```

### Load Balancer Configuration

```nginx
# nginx.conf
upstream constellation_api {
    least_conn;
    server 10.0.1.10:8080 max_fails=3 fail_timeout=30s;
    server 10.0.1.11:8080 max_fails=3 fail_timeout=30s;
    server 10.0.1.12:8080 max_fails=3 fail_timeout=30s;
}

server {
    listen 443 ssl;
    server_name constellation.example.com;
    
    ssl_certificate /etc/nginx/certs/constellation.crt;
    ssl_certificate_key /etc/nginx/certs/constellation.key;
    
    location / {
        proxy_pass http://constellation_api;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

---

## 💾 Backup & Recovery

### Backup Strategy

```bash
#!/bin/bash
# backup-constellation.sh

BACKUP_DIR="/backup/constellation"
DATE=$(date +%Y%m%d_%H%M%S)

# Backup configuration
tar -czf $BACKUP_DIR/config-$DATE.tar.gz /etc/constellation

# Backup data
constellation backup create --output=$BACKUP_DIR/data-$DATE.backup

# Backup logs
tar -czf $BACKUP_DIR/logs-$DATE.tar.gz /var/log/constellation

# Cleanup old backups (keep 30 days)
find $BACKUP_DIR -name "*.tar.gz" -mtime +30 -delete
find $BACKUP_DIR -name "*.backup" -mtime +30 -delete
```

### Disaster Recovery

```bash
# Stop constellation
sudo systemctl stop constellation

# Restore configuration
tar -xzf config-20260113.tar.gz -C /

# Restore data
constellation backup restore --input=data-20260113.backup

# Start constellation
sudo systemctl start constellation

# Verify cluster health
constellation cluster health
```

---

## 🔧 Troubleshooting

### Common Issues

**Issue: Node won't join cluster**

```bash
# Check network connectivity
ping 10.0.1.10

# Check firewall
sudo ufw status

# Check logs
journalctl -u constellation -f

# Verify token
constellation cluster verify-token
```

**Issue: High memory usage**

```bash
# Check instance count
constellation instances list | wc -l

# Check memory per instance
constellation instances stats

# Adjust limits in config.yaml
```

**Issue: Slow performance**

```bash
# Check system resources
top
iostat -x 1

# Check constellation metrics
curl http://localhost:9090/metrics

# Enable profiling
constellation debug profile --duration=60s
```

### Debug Mode

```bash
# Start in debug mode
constellation start --log-level=debug

# Enable profiling
constellation debug enable-profiling

# Collect diagnostics
constellation debug collect-diagnostics --output=diag.tar.gz
```

---

## ✅ Best Practices

### Deployment

1. **Use Infrastructure as Code**
   - Terraform, Ansible, or similar
   - Version control all configurations
   - Automate deployments

2. **Gradual Rollouts**
   - Deploy to staging first
   - Use canary deployments
   - Monitor metrics during rollout

3. **Resource Planning**
   - Size nodes appropriately
   - Plan for growth (2x capacity)
   - Monitor resource utilization

### Operations

1. **Monitoring**
   - Set up alerts for critical metrics
   - Monitor cluster health continuously
   - Review logs regularly

2. **Backups**
   - Automate backup process
   - Test restore procedures
   - Store backups off-site

3. **Updates**
   - Test updates in staging
   - Schedule maintenance windows
   - Have rollback plan ready

### Security

1. **Access Control**
   - Use strong authentication
   - Implement RBAC
   - Audit access logs

2. **Network Security**
   - Use private networks
   - Enable TLS everywhere
   - Implement firewall rules

3. **Compliance**
   - Follow security policies
   - Conduct regular audits
   - Document procedures

---

## 📈 Scaling Guide

### Horizontal Scaling

```bash
# Add nodes to cluster
for i in {4..10}; do
  constellation cluster add-node \
    --name=node-$i \
    --address=10.0.1.$i:7946
done
```

### Vertical Scaling

```yaml
# Increase node resources
resources:
  default_cpu_limit: 2.0
  default_memory_limit: "1GB"
  max_instances_per_node: 200
```

---

## 🎯 Production Checklist

### Pre-Deployment

- [ ] Hardware provisioned and tested
- [ ] Network configured and secured
- [ ] TLS certificates generated
- [ ] Configuration reviewed
- [ ] Backup strategy defined
- [ ] Monitoring configured
- [ ] Documentation complete

### Post-Deployment

- [ ] Cluster health verified
- [ ] Monitoring dashboards created
- [ ] Alerts configured
- [ ] Backup tested
- [ ] Disaster recovery tested
- [ ] Team trained
- [ ] Runbooks created

---

## 📞 Support

**Documentation:** https://docs.rads-lang.org/constellation  
**Community:** https://community.rads-lang.org  
**Issues:** https://github.com/rads-lang/rads/issues  
**Email:** support@rads-lang.org

---

**RADS v0.0.4 "Constellation"**
**Production-Ready. Battle-Tested. Enterprise-Grade.**

*Last Updated: January 13, 2026*
