#pragma once
#include "frame_data.h"
#include <mutex>
#include <atomic>

void runTcpReceiverThread(int port,FrameData& shared_frame,std::mutex& frame_mutex,std::atomic<bool>& app_is_running);
