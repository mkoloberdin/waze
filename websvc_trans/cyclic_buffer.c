
#include "../roadmap.h"
#include "string_parser.h"

#include "cyclic_buffer.h"

void cyclic_buffer_init( cyclic_buffer_ptr this)
{
   memset( this->buffer, 0, sizeof(this->buffer));
   
   this->read_size      =  0; // Size of the data read into the buffer
   this->read_processed =  0; // Out of 'read_size', how much was processed
   this->data_size      = -1; // Overall size of all expected data
   this->data_processed =  0; // Out of 'data_size', how much was processed
   
   //   Internal usage:
   this->next_read      = this->buffer;
   this->free_size      = CYCLIC_BUFFER_SIZE;
}

//   Recylce buffer before going into the next 'read' statement
void cyclic_buffer_recycle( cyclic_buffer_ptr this)
{
   //   Update overall bytes processed:
   this->data_processed += this->read_processed;

   //   Did we process all read data?
   if( this->read_processed == this->read_size)
   {
      this->read_size = 0;
      this->buffer[0] = '\0';
   
      //   Internal usage:
      this->next_read = this->buffer;
      this->free_size = CYCLIC_BUFFER_SIZE;
   }
   else
   {
      if( this->read_processed)
      {
         //   If anything was processed, then loose processed data, and copy the 
         //      unprocessed-data to the buffer beginning:
         int remained = (this->read_size - this->read_processed);
         
         //   Shift data:
         memmove( this->buffer, this->buffer + this->read_processed, remained);

         this->buffer[remained]  = '\0';     // Terminate string with a NULL
         this->read_size         = remained; // Update buffer size to unprocessed-buffer size
      }

      //   Internal usage:
      this->next_read = (this->buffer       + this->read_size);
      this->free_size = (CYCLIC_BUFFER_SIZE - this->read_size);
   }

   // Reset:
   this->read_processed = 0;
}

void cyclic_buffer_update_processed_data( 
                              cyclic_buffer_ptr this, 
                              const char*       data, 
                              const char*       data_to_skip)
{
   int read_processed;

   assert(data);

   if(data_to_skip && (*data_to_skip))
      data = EatChars( data, data_to_skip, TRIM_ALL_CHARS);
   
   read_processed = data - this->buffer;
   
   // Note:
   //    The value 'read_processed' includes any previous processed values
   
   assert( 0 <= read_processed);
   assert( read_processed <= this->read_size);
   
   this->read_processed = read_processed;   
}                       
       
const char* cyclic_buffer_get_unprocessed_data(
                              cyclic_buffer_ptr this)
{
   return this->buffer + this->read_processed;
}
