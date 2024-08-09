/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif
#include <stdio.h>
#include "aesd-circular-buffer.h"

/**
 * @param buffer - the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset - the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn - is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    size_t accumulated_size = 0;
    uint8_t index = 0;
    struct aesd_buffer_entry *current_entry;
    //using the macro to iterate over each entry in the circular buffer
    AESD_CIRCULAR_BUFFER_FOREACH(current_entry, buffer, index) {
        int real_index = (buffer->out_offs + index) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        current_entry = &((buffer)->entry[real_index]);
        //debug
        //printf("Index: %d, real_index: %d, out_offs: %d, in_offs: %d, accumulated_size: %zu, entry_size: %zu\n", 
        //		index, real_index, buffer->out_offs, buffer->in_offs, accumulated_size, current_entry->size);
        //verify when the index matches with the out offset or if the buffer is full         
        if (index == buffer->out_offs || buffer->full) {
            //printf("Processing entry at index %d\n", index);
            // Check if the desired char_offset falls within the current entry
            if (accumulated_size + current_entry->size > char_offset) {
                // Calculate the specific offset within this entry
                *entry_offset_byte_rtn = char_offset - accumulated_size;
                //printf("Found char_offset at index %d, entry_offset_byte_rtn: %zu\n", 
                //		index, *entry_offset_byte_rtn);
                return current_entry;
            }

            // Accumulate the size
            accumulated_size += current_entry->size;

            // Stop iterating when we reach the in_offs (unless buffer is full)
            if (real_index == buffer->in_offs && !buffer->full) {
                //printf("Reached in_offs. Exiting loop\n");
                break;
            }
        }
    }
    // If we exit the loop, the offset was not found in the buffer
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    // Add the new entry at the current in_offs location
    buffer->entry[buffer->in_offs] = *add_entry;
//    for (size_t i=0; i < buffer->entry[buffer->in_offs].size; i++){
//    	printf("%c", buffer->entry[buffer->in_offs].buffptr[i]);
//    } 

    if (buffer->full) {
        // If the buffer was full, increment out_offs to overwrite the oldest entry
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
//    printf("out_offs: %d\n", buffer->out_offs);

    // Move the in_offs to the next position
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
//    printf("in_offs: %d\n", buffer->in_offs);

    // If in_offs catches up with out_offs, the buffer is full
    if (buffer->in_offs == buffer->out_offs) {
        buffer->full = true;
//        printf("Buffer full\n");
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
