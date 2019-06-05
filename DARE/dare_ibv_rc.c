/**
 * Log adjustment phase
 *  - read the remote commit offset (note that if the remote server has 
 * no not committed entries, we need to set the end offset to the 
 * remote commit offset)
 *  - read the number of not committed entries
 *  - read the not committed entries
 *  - find offset of first non-matching entry and update remote end offset
 * Note: only the leader calls this function
 */
