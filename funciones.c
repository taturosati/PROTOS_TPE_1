// int parseCommand(int j, int* may_match, int* command, int* parse_end_idx) {
//     for (int k = 0; k < 3 && command == -1 && may_match_count > 0; k++) {
//         if (may_match[k]) {
//             const struct parser_event* state = parser_feed(client_socket[i].parsers[k], in_buffer[j]);
//             if (state->type == STRING_CMP_EQ) {				//matcheo uno de los comandos (echo, date o time)
//                 log(DEBUG, "matched after %d bytes", j);
//                 *command = k;
//                 *parse_end_idx = j + 1;
//                 client_socket[i].action = EXECUTING;
//                 client_socket[i].matched_idx = k;
//                 parser_reset(client_socket[i].parsers[k]);
//             }
//             else if (state->type == STRING_CMP_NEQ) {	//ya hay un comando q no matcheo
//                 may_match[k] = 0;
//                 may_match_count--;
//             }
//         }
//     }
//     return may_match_count;
// }

