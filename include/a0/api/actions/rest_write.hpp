#pragma once

#include <App.h>

#include <sstream>

#include "a0/api/request_message.hpp"
#include "a0/api/rest_helpers.hpp"

namespace a0::api {

// fetch(`http://${api_addr}/api/write`, {
//     method: "POST",
//     body: JSON.stringify({
//         path: "...",                      // required
//         standard_headers: false,          // optional
//         packet: {
//             headers: [                    // optional
//                 ["key", "val"],
//                 ...
//             ],
//             payload: "...",               // required
//         },
//         request_encoding: "none"          // optional, one of "none", "base64"
//     })
// })
// .then((r) => { return r.text() })
// .then((msg) => { console.assert(msg == "success", msg) })
static inline void rest_write(uWS::HttpResponse<false>* res,
                              uWS::HttpRequest* req) {
  res->onData([res, ss = std::stringstream()](std::string_view chunk, bool is_end) mutable {
    ss << chunk;
    if (!is_end) {
      return;
    }

    RequestMessage req_msg;
    bool standard_headers = false;
    try {
      // Parse input.
      req_msg = ParseRequestMessage(ss.str());

      // Check required fields.
      req_msg.require("path");
      req_msg.require(nlohmann::json::json_pointer("/packet/payload"));
      req_msg.maybe_get_to("standard_headers", standard_headers);
    } catch (std::exception& e) {
      rest_respond(res, "400", {}, e.what());
      return;
    }

    // Perform requested action.
    Writer w(File(req_msg.path));
    if (standard_headers) {
      w.push(add_standard_headers());
    }
    w.write(std::move(req_msg.pkt));

    rest_respond(res, "200", {}, "success");
  });

  res->onAborted([]() {});
}

}  // namespace a0::api