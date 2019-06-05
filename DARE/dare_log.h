/**
 * Seach for the last matching entry between local log and a remote NC-Buffer;
 * used during log adjustment to adjust the remote end offset;
 * called only by the leader
 * ! safe over RDMA
 */
static uint64_t 
log_find_remote_end_offset( dare_log_t* log, dare_nc_buf_t* nc_buf )
{
    uint64_t i;
    dare_log_entry_t *entry;
    uint64_t offset;
       
    for (i = 0; i < nc_buf->len; i++) {
        offset = nc_buf->entries[i].offset;
        entry = log_get_entry(log, &offset);
        if (NULL == entry) {
            /* No more local entries */
            return offset;
        }
        if ( (entry->idx != nc_buf->entries[i].idx) ||
            (entry->term != nc_buf->entries[i].term) )
        {
            return offset;
        }
        if (!log_fit_entry(log, offset, entry)) {
            /* Not enough place for an entry (with the command); that 
             * means the log entry continues on the other side */
            offset = 0;
        }
        offset += log_entry_len(entry);
    }
    return offset;
}

