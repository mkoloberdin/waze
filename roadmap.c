
#include "roadmap.h"
#include <stdio.h>

const char* roadmap_result_string( int res)
{
   static char str[64];

   if( (res < 0) || (all_errors_end <= res))
      return "<WRONG RESULT CODE>";

   switch(res)
   {
      case succeeded: 
         return "Success";


      // General errors:
      case err_failed:
         return "Operation failed";
         
      case err_no_memory:
         return "Out of memory";
         
      case err_invalid_argument:
         return "Invalid argument";
         
      case err_aborted:
         return "Operation aborted";
         
      case err_access_denied:
         return "Access denied";
         
      case err_timed_out:
         return "Operation timed-out";
      
      case err_internal_error:
         return "Internal error";


      // Network errors:
      case err_net_failed:
         return "NET: Network operation failed";

      case err_net_unknown_protocol:
         return "NET: Unknown protocol";
         
      case err_net_remote_error:
         return "NET: Remote error";
         
      case err_net_request_pending:
         return "NET: Request pending";
         
      case err_net_no_path_to_destination:
         return "NET: No path to destination";
 
 
      // Data parser:
      case err_parser_unexpected_data:
         return "PARSER: Unexpected data";
      
      case err_parser_failed:
         return "PARSER: Failed";
      
      case err_parser_missing_tag_handler:
         return "PARSER: Did not find parser-handler for tag";


      // Realtime
      case err_rt_no_data_to_send:
         return "RT: No data to send";

      case err_rt_login_failed:
         return "RT: Login failed";

      case err_rt_wrong_name_or_password:
         return "RT: Wrong name or password";

      case err_rt_wrong_network_settings:
         return "RT: Wrong network settings";

      case err_rt_unknown_login_id:
         return "RT: Unknown login id";

      // Address Search - String Resolution
      case err_as_could_not_find_matches:
         return "Could not find relevant results for this search";
         
      case err_as_wrong_input_string_size:
         return "More characters are needed to perform a search";
         
      case err_as_already_in_transaction:
         return "Previous search-transaction did not complete yet";
   }

   // Code should not get here 
   //    Unless wrong input, or something forgotten
   sprintf(str, "<RC:%d>", res);
   return str;
}
