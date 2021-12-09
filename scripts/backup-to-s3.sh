#!/usr/bin/env bash
set -euo pipefail

BUCKET="${geoengine_CAP_BACKUP_BUCKET:-geoengine-backups}"
REGION="${geoengine_CAP_BACKUP_REGION:-us-east-1}"
DATA_DIR="${geoengine_CAP_DATA_PATH:-/var/lib/geoengine}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SNAPSHOT_FILE="geoengine_snapshot_${TIMESTAMP}.tar.gz"

echo "[backup] Creating snapshot archive..."
tar -czf "/tmp/${SNAPSHOT_FILE}" -C "${DATA_DIR}" snapshots/

echo "[backup] Uploading to s3://${BUCKET}/daily/"
aws s3 cp "/tmp/${SNAPSHOT_FILE}" "s3://${BUCKET}/daily/${SNAPSHOT_FILE}" --region "${REGION}"

echo "[backup] Cleaning up archives older than 30 days..."
aws s3 ls "s3://${BUCKET}/daily/" --region "${REGION}" | awk '{print $4}' | while read -r file; do
    file_date=$(echo "$file" | grep -oP '\\d{8}' || true)
    if [ -n "$file_date" ]; then
        age=$(( ( $(date +%s) - $(date -d "$file_date" +%s) ) / 86400 ))
        if [ "$age" -gt 30 ]; then
            aws s3 rm "s3://${BUCKET}/daily/${file}" --region "${REGION}"
        fi
    fi
done

rm -f "/tmp/${SNAPSHOT_FILE}"
echo "[backup] Complete: ${SNAPSHOT_FILE}"
# script-rev: 1
# script-rev: 2
# script-rev: 3
# rev: 1
