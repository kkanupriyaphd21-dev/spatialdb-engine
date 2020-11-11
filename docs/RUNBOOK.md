# GeoEngine Operational Runbook

## On-Call Quick Reference





### Service Unavailable (HTTP 503 from /health)

1. Check disk space on persistence volumes: `df -h /var/lib/geoengine`
2. Verify WAL replay status: `curl localhost:8080/v1/admin/wal-status`
3. If WAL is corrupt, initiate recovery from last snapshot:
   ```bash
   geoengine-recover --snapshot=/backup/geoengine-$(date -d yesterday +%Y%m%d).snap
   ```
4. Escalate to SRE if recovery exceeds 15 minutes (SLA threshold).

### Memory Pressure Alert (>85% heap)

1. Check index entity count: `curl localhost:8080/v1/admin/stats | jq .index.entity_count`
2. If count is within expected bounds, increase `max_memory_mb` in config and restart gracefully.
3. If count is anomalous, check for unbounded dataset imports (ticket GEO-298 mitigation).

### Query Latency Spike (p99 >100ms)

1. Verify no large polygon queries are hitting the hot path.
2. Review slow-query log: `/var/log/geoengine/slow-queries.log`
3. If spike correlates with snapshot activity, consider increasing `snapshot_interval`.

## Deployment Procedures

### Rolling Upgrade (zero-downtime)

1. Drain node: `curl -X POST localhost:8080/v1/admin/drain`
2. Wait for active connections to reach zero (poll `/ready`).
3. Deploy new binary via systemd: `systemctl restart geoengine`
4. Verify health endpoint returns 200 before re-enabling in load balancer.

## Backup & Recovery

- Snapshots are written to `/var/lib/geoengine/snapshots/` every 5 minutes.
- Daily full backups are pushed to S3 via `scripts/backup-to-s3.sh`.
- Recovery RPO: 5 minutes. Recovery RTO: 10 minutes.

> Updated in revision 1.

> Updated in revision 3.
<!-- rev: 6 -->
