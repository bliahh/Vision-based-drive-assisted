#include "tcp_thread.h"
#include "tcp_client.h"
#include "parse_json.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <iostream>


void runTcpReceiverThread(int port,
                          FrameData& shared_frame,
                          std::mutex& frame_mutex,
                          std::atomic<bool>& app_is_running)
{
    TcpServer server;

    if (!server.start(port)) return;
    if (!server.waitForClient()) return;

    std::string accumulated_data;

    while (app_is_running) {

        std::string chunk;

        if (!server.readChunk(chunk)) break;

        if (chunk.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        accumulated_data += chunk;

        size_t newline_pos;

        while ((newline_pos = accumulated_data.find('\n')) != std::string::npos) {

            std::string json_line = accumulated_data.substr(0, newline_pos);
            accumulated_data.erase(0, newline_pos + 1);

            if (json_line.empty()) continue;

            try {
                auto json_obj = nlohmann::json::parse(json_line);

                FrameData new_frame;
                new_frame.fps           = json_obj.value("fps", 0.f);
                new_frame.tcp_ok        = true;
                new_frame.lane_lines    = parseLaneLines(json_obj);
                new_frame.drivable_area = parseDrivableArea(json_obj);

                if (json_obj.contains("cars"))   new_frame.detected_cars    = parseDetectedObjects(json_obj["cars"]);
                if (json_obj.contains("signs"))  new_frame.detected_signs   = parseDetectedObjects(json_obj["signs"]);
                if (json_obj.contains("person")) new_frame.detected_people  = parseDetectedObjects(json_obj["person"]);

                std::lock_guard<std::mutex> lock(frame_mutex);
                shared_frame = std::move(new_frame);

            } catch (...) {
            }
        }

        if (accumulated_data.size() > 65536) {
            accumulated_data.clear();
        }
    }

    server.closeConnection();
}
