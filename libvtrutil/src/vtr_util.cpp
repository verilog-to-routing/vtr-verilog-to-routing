#include "vtr_util.h"

namespace vtrutil {

std::vector<std::string> split(const std::string& text, const std::string delims) {
    std::vector<std::string> tokens;

    std::string curr_tok;
    for(char c : text) {
        if(delims.find(c) != std::string::npos) {
            if(!curr_tok.empty()) {
                //At the end of the token

                //Save it
                tokens.push_back(curr_tok);

                //Reset token
                curr_tok.clear();
            } else {
                //Pass
            }
        } else {
            //Append to token
            curr_tok += c;
        }
    }

    return tokens;
}

}
